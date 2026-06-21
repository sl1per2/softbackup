#include "incremental/common/incremental_engine.h"
#include <spdlog/spdlog.h>
#include <cstring>

namespace vovqa::incremental {

void IncrementalBackupEngine::init_db() {
    if (sqlite3_open("/tmp/vovqa_incremental.db", &db_) != SQLITE_OK) {
        spdlog::error("Failed to open incremental db");
        return;
    }
    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS bitmap_state (
            object_id TEXT PRIMARY KEY,
            block_size INTEGER,
            bitmap BLOB,
            last_job_id TEXT,
            last_success_at TEXT
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
            savings_bytes INTEGER DEFAULT 0
        );
    )";
    char* err = nullptr;
    sqlite3_exec(db_, sql, nullptr, nullptr, &err);
    if (err) { spdlog::error("Incremental DB init: {}", err); sqlite3_free(err); }
}

BitmapState IncrementalBackupEngine::load_bitmap(const std::string& object_id) {
    BitmapState state;
    std::lock_guard<std::mutex> lock(db_mutex_);
    if (!db_) init_db();
    const char* sql = "SELECT object_id, block_size, bitmap, last_job_id, last_success_at FROM bitmap_state WHERE object_id = ?";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, object_id.c_str(), -1, SQLITE_STATIC);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        state.object_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        state.block_size = sqlite3_column_int(stmt, 1);
        auto compressed = std::vector<uint8_t>(
            reinterpret_cast<const uint8_t*>(sqlite3_column_blob(stmt, 2)),
            reinterpret_cast<const uint8_t*>(sqlite3_column_blob(stmt, 2)) + sqlite3_column_bytes(stmt, 2));
        state.bitmap = decompress_rle(compressed);
        state.total_blocks = state.bitmap.size() * 8;
        state.last_job_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        state.last_success_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
    }
    sqlite3_finalize(stmt);
    return state;
}

void IncrementalBackupEngine::save_bitmap(const std::string& object_id, const BitmapState& state) {
    std::lock_guard<std::mutex> lock(db_mutex_);
    if (!db_) init_db();
    auto compressed = compress_rle(state.bitmap);
    const char* sql = "INSERT OR REPLACE INTO bitmap_state (object_id, block_size, bitmap, last_job_id, last_success_at) VALUES (?, ?, ?, ?, ?)";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, object_id.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, state.block_size);
    sqlite3_bind_blob(stmt, 3, compressed.data(), compressed.size(), SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, state.last_job_id.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, state.last_success_at.c_str(), -1, SQLITE_STATIC);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

std::vector<uint8_t> IncrementalBackupEngine::compress_rle(const std::vector<uint8_t>& data) {
    std::vector<uint8_t> result;
    if (data.empty()) return result;
    size_t i = 0;
    while (i < data.size()) {
        uint8_t val = data[i];
        uint16_t count = 1;
        while (i + count < data.size() && data[i + count] == val && count < 65535) count++;
        result.push_back(val);
        result.push_back(static_cast<uint8_t>((count >> 8) & 0xFF));
        result.push_back(static_cast<uint8_t>(count & 0xFF));
        i += count;
    }
    return result;
}

std::vector<uint8_t> IncrementalBackupEngine::decompress_rle(const std::vector<uint8_t>& data) {
    std::vector<uint8_t> result;
    for (size_t i = 0; i + 2 < data.size(); i += 3) {
        uint8_t val = data[i];
        uint16_t count = (static_cast<uint16_t>(data[i + 1]) << 8) | data[i + 2];
        result.insert(result.end(), count, val);
    }
    result.resize((result.size() * 8 + 7) / 8, 0);
    return result;
}

} // namespace vovqa::incremental
