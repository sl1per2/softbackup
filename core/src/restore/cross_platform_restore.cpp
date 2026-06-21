#include "restore/cross_platform_restore.h"
#include <spdlog/spdlog.h>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <regex>

namespace obs {

CrossPlatformRestore::CrossPlatformRestore() {
    driver_registry_["virtio"] = {
        {"virtio_blk", "storage", "linux", "", "", false},
        {"virtio_net", "network", "linux", "", "", false},
        {"virtio_scsi", "storage", "linux", "", "", false},
    };
    driver_registry_["vmxnet3"] = {
        {"vmxnet3", "network", "linux", "", "", false},
        {"pvscsi", "storage", "linux", "", "", false},
    };
    driver_registry_["hyperv"] = {
        {"hv_vmbus", "storage", "linux", "", "", false},
        {"hv_netvsc", "network", "linux", "", "", false},
        {"hv_storvsc", "storage", "linux", "", "", false},
    };
}

CrossPlatformRestore::~CrossPlatformRestore() {}

bool CrossPlatformRestore::convert_format(const std::string& src, const std::string& src_fmt,
                                          const std::string& dst, const std::string& dst_fmt) {
    std::string cmd = "qemu-img convert -f " + src_fmt + " -O " + dst_fmt + " " + src + " " + dst;
    spdlog::info("Converting {} ({}) -> {} ({})", src, src_fmt, dst, dst_fmt);
    return system(cmd.c_str()) == 0;
}

bool CrossPlatformRestore::vmdk_to_qcow2(const std::string& src, const std::string& dst) {
    return convert_format(src, "vmdk", dst, "qcow2");
}

bool CrossPlatformRestore::vmdk_to_vhdx(const std::string& src, const std::string& dst) {
    std::string tmp = dst + ".tmp.raw";
    bool ok = convert_format(src, "vmdk", tmp, "raw");
    if (ok) { ok = convert_format(tmp, "raw", dst, "vhdx"); std::remove(tmp.c_str()); }
    return ok;
}

bool CrossPlatformRestore::qcow2_to_vmdk(const std::string& src, const std::string& dst) {
    return convert_format(src, "qcow2", dst, "vmdk");
}

bool CrossPlatformRestore::qcow2_to_vhdx(const std::string& src, const std::string& dst) {
    std::string tmp = dst + ".tmp.raw";
    bool ok = convert_format(src, "qcow2", tmp, "raw");
    if (ok) { ok = convert_format(tmp, "raw", dst, "vhdx"); std::remove(tmp.c_str()); }
    return ok;
}

bool CrossPlatformRestore::vhdx_to_qcow2(const std::string& src, const std::string& dst) {
    std::string tmp = src + ".tmp.raw";
    bool ok = convert_format(src, "vhdx", tmp, "raw");
    if (ok) { ok = convert_format(tmp, "raw", dst, "qcow2"); std::remove(tmp.c_str()); }
    return ok;
}

bool CrossPlatformRestore::vhdx_to_vmdk(const std::string& src, const std::string& dst) {
    std::string tmp = src + ".tmp.raw";
    bool ok = convert_format(src, "vhdx", tmp, "raw");
    if (ok) { ok = convert_format(tmp, "raw", dst, "vmdk"); std::remove(tmp.c_str()); }
    return ok;
}

ConvertResult CrossPlatformRestore::convert_disk(const ConvertConfig& config) {
    spdlog::info("Converting {} ({}) -> {} ({})", config.source_path, config.source_format,
                 config.target_path, config.target_format);
    ConvertResult result;
    result.source_size = fs::exists(config.source_path) ? fs::file_size(config.source_path) : 0;

    auto start = std::chrono::steady_clock::now();

    if (config.source_format == config.target_format) {
        std::string cmd = "cp " + config.source_path + " " + config.target_path;
        result.success = system(cmd.c_str()) == 0;
    } else if (config.source_format == "vmdk" && config.target_format == "qcow2") {
        result.success = vmdk_to_qcow2(config.source_path, config.target_path);
    } else if (config.source_format == "vmdk" && config.target_format == "vhdx") {
        result.success = vmdk_to_vhdx(config.source_path, config.target_path);
    } else if (config.source_format == "qcow2" && config.target_format == "vmdk") {
        result.success = qcow2_to_vmdk(config.source_path, config.target_path);
    } else if (config.source_format == "qcow2" && config.target_format == "vhdx") {
        result.success = qcow2_to_vhdx(config.source_path, config.target_path);
    } else if (config.source_format == "vhdx" && config.target_format == "qcow2") {
        result.success = vhdx_to_qcow2(config.source_path, config.target_path);
    } else if (config.source_format == "vhdx" && config.target_format == "vmdk") {
        result.success = vhdx_to_vmdk(config.source_path, config.target_path);
    } else {
        std::string tmp = config.target_path + ".tmp.raw";
        result.success = convert_format(config.source_path, config.source_format, tmp, "raw");
        if (result.success) {
            result.success = convert_format(tmp, "raw", config.target_path, config.target_format);
            std::remove(tmp.c_str());
        }
    }

    result.target_size = fs::exists(config.target_path) ? fs::file_size(config.target_path) : 0;
    result.elapsed_seconds = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::steady_clock::now() - start).count();

