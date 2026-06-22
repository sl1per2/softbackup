#pragma once
#include "common.h"
#include <functional>
#include <thread>
#include <atomic>

namespace obs {

class DirtyBufferLogger;

enum class JobType { FULL, INCREMENTAL, DIFFERENTIAL };
enum class JobStatus { PENDING, RUNNING, COMPLETED, FAILED, CANCELLED };

struct BackupConfig {
    std::string job_id;
    std::string agent_id;
    std::vector<std::string> source_paths;
    std::vector<std::string> exclude_patterns;
    JobType type = JobType::FULL;
    int compression_level = 1;
    bool encryption_enabled = false;
    int storage_id = 0;
    std::string plugin_name = "generic";
};

struct JobMetrics {
    uint64_t size_bytes = 0;
    uint64_t size_transferred = 0;
    int32_t chunks_total = 0;
    int32_t chunks_cached = 0;
    uint64_t dedup_saved = 0;
    double compression_ratio = 0.0;
    double cache_hit_ratio = 0.0;
    int32_t zero_blocks_skipped = 0;
    std::string transport_mode_used;
    int64_t dirty_buffer_flush_ms = 0;
    bool dirty_buffer_flushed = false;
    std::string dirty_buffer_consistency;
};

class BackupJob {
public:
    using StatusCallback = std::function<void(const std::string& job_id, JobStatus status, const JobMetrics& metrics)>;

    explicit BackupJob(const BackupConfig& config);
    ~BackupJob();

    void run();
    void cancel();
    JobStatus get_status() const { return status_.load(); }
    JobMetrics get_metrics() const;
    std::string get_error() const { return error_; }
    void set_status_callback(StatusCallback cb) { callback_ = std::move(cb); }
    void set_dirty_buffer_logger(DirtyBufferLogger* logger) { dirty_buffer_logger_ = logger; }

private:
    void do_backup();
    bool should_exclude(const std::string& path) const;
    void flush_dirty_buffers_before_backup();

    BackupConfig config_;
    std::atomic<JobStatus> status_{JobStatus::PENDING};
    mutable std::mutex metrics_mutex_;
    JobMetrics metrics_;
    std::string error_;
    std::thread worker_;
    std::atomic<bool> cancelled_{false};
    StatusCallback callback_;
    DirtyBufferLogger* dirty_buffer_logger_ = nullptr;
};

} // namespace obs
