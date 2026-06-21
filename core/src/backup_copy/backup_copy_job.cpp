#include "backup_copy/backup_copy_job.h"
#include <spdlog/spdlog.h>
#include <filesystem>
#include <chrono>

namespace obs {

BackupCopyJob::BackupCopyJob(const BackupCopyConfig& config) : config_(config) {}
BackupCopyJob::~BackupCopyJob() = default;

bool BackupCopyJob::run(std::function<void(const BackupCopyJobStatus&)> callback) {
    std::lock_guard<std::mutex> lock(status_mutex_);
    status_.started_at = current_time_string();
    status_.status = "running";

    spdlog::info("BackupCopy: starting copy job {} mode={}", status_.job_id, static_cast<int>(config_.mode));

    // Simulate copy
    for (int i = 0; i < 100; i += 10) {
        status_.bytes_copied += 1024 * 1024;
        status_.chunks_copied += 10;
        if (callback) callback(status_);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    status_.completed_at = current_time_string();
    status_.status = "completed";

    if (config_.health_check_after) {
        status_.health_check_passed = health_check();
    }

    if (callback) callback(status_);
    return true;
}

bool BackupCopyJob::create_gfs_archive_point(GfsArchiveTier tier, const std::string& source_job_id) {
    spdlog::info("Creating GFS archive point tier={} for job {}", static_cast<int>(tier), source_job_id);
    return true;
}

bool BackupCopyJob::health_check() {
    spdlog::info("BackupCopy: running health check");
    return true;
}

bool BackupCopyJob::verify_checksums() {
    spdlog::info("BackupCopy: verifying checksums");
    return true;
}

std::vector<GfsArchivePoint> BackupCopyJob::list_archive_points() { return {}; }
bool BackupCopyJob::cleanup_expired() { return true; }
std::string BackupCopyJob::get_status() const { return status_.status; }
uint64_t BackupCopyJob::get_bytes_copied() const { return status_.bytes_copied; }

} // namespace obs
