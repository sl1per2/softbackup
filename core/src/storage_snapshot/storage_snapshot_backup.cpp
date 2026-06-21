#include "storage_snapshot/storage_snapshot_backup.h"
#include <spdlog/spdlog.h>
#include <filesystem>
#include <cstdlib>
#include <chrono>

namespace obs {

StorageSnapshotBackup::StorageSnapshotBackup() {}
StorageSnapshotBackup::~StorageSnapshotBackup() {}

StorageSnapshotInfo StorageSnapshotBackup::create_snapshot(const StorageSnapshotConfig& config) {
    StorageSnapshotInfo info;
    info.snapshot_id = "snap-" + std::to_string(std::time(nullptr));
    info.volume_id = config.volume_id;
    info.storage_endpoint = config.storage_endpoint;
    info.status = SnapshotStatus::CREATED;
    info.created_at = current_time_string();

    bool ok = false;
    switch (config.vendor) {
        case StorageVendor::NETAPP: ok = create_netapp_snapshot(config); break;
        case StorageVendor::GENERIC_LVM: ok = create_lvm_snapshot(config); break;
        default: ok = create_generic_snapshot(config); break;
    }

    if (!ok) info.status = SnapshotStatus::FAILED;

    std::lock_guard<std::mutex> lock(snapshots_mutex_);
    snapshots_[info.snapshot_id] = info;
    return info;
}

bool StorageSnapshotBackup::delete_snapshot(const std::string& snapshot_id, const std::string& storage_endpoint) {
    spdlog::info("Deleting snapshot {} from {}", snapshot_id, storage_endpoint);
    return true;
}

bool StorageSnapshotBackup::mount_snapshot(const std::string& snapshot_id, const std::string& mount_point) {
    spdlog::info("Mounting snapshot {} on {}", snapshot_id, mount_point);
    fs::create_directories(mount_point);
    return true;
}

bool StorageSnapshotBackup::unmount_snapshot(const std::string& mount_point) {
    spdlog::info("Unmounting {}", mount_point);
    return system(("umount " + mount_point + " 2>/dev/null").c_str()) == 0;
}

std::vector<StorageSnapshotInfo> StorageSnapshotBackup::list_snapshots(const std::string& storage_endpoint) {
    std::lock_guard<std::mutex> lock(snapshots_mutex_);
    std::vector<StorageSnapshotInfo> result;
    for (auto& [id, snap] : snapshots_) {
        if (snap.storage_endpoint == storage_endpoint) result.push_back(snap);
    }
    return result;
}

bool StorageSnapshotBackup::backup_from_snapshot(const std::string& snapshot_id, const std::string& mount_point,
                                                  const std::string& output_path, SnapshotCallback callback) {
    spdlog::info("Backing up from snapshot {} to {}", snapshot_id, output_path);
    StorageSnapshotInfo info;
    {
        std::lock_guard<std::mutex> lock(snapshots_mutex_);
        auto it = snapshots_.find(snapshot_id);
        if (it != snapshots_.end()) info = it->second;
    }
    info.status = SnapshotStatus::BACKING_UP;
    if (callback) callback(info);

    // Simulate backup
    std::this_thread::sleep_for(std::chrono::seconds(2));

    info.status = SnapshotStatus::COMPLETED;
    if (callback) callback(info);
    return true;
}

bool StorageSnapshotBackup::create_netapp_snapshot(const StorageSnapshotConfig& config) {
    spdlog::info("NetApp: creating snapshot {} for volume {}", config.snapshot_name, config.volume_id);
    // NetApp API: POST /storage/volumes/{volume}/snapshots
    return true;
}

bool StorageSnapshotBackup::create_lvm_snapshot(const StorageSnapshotConfig& config) {
    spdlog::info("LVM: creating snapshot {} for {}", config.snapshot_name, config.volume_id);
    std::string cmd = "lvcreate --snapshot -L 10G --name " + config.snapshot_name + " " + config.volume_id + " 2>/dev/null";
    return system(cmd.c_str()) == 0;
}

bool StorageSnapshotBackup::create_generic_snapshot(const StorageSnapshotConfig& config) {
    spdlog::info("Generic: creating snapshot {} for {}", config.snapshot_name, config.volume_id);
    return true;
}

} // namespace obs
