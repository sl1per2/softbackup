#include "incremental/common/bitmap_manager.h"
#include <spdlog/spdlog.h>
#include <cstring>
#include <sstream>
#include <iomanip>

namespace vovqa::incremental {

std::vector<uint8_t> compress_bitmap_rle(const std::vector<uint8_t>& bitmap) {
    std::vector<uint8_t> result;
    if (bitmap.empty()) return result;
    size_t i = 0;
    while (i < bitmap.size()) {
        uint8_t val = bitmap[i];
        uint16_t count = 1;
        while (i + count < bitmap.size() && bitmap[i + count] == val && count < 65535) count++;
        result.push_back(val);
        result.push_back(static_cast<uint8_t>((count >> 8) & 0xFF));
        result.push_back(static_cast<uint8_t>(count & 0xFF));
        i += count;
    }
    return result;
}

std::vector<uint8_t> decompress_bitmap_rle(const std::vector<uint8_t>& data, size_t expected_size) {
    std::vector<uint8_t> result;
    for (size_t i = 0; i + 2 < data.size(); i += 3) {
        uint8_t val = data[i];
        uint16_t count = (static_cast<uint16_t>(data[i + 1]) << 8) | data[i + 2];
        result.insert(result.end(), count, val);
    }
    result.resize((expected_size + 7) / 8, 0);
    return result;
}

BitmapManager::BitmapManager() { init_db(); }
BitmapManager::~BitmapManager() { if (db_) sqlite3_close(db_); }

void BitmapManager::init_db() {
    if (sqlite3_open("/tmp/vovqa_bitmap_manager.db", &db_) != SQLITE_OK) {
        spdlog::error("Failed to open bitmap manager db");
        return;
    }
    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS bitmap_store (
            object_id TEXT PRIMARY KEY,
            job_id TEXT,
            block_size INTEGER,
            bitmap BLOB,
            created_at TEXT DEFAULT CURRENT_TIMESTAMP
        );
        CREATE TABLE IF NOT EXISTS job_chain (
            job_id TEXT PRIMARY KEY,
            parent_id TEXT,
            job_type TEXT,
            bitmap_hash TEXT,
            chain_length INTEGER DEFAULT 0,
            created_at TEXT DEFAULT CURRENT_TIMESTAMP
        );
        CREATE TABLE IF NOT EXISTS incremental_metrics (
            job_id TEXT PRIMARY KEY,
            changed_blocks_total INTEGER DEFAULT 0,
            changed_blocks_read INTEGER DEFAULT 0,
            changed_blocks_skipped INTEGER DEFAULT 0,
            cbt_query_time_ms INTEGER DEFAULT 0,
            data_read_time_ms INTEGER DEFAULT 0,
            change_percent REAL DEFAULT 0,
            chain_length INTEGER DEFAULT 0,
            savings_bytes INTEGER DEFAULT 0,
            created_at TEXT DEFAULT CURRENT_TIMESTAMP
        );
    )";
    char* err = nullptr;
    sqlite3_exec(db_, sql, nullptr, nullptr, &err);
    if (err) { spdlog::error("BitmapManager DB: {}", err); sqlite3_free(err); }
}

bool BitmapManager::save_bitmap(const std::string& object_id, const std::string& job_id,
                                const std::vector<uint8_t>& bitmap, uint32_t block_size) {
    std::lock_guard<std::mutex> lock(db_mutex_);
    auto compressed = compress_bitmap_rle(bitmap);
    const char* sql = "INSERT OR REPLACE INTO bitmap_store (object_id, job_id, block_size, bitmap) VALUES (?, ?, ?, ?)";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, object_id.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, job_id.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, block_size);
    sqlite3_bind_blob(stmt, 4, compressed.data(), compressed.size(), SQLITE_STATIC);
    bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok;
}

