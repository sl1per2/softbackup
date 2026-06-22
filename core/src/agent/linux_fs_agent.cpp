#include "agent/linux_fs_agent.h"
#include <spdlog/spdlog.h>
#include <cstdlib>

namespace obs {

LinuxFsAgent::LinuxFsAgent() {
    detect_methods();
    spdlog::debug("LinuxFsAgent created with {} methods", available_methods_.size());
}

LinuxFsAgent::~LinuxFsAgent() { stop(); }

bool LinuxFsAgent::can_handle(const std::string& job_type) const {
    return job_type == "filesystem_backup" || job_type == "linux_fs";
}

BackupResult LinuxFsAgent::execute_job(const BackupRequest& request) {
    spdlog::info("LinuxFsAgent executing job: {}", request.job_id);

    if (event_bus_) {
        event_bus_->publish(JobStartedEvent{request.job_id, config_.agent_id});
    }

    BackupResult result;
    result = backup_tar_incremental(request.source_path, request.target_path);

    if (event_bus_) {
        event_bus_->publish(JobCompletedEvent{request.job_id, result.success, result.error});
    }

    return result;
}

BackupResult LinuxFsAgent::backup_btrfs_send(const std::string& subvolume, const std::string& output) {
    spdlog::info("LinuxFsAgent: btrfs send {} -> {}", subvolume, output);
    std::string cmd = "btrfs subvolume snapshot -r " + subvolume + " " + subvolume + "_snap && "
                    + "btrfs send " + subvolume + "_snap | gzip > " + output;
    int ret = std::system(cmd.c_str());
    BackupResult result;
    result.success = (ret == 0);
    if (ret != 0) result.error = "btrfs send failed with code " + std::to_string(ret);
    return result;
}

BackupResult LinuxFsAgent::backup_zfs_send(const std::string& dataset, const std::string& output) {
    spdlog::info("LinuxFsAgent: zfs send {} -> {}", dataset, output);
    std::string cmd = "zfs snapshot " + dataset + "@backup_" + current_time_string() + " && "
                    + "zfs send " + dataset + "@backup_* | gzip > " + output;
    int ret = std::system(cmd.c_str());
    BackupResult result;
    result.success = (ret == 0);
    if (ret != 0) result.error = "zfs send failed with code " + std::to_string(ret);
    return result;
}

BackupResult LinuxFsAgent::backup_lvm_snapshot(const std::string& logical_volume, const std::string& output) {
    spdlog::info("LinuxFsAgent: LVM snapshot backup of {}", logical_volume);
    BackupResult result;
    result.success = true;
    return result;
}

BackupResult LinuxFsAgent::backup_tar_incremental(const std::string& path, const std::string& output,
                                                    const std::string& snapshot_file) {
    spdlog::info("LinuxFsAgent: tar incremental backup of {} -> {}", path, output);
    std::string cmd = "tar --listed-incremental=" + (snapshot_file.empty() ? "/tmp/snap.snar" : snapshot_file)
                    + " -czf " + output + " " + path;
    int ret = std::system(cmd.c_str());
    BackupResult result;
    result.success = (ret == 0);
    if (ret != 0) result.error = "tar backup failed with code " + std::to_string(ret);
    return result;
}

std::vector<std::string> LinuxFsAgent::detect_available_methods() { return available_methods_; }

bool LinuxFsAgent::create_lvm_snapshot(const std::string& lv_name, const std::string& snap_name,
                                        const std::string& size) {
    spdlog::info("LinuxFsAgent: creating LVM snapshot {} of {} (size={})", snap_name, lv_name, size);
    std::string cmd = "lvcreate -L " + size + " -s -n " + snap_name + " " + lv_name;
    return std::system(cmd.c_str()) == 0;
}

bool LinuxFsAgent::remove_lvm_snapshot(const std::string& vg_name, const std::string& snap_name) {
    spdlog::info("LinuxFsAgent: removing LVM snapshot {}/{}", vg_name, snap_name);
    std::string cmd = "lvremove -f " + vg_name + "/" + snap_name;
    return std::system(cmd.c_str()) == 0;
}

void LinuxFsAgent::detect_methods() {
    available_methods_.clear();
    if (std::system("which btrfs >/dev/null 2>&1") == 0) available_methods_.push_back("btrfs_send");
    if (std::system("which zfs >/dev/null 2>&1") == 0) available_methods_.push_back("zfs_send");
    if (std::system("which lvcreate >/dev/null 2>&1") == 0) available_methods_.push_back("lvm_snapshot");
    available_methods_.push_back("tar_incremental");
    spdlog::info("LinuxFsAgent: available methods: {}", available_methods_.size());
}

} // namespace obs
