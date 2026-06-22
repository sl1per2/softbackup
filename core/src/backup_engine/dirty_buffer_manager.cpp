#include "backup_engine/dirty_buffer_manager.h"
#include <spdlog/spdlog.h>
#include <random>

namespace obs {

DirtyBufferManager::DirtyBufferManager(const std::string& db_path) {
    int rc = sqlite3_open(db_path.c_str(), &db_);
    if (rc != SQLITE_OK) {
        spdlog::error("DirtyBufferManager: failed to open database '{}': {}", db_path, sqlite3_errmsg(db_));
        return;
    }
    init_db();
    spdlog::info("DirtyBufferManager initialized: {}", db_path);
}

DirtyBufferManager::~DirtyBufferManager() {
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

void DirtyBufferManager::init_db() {
    ensure_table("dirty_buffers",
        "CREATE TABLE IF NOT EXISTS dirty_buffers ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  job_id TEXT NOT NULL,"
        "  source_path TEXT NOT NULL,"
        "  block_offset INTEGER NOT NULL,"
        "  block_size INTEGER NOT NULL,"
        "  lba INTEGER NOT NULL DEFAULT 0,"
        "  reason INTEGER NOT NULL DEFAULT 0,"
        "  dirty_at TEXT NOT NULL,"
        "  checksum TEXT DEFAULT '',"
        "  original_hash TEXT DEFAULT '',"
        "  is_backed_up INTEGER DEFAULT 0"
        ")");
    ensure_table("dirty_buffer_stats",
        "CREATE TABLE IF NOT EXISTS dirty_buffer_stats ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  job_id TEXT NOT NULL,"
        "  total_blocks_scanned INTEGER NOT NULL,"
        "  dirty_blocks_count INTEGER NOT NULL,"
        "  dirty_bytes INTEGER NOT NULL,"
        "  clean_bytes INTEGER NOT NULL,"
        "  scan_time_ms INTEGER NOT NULL,"
        "  recorded_at TEXT NOT NULL,"
        "  dirty_ratio REAL NOT NULL"
        ")");

    const char* indexes[] = {
        "CREATE INDEX IF NOT EXISTS idx_db_job ON dirty_buffers(job_id)",
        "CREATE INDEX IF NOT EXISTS idx_db_path ON dirty_buffers(source_path)",
        "CREATE INDEX IF NOT EXISTS idx_db_backed ON dirty_buffers(is_backed_up)",
        "CREATE INDEX IF NOT EXISTS idx_db_dirty_at ON dirty_buffers(dirty_at)",
        "CREATE INDEX IF NOT EXISTS idx_dbs_job ON dirty_buffer_stats(job_id)",
    };
    for (auto* sql : indexes) {
        sqlite3_exec(db_, sql, nullptr, nullptr, nullptr);
    }
}

void DirtyBufferManager::ensure_table(const char* /*name*/, const char* create_sql) {
    char* err = nullptr;
    sqlite3_exec(db_, create_sql, nullptr, nullptr, &err);
    if (err) {
        spdlog::error("DirtyBufferManager: table creation error: {}", err);
        sqlite3_free(err);
    }
}

void DirtyBufferManager::record_dirty_block(const DirtyBlock& block) {
    std::lock_guard<std::mutex> lock(db_mutex_);
    const char* sql =
        "INSERT INTO dirty_buffers (job_id, source_path, block_offset, block_size, lba, "
        "reason, dirty_at, checksum, original_hash, is_backed_up) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, 0)";

    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, block.job_id.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, block.source_path.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 3, block.block_offset);
    sqlite3_bind_int64(stmt, 4, block.block_size);
    sqlite3_bind_int64(stmt, 5, block.lba);
    sqlite3_bind_int(stmt, 6, static_cast<int>(block.reason));
    sqlite3_bind_text(stmt, 7, block.dirty_at.empty() ? current_time_string().c_str() : block.dirty_at.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 8, block.checksum.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 9, block.original_hash.c_str(), -1, SQLITE_STATIC);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

void DirtyBufferManager::record_dirty_blocks_batch(const std::vector<DirtyBlock>& blocks) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    sqlite3_exec(db_, "BEGIN TRANSACTION", nullptr, nullptr, nullptr);

    const char* sql =
        "INSERT INTO dirty_buffers (job_id, source_path, block_offset, block_size, lba, "
        "reason, dirty_at, checksum, original_hash, is_backed_up) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, 0)";

    std::string ts = current_time_string();
    for (const auto& block : blocks) {
        sqlite3_stmt* stmt = nullptr;
        sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
        sqlite3_bind_text(stmt, 1, block.job_id.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, block.source_path.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int64(stmt, 3, block.block_offset);
        sqlite3_bind_int64(stmt, 4, block.block_size);
        sqlite3_bind_int64(stmt, 5, block.lba);
        sqlite3_bind_int(stmt, 6, static_cast<int>(block.reason));
        sqlite3_bind_text(stmt, 7, block.dirty_at.empty() ? ts.c_str() : block.dirty_at.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 8, block.checksum.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 9, block.original_hash.c_str(), -1, SQLITE_STATIC);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }

    sqlite3_exec(db_, "COMMIT", nullptr, nullptr, nullptr);
}

void DirtyBufferManager::record_from_bitmap(const std::string& job_id,
                                             const std::string& source_path,
                                             const std::vector<bool>& bitmap,
                                             uint32_t block_size) {
    std::vector<DirtyBlock> blocks;
    blocks.reserve(bitmap.size() / 10);

    std::string ts = current_time_string();
    for (uint64_t i = 0; i < bitmap.size(); i++) {
        if (bitmap[i]) {
            DirtyBlock block;
            block.job_id = job_id;
            block.source_path = source_path;
            block.block_offset = i;
            block.block_size = block_size;
            block.lba = i * block_size;
            block.reason = DirtyReason::WRITE;
            block.dirty_at = ts;
            blocks.push_back(std::move(block));
        }
    }

    spdlog::info("DirtyBufferManager: recording {} dirty blocks from bitmap for job {}",
                 blocks.size(), job_id);
    record_dirty_blocks_batch(blocks);
}

std::vector<DirtyBlock> DirtyBufferManager::get_dirty_blocks(const std::string& job_id) const {
    std::lock_guard<std::mutex> lock(db_mutex_);
    std::vector<DirtyBlock> results;

    const char* sql = "SELECT id, job_id, source_path, block_offset, block_size, lba, "
                      "reason, dirty_at, checksum, original_hash, is_backed_up "
                      "FROM dirty_buffers WHERE job_id = ? ORDER BY block_offset";

    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, job_id.c_str(), -1, SQLITE_STATIC);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        DirtyBlock block;
        block.id = sqlite3_column_int64(stmt, 0);
        block.job_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        block.source_path = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        block.block_offset = sqlite3_column_int64(stmt, 3);
        block.block_size = sqlite3_column_int64(stmt, 4);
        block.lba = sqlite3_column_int64(stmt, 5);
        block.reason = static_cast<DirtyReason>(sqlite3_column_int(stmt, 6));
        auto dirty_at_ptr = sqlite3_column_text(stmt, 7);
        block.dirty_at = dirty_at_ptr ? reinterpret_cast<const char*>(dirty_at_ptr) : "";
        auto checksum_ptr = sqlite3_column_text(stmt, 8);
        block.checksum = checksum_ptr ? reinterpret_cast<const char*>(checksum_ptr) : "";
        auto hash_ptr = sqlite3_column_text(stmt, 9);
        block.original_hash = hash_ptr ? reinterpret_cast<const char*>(hash_ptr) : "";
        block.is_backed_up = sqlite3_column_int(stmt, 10) != 0;
        results.push_back(std::move(block));
    }
    sqlite3_finalize(stmt);
    return results;
}

