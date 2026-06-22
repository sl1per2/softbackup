#include "backup_engine/backup_job.h"
#include "backup_engine/dirty_buffer_logger.h"
#include "backup_engine/platform_flusher.h"
#include <fstream>
#include <filesystem>
#include <spdlog/spdlog.h>

namespace obs {

BackupJob::BackupJob(const BackupConfig& config) : config_(config) {}

BackupJob::~BackupJob() { cancel(); }

void BackupJob::run() {
    status_ = JobStatus::RUNNING;
    if (callback_) callback_(config_.job_id, JobStatus::RUNNING, metrics_);
    worker_ = std::thread(&BackupJob::do_backup, this);
}

void BackupJob::cancel() {
    cancelled_ = true;
    if (worker_.joinable()) {
        worker_.join();
    }
    if (status_ == JobStatus::RUNNING) {
        status_ = JobStatus::CANCELLED;
        if (callback_) callback_(config_.job_id, JobStatus::CANCELLED, metrics_);
    }
}

JobMetrics BackupJob::get_metrics() const {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    return metrics_;
}

bool BackupJob::should_exclude(const std::string& path) const {
    for (const auto& pattern : config_.exclude_patterns) {
        if (path.find(pattern) != std::string::npos) return true;
    }
    return false;
}

void BackupJob::flush_dirty_buffers_before_backup() {
    if (!dirty_buffer_logger_) {
        spdlog::debug("Job {}: no dirty buffer logger, skipping flush", config_.job_id);
        return;
    }

    spdlog::info("Job {}: flushing dirty buffers before backup (plugin={})",
                 config_.job_id, config_.plugin_name);

    PlatformBufferFlusher flusher(config_.plugin_name);
    auto result = dirty_buffer_logger_->flush_and_log(
        config_.job_id, config_.agent_id, &flusher);

    {
        std::lock_guard<std::mutex> lock(metrics_mutex_);
        metrics_.dirty_buffer_flush_ms = result.flush_duration_ms;
        metrics_.dirty_buffer_flushed = (result.status == FlushStatus::FLUSHED);
        metrics_.dirty_buffer_consistency = to_string(result.consistency);
    }

    if (result.status == FlushStatus::FLUSHED) {
        spdlog::info("Job {}: dirty buffer flush completed ({} ms, {})",
                     config_.job_id, result.flush_duration_ms,
                     to_string(result.consistency));
    } else {
        spdlog::warn("Job {}: dirty buffer flush did not succeed ({}), proceeding with backup",
                     config_.job_id, to_string(result.status));
    }
}

void BackupJob::do_backup() {
    try {
        flush_dirty_buffers_before_backup();
        if (cancelled_) return;

        uint64_t total_size = 0;
        int32_t total_chunks = 0;

        for (const auto& source : config_.source_paths) {
            if (!fs::exists(source)) {
                spdlog::warn("Source path does not exist: {}", source);
                continue;
            }
            if (fs::is_regular_file(source)) {
                if (should_exclude(source)) continue;
                total_size += fs::file_size(source);
                total_chunks++;
            } else if (fs::is_directory(source)) {
                for (auto& entry : fs::recursive_directory_iterator(source)) {
                    if (cancelled_) break;
                    if (!entry.is_regular_file()) continue;
                    if (should_exclude(entry.path().string())) continue;
                    total_size += entry.file_size();
                    total_chunks++;
                }
            }
        }

        {
            std::lock_guard<std::mutex> lock(metrics_mutex_);
            metrics_.size_bytes = total_size;
            metrics_.chunks_total = total_chunks;
            metrics_.size_transferred = total_size;
            metrics_.compression_ratio = 1.0;
        }

        for (int i = 0; i <= 100; i += 10) {
            if (cancelled_) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            if (callback_) {
                std::lock_guard<std::mutex> lock(metrics_mutex_);
                callback_(config_.job_id, JobStatus::RUNNING, metrics_);
            }
        }

        if (!cancelled_) {
            status_ = JobStatus::COMPLETED;
            if (callback_) callback_(config_.job_id, JobStatus::COMPLETED, metrics_);
            spdlog::info("Backup job {} completed: {} bytes, {} chunks",
                         config_.job_id, total_size, total_chunks);
        }
    } catch (const std::exception& e) {
        error_ = e.what();
        status_ = JobStatus::FAILED;
        spdlog::error("Backup job {} failed: {}", config_.job_id, e.what());
        if (callback_) callback_(config_.job_id, JobStatus::FAILED, metrics_);
    }
}

} // namespace obs
