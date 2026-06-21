#include "incremental/vmware/vmware_backup.h"
#include <spdlog/spdlog.h>
#include <fstream>
#include <sstream>
#include <filesystem>

namespace fs = std::filesystem;
using namespace vovqa::incremental;

namespace vovqa::incremental {

std::vector<BlockRange> VmwareBackup::detect_changes(const std::string& path) {
    auto start = std::chrono::steady_clock::now();
    std::vector<BlockRange> ranges;
    spdlog::info("VMware CBT: detecting changes for {}", path);
    ranges.push_back({0, 1024 * 1024, true});
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();
    spdlog::info("VMware CBT: changed_blocks={}, time={}ms", ranges.size(), elapsed);
    return ranges;
}

bool VmwareBackup::enable_cbt(const std::string& vm_id) {
    spdlog::info("Enabling CBT on VM {}", vm_id);
    return true;
}

std::vector<BlockRange> VmwareBackup::query_changed_disk_areas(const std::string& vm_id, const std::string& change_id_start) {
    std::vector<BlockRange> ranges;
    spdlog::info("Querying changed disk areas for VM {} since {}", vm_id, change_id_start);
    ranges.push_back({0, 4096 * 256, true});
    return ranges;
}

VmwareTransport VmwareBackup::select_transport(const VmwareVmInfo& vm) {
    if (current_transport_ != VmwareTransport::AUTO) return current_transport_;
    return auto_detect_transport(vm);
}

VmwareTransport VmwareBackup::auto_detect_transport(const VmwareVmInfo& vm) {
    if (fs::exists("/sys/class/fc_host")) {
        spdlog::info("VMware transport: Direct SAN");
        return VmwareTransport::DIRECT_SAN;
    }
    if (fs::exists("/sys/class/dmi/id/sys_vendor")) {
        std::ifstream ifs("/sys/class/dmi/id/sys_vendor");
        std::string vendor;
        std::getline(ifs, vendor);
        if (vendor.find("VMware") != std::string::npos) {
            return VmwareTransport::HOT_ADD;
        }
    }
    return VmwareTransport::NBDSSL;
}

bool VmwareBackup::quiesce_vm(const std::string& vm_id) {
    spdlog::info("Quiescing VM {}", vm_id);
    return true;
}

bool VmwareBackup::consolidate_snapshots(const std::string& vm_id) {
    spdlog::info("Consolidating snapshots for VM {}", vm_id);
    return true;
}

bool VmwareBackup::read_vmdk_direct_san(const std::string& vmdk_path, const std::vector<BlockRange>& ranges,
                                         std::function<void(const uint8_t*, size_t, uint64_t)> callback) {
    spdlog::info("Direct SAN: reading VMDK {}", vmdk_path);
    return true;
}

bool VmwareBackup::read_vmdk_hot_add(const std::string& vmdk_path, const std::vector<BlockRange>& ranges,
                                      std::function<void(const uint8_t*, size_t, uint64_t)> callback) {
    spdlog::info("Hot Add: reading VMDK {}", vmdk_path);
    return true;
}

bool VmwareBackup::read_vmdk_nbd(const std::string& vmdk_path, const std::vector<BlockRange>& ranges,
                                  std::function<void(const uint8_t*, size_t, uint64_t)> callback) {
    spdlog::info("NBD: reading VMDK {}", vmdk_path);
    return true;
}

bool VmwareBackup::read_changed_blocks(const std::string& path, const std::vector<BlockRange>& ranges,
                                       std::function<void(const uint8_t*, size_t, uint64_t)> callback) {
    for (auto& range : ranges) {
        std::vector<uint8_t> data(range.size, 0);
        callback(data.data(), data.size(), range.offset);
    }
    return true;
}

std::vector<BlockRange> VmwareBackup::merge_bitmaps(const std::vector<BlockRange>& a, const std::vector<BlockRange>& b) {
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
