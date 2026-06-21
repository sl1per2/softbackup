#include "replication_v2/vm_replication.h"
#include <spdlog/spdlog.h>
#include <chrono>

namespace obs {

VmReplication::VmReplication() {}
VmReplication::~VmReplication() { /* stop all */ }

std::string VmReplication::start(const VmReplicationConfig& config, ReplicationCallback callback) {
    auto state = std::make_shared<VmReplicationState>();
    state->job_id = "repl-" + std::to_string(std::time(nullptr));
    state->status = ReplicationStatus::INIT_SYNC;
    state->started_at = current_time_string();

    {
        std::lock_guard<std::mutex> lock(states_mutex_);
        states_[state->job_id] = state;
    }

    spdlog::info("VM Replication: starting {} (RPO={}s)", state->job_id, config.rpo_seconds);

    sync_threads_[state->job_id] = std::thread([this, state, config, callback]() {
        // Initial sync
        state->status = ReplicationStatus::INIT_SYNC;
        initial_sync(state->job_id);

        // Incremental loop
        while (state->status != ReplicationStatus::FAILED &&
               state->status != ReplicationStatus::PAUSED) {
            state->status = ReplicationStatus::SYNCING;
            incremental_sync(state->job_id);
            if (callback) callback(*state);
            std::this_thread::sleep_for(std::chrono::seconds(config.rpo_seconds));
        }
    });

    return state->job_id;
}

void VmReplication::stop(const std::string& job_id) {
    std::lock_guard<std::mutex> lock(states_mutex_);
    auto it = states_.find(job_id);
    if (it != states_.end()) {
        it->second->status = ReplicationStatus::FAILED;
        it->second->error = "Stopped by user";
    }
}

void VmReplication::pause(const std::string& job_id) {
    std::lock_guard<std::mutex> lock(states_mutex_);
    auto it = states_.find(job_id);
    if (it != states_.end()) it->second->status = ReplicationStatus::PAUSED;
}

void VmReplication::resume(const std::string& job_id) {
    std::lock_guard<std::mutex> lock(states_mutex_);
    auto it = states_.find(job_id);
    if (it != states_.end()) it->second->status = ReplicationStatus::SYNCING;
}

VmReplicationState VmReplication::get_state(const std::string& job_id) {
    std::lock_guard<std::mutex> lock(states_mutex_);
    auto it = states_.find(job_id);
    return it != states_.end() ? *it->second : VmReplicationState{};
}

bool VmReplication::failover(const std::string& job_id) {
    spdlog::info("VM Replication: failover {}", job_id);
    std::lock_guard<std::mutex> lock(states_mutex_);
    auto it = states_.find(job_id);
    if (it != states_.end()) {
        it->second->status = ReplicationStatus::FAILOVER;
        // Start VM on target host
        it->second->status = ReplicationStatus::FAILOVER_DONE;
        return true;
    }
    return false;
}

bool VmReplication::failback(const std::string& job_id) {
    spdlog::info("VM Replication: failback {}", job_id);
    return true;
}

std::vector<VmReplicationState> VmReplication::list_replications() {
    std::vector<VmReplicationState> result;
    std::lock_guard<std::mutex> lock(states_mutex_);
    for (auto& [id, state] : states_) result.push_back(*state);
    return result;
}

void VmReplication::initial_sync(const std::string& job_id) {
    spdlog::info("VM Replication: initial sync for {}", job_id);
    std::this_thread::sleep_for(std::chrono::seconds(5)); // Simulate
}

void VmReplication::incremental_sync(const std::string& job_id) {
    std::lock_guard<std::mutex> lock(states_mutex_);
    auto it = states_.find(job_id);
    if (it == states_.end()) return;
    it->second->bytes_replicated += 1024 * 1024;
    it->second->last_sync_at = current_time_string();
}

} // namespace obs