std::vector<DirtyBlock> DirtyBufferManager::get_dirty_blocks_since(
    const std::string& source_path, const std::string& since) const {
    std::lock_guard<std::mutex> lock(db_mutex_);
    std::vector<DirtyBlock> results;

    const char* sql = "SELECT id, job_id, source_path, block_offset, block_size, lba, "
                      "reason, dirty_at, checksum, original_hash, is_backed_up "
                      "FROM dirty_buffers WHERE source_path = ? AND dirty_at >= ? "
                      "ORDER BY dirty_at DESC, block_offset";

    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, source_path.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, since.c_str(), -1, SQLITE_STATIC);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        DirtyBlock block;
        block.id = sqlite3_column_int64(stmt, 0);
        block.job_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        block.source_path = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        block.block_offset = sqlite3_column_int64(stmt, 3);
        block.block_size = sqlite3_column_int64(stmt, 4);
        block.lba = sqlite3_column_int64(stmt, 5);
        block.reason = static_cast<DirtyReason>(sqlite3_column_int(stmt, 6));
        auto dirty_at_ptr = sqlite3_column_text(stmt, 7);
        block.dirty_at = dirty_at_ptr ? reinterpret_cast<const char*>(dirty_at_ptr) : "";
        auto checksum_ptr = sqlite3_column_text(stmt, 8);
        block.checksum = checksum_ptr ? reinterpret_cast<const char*>(checksum_ptr) : "";
        auto hash_ptr = sqlite3_column_text(stmt, 9);
        block.original_hash = hash_ptr ? reinterpret_cast<const char*>(hash_ptr) : "";
        block.is_backed_up = sqlite3_column_int(stmt, 10) != 0;
        results.push_back(std::move(block));
    }
    sqlite3_finalize(stmt);
    return results;
}

