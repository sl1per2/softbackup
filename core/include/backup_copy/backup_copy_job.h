#pragma once
#include "common.h"
#include <functional>
#include <vector>
#include <string>

namespace obs {

enum class CopyMode { IMMEDIATE, SCHEDULED, MIRROR };
enum class GfsArchiveTier { WEEKLY, MONTHLY, YEARLY };

struct BackupCopyConfig {
    std::string name;
    std::string source_storage_id;
    std::string dest_storage_id;
    CopyMode mode = CopyMode::IMMEDIATE;
    std::string cron_schedule;
    bool health_check_after = true;
    bool verify_checksums = true;
    bool encrypt_copy = false;
    int archive_retention_weekly = 4;
    int archive_retention_monthly = 12;
    int archive_retention_yearly = 7;
};

struct BackupCopyJobStatus {
    std::string job_id;
    std::string config_id;
    std::string source_job_id;
    CopyMode mode;
    std::string status;
    uint64_t bytes_copied = 0;
    int32_t chunks_copied = 0;
    bool health_check_passed = false;
    std::string started_at;
    std::string completed_at;
    std::string error;
};

struct GfsArchivePoint {
    std::string id;
    GfsArchiveTier tier;
    std::string source_job_id;
    std::string dest_storage_id;
    std::string created_at;
    uint64_t size_bytes = 0;
    bool verified = false;
};

class BackupCopyJob {
public:
    BackupCopyJob() = default;
    explicit BackupCopyJob(const BackupCopyConfig& config);
    ~BackupCopyJob();

    bool run(std::function<void(const BackupCopyJobStatus&)> callback = nullptr);
    bool create_gfs_archive_point(GfsArchiveTier tier, const std::string& source_job_id);
    bool health_check();
    bool verify_checksums();
    std::vector<GfsArchivePoint> list_archive_points();
    bool cleanup_expired();

    std::string get_status() const;
    uint64_t get_bytes_copied() const;

private:
    bool copy_backup(const std::string& source, const std::string& dest);
    bool verify_copy(const std::string& dest);

    BackupCopyConfig config_;
    BackupCopyJobStatus status_;
    mutable std::mutex status_mutex_;
};

} // namespace obs
