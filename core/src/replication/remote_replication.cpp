#include "replication/remote_replication.h"
#include <spdlog/spdlog.h>
#include <fstream>

namespace obs {

RemoteReplication::RemoteReplication() {}
RemoteReplication::~RemoteReplication() {
    std::lock_guard<std::mutex> lock(jobs_mutex_);
    for (auto& [id, job] : jobs_) {
        job->running = false;
        if (job->worker.joinable()) job->worker.join();
    }
}

std::string RemoteReplication::start(const ReplicationConfig& config) {
    auto job = std::make_shared<JobState>();
    job->config = config;
    job->progress.job_id = config.job_id;
    job->progress.status = ReplicationStatus::SYNCING;
    job->progress.started_at = current_time_string();
    job->running = true;

    std::string jid = config.job_id;
    {
        std::lock_guard<std::mutex> lock(jobs_mutex_);
        jobs_[jid] = job;
    }

    job->worker = std::thread(&RemoteReplication::do_replicate, this, jid, config);
    spdlog::info("Replication started: {} -> {}:{}", config.source_storage_id, config.dest_host, config.dest_port);
    return jid;
}

void RemoteReplication::pause(const std::string& job_id) {
    std::lock_guard<std::mutex> lock(jobs_mutex_);
    auto it = jobs_.find(job_id);
    if (it != jobs_.end()) it->second->paused = true;
}

void RemoteReplication::resume(const std::string& job_id) {
    std::lock_guard<std::mutex> lock(jobs_mutex_);
    auto it = jobs_.find(job_id);
    if (it != jobs_.end()) it->second->paused = false;
}

void RemoteReplication::cancel(const std::string& job_id) {
    std::lock_guard<std::mutex> lock(jobs_mutex_);
    auto it = jobs_.find(job_id);
    if (it != jobs_.end()) {
        it->second->running = false;
        it->second->progress.status = ReplicationStatus::FAILED;
        it->second->progress.last_error = "Cancelled by user";
    }
}

ReplicationProgress RemoteReplication::get_progress(const std::string& job_id) const {
    std::lock_guard<std::mutex> lock(jobs_mutex_);
    auto it = jobs_.find(job_id);
    if (it != jobs_.end()) return it->second->progress;
    return {};
}

bool RemoteReplication::verify_dest(const ReplicationConfig& /*config*/) {
    // In production: connect to dest, exchange chunk hashes, verify
    return true;
}

void RemoteReplication::do_replicate(const std::string& job_id, ReplicationConfig config) {
    auto it = jobs_.find(job_id);
    if (it == jobs_.end()) return;
    auto& job = it->second;

    try {
        // Simulate replication: read chunks from source storage, send to dest
        // In production: iterate chunk index, compare with remote, send missing
        uint64_t total = 1024 * 1024 * 100; // 100MB simulated
        uint64_t transferred = 0;
        const size_t chunk_size = 4 * 1024 * 1024;

        job->progress.total_bytes = total;
        job->progress.chunks_total = total / chunk_size;

        while (transferred < total && job->running) {
            if (job->paused) {
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                continue;
            }

            size_t to_send = std::min((uint64_t)chunk_size, total - transferred);
            std::vector<uint8_t> buf(to_send, 0xAB);

            if (!send_chunk(config.dest_host, config.dest_port, buf.data(), to_send)) {
                job->progress.status = ReplicationStatus::FAILED;
                job->progress.last_error = "Connection lost";
                notify(job->progress);
                return;
            }

            transferred += to_send;
            job->progress.transferred_bytes = transferred;
            job->progress.chunks_synced++;
            job->progress.speed_mbps = 100.0; // simulated
            notify(job->progress);

            if (config.bandwidth_limit_kbps > 0) {
                double delay = (double)(to_send) / (config.bandwidth_limit_kbps * 1024.0);
                std::this_thread::sleep_for(std::chrono::microseconds((int64_t)(delay * 1000000)));
            }
        }

        job->progress.status = ReplicationStatus::COMPLETED;
        job->progress.finished_at = current_time_string();
        notify(job->progress);
        spdlog::info("Replication completed: {}", job_id);

    } catch (const std::exception& e) {
        job->progress.status = ReplicationStatus::FAILED;
        job->progress.last_error = e.what();
        notify(job->progress);
    }
    job->running = false;
}

bool RemoteReplication::send_chunk(const std::string& host, uint16_t port, const uint8_t* data, size_t size) {
    // Simplified: in production, use encrypted TCP channel
    // For now, just simulate success
    return true;
}

} // namespace obs
