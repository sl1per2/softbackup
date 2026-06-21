#pragma once
#include "common.h"
#include <functional>
#include <vector>
#include <string>

namespace obs {

enum class DiskFormat { VMDK, QCOW2, VHDX, RAW, VDI, VHD };

struct ConvertConfig {
    std::string source_path;
    std::string source_format;
    std::string target_path;
    std::string target_format;
    bool preserve_uuid = false;
    bool auto_inject_drivers = true;
    bool update_initramfs = true;
    bool adapt_config = true;
    std::string vm_config_path;
    int64_t disk_size_bytes = 0;
};

struct DriverInfo {
    std::string name;
    std::string type; // "storage", "network", "memory_balloon"
    std::string source_os; // "linux", "windows"
    std::string driver_path;
    std::string inf_path;
    bool installed = false;
};

struct ConvertResult {
    bool success = false;
    uint64_t source_size = 0;
    uint64_t target_size = 0;
    double conversion_ratio = 0;
    int32_t elapsed_seconds = 0;
    std::vector<DriverInfo> drivers_injected;
    bool initramfs_updated = false;
    bool config_adapted = false;
    std::string error;
};

struct VmConfig {
    std::string vm_type; // "vmware", "kvm", "hyperv"
    std::string vm_id;
    std::string vm_name;
    int32_t cpu_count = 1;
    int64_t memory_mb = 1024;
    std::vector<std::string> disk_paths;
    std::string network_adapter;
    std::string os_type;
    std::string os_version;
    std::string config_path;
};

class CrossPlatformRestore {
public:
    CrossPlatformRestore();
    ~CrossPlatformRestore();

    ConvertResult convert_disk(const ConvertConfig& config);
    bool inject_drivers(const std::string& disk_path, const std::string& disk_format,
                       const std::string& os_type, const std::string& os_version);
    bool update_initramfs(const std::string& disk_path, const std::string& mount_point);
    bool adapt_vm_config(const VmConfig& source, VmConfig& target);

    ConvertResult restore_vm(const std::string& backup_path, const std::string& source_format,
                            const std::string& target_format, const std::string& target_path,
                            const VmConfig& target_config);

    std::vector<DriverInfo> detect_required_drivers(const std::string& os_type, const std::string& os_version,
                                                     const std::string& target_hypervisor);

    // Specific conversions
    bool vmdk_to_qcow2(const std::string& src, const std::string& dst);
    bool vmdk_to_vhdx(const std::string& src, const std::string& dst);
    bool qcow2_to_vmdk(const std::string& src, const std::string& dst);
    bool qcow2_to_vhdx(const std::string& src, const std::string& dst);
    bool vhdx_to_qcow2(const std::string& src, const std::string& dst);
    bool vhdx_to_vmdk(const std::string& src, const std::string& dst);

private:
    bool convert_format(const std::string& src, const std::string& src_fmt,
                        const std::string& dst, const std::string& dst_fmt);
    bool mount_disk(const std::string& disk_path, const std::string& mount_point, const std::string& format);
    bool unmount_disk(const std::string& mount_point);
    bool inject_linux_drivers(const std::string& mount_point, const std::string& target_hypervisor);
    bool inject_windows_drivers(const std::string& mount_point, const std::string& target_hypervisor);
    bool update_linux_initramfs(const std::string& mount_point);
    bool update_windows_drivers(const std::string& mount_point, const std::string& target_hypervisor);

    bool adapt_vmware_config(const VmConfig& source, VmConfig& target);
    bool adapt_kvm_config(const VmConfig& source, VmConfig& target);
    bool adapt_hyperv_config(const VmConfig& source, VmConfig& target);

    std::map<std::string, std::vector<DriverInfo>> driver_registry_;
};

} // namespace obs
