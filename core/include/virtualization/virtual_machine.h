#pragma once
#include "common.h"
#include <functional>
#include <memory>

namespace obs {

enum class HypervisorType { VMWARE, HYPERV, PROXMOX, KVM, UNKNOWN };
enum class VmPowerState { POWERED_ON, POWERED_OFF, SUSPENDED, UNKNOWN };
enum class BackupTransport { HOT_ADD, NBD, NETWORK, DIRECT_SAN, HOTADD_NBD, SAN };

struct VirtualMachine {
    std::string vm_id;
    std::string name;
    std::string host;
    std::string datacenter;
    std::string cluster;
    std::string resource_pool;
    std::string datastore;
    HypervisorType hypervisor;
    VmPowerState power_state;
    int32_t cpu_count = 0;
    int64_t memory_mb = 0;
    int64_t disk_total_bytes = 0;
    int64_t disk_used_bytes = 0;
    std::string os_type;
    std::string ip_address;
    std::vector<std::string> disk_paths;
    std::string uuid;
    bool tools_installed = false;
};

struct VmSnapshot {
    std::string snapshot_id;
    std::string vm_id;
    std::string name;
    std::string description;
    std::string created_at;
    bool quiesced = false;
    uint64_t size_bytes = 0;
};

struct VmBackupJob {
    std::string job_id;
    std::string vm_id;
    std::string vm_name;
    HypervisorType hypervisor;
    BackupTransport transport;
    std::string status;
    uint64_t total_bytes = 0;
    uint64_t transferred_bytes = 0;
    double speed_mbps = 0;
    std::string snapshot_id;
    std::string error;
};

struct VmBackupConfig {
    std::string vm_id;
    std::string storage_id;
    BackupTransport transport = BackupTransport::NETWORK;
    bool use_cbt = true;
    bool quiesce = true;
    bool encryption_enabled = false;
    std::string encryption_key;
    int compression_level = 1;
    int bandwidth_limit_kbps = 0;
    bool create_snapshot = true;
    bool remove_snapshot_after = true;
    std::string exclude_disks; // comma-separated
};

struct VmRestoreConfig {
    std::string backup_id;
    std::string vm_id;
    std::string target_host;
    std::string target_datastore;
    std::string target_resource_pool;
    bool power_on_after = false;
    std::string new_vm_name;
    std::string network_mapping; // old=new pairs
};

struct ChangedBlockInfo {
    uint64_t offset;
    uint32_t size;
    bool dirty;
};

using VmProgressCallback = std::function<void(const VmBackupJob&)>;

class VirtualizationAdapter {
public:
    virtual ~VirtualizationAdapter() = default;

    virtual HypervisorType type() const = 0;
    virtual std::string name() const = 0;

    virtual bool connect(const std::string& host, uint16_t port,
                        const std::string& username, const std::string& password) = 0;
    virtual void disconnect() = 0;
    virtual bool is_connected() const = 0;

    virtual std::vector<VirtualMachine> list_vms() = 0;
    virtual VirtualMachine get_vm(const std::string& vm_id) = 0;
    virtual std::vector<VmSnapshot> list_snapshots(const std::string& vm_id) = 0;

    virtual std::string create_snapshot(const std::string& vm_id, const std::string& name,
                                        bool quiesce = true) = 0;
    virtual bool remove_snapshot(const std::string& vm_id, const std::string& snapshot_id) = 0;
    virtual bool revert_to_snapshot(const std::string& vm_id, const std::string& snapshot_id) = 0;

    virtual bool power_on(const std::string& vm_id) = 0;
    virtual bool power_off(const std::string& vm_id) = 0;
    virtual bool suspend(const std::string& vm_id) = 0;

    virtual bool backup_vm(const VmBackupConfig& config, VmProgressCallback callback) = 0;
    virtual bool restore_vm(const VmRestoreConfig& config, VmProgressCallback callback) = 0;
    virtual bool export_vm(const std::string& vm_id, const std::string& format,
                          const std::string& output_path, VmProgressCallback callback) = 0;
    virtual bool import_vm(const std::string& import_path, const std::string& target_host,
                          const std::string& target_datastore, VmProgressCallback callback) = 0;

    virtual std::vector<ChangedBlockInfo> get_changed_blocks(
        const std::string& vm_id, const std::string& from_snapshot, const std::string& to_snapshot) = 0;

    virtual bool is_hot_add_supported() const = 0;
    virtual bool is_cbt_supported() const = 0;
    virtual bool is_quiesce_supported() const = 0;
    virtual std::vector<BackupTransport> supported_transports() const = 0;
};

} // namespace obs
