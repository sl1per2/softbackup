#include "backup_engine/dirty_buffer_logger.h"
#include <spdlog/spdlog.h>
#include <curl/curl.h>
#include <sstream>

namespace obs {

static size_t curl_write_cb(void* ptr, size_t size, size_t nmemb, std::string* data) {
    data->append(static_cast<char*>(ptr), size * nmemb);
    return size * nmemb;
}

DirtyBufferLogger::DirtyBufferLogger(const std::string& db_path,
                                     const std::string& server_url)
    : server_url_(server_url) {
    int rc = sqlite3_open(db_path.c_str(), &db_);
    if (rc != SQLITE_OK) {
        spdlog::error("DirtyBufferLogger: cannot open db '{}': {}", db_path, sqlite3_errmsg(db_));
        return;
    }
    init_db();
    initialized_ = true;
    spdlog::info("DirtyBufferLogger: initialized (db={}, server={})", db_path,
                 server_url.empty() ? "none" : server_url);
}

DirtyBufferLogger::~DirtyBufferLogger() {
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

void DirtyBufferLogger::init_db() {
    const char* sql =
        "CREATE TABLE IF NOT EXISTS dirty_buffer_logs ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  backup_job_id TEXT NOT NULL,"
        "  agent_id TEXT NOT NULL,"
        "  plugin_name TEXT NOT NULL,"
        "  before_page_count INTEGER NOT NULL DEFAULT 0,"
        "  before_size_bytes INTEGER NOT NULL DEFAULT 0,"
        "  before_percent_ram REAL NOT NULL DEFAULT 0.0,"
        "  after_page_count INTEGER NOT NULL DEFAULT 0,"
        "  after_size_bytes INTEGER NOT NULL DEFAULT 0,"
        "  after_percent_ram REAL NOT NULL DEFAULT 0.0,"
        "  flush_status TEXT NOT NULL DEFAULT 'not_flushed',"
        "  flush_duration_ms INTEGER NOT NULL DEFAULT 0,"
        "  error_message TEXT DEFAULT '',"
        "  component_details_json TEXT DEFAULT '[]',"
        "  consistency_level TEXT NOT NULL DEFAULT 'crash_consistent',"
        "  is_consistent INTEGER NOT NULL DEFAULT 0,"
        "  total_ram_bytes INTEGER NOT NULL DEFAULT 0,"
        "  buffer_pool_size INTEGER NOT NULL DEFAULT 0,"
        "  flush_started_at TEXT NOT NULL DEFAULT '',"
        "  flush_finished_at TEXT NOT NULL DEFAULT '',"
        "  created_at TEXT NOT NULL DEFAULT ''"
        ")";

    char* err = nullptr;
    sqlite3_exec(db_, sql, nullptr, nullptr, &err);
    if (err) {
        spdlog::error("DirtyBufferLogger: init_db error: {}", err);
        sqlite3_free(err);
    }

    const char* indexes[] = {
        "CREATE INDEX IF NOT EXISTS idx_dbl_job ON dirty_buffer_logs(backup_job_id)",
        "CREATE INDEX IF NOT EXISTS idx_dbl_plugin ON dirty_buffer_logs(plugin_name)",
        "CREATE INDEX IF NOT EXISTS idx_dbl_status ON dirty_buffer_logs(flush_status)",
        "CREATE INDEX IF NOT EXISTS idx_dbl_created ON dirty_buffer_logs(created_at)",
    };
    for (auto* idx : indexes) {
        sqlite3_exec(db_, idx, nullptr, nullptr, nullptr);
    }
}

FlushResult DirtyBufferLogger::flush_and_log(const std::string& job_id,
                                              const std::string& agent_id,
                                              IBufferFlusher* flusher) {
    FlushResult result;
    result.job_id = job_id;
    result.agent_id = agent_id;
    result.plugin_name = flusher ? flusher->plugin_name() : "unknown";
    result.created_at = current_time_string();

    if (!flusher) {
        result.status = FlushStatus::FAILED;
        result.error_message = "No flusher provided";
        spdlog::error("[DirtyBuffer] Job {}: no flusher plugin", job_id);
        save_to_db(result);
        return result;
    }

    spdlog::info("============================================================");
    spdlog::info("[DirtyBuffer] Job {} | Plugin: {} | Agent: {}",
                 job_id, result.plugin_name, agent_id);
    spdlog::info("============================================================");

    result.total_ram_bytes = flusher->get_total_ram();
    result.buffer_pool_size = flusher->get_buffer_pool_size();

    spdlog::info("[DirtyBuffer]  Step 1/6: Capturing BEFORE state");
    result.before = flusher->query_dirty_state();
    result.flush_started_at = current_time_string();
    spdlog::info("[DirtyBuffer]  Dirty buffers before: {} pages ({}, {:.1f}% of RAM)",
                 result.before.page_count,
                 bytes_to_human(result.before.size_bytes),
                 result.before.percent_of_ram);

    spdlog::info("[DirtyBuffer]  Step 2/6: Quiescing application");
    bool quiesced = flusher->quiesce_application();
    if (!quiesced) {
        spdlog::warn("[DirtyBuffer]  Application quiesce failed, proceeding with best-effort flush");
    }

    spdlog::info("[DirtyBuffer]  Step 3/6: Flushing dirty buffers to disk");
    auto t0 = std::chrono::steady_clock::now();

    bool flush_ok = false;
    try {
        flush_ok = flusher->flush_dirty_buffers();
    } catch (const std::exception& e) {
        result.error_message = e.what();
        spdlog::error("[DirtyBuffer]  Flush exception: {}", e.what());
    }

    auto t1 = std::chrono::steady_clock::now();
    result.flush_duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();

    if (flush_ok) {
        result.status = FlushStatus::FLUSHED;
        spdlog::info("[DirtyBuffer]  Flushed {} pages in {:.2f} seconds",
                     result.before.page_count, result.flush_duration_ms / 1000.0);
    } else {
        result.status = result.error_message.empty() ? FlushStatus::FAILED : FlushStatus::FAILED;
        if (result.error_message.empty()) {
            result.error_message = "flush_dirty_buffers() returned false";
        }
        spdlog::error("[DirtyBuffer]  Flush FAILED: {}", result.error_message);
    }

    spdlog::info("[DirtyBuffer]  Step 4/6: Unquiescing application");
    flusher->unquiesce_application();

    spdlog::info("[DirtyBuffer]  Step 5/6: Capturing AFTER state");
    result.after = flusher->query_dirty_state();
    result.flush_finished_at = current_time_string();
    spdlog::info("[DirtyBuffer]  Dirty buffers after: {} pages ({}, {:.1f}% of RAM)",
                 result.after.page_count,
                 bytes_to_human(result.after.size_bytes),
                 result.after.percent_of_ram);

    spdlog::info("[DirtyBuffer]  Step 6/6: Evaluating consistency");
    result.component_details = flusher->get_component_details();

    int64_t pages_flushed = result.before.page_count - result.after.page_count;
    if (result.status == FlushStatus::FLUSHED && pages_flushed > 0 &&
        result.after.page_count == 0) {
        result.consistency = ConsistencyLevel::APPLICATION_CONSISTENT;
        result.is_consistent = true;
        spdlog::info("[DirtyBuffer]  Consistency: APPLICATION_CONSISTENT (all dirty pages flushed)");
    } else if (result.status == FlushStatus::FLUSHED && pages_flushed > 0) {
        result.consistency = ConsistencyLevel::FILE_CONSISTENT;
        result.is_consistent = false;
        spdlog::warn("[DirtyBuffer]  Consistency: FILE_CONSISTENT ({} pages remain dirty)",
                     result.after.page_count);
    } else if (result.status == FlushStatus::FLUSHED) {
        result.consistency = ConsistencyLevel::APPLICATION_CONSISTENT;
        result.is_consistent = true;
        spdlog::info("[DirtyBuffer]  Consistency: APPLICATION_CONSISTENT (no dirty pages detected)");
    } else {
        result.consistency = ConsistencyLevel::CRASH_CONSISTENT;
        result.is_consistent = false;
        spdlog::warn("[DirtyBuffer]  Consistency: CRASH_CONSISTENT (flush did not complete)");
    }

    spdlog::info("[DirtyBuffer] ---------------------------------------------------------");
    spdlog::info("[DirtyBuffer] SUMMARY: {} | Status: {} | Duration: {} ms",
                 result.plugin_name, to_string(result.status), result.flush_duration_ms);
    spdlog::info("[DirtyBuffer]   Before:  {} pages ({})",
                 result.before.page_count, bytes_to_human(result.before.size_bytes));
    spdlog::info("[DirtyBuffer]   After:   {} pages ({})",
                 result.after.page_count, bytes_to_human(result.after.size_bytes));
    spdlog::info("[DirtyBuffer]   Flushed: {} pages ({})",
                 pages_flushed, bytes_to_human(result.before.size_bytes - result.after.size_bytes));
    spdlog::info("[DirtyBuffer]   Consistency: {} (consistent={})",
                 to_string(result.consistency), result.is_consistent);
    if (!result.component_details.empty()) {
        spdlog::info("[DirtyBuffer]   Components:");
        for (const auto& c : result.component_details) {
            spdlog::info("[DirtyBuffer]     - {}: {} pages ({})",
                         c.name, c.dirty_pages, bytes_to_human(c.size_bytes));
        }
    }
    spdlog::info("============================================================");

    save_to_db(result);
    send_to_server(result);

    return result;
}

void DirtyBufferLogger::save_to_db(const FlushResult& result) {
    if (!initialized_) return;
    std::lock_guard<std::mutex> lock(db_mutex_);

    const char* sql =
        "INSERT INTO dirty_buffer_logs ("
        "  backup_job_id, agent_id, plugin_name,"
        "  before_page_count, before_size_bytes, before_percent_ram,"
        "  after_page_count, after_size_bytes, after_percent_ram,"
        "  flush_status, flush_duration_ms, error_message,"
        "  component_details_json, consistency_level, is_consistent,"
        "  total_ram_bytes, buffer_pool_size,"
        "  flush_started_at, flush_finished_at, created_at"
        ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);

    sqlite3_bind_text(stmt, 1, result.job_id.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, result.agent_id.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, result.plugin_name.c_str(), -1, SQLITE_STATIC);

    sqlite3_bind_int64(stmt, 4, result.before.page_count);
    sqlite3_bind_int64(stmt, 5, result.before.size_bytes);
    sqlite3_bind_double(stmt, 6, result.before.percent_of_ram);

    sqlite3_bind_int64(stmt, 7, result.after.page_count);
    sqlite3_bind_int64(stmt, 8, result.after.size_bytes);
    sqlite3_bind_double(stmt, 9, result.after.percent_of_ram);

    sqlite3_bind_text(stmt, 10, to_string(result.status), -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 11, result.flush_duration_ms);
    sqlite3_bind_text(stmt, 12, result.error_message.c_str(), -1, SQLITE_STATIC);

    std::string details_json = components_to_json(result.component_details);
    sqlite3_bind_text(stmt, 13, details_json.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 14, to_string(result.consistency), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 15, result.is_consistent ? 1 : 0);

    sqlite3_bind_int64(stmt, 16, result.total_ram_bytes);
    sqlite3_bind_int64(stmt, 17, result.buffer_pool_size);

    sqlite3_bind_text(stmt, 18, result.flush_started_at.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 19, result.flush_finished_at.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 20, result.created_at.c_str(), -1, SQLITE_STATIC);

    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    spdlog::info("[DirtyBuffer] Log saved to database (job={})", result.job_id);
}

std::vector<DirtyBufferLogEntry> DirtyBufferLogger::get_logs(const std::string& job_id) const {
    std::lock_guard<std::mutex> lock(db_mutex_);
    std::vector<DirtyBufferLogEntry> results;

    const char* sql =
        "SELECT id, backup_job_id, agent_id, plugin_name,"
        "before_page_count, before_size_bytes, before_percent_ram,"
        "after_page_count, after_size_bytes, after_percent_ram,"
        "flush_status, flush_duration_ms, error_message,"
        "component_details_json, consistency_level, is_consistent,"
        "total_ram_bytes, buffer_pool_size,"
        "flush_started_at, flush_finished_at, created_at "
        "FROM dirty_buffer_logs WHERE backup_job_id = ? ORDER BY id";

    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, job_id.c_str(), -1, SQLITE_STATIC);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        DirtyBufferLogEntry e;
        e.id = sqlite3_column_int64(stmt, 0);
        auto p1 = sqlite3_column_text(stmt, 1); e.backup_job_id = p1 ? reinterpret_cast<const char*>(p1) : "";
        auto p2 = sqlite3_column_text(stmt, 2); e.agent_id = p2 ? reinterpret_cast<const char*>(p2) : "";
        auto p3 = sqlite3_column_text(stmt, 3); e.plugin_name = p3 ? reinterpret_cast<const char*>(p3) : "";
        e.before_page_count = sqlite3_column_int64(stmt, 4);
        e.before_size_bytes = sqlite3_column_int64(stmt, 5);
        e.before_percent_ram = sqlite3_column_double(stmt, 6);
        e.after_page_count = sqlite3_column_int64(stmt, 7);
        e.after_size_bytes = sqlite3_column_int64(stmt, 8);
        e.after_percent_ram = sqlite3_column_double(stmt, 9);
        auto p10 = sqlite3_column_text(stmt, 10); e.flush_status = p10 ? reinterpret_cast<const char*>(p10) : "";
        e.flush_duration_ms = sqlite3_column_int64(stmt, 11);
        auto p12 = sqlite3_column_text(stmt, 12); e.error_message = p12 ? reinterpret_cast<const char*>(p12) : "";
        auto p13 = sqlite3_column_text(stmt, 13); e.component_details_json = p13 ? reinterpret_cast<const char*>(p13) : "[]";
        auto p14 = sqlite3_column_text(stmt, 14); e.consistency_level = p14 ? reinterpret_cast<const char*>(p14) : "";
        e.is_consistent = sqlite3_column_int(stmt, 15) != 0;
        e.total_ram_bytes = sqlite3_column_int64(stmt, 16);
        e.buffer_pool_size = sqlite3_column_int64(stmt, 17);
        auto p18 = sqlite3_column_text(stmt, 18); e.flush_started_at = p18 ? reinterpret_cast<const char*>(p18) : "";
        auto p19 = sqlite3_column_text(stmt, 19); e.flush_finished_at = p19 ? reinterpret_cast<const char*>(p19) : "";
        auto p20 = sqlite3_column_text(stmt, 20); e.created_at = p20 ? reinterpret_cast<const char*>(p20) : "";
        results.push_back(std::move(e));
    }
    sqlite3_finalize(stmt);
    return results;
}

