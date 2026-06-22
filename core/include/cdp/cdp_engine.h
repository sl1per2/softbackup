#pragma once
#include "common.h"
#include "engine/i_cdp_engine.h"
#include <sqlite3.h>
#include <thread>

namespace obs {

struct CdpSessionConfig {
    std::string policy_id;
    std::string agent_id;
    std::string source_path;
    int interval_seconds = 5;
    int max_latency_ms = 1000;
    int retention_minutes = 60;
};

enum class CdpStatus { ACTIVE, PAUSED, STOPPED, ERROR };

struct CdpSessionInfo {
    std::string session_id;
    CdpStatus status = CdpStatus::STOPPED;
    int64_t lag_ms = 0;
    double iops = 0;
    double throughput_mbps = 0;
    int64_t blocks_tracked = 0;
    std::string started_at;
};

struct CdpDetailedRecoveryPoint {
    std::string id;
    std::string session_id;
    std::string timestamp;
    int64_t block_count = 0;
    uint64_t size_bytes = 0;
    bool is_consistent = false;
};

struct BlockChange {
    uint64_t lba;
    uint32_t size;
    std::string timestamp;
};

class CdpEngine : public ICdpEngine {
public:
    CdpEngine();
    ~CdpEngine() override;

    std::string component_name() const override { return "CdpEngine"; }
    void initialize() override;
    void shutdown() override;

    std::string start_session(const CdpConfig& config) override;
    void stop_session(const std::string& session_id) override;
    std::vector<CdpRecoveryPoint> get_recovery_points(const std::string& session_id) override;
    bool restore_to_point(const std::string& session_id, const std::string& recovery_point_id) override;

    CdpSessionInfo get_session_status(const std::string& session_id);

private:
    void monitor_loop(const std::string& session_id);
    void init_journal_db();
    void log_block_change(const std::string& session_id, const BlockChange& change);
    void create_recovery_point(const std::string& session_id);

    struct Session {
        CdpSessionConfig config;
        CdpSessionInfo info;
        std::thread monitor_thread;
        std::atomic<bool> running{false};
    };

    std::map<std::string, std::shared_ptr<Session>> sessions_;
    mutable std::mutex sessions_mutex_;
    sqlite3* journal_db_ = nullptr;
    mutable std::mutex journal_mutex_;
};

} // namespace obs
