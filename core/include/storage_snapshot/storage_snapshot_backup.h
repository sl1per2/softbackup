#pragma once
#include "common.h"
#include <functional>
#include <vector>
#include <string>

namespace obs {

enum class StorageVendor { NETAPP, DELL_EMC, HPE, PURE, IBM, GENERIC_LVM };
enum class SnapshotStatus { PENDING, CREATED, MOUNTED, BACKING_UP, COMPLETED, FAILED, DELETED };

struct StorageSnapshotConfig {
    StorageVendor vendor = StorageVendor::GENERIC_LVM;
    std::string storage_endpoint;
    std::string storage_credentials;
    std::string volume_id;
    std::string snapshot_name;
    bool delete_after_backup = true;
};

struct StorageSnapshotInfo {
    std::string snapshot_id;
    std::string volume_id;
    std::string storage_endpoint;
    SnapshotStatus status = SnapshotStatus::PENDING;
    std::string mount_point;
    uint64_t size_bytes = 0;
    std::string created_at;
    std::string error;
};

using SnapshotCallback = std::function<void(const StorageSnapshotInfo&)>;

class StorageSnapshotBackup {
public:
    StorageSnapshotBackup();
    ~StorageSnapshotBackup();

    StorageSnapshotInfo create_snapshot(const StorageSnapshotConfig& config);
    bool delete_snapshot(const std::string& snapshot_id, const std::string& storage_endpoint);
    bool mount_snapshot(const std::string& snapshot_id, const std::string& mount_point);
    bool unmount_snapshot(const std::string& mount_point);
    std::vector<StorageSnapshotInfo> list_snapshots(const std::string& storage_endpoint);

    bool backup_from_snapshot(const std::string& snapshot_id, const std::string& mount_point,
                             const std::string& output_path, SnapshotCallback callback = nullptr);

private:
    bool create_netapp_snapshot(const StorageSnapshotConfig& config);
    bool create_lvm_snapshot(const StorageSnapshotConfig& config);
    bool create_generic_snapshot(const StorageSnapshotConfig& config);
    bool mount_nfs_snapshot(const std::string& export_path, const std::string& mount_point);
    bool mount_lvm_snapshot(const std::string& lv_path, const std::string& mount_point);
    bool umount_point(const std::string& mount_point);

    std::map<std::string, StorageSnapshotInfo> snapshots_;
    mutable std::mutex snapshots_mutex_;
};

} // namespace obs
