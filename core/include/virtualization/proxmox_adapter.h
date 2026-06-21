#pragma once
#include "virtualization/virtual_machine.h"
#include <curl/curl.h>

namespace obs {

struct ProxmoxConfig {
    std::string host;
    uint16_t port = 8006;
    bool use_ssl = true;
    std::string username; // e.g. "root@pam"
    std::string password;
    std::string api_token; // alternatively use API token
    std::string node_name;
    std::string storage_name;
    std::string backup_storage; // e.g. "local", "nfs", "zfs"
    int parallel_jobs = 4;
};

class ProxmoxAdapter : public VirtualizationAdapter {
public:
    ProxmoxAdapter();
    ~ProxmoxAdapter() override;

    HypervisorType type() const override { return HypervisorType::PROXMOX; }
    std::string name() const override { return "Proxmox VE"; }

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

    bool is_hot_add_supported() const override { return false; }
    bool is_cbt_supported() const override { return false; }
    bool is_quiesce_supported() const override { return true; }
    std::vector<BackupTransport> supported_transports() const override {
        return {BackupTransport::NETWORK};
    }

    // Proxmox-specific: vzdump backup
    bool vzdump_backup(const std::string& vm_id, const std::string& storage,
                      bool snapshot = true, bool stop = false, VmProgressCallback callback = nullptr);

    // Proxmox-specific: vzdump restore
    bool vzdump_restore(const std::string& archive_path, const std::string& vm_id, VmProgressCallback callback = nullptr);

    // Proxmox-specific: qm disk operations
    bool resize_disk(const std::string& vm_id, int disk_index, const std::string& size);
    bool move_disk(const std::string& vm_id, int disk_index, const std::string& target_storage);

    // Proxmox-specific: backup without temporary storage (direct streaming)
    bool backup_direct_stream(const std::string& vm_id, const std::string& output_path, VmProgressCallback callback = nullptr);

    // Proxmox-specific: list available storages
    std::vector<std::string> list_storages();

    // Proxmox-specific: import OVA/OVF
    bool import_ova(const std::string& ova_path, const std::string& vm_name, VmProgressCallback callback = nullptr);

private:
    std::string api_get(const std::string& path);
    std::string api_post(const std::string& path, const std::string& body = "");
    std::string api_delete(const std::string& path);
    bool authenticate();
    std::string parse_vm_id(const std::string& vm_id); // "vm/100" -> "100"
    std::string get_vmid_from_api(const std::string& vm_id);

    ProxmoxConfig config_;
    std::string ticket_; // CSRF token
    std::string cookie_; // authentication cookie
    bool connected_ = false;
    CURL* curl_ = nullptr;
};

} // namespace obs
