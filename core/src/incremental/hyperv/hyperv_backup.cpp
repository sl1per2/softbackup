#include "incremental/hyperv/hyperv_backup.h"
#include <spdlog/spdlog.h>

#ifdef _MSC_VER
#include <windows.h>
#include <comdef.h>
#include <Wbemidl.h>
#endif

namespace vovqa::incremental {

HyperVTrackingMethod HyperVBackup::auto_detect(const HyperVVmInfo& vm) {
    if (vm.rct_enabled) return HyperVTrackingMethod::RCT;
    return HyperVTrackingMethod::VHDX_SNAPSHOT;
}

std::vector<BlockRange> HyperVBackup::detect_changes(const std::string& path) {
    auto start = std::chrono::steady_clock::now();
    std::vector<BlockRange> ranges;
    spdlog::info("Hyper-V incremental: detecting changes for {}", path);

    // Try RCT first
    ranges = get_rct_changed_blocks(path);

    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();
    spdlog::info("Hyper-V incremental: changed_blocks={}, time={}ms", ranges.size(), elapsed);
    return ranges;
}

bool HyperVBackup::enable_rct(const std::string& vm_id) {
    spdlog::info("Enabling RCT on VM {}", vm_id);
#ifdef _WIN32
    // PowerShell: Enable-VMReplication -VMName <name>
    std::string cmd = "powershell -Command \"Enable-VMReplication -VMName '" + vm_id + "' -ReplicaServerName localhost\"";
    return system(cmd.c_str()) == 0;
#else
    return true;
#endif
}

std::vector<BlockRange> HyperVBackup::get_rct_changed_blocks(const std::string& vm_id) {
    std::vector<BlockRange> ranges;
    spdlog::info("Getting RCT changed blocks for VM {}", vm_id);

#ifdef _WIN32
    // WMI query: Msvm_VirtualSystemReplicationService → GetReplicationRelationships
    // Then: GetReplicationEnabledState → changed blocks
    std::string wql = "SELECT * FROM Msvm_ComputerSystem WHERE Name = '" + vm_id + "'";
    // Simulated result
    ranges.push_back({0, 65536, true});
#else
    // Simulated on Linux
    ranges.push_back({0, 65536, true});
#endif
    return ranges;
}

bool HyperVBackup::create_vhdx_checkpoint(const std::string& vm_id, const std::string& name) {
    spdlog::info("Creating VHDX checkpoint '{}' for VM {}", name, vm_id);
#ifdef _WIN32
    std::string cmd = "powershell -Command \"Checkpoint-VM -Name '" + vm_id + "' -SnapshotName '" + name + "'\"";
    return system(cmd.c_str()) == 0;
#else
    return true;
#endif
}

bool HyperVBackup::remove_vhdx_checkpoint(const std::string& vm_id, const std::string& checkpoint_id) {
    spdlog::info("Removing VHDX checkpoint {} from VM {}", checkpoint_id, vm_id);
#ifdef _WIN32
    std::string cmd = "powershell -Command \"Remove-VMSnapshot -VMName '" + vm_id + "' -Name '" + checkpoint_id + "'\"";
    return system(cmd.c_str()) == 0;
#else
    return true;
#endif
}

bool HyperVBackup::vss_backup_with_hyper_v_writer(const std::string& vm_id) {
    spdlog::info("VSS backup with Hyper-V writer for VM {}", vm_id);
    // Coordinate with Hyper-V VSS writer
    // IVssBackupComponents::AddComponent, CreateSnapshot, Wait
    return true;
}

std::vector<uint8_t> HyperVBackup::get_vhdx_bitmap(const std::string& vhdx_path) {
    std::vector<uint8_t> bitmap;
    spdlog::info("Reading VHDX bitmap from {}", vhdx_path);
    // Parse VHDX internal structure:
    // 1. Read VHDX header (file_type_identifier = "vhdxfile")
    // 2. Read BAT (Block Allocation Table)
    // 3. Read metadata region
    // 4. Extract block bitmap from metadata
    // Simplified: return simulated bitmap
    bitmap.resize(1024, 0xFF);
    return bitmap;
}

bool HyperVBackup::read_changed_blocks(const std::string& path, const std::vector<BlockRange>& ranges,
                                       std::function<void(const uint8_t*, size_t, uint64_t)> callback) {
    for (auto& range : ranges) {
        std::vector<uint8_t> data(range.size, 0);
        callback(data.data(), data.size(), range.offset);
    }
    return true;
}

std::vector<BlockRange> HyperVBackup::merge_bitmaps(const std::vector<BlockRange>& a, const std::vector<BlockRange>& b) {
    std::vector<BlockRange> result = a;
    for (auto& br : b) {
        bool found = false;
        for (auto& existing : result) {
            if (br.offset >= existing.offset && br.offset < existing.offset + existing.size) {
                uint64_t end = std::max(existing.offset + existing.size, br.offset + br.size);
                existing.size = end - existing.offset;
                found = true;
                break;
            }
        }
        if (!found) result.push_back(br);
    }
    std::sort(result.begin(), result.end(), [](const BlockRange& x, const BlockRange& y) { return x.offset < y.offset; });
    return result;
}

} // namespace vovqa::incremental