std::vector<DirtyBufferLogEntry> DirtyBufferLogger::get_all_logs(int limit) const {
    std::lock_guard<std::mutex> lock(db_mutex_);
    std::vector<DirtyBufferLogEntry> results;

    std::string query =
        "SELECT id, backup_job_id, agent_id, plugin_name,"
        "before_page_count, before_size_bytes, before_percent_ram,"
        "after_page_count, after_size_bytes, after_percent_ram,"
        "flush_status, flush_duration_ms, error_message,"
        "component_details_json, consistency_level, is_consistent,"
        "total_ram_bytes, buffer_pool_size,"
        "flush_started_at, flush_finished_at, created_at "
        "FROM dirty_buffer_logs ORDER BY id DESC LIMIT " + std::to_string(limit);

    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, query.c_str(), -1, &stmt, nullptr);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        DirtyBufferLogEntry e;
        e.id = sqlite3_column_int64(stmt, 0);
        auto p1 = sqlite3_column_text(stmt, 1); e.backup_job_id = p1 ? reinterpret_cast<const char*>(p1) : "";
        auto p2 = sqlite3_column_text(stmt, 2); e.agent_id = p2 ? reinterpret_cast<const char*>(p2) : "";
        auto p3 = sqlite3_column_text(stmt, 3); e.plugin_name = p3 ? reinterpret_cast<const char*>(p3) : "";
        e.before_page_count = sqlite3_column_int64(stmt, 4);
        e.before_size_bytes = sqlite3_column_int64(stmt, 5);
        e.before_percent_ram = sqlite3_column_double(stmt, 6);
        e.after_page_count = sqlite3_column_int64(stmt, 7);
        e.after_size_bytes = sqlite3_column_int64(stmt, 8);
        e.after_percent_ram = sqlite3_column_double(stmt, 9);
        auto p10 = sqlite3_column_text(stmt, 10); e.flush_status = p10 ? reinterpret_cast<const char*>(p10) : "";
        e.flush_duration_ms = sqlite3_column_int64(stmt, 11);
        auto p12 = sqlite3_column_text(stmt, 12); e.error_message = p12 ? reinterpret_cast<const char*>(p12) : "";
        auto p13 = sqlite3_column_text(stmt, 13); e.component_details_json = p13 ? reinterpret_cast<const char*>(p13) : "[]";
        auto p14 = sqlite3_column_text(stmt, 14); e.consistency_level = p14 ? reinterpret_cast<const char*>(p14) : "";
        e.is_consistent = sqlite3_column_int(stmt, 15) != 0;
        e.total_ram_bytes = sqlite3_column_int64(stmt, 16);
        e.buffer_pool_size = sqlite3_column_int64(stmt, 17);
        auto p18 = sqlite3_column_text(stmt, 18); e.flush_started_at = p18 ? reinterpret_cast<const char*>(p18) : "";
        auto p19 = sqlite3_column_text(stmt, 19); e.flush_finished_at = p19 ? reinterpret_cast<const char*>(p19) : "";
        auto p20 = sqlite3_column_text(stmt, 20); e.created_at = p20 ? reinterpret_cast<const char*>(p20) : "";
        results.push_back(std::move(e));
    }
    sqlite3_finalize(stmt);
    return results;
}

