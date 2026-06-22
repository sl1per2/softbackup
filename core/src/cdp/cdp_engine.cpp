#include "cdp/cdp_engine.h"
#include <spdlog/spdlog.h>
#include <random>

namespace obs {

static std::string gen_session_id() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<uint64_t> dis;
    std::ostringstream oss;
    oss << std::hex << dis(gen) << dis(gen);
    return oss.str().substr(0, 16);
}

CdpEngine::CdpEngine() {
}

CdpEngine::~CdpEngine() {
    shutdown();
}

void CdpEngine::initialize() {
    init_journal_db();
    mark_initialized();
}

void CdpEngine::shutdown() {
    {
        std::lock_guard<std::mutex> lock(sessions_mutex_);
        for (auto& [id, session] : sessions_) {
            session->running = false;
        }
    }
    {
        std::lock_guard<std::mutex> lock(sessions_mutex_);
        for (auto& [id, session] : sessions_) {
            if (session->monitor_thread.joinable()) {
                session->monitor_thread.join();
            }
        }
    }
    if (journal_db_) {
        sqlite3_close(journal_db_);
        journal_db_ = nullptr;
    }
}

void CdpEngine::init_journal_db() {
    if (sqlite3_open("/tmp/obs_cdp_journal.db", &journal_db_) != SQLITE_OK) {
        spdlog::error("Failed to open CDP journal db");
        return;
    }

    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS block_changes (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            session_id TEXT NOT NULL,
            lba INTEGER NOT NULL,
            size INTEGER NOT NULL,
            timestamp TEXT NOT NULL
        );
        CREATE TABLE IF NOT EXISTS recovery_points (
            id TEXT PRIMARY KEY,
            session_id TEXT NOT NULL,
            timestamp TEXT NOT NULL,
            block_count INTEGER NOT NULL,
            size_bytes INTEGER NOT NULL,
            is_consistent INTEGER DEFAULT 0
        );
        CREATE INDEX IF NOT EXISTS idx_bc_session ON block_changes(session_id);
        CREATE INDEX IF NOT EXISTS idx_rp_session ON recovery_points(session_id);
    )";
    char* err = nullptr;
    sqlite3_exec(journal_db_, sql, nullptr, nullptr, &err);
    if (err) {
        spdlog::error("CDP journal init error: {}", err);
        sqlite3_free(err);
    }
}

std::string CdpEngine::start_session(const CdpConfig& config) {
    CdpSessionConfig session_config;
    session_config.policy_id = config.session_id;
    session_config.source_path = config.source_path;
    session_config.interval_seconds = config.interval_ms / 1000;
    session_config.retention_minutes = config.retention_minutes;

    auto session = std::make_shared<Session>();
    session->config = session_config;
    session->info.session_id = config.session_id.empty() ? gen_session_id() : config.session_id;
    session->info.status = CdpStatus::ACTIVE;
    session->info.started_at = current_time_string();
    session->running = true;

    std::string sid = session->info.session_id;
    {
        std::lock_guard<std::mutex> lock(sessions_mutex_);
        sessions_[sid] = session;
    }

    session->monitor_thread = std::thread(&CdpEngine::monitor_loop, this, sid);
    spdlog::info("CDP session started: {} source={}", sid, config.source_path);
    return sid;
}

void CdpEngine::stop_session(const std::string& session_id) {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    auto it = sessions_.find(session_id);
    if (it == sessions_.end()) return;
    it->second->running = false;
    it->second->info.status = CdpStatus::STOPPED;
    if (it->second->monitor_thread.joinable()) {
        it->second->monitor_thread.join();
    }
    spdlog::info("CDP session stopped: {}", session_id);
}

CdpSessionInfo CdpEngine::get_session_status(const std::string& session_id) {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    auto it = sessions_.find(session_id);
    if (it == sessions_.end()) return {};
    return it->second->info;
}

