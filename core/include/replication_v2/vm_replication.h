#pragma once
#include "common.h"
#include <functional>
#include <vector>
#include <string>
#include <thread>

namespace obs {

enum class ReplicationStatus { IDLE, INIT_SYNC, SYNCING, PAUSED, FAILOVER, FAILOVER_DONE, FAILED };
enum class ReplicationMode { CONTINUOUS, SCHEDULED, ON_DEMAND };

struct VmReplicationConfig {
    std::string name;
    std::string source_vm_id;
    std::string source_host;
    std::string target_host;
    std::string target_datastore;
    std::string target_network;
    ReplicationMode mode = ReplicationMode::CONTINUOUS;
    int rpo_seconds = 300;
    int64_t memory_mb = 1024;
    int32_t cpu_count = 2;
    std::string ip_mapping; // JSON: {"original_ip":"replica_ip"}
};

struct VmReplicationState {
    std::string job_id;
    ReplicationStatus status = ReplicationStatus::IDLE;
    uint64_t bytes_replicated = 0;
    uint64_t total_bytes = 0;
    int64_t lag_seconds = 0;
    std::string last_sync_at;
    std::string started_at;
    std::string error;
    std::string replica_vm_id;
};

using ReplicationCallback = std::function<void(const VmReplicationState&)>;

class VmReplication {
public:
    VmReplication();
    ~VmReplication();

    std::string start(const VmReplicationConfig& config, ReplicationCallback callback = nullptr);
    void stop(const std::string& job_id);
    void pause(const std::string& job_id);
    void resume(const std::string& job_id);
    VmReplicationState get_state(const std::string& job_id);

    bool failover(const std::string& job_id);
    bool failback(const std::string& job_id);
    std::vector<VmReplicationState> list_replications();

private:
    void initial_sync(const std::string& job_id);
    void incremental_sync(const std::string& job_id);
    void monitor_loop(const std::string& job_id);

    std::map<std::string, std::shared_ptr<VmReplicationState>> states_;
    std::map<std::string, std::thread> sync_threads_;
    mutable std::mutex states_mutex_;
};

} // namespace obs