DirtyBufferLogEntry DirtyBufferLogger::get_latest_log(const std::string& plugin_name) const {
    std::lock_guard<std::mutex> lock(db_mutex_);
    DirtyBufferLogEntry e;

    const char* sql =
        "SELECT id, backup_job_id, agent_id, plugin_name,"
        "before_page_count, before_size_bytes, before_percent_ram,"
        "after_page_count, after_size_bytes, after_percent_ram,"
        "flush_status, flush_duration_ms, error_message,"
        "component_details_json, consistency_level, is_consistent,"
        "total_ram_bytes, buffer_pool_size,"
        "flush_started_at, flush_finished_at, created_at "
        "FROM dirty_buffer_logs WHERE plugin_name = ? ORDER BY id DESC LIMIT 1";

    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, plugin_name.c_str(), -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        e.id = sqlite3_column_int64(stmt, 0);
        auto p1 = sqlite3_column_text(stmt, 1); e.backup_job_id = p1 ? reinterpret_cast<const char*>(p1) : "";
        auto p2 = sqlite3_column_text(stmt, 2); e.agent_id = p2 ? reinterpret_cast<const char*>(p2) : "";
        auto p3 = sqlite3_column_text(stmt, 3); e.plugin_name = p3 ? reinterpret_cast<const char*>(p3) : "";
        e.before_page_count = sqlite3_column_int64(stmt, 4);
        e.before_size_bytes = sqlite3_column_int64(stmt, 5);
        e.before_percent_ram = sqlite3_column_double(stmt, 6);
        e.after_page_count = sqlite3_column_int64(stmt, 7);
        e.after_size_bytes = sqlite3_column_int64(stmt, 8);
        e.after_percent_ram = sqlite3_column_double(stmt, 9);
        auto p10 = sqlite3_column_text(stmt, 10); e.flush_status = p10 ? reinterpret_cast<const char*>(p10) : "";
        e.flush_duration_ms = sqlite3_column_int64(stmt, 11);
        auto p12 = sqlite3_column_text(stmt, 12); e.error_message = p12 ? reinterpret_cast<const char*>(p12) : "";
        auto p13 = sqlite3_column_text(stmt, 13); e.component_details_json = p13 ? reinterpret_cast<const char*>(p13) : "[]";
        auto p14 = sqlite3_column_text(stmt, 14); e.consistency_level = p14 ? reinterpret_cast<const char*>(p14) : "";
        e.is_consistent = sqlite3_column_int(stmt, 15) != 0;
        e.total_ram_bytes = sqlite3_column_int64(stmt, 16);
        e.buffer_pool_size = sqlite3_column_int64(stmt, 17);
        auto p18 = sqlite3_column_text(stmt, 18); e.flush_started_at = p18 ? reinterpret_cast<const char*>(p18) : "";
        auto p19 = sqlite3_column_text(stmt, 19); e.flush_finished_at = p19 ? reinterpret_cast<const char*>(p19) : "";
        auto p20 = sqlite3_column_text(stmt, 20); e.created_at = p20 ? reinterpret_cast<const char*>(p20) : "";
    }
    sqlite3_finalize(stmt);
    return e;
}