bool BitmapManager::load_bitmap(const std::string& object_id, std::vector<uint8_t>& bitmap, uint32_t& block_size) {
    std::lock_guard<std::mutex> lock(db_mutex_);
    const char* sql = "SELECT block_size, bitmap FROM bitmap_store WHERE object_id = ?";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, object_id.c_str(), -1, SQLITE_STATIC);
    bool found = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        block_size = sqlite3_column_int(stmt, 0);
        auto compressed = std::vector<uint8_t>(
            reinterpret_cast<const uint8_t*>(sqlite3_column_blob(stmt, 1)),
            reinterpret_cast<const uint8_t*>(sqlite3_column_blob(stmt, 1)) + sqlite3_column_bytes(stmt, 1));
        bitmap = decompress_bitmap_rle(compressed, compressed.size() * 8);
        found = true;
    }
    sqlite3_finalize(stmt);
    return found;
}

bool BitmapManager::sync_bitmap_to_server(const std::string& object_id, const std::string& server_url) {
    spdlog::info("Syncing bitmap for {} to {}", object_id, server_url);
    return true;
}

ChainInfo BitmapManager::get_chain_info(const std::string& job_id) {
    std::lock_guard<std::mutex> lock(db_mutex_);
    ChainInfo info;
    const char* sql = "SELECT job_id, parent_id, job_type, bitmap_hash, chain_length FROM job_chain WHERE job_id = ?";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, job_id.c_str(), -1, SQLITE_STATIC);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        info.base_job_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        info.parent_job_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        info.job_type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        info.bitmap_hash = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        info.chain_length = sqlite3_column_int(stmt, 4);
        info.is_broken = false;
    }
    sqlite3_finalize(stmt);
    return info;
}

bool BitmapManager::verify_chain_integrity(const std::string& base_job_id) {
    std::lock_guard<std::mutex> lock(db_mutex_);
    std::string current = base_job_id;
    int depth = 0;
    while (!current.empty() && depth < 1000) {
        const char* sql = "SELECT parent_id FROM job_chain WHERE job_id = ?";
        sqlite3_stmt* stmt = nullptr;
        sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
        sqlite3_bind_text(stmt, 1, current.c_str(), -1, SQLITE_STATIC);
        if (sqlite3_step(stmt) != SQLITE_ROW) {
            sqlite3_finalize(stmt);
            spdlog::warn("Chain broken at job {}", current);
            return false;
        }
        std::string parent = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        sqlite3_finalize(stmt);
        if (parent.empty()) break;
        current = parent;
        depth++;
    }
    return true;
}

void BitmapManager::record_job(const std::string& job_id, const std::string& parent_id,
                               const std::string& type, const std::string& bitmap_hash) {
    std::lock_guard<std::mutex> lock(db_mutex_);
    int chain_length = 0;
    if (!parent_id.empty()) {
        const char* cl_sql = "SELECT chain_length FROM job_chain WHERE job_id = ?";
        sqlite3_stmt* cl_stmt = nullptr;
        sqlite3_prepare_v2(db_, cl_sql, -1, &cl_stmt, nullptr);
        sqlite3_bind_text(cl_stmt, 1, parent_id.c_str(), -1, SQLITE_STATIC);
        if (sqlite3_step(cl_stmt) == SQLITE_ROW) {
            chain_length = sqlite3_column_int(cl_stmt, 0) + 1;
        }
        sqlite3_finalize(cl_stmt);
    }

    const char* sql = "INSERT OR REPLACE INTO job_chain (job_id, parent_id, job_type, bitmap_hash, chain_length) VALUES (?, ?, ?, ?, ?)";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, job_id.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, parent_id.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, type.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, bitmap_hash.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 5, chain_length);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

IncrementalMetrics BitmapManager::get_metrics(const std::string& job_id) {
    std::lock_guard<std::mutex> lock(db_mutex_);
    IncrementalMetrics m;
    const char* sql = "SELECT * FROM incremental_metrics WHERE job_id = ?";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, job_id.c_str(), -1, SQLITE_STATIC);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        m.changed_blocks_total = sqlite3_column_int64(stmt, 1);
        m.changed_blocks_read = sqlite3_column_int64(stmt, 2);
        m.changed_blocks_skipped = sqlite3_column_int64(stmt, 3);
        m.cbt_query_time_ms = sqlite3_column_int64(stmt, 4);
        m.data_read_time_ms = sqlite3_column_int64(stmt, 5);
        m.change_percent = sqlite3_column_double(stmt, 6);
        m.incremental_chain_length = sqlite3_column_int(stmt, 7);
        m.incremental_savings_bytes = sqlite3_column_int64(stmt, 8);
    }
    sqlite3_finalize(stmt);
    return m;
}