    if (result.success && config.auto_inject_drivers) {
        result.drivers_injected = detect_required_drivers("linux", "", "kvm");
    }
    return result;
}

bool CrossPlatformRestore::mount_disk(const std::string& disk_path, const std::string& mount_point,
                                       const std::string& format) {
    fs::create_directories(mount_point);
    std::string cmd = "guestmount -a " + disk_path + " -i --ro " + mount_point + " 2>/dev/null";
    spdlog::info("Mounting {} on {}", disk_path, mount_point);
    return system(cmd.c_str()) == 0;
}

bool CrossPlatformRestore::unmount_disk(const std::string& mount_point) {
    std::string cmd = "fusermount -u " + mount_point + " 2>/dev/null || umount " + mount_point + " 2>/dev/null";
    return system(cmd.c_str()) == 0;
}

bool CrossPlatformRestore::inject_drivers(const std::string& disk_path, const std::string& disk_format,
                                          const std::string& os_type, const std::string& os_version) {
    spdlog::info("Injecting drivers for {} ({}) into {}", os_type, os_version, disk_path);
    std::string mount_point = "/tmp/vovqa_mount_" + std::to_string(std::time(nullptr));
    if (!mount_disk(disk_path, mount_point, disk_format)) {
        spdlog::error("Failed to mount disk for driver injection");
        return false;
    }
    bool ok = false;
    if (os_type == "linux") ok = inject_linux_drivers(mount_point, "kvm");
    else if (os_type == "windows") ok = inject_windows_drivers(mount_point, "kvm");
    unmount_disk(mount_point);
    fs::remove_all(mount_point);
    return ok;
}

bool CrossPlatformRestore::inject_linux_drivers(const std::string& mount_point, const std::string& target_hypervisor) {
    spdlog::info("Injecting {} drivers into Linux guest", target_hypervisor);
    std::string drivers_path = "/opt/obs/drivers/" + target_hypervisor;
    std::string cmd = "cp -r " + drivers_path + "/* " + mount_point + "/lib/modules/ 2>/dev/null";
    system(cmd.c_str());
    cmd = "chroot " + mount_point + " depmod -a 2>/dev/null";
    system(cmd.c_str());
    return true;
}

bool CrossPlatformRestore::inject_windows_drivers(const std::string& mount_point, const std::string& target_hypervisor) {
    spdlog::info("Injecting {} drivers into Windows guest", target_hypervisor);
    std::string drivers_path = "/opt/obs/drivers/" + target_hypervisor + "/windows";
    std::string cmd = "cp -r " + drivers_path + "/*.inf " + mount_point + "/Windows/INF/ 2>/dev/null";
    system(cmd.c_str());
    cmd = "cp -r " + drivers_path + "/amd64/* " + mount_point + "/Windows/System32/drivers/ 2>/dev/null";
    system(cmd.c_str());
    return true;
}

bool CrossPlatformRestore::update_initramfs(const std::string& disk_path, const std::string& mount_point) {
    spdlog::info("Updating initramfs on {}", mount_point);
    if (fs::exists(mount_point + "/usr/bin/dracut")) {
        std::string cmd = "chroot " + mount_point + " dracut --force 2>/dev/null";
        return system(cmd.c_str()) == 0;
    } else if (fs::exists(mount_point + "/sbin/mkinitramfs")) {
        std::string cmd = "chroot " + mount_point + " update-initramfs -u 2>/dev/null";
        return system(cmd.c_str()) == 0;
    }
    return false;
}

bool CrossPlatformRestore::update_windows_drivers(const std::string& mount_point, const std::string& target_hypervisor) {
    return inject_windows_drivers(mount_point, target_hypervisor);
}

bool CrossPlatformRestore::adapt_vm_config(const VmConfig& source, VmConfig& target) {
    if (target.vm_type == "vmware") return adapt_vmware_config(source, target);
    if (target.vm_type == "kvm") return adapt_kvm_config(source, target);
    if (target.vm_type == "hyperv") return adapt_hyperv_config(source, target);
    return false;
}

bool CrossPlatformRestore::adapt_vmware_config(const VmConfig& source, VmConfig& target) {
    std::ofstream ofs(target.config_path);
    if (!ofs) return false;
    ofs << "# VMware VMX (OBS Backup)\n";
    ofs << "config.version = \"8\"\nvirtualHW.version = \"21\"\n";
    ofs << "displayName = \"" << target.vm_name << "\"\n";
    ofs << "numvcpus = \"" << target.cpu_count << "\"\nmemsize = \"" << target.memory_mb << "\"\n";
    for (size_t i = 0; i < target.disk_paths.size(); i++) {
        ofs << "scsi0:" << i << ".fileName = \"" << target.disk_paths[i] << "\"\n";
        ofs << "scsi0:" << i << ".present = \"TRUE\"\n";
    }
    ofs << "ethernet0.virtualDev = \"vmxnet3\"\nethernet0.networkName = \"VM Network\"\n";
    ofs << "ethernet0.present = \"TRUE\"\nguestOS = \"other\"\n";
    return true;
}

