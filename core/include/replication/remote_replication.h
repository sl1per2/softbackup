#pragma once
#include "common.h"
#include <thread>
#include <atomic>
#include <functional>

namespace obs {

struct ReplicationConfig {
    std::string job_id;
    std::string source_storage_id;
    std::string dest_storage_id;
    std::string dest_host;
    uint16_t dest_port = 9100;
    bool compress = true;
    bool encrypt = true;
    int bandwidth_limit_kbps = 0;
    bool incremental_only = true;
    std::string encryption_key_id;
};

enum class ReplicationStatus { IDLE, SYNCING, COMPLETED, FAILED, PAUSED };

struct ReplicationProgress {
    std::string job_id;
    ReplicationStatus status = ReplicationStatus::IDLE;
    uint64_t total_bytes = 0;
    uint64_t transferred_bytes = 0;
    double speed_mbps = 0;
    int32_t chunks_synced = 0;
    int32_t chunks_total = 0;
    std::string last_error;
    std::string started_at;
    std::string finished_at;
};

using ReplicationCallback = std::function<void(const ReplicationProgress&)>;

class RemoteReplication {
public:
    RemoteReplication();
    ~RemoteReplication();

    std::string start(const ReplicationConfig& config);
    void pause(const std::string& job_id);
    void resume(const std::string& job_id);
    void cancel(const std::string& job_id);
    ReplicationProgress get_progress(const std::string& job_id) const;
    void set_callback(ReplicationCallback cb) { callback_ = std::move(cb); }

    // Verify destination has all required chunks before starting
    bool verify_dest(const ReplicationConfig& config);

private:
    void do_replicate(const std::string& job_id, ReplicationConfig config);
    bool send_chunk(const std::string& host, uint16_t port, const uint8_t* data, size_t size);
    void notify(const ReplicationProgress& p) { if (callback_) callback_(p); }

    struct JobState {
        ReplicationConfig config;
        ReplicationProgress progress;
        std::atomic<bool> running{false};
        std::atomic<bool> paused{false};
        std::thread worker;
    };

    std::map<std::string, std::shared_ptr<JobState>> jobs_;
    mutable std::mutex jobs_mutex_;
    ReplicationCallback callback_;
};

} // namespace obs