std::vector<DirtyBufferLogEntry> DirtyBufferLogger::get_failed_flushes() const {
    std::lock_guard<std::mutex> lock(db_mutex_);
    std::vector<DirtyBufferLogEntry> results;

    const char* sql =
        "SELECT id, backup_job_id, agent_id, plugin_name,"
        "before_page_count, before_size_bytes, before_percent_ram,"
        "after_page_count, after_size_bytes, after_percent_ram,"
        "flush_status, flush_duration_ms, error_message,"
        "component_details_json, consistency_level, is_consistent,"
        "total_ram_bytes, buffer_pool_size,"
        "flush_started_at, flush_finished_at, created_at "
        "FROM dirty_buffer_logs WHERE flush_status IN ('failed', 'timeout') "
        "ORDER BY id DESC";

    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        DirtyBufferLogEntry e;
        e.id = sqlite3_column_int64(stmt, 0);
        auto p1 = sqlite3_column_text(stmt, 1); e.backup_job_id = p1 ? reinterpret_cast<const char*>(p1) : "";
        auto p2 = sqlite3_column_text(stmt, 2); e.agent_id = p2 ? reinterpret_cast<const char*>(p2) : "";
        auto p3 = sqlite3_column_text(stmt, 3); e.plugin_name = p3 ? reinterpret_cast<const char*>(p3) : "";
        e.before_page_count = sqlite3_column_int64(stmt, 4);
        e.before_size_bytes = sqlite3_column_int64(stmt, 5);
        e.before_percent_ram = sqlite3_column_double(stmt, 6);
        e.after_page_count = sqlite3_column_int64(stmt, 7);
        e.after_size_bytes = sqlite3_column_int64(stmt, 8);
        e.after_percent_ram = sqlite3_column_double(stmt, 9);
        auto p10 = sqlite3_column_text(stmt, 10); e.flush_status = p10 ? reinterpret_cast<const char*>(p10) : "";
        e.flush_duration_ms = sqlite3_column_int64(stmt, 11);
        auto p12 = sqlite3_column_text(stmt, 12); e.error_message = p12 ? reinterpret_cast<const char*>(p12) : "";
        auto p13 = sqlite3_column_text(stmt, 13); e.component_details_json = p13 ? reinterpret_cast<const char*>(p13) : "[]";
        auto p14 = sqlite3_column_text(stmt, 14); e.consistency_level = p14 ? reinterpret_cast<const char*>(p14) : "";
        e.is_consistent = sqlite3_column_int(stmt, 15) != 0;
        e.total_ram_bytes = sqlite3_column_int64(stmt, 16);
        e.buffer_pool_size = sqlite3_column_int64(stmt, 17);
        auto p18 = sqlite3_column_text(stmt, 18); e.flush_started_at = p18 ? reinterpret_cast<const char*>(p18) : "";
        auto p19 = sqlite3_column_text(stmt, 19); e.flush_finished_at = p19 ? reinterpret_cast<const char*>(p19) : "";
        auto p20 = sqlite3_column_text(stmt, 20); e.created_at = p20 ? reinterpret_cast<const char*>(p20) : "";
        results.push_back(std::move(e));
    }
    sqlite3_finalize(stmt);
    return results;
}