bool CrossPlatformRestore::adapt_kvm_config(const VmConfig& source, VmConfig& target) {
    std::ofstream ofs(target.config_path);
    if (!ofs) return false;
    ofs << "<domain type='kvm'>\n  <name>" << target.vm_name << "</name>\n";
    ofs << "  <memory unit='MiB'>" << target.memory_mb << "</memory>\n";
    ofs << "  <vcpu>" << target.cpu_count << "</vcpu>\n";
    ofs << "  <os><type arch='x86_64' machine='pc'>hvm</type></os>\n  <devices>\n";
    for (size_t i = 0; i < target.disk_paths.size(); i++) {
        ofs << "    <disk type='file' device='disk'><driver name='qemu' type='qcow2'/>\n";
        ofs << "      <source file='" << target.disk_paths[i] << "'/>\n";
        ofs << "      <target dev='vd" << static_cast<char>('a' + i) << "' bus='virtio'/></disk>\n";
    }
    ofs << "    <interface type='network'><source network='default'/><model type='virtio'/></interface>\n";
    ofs << "  </devices>\n</domain>\n";
    return true;
}

bool CrossPlatformRestore::adapt_hyperv_config(const VmConfig& source, VmConfig& target) {
    std::ofstream ofs(target.config_path + ".ps1");
    if (!ofs) return false;
    ofs << "# Hyper-V VM creation (OBS Backup)\n";
    ofs << "New-VM -Name '" << target.vm_name << "' -MemoryStartupBytes " << target.memory_mb << "MB -Generation 2\n";
    ofs << "Set-VM -Name '" << target.vm_name << "' -ProcessorCount " << target.cpu_count << "\n";
    for (auto& disk : target.disk_paths) {
        ofs << "Add-VMHardDiskDrive -VMName '" << target.vm_name << "' -Path '" << disk << "'\n";
    }
    return true;
}

ConvertResult CrossPlatformRestore::restore_vm(const std::string& backup_path, const std::string& source_format,
                                               const std::string& target_format, const std::string& target_path,
                                               const VmConfig& target_config) {
    spdlog::info("Cross-platform restore: {} ({}) -> {} ({})", backup_path, source_format, target_path, target_format);
    ConvertResult result;
    auto start = std::chrono::steady_clock::now();

    ConvertConfig conv;
    conv.source_path = backup_path;
    conv.source_format = source_format;
    conv.target_path = target_path;
    conv.target_format = target_format;
    conv.auto_inject_drivers = true;
    result = convert_disk(conv);

    if (result.success) {
        std::string mount_point = "/tmp/vovqa_restore_" + std::to_string(std::time(nullptr));
        if (mount_disk(target_path, mount_point, target_format)) {
            bool is_linux = fs::exists(mount_point + "/etc") || fs::exists(mount_point + "/boot");
            bool is_windows = fs::exists(mount_point + "/Windows/System32");
            if (is_linux) {
                inject_linux_drivers(mount_point, target_config.vm_type);
                update_initramfs(target_path, mount_point);
                result.initramfs_updated = true;
            } else if (is_windows) {
                inject_windows_drivers(mount_point, target_config.vm_type);
            }
            unmount_disk(mount_point);
        }

        VmConfig adapted = target_config;
        adapted.disk_paths = {target_path};
        adapt_vm_config(target_config, adapted);
        result.config_adapted = true;
    }

    result.elapsed_seconds = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::steady_clock::now() - start).count();
    return result;
}

std::vector<DriverInfo> CrossPlatformRestore::detect_required_drivers(const std::string& os_type,
                                                                     const std::string& os_version,
                                                                     const std::string& target_hypervisor) {
    std::vector<DriverInfo> drivers;
    if (target_hypervisor == "kvm" || target_hypervisor == "qemu") {
        if (os_type == "linux") {
            drivers.push_back({"virtio_blk", "storage", "linux", "", "", false});
            drivers.push_back({"virtio_net", "network", "linux", "", "", false});
        } else if (os_type == "windows") {
            drivers.push_back({"viostor", "storage", "windows", "", "viostor.inf", false});
            drivers.push_back({"NetKVM", "network", "windows", "", "NetKVM.inf", false});
        }
    } else if (target_hypervisor == "vmware") {
        if (os_type == "linux") {
            drivers.push_back({"vmxnet3", "network", "linux", "", "", false});
            drivers.push_back({"pvscsi", "storage", "linux", "", "", false});
        }
    } else if (target_hypervisor == "hyperv") {
        if (os_type == "linux") {
            drivers.push_back({"hv_vmbus", "storage", "linux", "", "", false});
            drivers.push_back({"hv_netvsc", "network", "linux", "", "", false});
        }
    }
    return drivers;
}

} // namespace obs
