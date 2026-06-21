#pragma once
#include "virtualization/virtual_machine.h"
#include <curl/curl.h>

namespace obs {

struct VmwareConfig {
    std::string vcenter_host;
    uint16_t vcenter_port = 443;
    std::string username;
    std::string password;
    std::string datacenter;
    std::string cluster;
    bool verify_ssl = false;
    std::string vddk_path; // VMware VDDK SDK path
    std::string nbd_server;
    uint16_t nbd_port = 9092;
};

class VMwareAdapter : public VirtualizationAdapter {
public:
    VMwareAdapter();
    ~VMwareAdapter() override;

    HypervisorType type() const override { return HypervisorType::VMWARE; }
    std::string name() const override { return "VMware vSphere/ESXi"; }

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
    bool is_cbt_supported() const override { return true; }
    bool is_quiesce_supported() const override { return true; }
    std::vector<BackupTransport> supported_transports() const override {
        return {BackupTransport::HOT_ADD, BackupTransport::NBD, BackupTransport::NETWORK,
                BackupTransport::DIRECT_SAN, BackupTransport::HOTADD_NBD};
    }

    // VMware-specific: get virtual disk changed blocks via CBT
    std::vector<ChangedBlockInfo> get_cbt_changed_blocks(const std::string& vm_id,
                                                          uint64_t change_id_start,
                                                          uint64_t change_id_end);

    // VMware-specific: Hot Add virtual disk
    bool hot_add_disk(const std::string& vm_id, const std::string& disk_path, int controller);

    // VMware-specific: NBD export
    bool nbd_export_disk(const std::string& vm_id, const std::string& disk_id,
                        const std::string& output_path, VmProgressCallback callback);

private:
    std::string api_get(const std::string& path);
    std::string api_post(const std::string& path, const std::string& body);
    std::string api_delete(const std::string& path);
    bool login();
    std::string get_vm_mor(const std::string& vm_id);

    VmwareConfig config_;
    std::string session_id_;
    bool connected_ = false;
    CURL* curl_ = nullptr;
    std::map<std::string, VirtualMachine> vm_cache_;
};

} // namespace obs