std::vector<DirtyBlock> DirtyBufferManager::get_unbacked_blocks(const std::string& job_id) const {
    std::lock_guard<std::mutex> lock(db_mutex_);
    std::vector<DirtyBlock> results;

    const char* sql = "SELECT id, job_id, source_path, block_offset, block_size, lba, "
                      "reason, dirty_at, checksum, original_hash, is_backed_up "
                      "FROM dirty_buffers WHERE job_id = ? AND is_backed_up = 0 "
                      "ORDER BY block_offset";

    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, job_id.c_str(), -1, SQLITE_STATIC);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        DirtyBlock block;
        block.id = sqlite3_column_int64(stmt, 0);
        block.job_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        block.source_path = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        block.block_offset = sqlite3_column_int64(stmt, 3);
        block.block_size = sqlite3_column_int64(stmt, 4);
        block.lba = sqlite3_column_int64(stmt, 5);
        block.reason = static_cast<DirtyReason>(sqlite3_column_int(stmt, 6));
        auto dirty_at_ptr = sqlite3_column_text(stmt, 7);
        block.dirty_at = dirty_at_ptr ? reinterpret_cast<const char*>(dirty_at_ptr) : "";
        auto checksum_ptr = sqlite3_column_text(stmt, 8);
        block.checksum = checksum_ptr ? reinterpret_cast<const char*>(checksum_ptr) : "";
        auto hash_ptr = sqlite3_column_text(stmt, 9);
        block.original_hash = hash_ptr ? reinterpret_cast<const char*>(hash_ptr) : "";
        block.is_backed_up = sqlite3_column_int(stmt, 10) != 0;
        results.push_back(std::move(block));
    }
    sqlite3_finalize(stmt);
    return results;
}

DirtyBufferStats DirtyBufferManager::get_stats(const std::string& job_id) const {
    std::lock_guard<std::mutex> lock(db_mutex_);
    DirtyBufferStats stats;
    stats.job_id = job_id;

    const char* sql =
        "SELECT COUNT(*), COALESCE(SUM(block_size), 0), "
        "MIN(dirty_at), MAX(dirty_at) "
        "FROM dirty_buffers WHERE job_id = ?";

    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, job_id.c_str(), -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        stats.dirty_blocks_count = sqlite3_column_int64(stmt, 0);
        stats.dirty_bytes = sqlite3_column_int64(stmt, 1);
    }
    sqlite3_finalize(stmt);

    stats.recorded_at = current_time_string();
    if (stats.total_blocks_scanned > 0) {
        stats.dirty_ratio = static_cast<double>(stats.dirty_blocks_count) / stats.total_blocks_scanned;
    }
    return stats;
}

void DirtyBufferManager::record_stats(const DirtyBufferStats& stats) {
    std::lock_guard<std::mutex> lock(db_mutex_);
    const char* sql =
        "INSERT INTO dirty_buffer_stats (job_id, total_blocks_scanned, dirty_blocks_count, "
        "dirty_bytes, clean_bytes, scan_time_ms, recorded_at, dirty_ratio) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?)";

    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, stats.job_id.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 2, stats.total_blocks_scanned);
    sqlite3_bind_int64(stmt, 3, stats.dirty_blocks_count);
    sqlite3_bind_int64(stmt, 4, stats.dirty_bytes);
    sqlite3_bind_int64(stmt, 5, stats.clean_bytes);
    sqlite3_bind_int64(stmt, 6, stats.scan_time_ms);
    sqlite3_bind_text(stmt, 7, stats.recorded_at.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_double(stmt, 8, stats.dirty_ratio);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

void DirtyBufferManager::mark_backed_up(const std::string& job_id) {
    std::lock_guard<std::mutex> lock(db_mutex_);
    const char* sql = "UPDATE dirty_buffers SET is_backed_up = 1 WHERE job_id = ?";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, job_id.c_str(), -1, SQLITE_STATIC);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    spdlog::info("DirtyBufferManager: marked all blocks as backed up for job {}", job_id);
}

