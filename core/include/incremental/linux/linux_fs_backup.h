#pragma once
#include "incremental/common/incremental_engine.h"

namespace vovqa::incremental {

enum class LinuxTrackingMethod { BTRFS_SEND, ZFS_SEND, LVM_THIN, DM_ERA, FANOTIFY, TAR_INCREMENTAL, AUTO };

class LinuxFsBackup : public IncrementalBackupEngine {
public:
    std::string name() const override { return "Linux FS Incremental"; }
    std::vector<BlockRange> detect_changes(const std::string& path) override;
    bool read_changed_blocks(const std::string& path, const std::vector<BlockRange>& ranges,
                             std::function<void(const uint8_t*, size_t, uint64_t)> callback) override;
    std::vector<BlockRange> merge_bitmaps(const std::vector<BlockRange>& a, const std::vector<BlockRange>& b) override;

    LinuxTrackingMethod detect_method(const std::string& path);
    void set_tracking_method(LinuxTrackingMethod m) { method_ = m; }

private:
    std::vector<BlockRange> detect_btrfs(const std::string& path);
    std::vector<BlockRange> detect_zfs(const std::string& path);
    std::vector<BlockRange> detect_lvm(const std::string& path);
    std::vector<BlockRange> detect_dm_era(const std::string& path);
    std::vector<BlockRange> detect_fanotify(const std::string& path);
    std::vector<BlockRange> detect_tar_incremental(const std::string& path);

    bool btrfs_send_incremental(const std::string& path, const std::string& parent_snapshot);
    bool zfs_send_incremental(const std::string& dataset, const std::string& from_snap, const std::string& to_snap);
    bool parse_dm_era_status(const std::string& status, std::vector<BlockRange>& ranges);
    bool create_lvm_snapshot(const std::string& vg, const std::string& lv, const std::string& snap_name);

    LinuxTrackingMethod method_ = LinuxTrackingMethod::AUTO;
    std::string current_snapshot_;
    std::string previous_snapshot_;
};

} // namespace vovqa::incremental
