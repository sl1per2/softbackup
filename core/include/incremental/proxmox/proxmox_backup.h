#pragma once
#include "incremental/common/incremental_engine.h"

namespace vovqa::incremental {

enum class ProxmoxTrackingMethod { PBS_API, QEMU_DIRTY_BITMAP, QCOW2_SNAP, LVM_THIN, ZFS, QGA_FSFREEZE, AUTO };

struct ProxmoxVmInfo {
    std::string vmid;
    std::string name;
    std::string storage;       // local, zfs, ceph, nfs
    std::string disk_format;   // qcow2, raw
    std::string disk_path;
    bool qga_installed;
};

class ProxmoxBackup : public IncrementalBackupEngine {
public:
    std::string name() const override { return "Proxmox VE Incremental"; }
    std::vector<BlockRange> detect_changes(const std::string& path) override;
    bool read_changed_blocks(const std::string& path, const std::vector<BlockRange>& ranges,
                             std::function<void(const uint8_t*, size_t, uint64_t)> callback) override;
    std::vector<BlockRange> merge_bitmaps(const std::vector<BlockRange>& a, const std::vector<BlockRange>& b) override;

    bool create_qemu_dirty_bitmap(const std::string& disk_path, const std::string& bitmap_name);
    std::vector<BlockRange> dump_qemu_dirty_bitmap(const std::string& disk_path, const std::string& bitmap_name);
    bool clear_qemu_dirty_bitmap(const std::string& disk_path, const std::string& bitmap_name);
    bool create_qcow2_snapshot(const std::string& disk_path, const std::string& snap_name);
    bool qga_fsfreeze(const std::string& vmid);
    bool qga_fsthaw(const std::string& vmid);
    bool vzdump_incremental(const std::string& vmid, const std::string& storage);

private:
    ProxmoxTrackingMethod auto_detect(const ProxmoxVmInfo& vm);
    std::vector<BlockRange> parse_qemu_bitmap_output(const std::string& json_output);
};

} // namespace vovqa::incremental
