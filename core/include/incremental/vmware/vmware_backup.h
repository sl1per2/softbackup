#pragma once
#include "incremental/common/incremental_engine.h"

namespace vovqa::incremental {

enum class VmwareTransport { AUTO, DIRECT_SAN, HOT_ADD, NBD, NBDSSL, NETWORK };

struct VmwareVmInfo {
    std::string vm_id;
    std::string vmx_path;
    std::string datastore;
    std::string esxi_host;
    bool cbt_enabled;
    std::string change_id;
    std::vector<std::string> disk_paths;
};

class VmwareBackup : public IncrementalBackupEngine {
public:
    std::string name() const override { return "VMware vSphere Incremental"; }
    std::vector<BlockRange> detect_changes(const std::string& path) override;
    bool read_changed_blocks(const std::string& path, const std::vector<BlockRange>& ranges,
                             std::function<void(const uint8_t*, size_t, uint64_t)> callback) override;
    std::vector<BlockRange> merge_bitmaps(const std::vector<BlockRange>& a, const std::vector<BlockRange>& b) override;

    bool enable_cbt(const std::string& vm_id);
    std::vector<BlockRange> query_changed_disk_areas(const std::string& vm_id, const std::string& change_id_start);
    VmwareTransport select_transport(const VmwareVmInfo& vm);
    bool quiesce_vm(const std::string& vm_id);
    bool consolidate_snapshots(const std::string& vm_id);

private:
    VmwareTransport auto_detect_transport(const VmwareVmInfo& vm);
    bool read_vmdk_direct_san(const std::string& vmdk_path, const std::vector<BlockRange>& ranges,
                              std::function<void(const uint8_t*, size_t, uint64_t)> callback);
    bool read_vmdk_hot_add(const std::string& vmdk_path, const std::vector<BlockRange>& ranges,
                           std::function<void(const uint8_t*, size_t, uint64_t)> callback);
    bool read_vmdk_nbd(const std::string& vmdk_path, const std::vector<BlockRange>& ranges,
                       std::function<void(const uint8_t*, size_t, uint64_t)> callback);

    VmwareTransport current_transport_ = VmwareTransport::AUTO;
};

} // namespace vovqa::incremental
