#include "incremental/proxmox/proxmox_backup.h"
#include <filesystem>
namespace fs = std::filesystem;
using namespace vovqa::incremental;
#include <spdlog/spdlog.h>
#include <filesystem>
#include <cstdlib>
#include <fstream>
#include <sstream>

namespace vovqa::incremental {

ProxmoxTrackingMethod ProxmoxBackup::auto_detect(const ProxmoxVmInfo& vm) {
    if (vm.storage.find("zfs") != std::string::npos) return ProxmoxTrackingMethod::ZFS;
    if (vm.storage.find("local") != std::string::npos && vm.disk_format == "qcow2") return ProxmoxTrackingMethod::QEMU_DIRTY_BITMAP;
    if (vm.storage.find("lvm") != std::string::npos) return ProxmoxTrackingMethod::LVM_THIN;
    if (vm.qga_installed) return ProxmoxTrackingMethod::QGA_FSFREEZE;
    return ProxmoxTrackingMethod::QEMU_DIRTY_BITMAP;
}

std::vector<BlockRange> ProxmoxBackup::detect_changes(const std::string& path) {
    auto start = std::chrono::steady_clock::now();
    std::vector<BlockRange> ranges;
    spdlog::info("Proxmox incremental: detecting changes for {}", path);

    // Try QEMU dirty bitmap
    if (create_qemu_dirty_bitmap(path, "vovqa_bitmap")) {
        ranges = dump_qemu_dirty_bitmap(path, "vovqa_bitmap");
    }

    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();
    spdlog::info("Proxmox incremental: changed_blocks={}, time={}ms", ranges.size(), elapsed);
    return ranges;
}

bool ProxmoxBackup::create_qemu_dirty_bitmap(const std::string& disk_path, const std::string& bitmap_name) {
    std::string cmd = "qemu-img bitmap --add " + disk_path + " " + bitmap_name + " 2>/dev/null";
    int ret = system(cmd.c_str());
    if (ret != 0) {
        spdlog::warn("Failed to create dirty bitmap on {}", disk_path);
        return false;
    }
    spdlog::info("Created dirty bitmap '{}' on {}", bitmap_name, disk_path);
    return true;
}

std::vector<BlockRange> ProxmoxBackup::dump_qemu_dirty_bitmap(const std::string& disk_path, const std::string& bitmap_name) {
    std::vector<BlockRange> ranges;
    std::string cmd = "qemu-img bitmap --dump --output json " + disk_path + " " + bitmap_name + " 2>/dev/null";
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return ranges;

    std::string output;
    char buf[4096];
    while (fgets(buf, sizeof(buf), pipe)) output += buf;
    pclose(pipe);

    ranges = parse_qemu_bitmap_output(output);
    spdlog::info("QEMU dirty bitmap: {} dirty regions on {}", ranges.size(), disk_path);
    return ranges;
}

std::vector<BlockRange> ProxmoxBackup::parse_qemu_bitmap_output(const std::string& json_output) {
    std::vector<BlockRange> ranges;
    // Parse JSON output: [{"start": 0, "length": 4096}, ...]
    auto start_pos = json_output.find('[');
    auto end_pos = json_output.rfind(']');
    if (start_pos == std::string::npos || end_pos == std::string::npos) return ranges;

    std::string arr = json_output.substr(start_pos + 1, end_pos - start_pos - 1);
    std::istringstream iss(arr);
    std::string token;
    while (std::getline(iss, token, '}')) {
        auto s_pos = token.find("\"start\"");
        auto l_pos = token.find("\"length\"");
        if (s_pos != std::string::npos && l_pos != std::string::npos) {
            uint64_t start = std::stoull(token.substr(s_pos + 8));
            uint64_t length = std::stoull(token.substr(l_pos + 9));
            ranges.push_back({start, static_cast<uint32_t>(length), true});
        }
    }
    return ranges;
}

bool ProxmoxBackup::clear_qemu_dirty_bitmap(const std::string& disk_path, const std::string& bitmap_name) {
    std::string cmd = "qemu-img bitmap --clear " + disk_path + " " + bitmap_name + " 2>/dev/null";
    return system(cmd.c_str()) == 0;
}

bool ProxmoxBackup::create_qcow2_snapshot(const std::string& disk_path, const std::string& snap_name) {
    std::string cmd = "qemu-img snapshot -c " + snap_name + " " + disk_path + " 2>/dev/null";
    return system(cmd.c_str()) == 0;
}

bool ProxmoxBackup::qga_fsfreeze(const std::string& vmid) {
    std::string cmd = "qm guest cmd " + vmid + " fsfreeze-freeze 2>/dev/null";
    spdlog::info("QGA: freezing filesystem for VM {}", vmid);
    return system(cmd.c_str()) == 0;
}

bool ProxmoxBackup::qga_fsthaw(const std::string& vmid) {
    std::string cmd = "qm guest cmd " + vmid + " fsfreeze-thaw 2>/dev/null";
    spdlog::info("QGA: thawing filesystem for VM {}", vmid);
    return system(cmd.c_str()) == 0;
}

bool ProxmoxBackup::vzdump_incremental(const std::string& vmid, const std::string& storage) {
    std::string cmd = "vzdump " + vmid + " --storage " + storage + " --mode snapshot --compress zstd 2>/dev/null";
    spdlog::info("Running vzdump incremental for VM {}", vmid);
    return system(cmd.c_str()) == 0;
}

bool ProxmoxBackup::read_changed_blocks(const std::string& path, const std::vector<BlockRange>& ranges,
                                         std::function<void(const uint8_t*, size_t, uint64_t)> callback) {
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs) return false;
    const size_t buf_size = 64 * 1024;
    std::vector<uint8_t> buf(buf_size);
    for (auto& range : ranges) {
        ifs.seekg(range.offset);
        uint64_t remaining = range.size;
        while (remaining > 0 && ifs) {
            size_t to_read = std::min(static_cast<uint64_t>(buf_size), remaining);
            ifs.read(reinterpret_cast<char*>(buf.data()), to_read);
            size_t got = ifs.gcount();
            if (got > 0) callback(buf.data(), got, range.offset + (range.size - remaining));
            remaining -= got;
        }
    }
    return true;
}

std::vector<BlockRange> ProxmoxBackup::merge_bitmaps(const std::vector<BlockRange>& a, const std::vector<BlockRange>& b) {
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
