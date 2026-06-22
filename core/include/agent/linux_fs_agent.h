#pragma once
#include "agent/generic_agent.h"
#include "incremental/linux/linux_fs_backup.h"

namespace obs {

class LinuxFsAgent : public GenericAgent {
public:
    LinuxFsAgent();
    ~LinuxFsAgent() override;

    AgentType type() const override { return AgentType::LINUX_FS; }
    std::string type_name() const override { return "linux_fs"; }
    std::string component_name() const override { return "LinuxFsAgent"; }

    bool can_handle(const std::string& job_type) const override;
    BackupResult execute_job(const BackupRequest& request) override;

    BackupResult backup_btrfs_send(const std::string& subvolume, const std::string& output);
    BackupResult backup_zfs_send(const std::string& dataset, const std::string& output);
    BackupResult backup_lvm_snapshot(const std::string& logical_volume, const std::string& output);
    BackupResult backup_tar_incremental(const std::string& path, const std::string& output,
                                         const std::string& snapshot_file = "");
    std::vector<std::string> detect_available_methods();
    bool create_lvm_snapshot(const std::string& lv_name, const std::string& snap_name, const std::string& size);
    bool remove_lvm_snapshot(const std::string& vg_name, const std::string& snap_name);

private:
    std::vector<std::string> available_methods_;
    void detect_methods();
};

} // namespace obs
