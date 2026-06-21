#pragma once
#include "virtualization/virtual_machine.h"
#ifdef _MSC_VER
#include <windows.h>
#include <comdef.h>
#include <Wbemidl.h>
#endif

namespace obs {

struct HyperVConfig {
    std::string host;
    std::string username;
    std::string password;
    std::string domain;
    bool use_wmi = true;
    bool use_csv = true; // Cluster Shared Volumes
    std::string vss_provider; // VSS writer ID
    std::string smb_share; // for remote restore
};

class HyperVAdapter : public VirtualizationAdapter {
public:
    HyperVAdapter();
    ~HyperVAdapter() override;

    HypervisorType type() const override { return HypervisorType::HYPERV; }
    std::string name() const override { return "Microsoft Hyper-V"; }

    bool connect(const std::string& host, uint16_t port,
                const std::string& username, const std::string& password) override;
    void disconnect() override;
    bool is_connected() const override;

    std::vector<VirtualMachine> list_vms() override;
    VirtualMachine get_vm(const std::string& vm_id) override;
    std::vector<VmSnapshot> list_snapshots(const std::string& vm_id) override;

    std::string create_snapshot(const std::string& vm_id, const std::string& name,
                                bool quiesce = true) override;
    bool remove_snapshot(const std::string& vm_id, const std::string& snapshot_id) override;
    bool revert_to_snapshot(const std::string& vm_id, const std::string& snapshot_id) override;

    bool power_on(const std::string& vm_id) override;
    bool power_off(const std::string& vm_id) override;
    bool suspend(const std::string& vm_id) override;

    bool backup_vm(const VmBackupConfig& config, VmProgressCallback callback) override;
    bool restore_vm(const VmRestoreConfig& config, VmProgressCallback callback) override;
    bool export_vm(const std::string& vm_id, const std::string& format,
                  const std::string& output_path, VmProgressCallback callback) override;
    bool import_vm(const std::string& import_path, const std::string& target_host,
                  const std::string& target_datastore, VmProgressCallback callback) override;

    std::vector<ChangedBlockInfo> get_changed_blocks(
        const std::string& vm_id, const std::string& from_snapshot, const std::string& to_snapshot) override;

    bool is_hot_add_supported() const override { return true; }
    bool is_cbt_supported() const override { return true; } // RCT
    bool is_quiesce_supported() const override { return true; }
    std::vector<BackupTransport> supported_transports() const override {
        return {BackupTransport::NETWORK, BackupTransport::HOT_ADD};
    }

    // Hyper-V specific: Resilient Change Tracking
    std::vector<ChangedBlockInfo> get_rct_changed_blocks(const std::string& vm_id);

    // Hyper-V specific: VSS checkpoint (consistent snapshot)
    bool create_vss_checkpoint(const std::string& vm_id, const std::string& name);

    // Hyper-V specific: export to VHD/VHDX
    bool export_to_vhdx(const std::string& vm_id, const std::string& output_path, VmProgressCallback callback);

    // Hyper-V specific: CSV (Cluster Shared Volume) awareness
    std::string get_csv_path(const std::string& vm_id);

private:
    bool wmi_connect();
    std::string wmi_query(const std::string& wql);
    bool vss_create_checkpoint(const std::string& vm_id);
    bool vss_release();

    HyperVConfig config_;
    bool connected_ = false;
#ifdef _MSC_VER
    IWbemLocator* wmi_loc_ = nullptr;
    IWbemServices* wmi_svc_ = nullptr;
#endif
    std::map<std::string, VirtualMachine> vm_cache_;
};

} // namespace obs
