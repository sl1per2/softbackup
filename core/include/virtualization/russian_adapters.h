#pragma once
#include "virtualization/virtual_machine.h"
#include <curl/curl.h>

namespace obs {

// VMmanager
struct VMmanagerConfig {
    std::string host;
    uint16_t port = 80;
    std::string api_key;
    bool use_ssl = false;
};

class VMmanagerAdapter : public VirtualizationAdapter {
public:
    HypervisorType type() const override { return HypervisorType::UNKNOWN; }
    std::string name() const override { return "VMmanager"; }
    bool connect(const std::string& host, uint16_t port, const std::string& user, const std::string& pass) override;
    void disconnect() override;
    bool is_connected() const override;
    std::vector<VirtualMachine> list_vms() override;
    VirtualMachine get_vm(const std::string& vm_id) override;
    std::vector<VmSnapshot> list_snapshots(const std::string& vm_id) override;
    std::string create_snapshot(const std::string& vm_id, const std::string& name, bool quiesce = true) override;
    bool remove_snapshot(const std::string& vm_id, const std::string& snapshot_id) override;
    bool revert_to_snapshot(const std::string& vm_id, const std::string& snapshot_id) override;
    bool power_on(const std::string& vm_id) override;
    bool power_off(const std::string& vm_id) override;
    bool suspend(const std::string& vm_id) override;
    bool backup_vm(const VmBackupConfig& config, VmProgressCallback callback) override;
    bool restore_vm(const VmRestoreConfig& config, VmProgressCallback callback) override;
    bool export_vm(const std::string& vm_id, const std::string& format, const std::string& output_path, VmProgressCallback callback) override;
    bool import_vm(const std::string& import_path, const std::string& target_host, const std::string& target_datastore, VmProgressCallback callback) override;
    std::vector<ChangedBlockInfo> get_changed_blocks(const std::string& vm_id, const std::string& from_snap, const std::string& to_snap) override;
    bool is_hot_add_supported() const override { return false; }
    bool is_cbt_supported() const override { return false; }
    bool is_quiesce_supported() const override { return true; }
    std::vector<BackupTransport> supported_transports() const override { return {BackupTransport::NETWORK}; }
    std::vector<std::string> supported_versions() const { return {"VMmanager 5.x-6.x"}; }
private:
    bool connected_ = false;
};

// Альт Виртуализация
class AltVirtualizationAdapter : public VirtualizationAdapter {
public:
    HypervisorType type() const override { return HypervisorType::UNKNOWN; }
    std::string name() const override { return "Альт Виртуализация"; }
    bool connect(const std::string& host, uint16_t port, const std::string& user, const std::string& pass) override;
    void disconnect() override;
    bool is_connected() const override;
    std::vector<VirtualMachine> list_vms() override;
    VirtualMachine get_vm(const std::string& vm_id) override;
    std::vector<VmSnapshot> list_snapshots(const std::string& vm_id) override;
    std::string create_snapshot(const std::string& vm_id, const std::string& name, bool quiesce = true) override;
    bool remove_snapshot(const std::string& vm_id, const std::string& snapshot_id) override;
    bool revert_to_snapshot(const std::string& vm_id, const std::string& snapshot_id) override;
    bool power_on(const std::string& vm_id) override;
    bool power_off(const std::string& vm_id) override;
    bool suspend(const std::string& vm_id) override;
    bool backup_vm(const VmBackupConfig& config, VmProgressCallback callback) override;
    bool restore_vm(const VmRestoreConfig& config, VmProgressCallback callback) override;
    bool export_vm(const std::string& vm_id, const std::string& format, const std::string& output_path, VmProgressCallback callback) override;
    bool import_vm(const std::string& import_path, const std::string& target_host, const std::string& target_datastore, VmProgressCallback callback) override;
    std::vector<ChangedBlockInfo> get_changed_blocks(const std::string& vm_id, const std::string& from_snap, const std::string& to_snap) override;
    bool is_hot_add_supported() const override { return false; }
    bool is_cbt_supported() const override { return false; }
    bool is_quiesce_supported() const override { return true; }
    std::vector<BackupTransport> supported_transports() const override { return {BackupTransport::NETWORK}; }
    std::vector<std::string> supported_versions() const { return {"Альт Виртуализация 9.x-10.x"}; }
private:
    bool connected_ = false;
};

// Basis Dynamix
class BasisDynamixAdapter : public VirtualizationAdapter {
public:
    HypervisorType type() const override { return HypervisorType::UNKNOWN; }
    std::string name() const override { return "Basis Dynamix"; }
    bool connect(const std::string& host, uint16_t port, const std::string& user, const std::string& pass) override;
    void disconnect() override;
    bool is_connected() const override;
    std::vector<VirtualMachine> list_vms() override;
    VirtualMachine get_vm(const std::string& vm_id) override;
    std::vector<VmSnapshot> list_snapshots(const std::string& vm_id) override;
    std::string create_snapshot(const std::string& vm_id, const std::string& name, bool quiesce = true) override;
    bool remove_snapshot(const std::string& vm_id, const std::string& snapshot_id) override;
    bool revert_to_snapshot(const std::string& vm_id, const std::string& snapshot_id) override;
    bool power_on(const std::string& vm_id) override;
    bool power_off(const std::string& vm_id) override;
    bool suspend(const std::string& vm_id) override;
    bool backup_vm(const VmBackupConfig& config, VmProgressCallback callback) override;
    bool restore_vm(const VmRestoreConfig& config, VmProgressCallback callback) override;
    bool export_vm(const std::string& vm_id, const std::string& format, const std::string& output_path, VmProgressCallback callback) override;
    bool import_vm(const std::string& import_path, const std::string& target_host, const std::string& target_datastore, VmProgressCallback callback) override;
    std::vector<ChangedBlockInfo> get_changed_blocks(const std::string& vm_id, const std::string& from_snap, const std::string& to_snap) override;
    bool is_hot_add_supported() const override { return false; }
    bool is_cbt_supported() const override { return false; }
    bool is_quiesce_supported() const override { return true; }
    std::vector<BackupTransport> supported_transports() const override { return {BackupTransport::NETWORK}; }
    std::vector<std::string> supported_versions() const { return {"Basis Dynamix Enterprise", "Basis Dynamix Standard"}; }
private:
    bool connected_ = false;
};

// АЭРОДИСК vAir
class AerodiskVairAdapter : public VirtualizationAdapter {
public:
    HypervisorType type() const override { return HypervisorType::UNKNOWN; }
    std::string name() const override { return "АЭРОДИСК vAir"; }
    bool connect(const std::string& host, uint16_t port, const std::string& user, const std::string& pass) override;
    void disconnect() override;
    bool is_connected() const override;
    std::vector<VirtualMachine> list_vms() override;
    VirtualMachine get_vm(const std::string& vm_id) override;
    std::vector<VmSnapshot> list_snapshots(const std::string& vm_id) override;
    std::string create_snapshot(const std::string& vm_id, const std::string& name, bool quiesce = true) override;
    bool remove_snapshot(const std::string& vm_id, const std::string& snapshot_id) override;
    bool revert_to_snapshot(const std::string& vm_id, const std::string& snapshot_id) override;
    bool power_on(const std::string& vm_id) override;
    bool power_off(const std::string& vm_id) override;
    bool suspend(const std::string& vm_id) override;
    bool backup_vm(const VmBackupConfig& config, VmProgressCallback callback) override;
    bool restore_vm(const VmRestoreConfig& config, VmProgressCallback callback) override;
    bool export_vm(const std::string& vm_id, const std::string& format, const std::string& output_path, VmProgressCallback callback) override;
    bool import_vm(const std::string& import_path, const std::string& target_host, const std::string& target_datastore, VmProgressCallback callback) override;
    std::vector<ChangedBlockInfo> get_changed_blocks(const std::string& vm_id, const std::string& from_snap, const std::string& to_snap) override;
    bool is_hot_add_supported() const override { return false; }
    bool is_cbt_supported() const override { return false; }
    bool is_quiesce_supported() const override { return true; }
    std::vector<BackupTransport> supported_transports() const override { return {BackupTransport::NETWORK}; }
private:
    bool connected_ = false;
};

// Р-Виртуализация
class RVirtualizationAdapter : public VirtualizationAdapter {
public:
    HypervisorType type() const override { return HypervisorType::UNKNOWN; }
    std::string name() const override { return "Р-Виртуализация"; }
    bool connect(const std::string& host, uint16_t port, const std::string& user, const std::string& pass) override;
    void disconnect() override;
    bool is_connected() const override;
    std::vector<VirtualMachine> list_vms() override;
    VirtualMachine get_vm(const std::string& vm_id) override;
    std::vector<VmSnapshot> list_snapshots(const std::string& vm_id) override;
    std::string create_snapshot(const std::string& vm_id, const std::string& name, bool quiesce = true) override;
    bool remove_snapshot(const std::string& vm_id, const std::string& snapshot_id) override;
    bool revert_to_snapshot(const std::string& vm_id, const std::string& snapshot_id) override;
    bool power_on(const std::string& vm_id) override;
    bool power_off(const std::string& vm_id) override;
    bool suspend(const std::string& vm_id) override;
    bool backup_vm(const VmBackupConfig& config, VmProgressCallback callback) override;
    bool restore_vm(const VmRestoreConfig& config, VmProgressCallback callback) override;
    bool export_vm(const std::string& vm_id, const std::string& format, const std::string& output_path, VmProgressCallback callback) override;
    bool import_vm(const std::string& import_path, const std::string& target_host, const std::string& target_datastore, VmProgressCallback callback) override;
    std::vector<ChangedBlockInfo> get_changed_blocks(const std::string& vm_id, const std::string& from_snap, const std::string& to_snap) override;
    bool is_hot_add_supported() const override { return false; }
    bool is_cbt_supported() const override { return false; }
    bool is_quiesce_supported() const override { return true; }
    std::vector<BackupTransport> supported_transports() const override { return {BackupTransport::NETWORK}; }
private:
    bool connected_ = false;
};

// РУСТЭК
class RustackAdapter : public VirtualizationAdapter {
public:
    HypervisorType type() const override { return HypervisorType::UNKNOWN; }
    std::string name() const override { return "РУСТЭК"; }
    bool connect(const std::string& host, uint16_t port, const std::string& user, const std::string& pass) override;
    void disconnect() override;
    bool is_connected() const override;
    std::vector<VirtualMachine> list_vms() override;
    VirtualMachine get_vm(const std::string& vm_id) override;
    std::vector<VmSnapshot> list_snapshots(const std::string& vm_id) override;
    std::string create_snapshot(const std::string& vm_id, const std::string& name, bool quiesce = true) override;
    bool remove_snapshot(const std::string& vm_id, const std::string& snapshot_id) override;
    bool revert_to_snapshot(const std::string& vm_id, const std::string& snapshot_id) override;
    bool power_on(const std::string& vm_id) override;
    bool power_off(const std::string& vm_id) override;
    bool suspend(const std::string& vm_id) override;
    bool backup_vm(const VmBackupConfig& config, VmProgressCallback callback) override;
    bool restore_vm(const VmRestoreConfig& config, VmProgressCallback callback) override;
    bool export_vm(const std::string& vm_id, const std::string& format, const std::string& output_path, VmProgressCallback callback) override;
    bool import_vm(const std::string& import_path, const std::string& target_host, const std::string& target_datastore, VmProgressCallback callback) override;
    std::vector<ChangedBlockInfo> get_changed_blocks(const std::string& vm_id, const std::string& from_snap, const std::string& to_snap) override;
    bool is_hot_add_supported() const override { return false; }
    bool is_cbt_supported() const override { return false; }
    bool is_quiesce_supported() const override { return true; }
    std::vector<BackupTransport> supported_transports() const override { return {BackupTransport::NETWORK}; }
private:
    bool connected_ = false;
};

// OpenStack
class OpenStackAdapter : public VirtualizationAdapter {
public:
    HypervisorType type() const override { return HypervisorType::UNKNOWN; }
    std::string name() const override { return "OpenStack"; }
    bool connect(const std::string& host, uint16_t port, const std::string& user, const std::string& pass) override;
    void disconnect() override;
    bool is_connected() const override;
    std::vector<VirtualMachine> list_vms() override;
    VirtualMachine get_vm(const std::string& vm_id) override;
    std::vector<VmSnapshot> list_snapshots(const std::string& vm_id) override;
    std::string create_snapshot(const std::string& vm_id, const std::string& name, bool quiesce = true) override;
    bool remove_snapshot(const std::string& vm_id, const std::string& snapshot_id) override;
    bool revert_to_snapshot(const std::string& vm_id, const std::string& snapshot_id) override;
    bool power_on(const std::string& vm_id) override;
    bool power_off(const std::string& vm_id) override;
    bool suspend(const std::string& vm_id) override;
    bool backup_vm(const VmBackupConfig& config, VmProgressCallback callback) override;
    bool restore_vm(const VmRestoreConfig& config, VmProgressCallback callback) override;
    bool export_vm(const std::string& vm_id, const std::string& format, const std::string& output_path, VmProgressCallback callback) override;
    bool import_vm(const std::string& import_path, const std::string& target_host, const std::string& target_datastore, VmProgressCallback callback) override;
    std::vector<ChangedBlockInfo> get_changed_blocks(const std::string& vm_id, const std::string& from_snap, const std::string& to_snap) override;
    bool is_hot_add_supported() const override { return false; }
    bool is_cbt_supported() const override { return false; }
    bool is_quiesce_supported() const override { return true; }
    std::vector<BackupTransport> supported_transports() const override { return {BackupTransport::NETWORK}; }
    std::vector<std::string> supported_versions() const { return {"OpenStack Newton-Ussuri"}; }
    bool backup_volume(const std::string& volume_id, const std::string& output_path);
    bool restore_volume(const std::string& backup_path, const std::string& volume_id);
private:
    bool connected_ = false;
    std::string project_id_;
};

// SpaceVM
class SpaceVMAdapter : public VirtualizationAdapter {
public:
    HypervisorType type() const override { return HypervisorType::UNKNOWN; }
    std::string name() const override { return "Space VM"; }
    bool connect(const std::string& host, uint16_t port, const std::string& user, const std::string& pass) override;
    void disconnect() override;
    bool is_connected() const override;
    std::vector<VirtualMachine> list_vms() override;
    VirtualMachine get_vm(const std::string& vm_id) override;
    std::vector<VmSnapshot> list_snapshots(const std::string& vm_id) override;
    std::string create_snapshot(const std::string& vm_id, const std::string& name, bool quiesce = true) override;
    bool remove_snapshot(const std::string& vm_id, const std::string& snapshot_id) override;
    bool revert_to_snapshot(const std::string& vm_id, const std::string& snapshot_id) override;
    bool power_on(const std::string& vm_id) override;
    bool power_off(const std::string& vm_id) override;
    bool suspend(const std::string& vm_id) override;
    bool backup_vm(const VmBackupConfig& config, VmProgressCallback callback) override;
    bool restore_vm(const VmRestoreConfig& config, VmProgressCallback callback) override;
    bool export_vm(const std::string& vm_id, const std::string& format, const std::string& output_path, VmProgressCallback callback) override;
    bool import_vm(const std::string& import_path, const std::string& target_host, const std::string& target_datastore, VmProgressCallback callback) override;
    std::vector<ChangedBlockInfo> get_changed_blocks(const std::string& vm_id, const std::string& from_snap, const std::string& to_snap) override;
    bool is_hot_add_supported() const override { return false; }
    bool is_cbt_supported() const override { return false; }
    bool is_quiesce_supported() const override { return true; }
    std::vector<BackupTransport> supported_transports() const override { return {BackupTransport::NETWORK}; }
private:
    bool connected_ = false;
};

// Tionix
class TionixAdapter : public VirtualizationAdapter {
public:
    HypervisorType type() const override { return HypervisorType::UNKNOWN; }
    std::string name() const override { return "Tionix"; }
    bool connect(const std::string& host, uint16_t port, const std::string& user, const std::string& pass) override;
    void disconnect() override;
    bool is_connected() const override;
    std::vector<VirtualMachine> list_vms() override;
    VirtualMachine get_vm(const std::string& vm_id) override;
    std::vector<VmSnapshot> list_snapshots(const std::string& vm_id) override;
    std::string create_snapshot(const std::string& vm_id, const std::string& name, bool quiesce = true) override;
    bool remove_snapshot(const std::string& vm_id, const std::string& snapshot_id) override;
    bool revert_to_snapshot(const std::string& vm_id, const std::string& snapshot_id) override;
    bool power_on(const std::string& vm_id) override;
    bool power_off(const std::string& vm_id) override;
    bool suspend(const std::string& vm_id) override;
    bool backup_vm(const VmBackupConfig& config, VmProgressCallback callback) override;
    bool restore_vm(const VmRestoreConfig& config, VmProgressCallback callback) override;
    bool export_vm(const std::string& vm_id, const std::string& format, const std::string& output_path, VmProgressCallback callback) override;
    bool import_vm(const std::string& import_path, const std::string& target_host, const std::string& target_datastore, VmProgressCallback callback) override;
    std::vector<ChangedBlockInfo> get_changed_blocks(const std::string& vm_id, const std::string& from_snap, const std::string& to_snap) override;
    bool is_hot_add_supported() const override { return false; }
    bool is_cbt_supported() const override { return false; }
    bool is_quiesce_supported() const override { return true; }
    std::vector<BackupTransport> supported_transports() const override { return {BackupTransport::NETWORK}; }
private:
    bool connected_ = false;
};

} // namespace obs