std::vector<DirtyBufferLogEntry> DirtyBufferLogger::get_inconsistent_backups() const {
    std::lock_guard<std::mutex> lock(db_mutex_);
    std::vector<DirtyBufferLogEntry> results;

    const char* sql =
        "SELECT id, backup_job_id, agent_id, plugin_name,"
        "before_page_count, before_size_bytes, before_percent_ram,"
        "after_page_count, after_size_bytes, after_percent_ram,"
        "flush_status, flush_duration_ms, error_message,"
        "component_details_json, consistency_level, is_consistent,"
        "total_ram_bytes, buffer_pool_size,"
        "flush_started_at, flush_finished_at, created_at "
        "FROM dirty_buffer_logs WHERE is_consistent = 0 "
        "ORDER BY id DESC";

    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        DirtyBufferLogEntry e;
        e.id = sqlite3_column_int64(stmt, 0);
        auto p1 = sqlite3_column_text(stmt, 1); e.backup_job_id = p1 ? reinterpret_cast<const char*>(p1) : "";
        auto p2 = sqlite3_column_text(stmt, 2); e.agent_id = p2 ? reinterpret_cast<const char*>(p2) : "";
        auto p3 = sqlite3_column_text(stmt, 3); e.plugin_name = p3 ? reinterpret_cast<const char*>(p3) : "";
        e.before_page_count = sqlite3_column_int64(stmt, 4);
        e.before_size_bytes = sqlite3_column_int64(stmt, 5);
        e.before_percent_ram = sqlite3_column_double(stmt, 6);
        e.after_page_count = sqlite3_column_int64(stmt, 7);
        e.after_size_bytes = sqlite3_column_int64(stmt, 8);
        e.after_percent_ram = sqlite3_column_double(stmt, 9);
        auto p10 = sqlite3_column_text(stmt, 10); e.flush_status = p10 ? reinterpret_cast<const char*>(p10) : "";
        e.flush_duration_ms = sqlite3_column_int64(stmt, 11);
        auto p12 = sqlite3_column_text(stmt, 12); e.error_message = p12 ? reinterpret_cast<const char*>(p12) : "";
        auto p13 = sqlite3_column_text(stmt, 13); e.component_details_json = p13 ? reinterpret_cast<const char*>(p13) : "[]";
        auto p14 = sqlite3_column_text(stmt, 14); e.consistency_level = p14 ? reinterpret_cast<const char*>(p14) : "";
        e.is_consistent = sqlite3_column_int(stmt, 15) != 0;
        e.total_ram_bytes = sqlite3_column_int64(stmt, 16);
        e.buffer_pool_size = sqlite3_column_int64(stmt, 17);
        auto p18 = sqlite3_column_text(stmt, 18); e.flush_started_at = p18 ? reinterpret_cast<const char*>(p18) : "";
        auto p19 = sqlite3_column_text(stmt, 19); e.flush_finished_at = p19 ? reinterpret_cast<const char*>(p19) : "";
        auto p20 = sqlite3_column_text(stmt, 20); e.created_at = p20 ? reinterpret_cast<const char*>(p20) : "";
        results.push_back(std::move(e));
    }
    sqlite3_finalize(stmt);
    return results;
}

