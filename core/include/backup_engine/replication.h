#pragma once
#include "common.h"
#include <thread>
#include <atomic>

namespace obs {

struct ReplicationConfig {
    std::string source_storage;
    std::string dest_storage;
    std::string dest_host;
    int dest_port = 0;
    bool encrypted = true;
    int bandwidth_limit_kbps = 0;
    bool compress = true;
    std::string filter_pattern;   // Only replicate matching jobs
};

struct ReplicationStatus {
    bool running = false;
    uint64_t bytes_replicated = 0;
    uint64_t total_bytes = 0;
    int chunks_replicated = 0;
    int chunks_skipped = 0;
    double speed_mbps = 0;
    std::string last_error;
};

using ReplicationProgressCallback = std::function<void(const ReplicationStatus&)>;

class ReplicationEngine {
public:
    ReplicationEngine();
    ~ReplicationEngine();

    bool start_replication(const ReplicationConfig& config);
    void stop_replication();
    ReplicationStatus get_status() const;
    void set_progress_callback(ReplicationProgressCallback cb) { callback_ = std::move(cb); }

private:
    void replication_loop();
    bool replicate_chunk(const std::string& chunk_path, const std::string& dest_path);

    ReplicationConfig config_;
    std::atomic<bool> running_{false};
    std::thread worker_;
    ReplicationStatus status_;
    mutable std::mutex status_mutex_;
    ReplicationProgressCallback callback_;
};

} // namespace obs