void BitmapManager::record_metrics(const std::string& job_id, const IncrementalMetrics& m) {
    std::lock_guard<std::mutex> lock(db_mutex_);
    const char* sql = "INSERT OR REPLACE INTO incremental_metrics VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, datetime('now'))";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, job_id.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 2, m.changed_blocks_total);
    sqlite3_bind_int64(stmt, 3, m.changed_blocks_read);
    sqlite3_bind_int64(stmt, 4, m.changed_blocks_skipped);
    sqlite3_bind_int64(stmt, 5, m.cbt_query_time_ms);
    sqlite3_bind_int64(stmt, 6, m.data_read_time_ms);
    sqlite3_bind_double(stmt, 7, m.change_percent);
    sqlite3_bind_int(stmt, 8, m.incremental_chain_length);
    sqlite3_bind_int64(stmt, 9, m.incremental_savings_bytes);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

double BitmapManager::estimate_change_percent(const std::string& object_id, uint64_t total_blocks) {
    std::vector<uint8_t> bitmap;
    uint32_t bs;
    if (!load_bitmap(object_id, bitmap, bs) || total_blocks == 0) return 1.0;
    size_t set_bits = 0;
    for (uint8_t byte : bitmap) {
        for (int b = 0; b < 8; b++) {
            if (byte & (1 << b)) set_bits++;
        }
    }
    return static_cast<double>(set_bits) / total_blocks;
}

bool BitmapManager::should_do_full(const std::string& object_id, uint64_t total_blocks, double threshold) {
    double pct = estimate_change_percent(object_id, total_blocks);
    if (pct > threshold) {
        spdlog::info("Change percent {:.1f}% > threshold {:.1f}%, recommending full backup", pct * 100, threshold * 100);
        return true;
    }
    return false;
}

ChangeJournal::ChangeJournal() { init_db(); }
ChangeJournal::~ChangeJournal() { if (db_) sqlite3_close(db_); }

void ChangeJournal::init_db() {
    if (sqlite3_open("/tmp/vovqa_change_journal.db", &db_) != SQLITE_OK) return;
    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS block_changes (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            session_id TEXT,
            lba INTEGER,
            size INTEGER,
            operation TEXT,
            timestamp TEXT DEFAULT CURRENT_TIMESTAMP
        );
        CREATE INDEX IF NOT EXISTS idx_bc_session ON block_changes(session_id);
    )";
    char* err = nullptr;
    sqlite3_exec(db_, sql, nullptr, nullptr, &err);
    if (err) sqlite3_free(err);
}

void ChangeJournal::log_change(const std::string& session_id, uint64_t lba, uint32_t size, const std::string& operation) {
    std::lock_guard<std::mutex> lock(db_mutex_);
    const char* sql = "INSERT INTO block_changes (session_id, lba, size, operation) VALUES (?, ?, ?, ?)";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, session_id.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 2, lba);
    sqlite3_bind_int(stmt, 3, size);
    sqlite3_bind_text(stmt, 4, operation.c_str(), -1, SQLITE_STATIC);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

std::vector<BlockRange> ChangeJournal::get_changes_since(const std::string& session_id, const std::string& since) {
    std::vector<BlockRange> ranges;
    std::lock_guard<std::mutex> lock(db_mutex_);
    const char* sql = "SELECT lba, size FROM block_changes WHERE session_id = ? AND timestamp > ? ORDER BY lba";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, session_id.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, since.c_str(), -1, SQLITE_STATIC);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        ranges.push_back({static_cast<uint64_t>(sqlite3_column_int64(stmt, 0)),
                          static_cast<uint32_t>(sqlite3_column_int(stmt, 1)), true});
    }
    sqlite3_finalize(stmt);
    return ranges;
}

void ChangeJournal::cleanup_old_entries(int retention_minutes) {
    std::lock_guard<std::mutex> lock(db_mutex_);
    const char* sql = "DELETE FROM block_changes WHERE timestamp < datetime('now', ?)";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    std::string mins = "-" + std::to_string(retention_minutes) + " minutes";
    sqlite3_bind_text(stmt, 1, mins.c_str(), -1, SQLITE_STATIC);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

} // namespace vovqa::incremental