bool DirtyBufferLogger::send_to_server(const FlushResult& result) {
    if (server_url_.empty()) {
        spdlog::debug("[DirtyBuffer] No server URL configured, skipping send");
        return false;
    }

    std::string json = build_json(result);
    std::string url = server_url_ + "/api/dirty-buffers/log";

    CURL* curl = curl_easy_init();
    if (!curl) return false;

    std::string response;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res == CURLE_OK) {
        spdlog::info("[DirtyBuffer] Report sent to server (job={})", result.job_id);
        return true;
    } else {
        spdlog::warn("[DirtyBuffer] Failed to send report to server: {}",
                     curl_easy_strerror(res));
        return false;
    }
}

std::string DirtyBufferLogger::build_json(const FlushResult& result) const {
    std::ostringstream j;
    j << "{";
    j << R"("backup_job_id":")" << result.job_id << R"(",)";
    j << R"("agent_id":")" << result.agent_id << R"(",)";
    j << R"("plugin_name":")" << result.plugin_name << R"(",)";

    j << R"("before":{)";
    j << R"("page_count":)" << result.before.page_count << ",";
    j << R"("size_bytes":)" << result.before.size_bytes << ",";
    j << R"("percent_ram":)" << result.before.percent_of_ram;
    j << "},";

    j << R"("after":{)";
    j << R"("page_count":)" << result.after.page_count << ",";
    j << R"("size_bytes":)" << result.after.size_bytes << ",";
    j << R"("percent_ram":)" << result.after.percent_of_ram;
    j << "},";

    j << R"("flush_status":")" << to_string(result.status) << R"(",)";
    j << R"("flush_duration_ms":)" << result.flush_duration_ms << ",";
    j << R"("error_message":")" << result.error_message << R"(",)";
    j << R"("component_details":)" << components_to_json(result.component_details) << ",";
    j << R"("consistency_level":")" << to_string(result.consistency) << R"(",)";
    j << R"("is_consistent":)" << (result.is_consistent ? "true" : "false") << ",";
    j << R"("total_ram_bytes":)" << result.total_ram_bytes << ",";
    j << R"("buffer_pool_size":)" << result.buffer_pool_size << ",";
    j << R"("flush_started_at":")" << result.flush_started_at << R"(",)";
    j << R"("flush_finished_at":")" << result.flush_finished_at << R"(",)";
    j << R"("created_at":")" << result.created_at << R"(")";
    j << "}";
    return j.str();
}