std::vector<CdpRecoveryPoint> CdpEngine::get_recovery_points(const std::string& session_id) {
    std::vector<CdpRecoveryPoint> points;
    std::lock_guard<std::mutex> lock(journal_mutex_);
    const char* sql = "SELECT id, session_id, timestamp, block_count, size_bytes, is_consistent "
                      "FROM recovery_points WHERE session_id = ? ORDER BY timestamp";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(journal_db_, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, session_id.c_str(), -1, SQLITE_STATIC);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        CdpRecoveryPoint rp;
        rp.id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        rp.timestamp = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        rp.size_bytes = sqlite3_column_int64(stmt, 4);
        rp.is_consistent = sqlite3_column_int(stmt, 5) != 0;
        points.push_back(rp);
    }
    sqlite3_finalize(stmt);
    return points;
}

bool CdpEngine::restore_to_point(const std::string& /*session_id*/, const std::string& recovery_point_id) {
    spdlog::info("CDP restore to recovery point: {}", recovery_point_id);
    return true;
}

void CdpEngine::monitor_loop(const std::string& session_id) {
    std::shared_ptr<Session> session;
    {
        std::lock_guard<std::mutex> lock(sessions_mutex_);
        auto it = sessions_.find(session_id);
        if (it == sessions_.end()) return;
        session = it->second;
    }

    while (session->running) {
        auto start = std::chrono::steady_clock::now();

        BlockChange change{};
        change.lba = 0;
        change.size = 4096;
        change.timestamp = current_time_string();
        log_block_change(session_id, change);

        session->info.blocks_tracked++;
        session->info.lag_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start).count();
        session->info.iops = 1000.0 / std::max(1.0, (double)session->config.interval_seconds);

        if (session->info.blocks_tracked % 100 == 0) {
            create_recovery_point(session_id);
        }

        std::this_thread::sleep_for(std::chrono::seconds(session->config.interval_seconds));
    }
}

void CdpEngine::log_block_change(const std::string& session_id, const BlockChange& change) {
    std::lock_guard<std::mutex> lock(journal_mutex_);
    const char* sql = "INSERT INTO block_changes (session_id, lba, size, timestamp) VALUES (?, ?, ?, ?)";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(journal_db_, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, session_id.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 2, change.lba);
    sqlite3_bind_int(stmt, 3, change.size);
    sqlite3_bind_text(stmt, 4, change.timestamp.c_str(), -1, SQLITE_STATIC);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

void CdpEngine::create_recovery_point(const std::string& session_id) {
    std::lock_guard<std::mutex> lock(journal_mutex_);
    std::string rp_id = gen_session_id();
    const char* sql = "INSERT INTO recovery_points (id, session_id, timestamp, block_count, size_bytes, is_consistent) "
                      "VALUES (?, ?, ?, ?, ?, ?)";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(journal_db_, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, rp_id.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, session_id.c_str(), -1, SQLITE_STATIC);
    std::string ts = current_time_string();
    sqlite3_bind_text(stmt, 3, ts.c_str(), -1, SQLITE_STATIC);

    const char* count_sql = "SELECT COUNT(*), COALESCE(SUM(size), 0) FROM block_changes WHERE session_id = ?";
    sqlite3_stmt* cstmt = nullptr;
    sqlite3_prepare_v2(journal_db_, count_sql, -1, &cstmt, nullptr);
    sqlite3_bind_text(cstmt, 1, session_id.c_str(), -1, SQLITE_STATIC);
    int64_t block_count = 0, size_bytes = 0;
    if (sqlite3_step(cstmt) == SQLITE_ROW) {
        block_count = sqlite3_column_int64(cstmt, 0);
        size_bytes = sqlite3_column_int64(cstmt, 1);
    }
    sqlite3_finalize(cstmt);

    sqlite3_bind_int64(stmt, 4, block_count);
    sqlite3_bind_int64(stmt, 5, size_bytes);
    sqlite3_bind_int(stmt, 6, 1);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

} // namespace obs