void DirtyBufferManager::mark_block_backed_up(int64_t block_id) {
    std::lock_guard<std::mutex> lock(db_mutex_);
    const char* sql = "UPDATE dirty_buffers SET is_backed_up = 1 WHERE id = ?";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    sqlite3_bind_int64(stmt, 1, block_id);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

std::vector<HotspotEntry> DirtyBufferManager::get_hotspots(int min_change_count) const {
    std::lock_guard<std::mutex> lock(db_mutex_);
    std::vector<HotspotEntry> results;

    const char* sql =
        "SELECT source_path, block_offset, COUNT(*) as change_count, "
        "MIN(dirty_at) as first_seen, MAX(dirty_at) as last_seen "
        "FROM dirty_buffers "
        "GROUP BY source_path, block_offset "
        "HAVING change_count >= ? "
        "ORDER BY change_count DESC "
        "LIMIT 100";

    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, min_change_count);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        HotspotEntry entry;
        entry.source_path = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        entry.block_offset = sqlite3_column_int64(stmt, 1);
        entry.change_count = sqlite3_column_int(stmt, 2);
        auto first_ptr = sqlite3_column_text(stmt, 3);
        entry.first_seen = first_ptr ? reinterpret_cast<const char*>(first_ptr) : "";
        auto last_ptr = sqlite3_column_text(stmt, 4);
        entry.last_seen = last_ptr ? reinterpret_cast<const char*>(last_ptr) : "";
        results.push_back(std::move(entry));
    }
    sqlite3_finalize(stmt);
    return results;
}

std::vector<ChangeFrequency> DirtyBufferManager::get_change_frequency(int hours) const {
    std::lock_guard<std::mutex> lock(db_mutex_);
    std::vector<ChangeFrequency> results;

    const char* sql =
        "SELECT strftime('%Y-%m-%d %H:00:00', dirty_at) as time_bucket, "
        "COUNT(*) as change_count, COALESCE(SUM(block_size), 0) as bytes_changed "
        "FROM dirty_buffers "
        "WHERE dirty_at >= datetime('now', ? || ' hours') "
        "GROUP BY time_bucket "
        "ORDER BY time_bucket";

    std::string hours_param = "-" + std::to_string(hours);
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, hours_param.c_str(), -1, SQLITE_STATIC);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        ChangeFrequency freq;
        auto bucket_ptr = sqlite3_column_text(stmt, 0);
        freq.time_bucket = bucket_ptr ? reinterpret_cast<const char*>(bucket_ptr) : "";
        freq.change_count = sqlite3_column_int64(stmt, 1);
        freq.bytes_changed = sqlite3_column_int64(stmt, 2);
        results.push_back(std::move(freq));
    }
    sqlite3_finalize(stmt);
    return results;
}

int64_t DirtyBufferManager::get_total_dirty_blocks(const std::string& source_path) const {
    std::lock_guard<std::mutex> lock(db_mutex_);
    const char* sql = "SELECT COUNT(DISTINCT block_offset) FROM dirty_buffers WHERE source_path = ?";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, source_path.c_str(), -1, SQLITE_STATIC);
    int64_t count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = sqlite3_column_int64(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return count;
}

void DirtyBufferManager::cleanup_old_records(int retention_days) {
    std::lock_guard<std::mutex> lock(db_mutex_);
    std::string days_param = "-" + std::to_string(retention_days);

    const char* sql = "DELETE FROM dirty_buffers WHERE dirty_at < datetime('now', ?)";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, days_param.c_str(), -1, SQLITE_STATIC);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    int64_t deleted = sqlite3_changes(db_);
    if (deleted > 0) {
        spdlog::info("DirtyBufferManager: cleaned up {} records older than {} days", deleted, retention_days);
    }
}

void DirtyBufferManager::vacuum() {
    std::lock_guard<std::mutex> lock(db_mutex_);
    sqlite3_exec(db_, "VACUUM", nullptr, nullptr, nullptr);
    spdlog::info("DirtyBufferManager: vacuum completed");
}

} // namespace obs