std::string DirtyBufferLogger::components_to_json(
    const std::vector<ComponentDetail>& details) const {
    std::ostringstream j;
    j << "[";
    for (size_t i = 0; i < details.size(); i++) {
        if (i > 0) j << ",";
        j << R"({"name":")" << details[i].name << R"(",)";
        j << R"("dirty_pages":)" << details[i].dirty_pages << ",";
        j << R"("size_bytes":)" << details[i].size_bytes << "}";
    }
    j << "]";
    return j.str();
}

std::string DirtyBufferLogger::bytes_to_human(uint64_t bytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    double size = static_cast<double>(bytes);
    int unit = 0;
    while (size >= 1024.0 && unit < 4) {
        size /= 1024.0;
        unit++;
    }
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << size << " " << units[unit];
    return oss.str();
}

void DirtyBufferLogger::log_before(const std::string& job_id, const BufferState& state) {
    spdlog::info("[DirtyBuffer] Job {}: BEFORE - {} pages ({}, {:.1f}% RAM)",
                 job_id, state.page_count, bytes_to_human(state.size_bytes), state.percent_of_ram);
}

void DirtyBufferLogger::log_flush_start(const std::string& job_id, const std::string& plugin) {
    spdlog::info("[DirtyBuffer] Job {}: starting flush via {}", job_id, plugin);
}

void DirtyBufferLogger::log_flush_complete(const FlushResult& result) {
    spdlog::info("[DirtyBuffer] Job {}: flush {} in {} ms",
                 result.job_id, to_string(result.status), result.flush_duration_ms);
}

void DirtyBufferLogger::log_consistency(const std::string& job_id,
                                         ConsistencyLevel level, bool consistent) {
    if (consistent) {
        spdlog::info("[DirtyBuffer] Job {}: {} (consistent)", job_id, to_string(level));
    } else {
        spdlog::warn("[DirtyBuffer] Job {}: {} (NOT consistent)", job_id, to_string(level));
    }
}

} // namespace obs
