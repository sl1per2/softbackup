#pragma once
#include "incremental/common/incremental_engine.h"

namespace vovqa::incremental {

enum class HyperVTrackingMethod { RCT, VHDX_SNAPSHOT, VSS_WRITER, AUTO };

struct HyperVVmInfo {
    std::string vm_id;
    std::string vm_name;
    std::string vhdx_path;
    std::string host;
    bool rct_enabled;
    bool replication_enabled;
};

class HyperVBackup : public IncrementalBackupEngine {
public:
    std::string name() const override { return "Hyper-V Incremental"; }
    std::vector<BlockRange> detect_changes(const std::string& path) override;
    bool read_changed_blocks(const std::string& path, const std::vector<BlockRange>& ranges,
                             std::function<void(const uint8_t*, size_t, uint64_t)> callback) override;
    std::vector<BlockRange> merge_bitmaps(const std::vector<BlockRange>& a, const std::vector<BlockRange>& b) override;

    bool enable_rct(const std::string& vm_id);
    std::vector<BlockRange> get_rct_changed_blocks(const std::string& vm_id);
    bool create_vhdx_checkpoint(const std::string& vm_id, const std::string& name);
    bool remove_vhdx_checkpoint(const std::string& vm_id, const std::string& checkpoint_id);
    bool vss_backup_with_hyper_v_writer(const std::string& vm_id);

private:
    HyperVTrackingMethod auto_detect(const HyperVVmInfo& vm);
    std::vector<uint8_t> get_vhdx_bitmap(const std::string& vhdx_path);
};

} // namespace vovqa::incremental
