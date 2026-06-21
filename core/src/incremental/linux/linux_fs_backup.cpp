#include "incremental/linux/linux_fs_backup.h"
#include <spdlog/spdlog.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <cstdlib>

namespace fs = std::filesystem;
using namespace vovqa::incremental;

namespace vovqa::incremental {

LinuxTrackingMethod LinuxFsBackup::detect_method(const std::string& path) {
    if (fs::exists("/sys/fs/btrfs") || system("btrfs filesystem show 2>/dev/null >/dev/null") == 0) {
        spdlog::info("Detected Btrfs filesystem");
        return LinuxTrackingMethod::BTRFS_SEND;
    }
    if (system("zpool list 2>/dev/null >/dev/null") == 0) {
        spdlog::info("Detected ZFS pool");
        return LinuxTrackingMethod::ZFS_SEND;
    }
    if (fs::exists("/dev/mapper")) {
        for (auto& entry : fs::directory_iterator("/dev/mapper")) {
            std::string name = entry.path().filename().string();
            if (name.find("snap") != std::string::npos) {
                spdlog::info("Detected LVM snapshot: {}", name);
                return LinuxTrackingMethod::LVM_THIN;
            }
        }
    }
    if (fs::exists("/sys/module/dm_era") || system("dmsetup targets 2>/dev/null | grep era >/dev/null") == 0) {
        spdlog::info("Detected dm-era target");
        return LinuxTrackingMethod::DM_ERA;
    }
    spdlog::info("Using tar --listed-incremental as fallback");
    return LinuxTrackingMethod::TAR_INCREMENTAL;
}

std::vector<BlockRange> LinuxFsBackup::detect_changes(const std::string& path) {
    auto start = std::chrono::steady_clock::now();
    std::vector<BlockRange> ranges;
    LinuxTrackingMethod method = (method_ == LinuxTrackingMethod::AUTO) ? detect_method(path) : method_;

    switch (method) {
        case LinuxTrackingMethod::BTRFS_SEND: ranges = detect_btrfs(path); break;
        case LinuxTrackingMethod::ZFS_SEND: ranges = detect_zfs(path); break;
        case LinuxTrackingMethod::LVM_THIN: ranges = detect_lvm(path); break;
        case LinuxTrackingMethod::DM_ERA: ranges = detect_dm_era(path); break;
        case LinuxTrackingMethod::FANOTIFY: ranges = detect_fanotify(path); break;
        case LinuxTrackingMethod::TAR_INCREMENTAL: ranges = detect_tar_incremental(path); break;
        default: ranges = detect_tar_incremental(path); break;
    }

    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();
    spdlog::info("Linux CBT: method={}, changed_blocks={}, time={}ms", static_cast<int>(method), ranges.size(), elapsed);
    return ranges;
}

std::vector<BlockRange> LinuxFsBackup::detect_btrfs(const std::string& path) {
    std::vector<BlockRange> ranges;
    // Create read-only snapshot
    std::string snap_name = "vovqa_" + std::to_string(std::time(nullptr));
    std::string cmd = "btrfs subvolume snapshot -r " + path + " /tmp/" + snap_name + " 2>/dev/null";
    int ret = system(cmd.c_str());
    if (ret != 0) {
        spdlog::warn("Btrfs snapshot failed, using fallback");
        return detect_tar_incremental(path);
    }

    // Compare with previous snapshot
    if (!previous_snapshot_.empty()) {
        std::string send_cmd = "btrfs send -p " + previous_snapshot_ + " /tmp/" + snap_name + " 2>/dev/null | wc -c";
        FILE* pipe = popen(send_cmd.c_str(), "r");
        if (pipe) {
            char buf[64];
            fgets(buf, sizeof(buf), pipe);
            pclose(pipe);
            uint64_t size = std::stoull(buf);
            ranges.push_back({0, static_cast<uint32_t>(size), true});
        }
    } else {
        // First backup - everything is changed
        uint64_t total = 0;
        for (auto& entry : fs::recursive_directory_iterator(path)) {
            if (entry.is_regular_file()) total += entry.file_size();
        }
        ranges.push_back({0, static_cast<uint32_t>(total), true});
    }

    // Cleanup old snapshot, keep current
    if (!previous_snapshot_.empty()) {
        system(("btrfs subvolume delete " + previous_snapshot_ + " 2>/dev/null").c_str());
    }
    previous_snapshot_ = "/tmp/" + snap_name;
    return ranges;
}

std::vector<BlockRange> LinuxFsBackup::detect_zfs(const std::string& path) {
    std::vector<BlockRange> ranges;
    std::string snap_name = "vovqa_" + std::to_string(std::time(nullptr));

    // Create snapshot
    std::string cmd = "zfs snapshot " + path + "@" + snap_name + " 2>/dev/null";
    if (system(cmd.c_str()) != 0) {
        spdlog::warn("ZFS snapshot failed");
        return detect_tar_incremental(path);
    }

    if (!current_snapshot_.empty()) {
        std::string send_cmd = "zfs send -i " + path + "@" + current_snapshot_ + " " + path + "@" + snap_name + " 2>/dev/null | wc -c";
        FILE* pipe = popen(send_cmd.c_str(), "r");
        if (pipe) {
            char buf[64];
            fgets(buf, sizeof(buf), pipe);
            pclose(pipe);
            uint64_t size = std::stoull(buf);
            ranges.push_back({0, static_cast<uint32_t>(size), true});
        }
        // Destroy old snapshot
        system(("zfs destroy " + path + "@" + current_snapshot_ + " 2>/dev/null").c_str());
    } else {
        uint64_t total = 0;
        for (auto& entry : fs::recursive_directory_iterator(path)) {
            if (entry.is_regular_file()) total += entry.file_size();
        }
        ranges.push_back({0, static_cast<uint32_t>(total), true});
    }

    current_snapshot_ = snap_name;
    return ranges;
}

std::vector<BlockRange> LinuxFsBackup::detect_lvm(const std::string& path) {
    std::vector<BlockRange> ranges;
    spdlog::info("LVM tracking: scanning for changed blocks via dm-snap");
    // Parse dmsetup status for snapshot changes
    FILE* pipe = popen("dmsetup status --target snapshot 2>/dev/null", "r");
    if (pipe) {
        char buf[512];
        while (fgets(buf, sizeof(buf), pipe)) {
            std::string line(buf);
            // Parse "X/Y" sectors changed
            auto pos = line.find('/');
            if (pos != std::string::npos) {
                uint64_t changed = std::stoull(line.substr(0, pos));
                ranges.push_back({0, static_cast<uint32_t>(changed * 512), true});
            }
        }
        pclose(pipe);
    }
    if (ranges.empty()) ranges = detect_tar_incremental(path);
    return ranges;
}

std::vector<BlockRange> LinuxFsBackup::detect_dm_era(const std::string& path) {
    std::vector<BlockRange> ranges;
    spdlog::info("dm-era tracking: reading changed block metadata");
    // Parse dmsetup status for era
    FILE* pipe = popen("dmsetup status --target era 2>/dev/null", "r");
    if (pipe) {
        char buf[512];
        while (fgets(buf, sizeof(buf), pipe)) {
            spdlog::debug("dm-era status: {}", buf);
            // Parse era metadata blocks
            std::string line(buf);
            // Simplified: mark entire device as changed
            ranges.push_back({0, 1024 * 1024, true});
        }
        pclose(pipe);
    }
    if (ranges.empty()) ranges = detect_tar_incremental(path);
    return ranges;
}

std::vector<BlockRange> LinuxFsBackup::detect_fanotify(const std::string& path) {
    std::vector<BlockRange> ranges;
    spdlog::info("fanotify tracking: scanning modified files");
    std::string snar_path = "/tmp/vovqa_fanotify_" + fs::path(path).filename().string();
    std::string cmd = "find " + path + " -newer " + snar_path + " -type f 2>/dev/null | wc -l";
    FILE* pipe = popen(cmd.c_str(), "r");
    int count = 0;
    if (pipe) {
        char buf[32];
        if (fgets(buf, sizeof(buf), pipe)) count = std::atoi(buf);
        pclose(pipe);
    }
    if (count > 0) {
        ranges.push_back({0, static_cast<uint32_t>(count * 4096), true});
    }
    // Update snar marker
    system(("touch " + snar_path).c_str());
    return ranges;
}

std::vector<BlockRange> LinuxFsBackup::detect_tar_incremental(const std::string& path) {
    std::vector<BlockRange> ranges;
    std::string snar = "/tmp/vovqa_snar_" + fs::path(path).filename().string();
    std::string cmd = "tar -czf /dev/null --listed-incremental=" + snar + " " + path + " 2>&1 | grep -c 'file' || echo 0";
    FILE* pipe = popen(cmd.c_str(), "r");
    if (pipe) {
        char buf[32];
        if (fgets(buf, sizeof(buf), pipe)) {
            int files = std::atoi(buf);
            if (files > 0) ranges.push_back({0, static_cast<uint32_t>(files * 4096), true});
        }
        pclose(pipe);
    }
    return ranges;
}

bool LinuxFsBackup::read_changed_blocks(const std::string& path, const std::vector<BlockRange>& ranges,
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

std::vector<BlockRange> LinuxFsBackup::merge_bitmaps(const std::vector<BlockRange>& a, const std::vector<BlockRange>& b) {
    std::vector<BlockRange> result = a;
    for (auto& br : b) {
        bool merged = false;
        for (auto& existing : result) {
            if (br.offset >= existing.offset && br.offset < existing.offset + existing.size) {
                uint64_t end = std::max(existing.offset + existing.size, br.offset + br.size);
                existing.size = end - existing.offset;
                merged = true;
                break;
            }
        }
        if (!merged) result.push_back(br);
    }
    std::sort(result.begin(), result.end(), [](const BlockRange& a, const BlockRange& b) { return a.offset < b.offset; });
    return result;
}

bool LinuxFsBackup::btrfs_send_incremental(const std::string& path, const std::string& parent_snapshot) {
    std::string new_snap = "vovqa_" + std::to_string(std::time(nullptr));
    std::string cmd = "btrfs subvolume snapshot -r " + path + " /tmp/" + new_snap + " && "
                    + "btrfs send -p " + parent_snapshot + " /tmp/" + new_snap;
    return system(cmd.c_str()) == 0;
}

bool LinuxFsBackup::zfs_send_incremental(const std::string& dataset, const std::string& from_snap, const std::string& to_snap) {
    std::string cmd = "zfs send -i " + dataset + "@" + from_snap + " " + dataset + "@" + to_snap;
    return system(cmd.c_str()) == 0;
}

bool LinuxFsBackup::create_lvm_snapshot(const std::string& vg, const std::string& lv, const std::string& snap_name) {
    std::string cmd = "lvcreate --snapshot -L 10G --name " + snap_name + " " + vg + "/" + lv;
    return system(cmd.c_str()) == 0;
}

bool LinuxFsBackup::parse_dm_era_status(const std::string& status, std::vector<BlockRange>& ranges) {
    // Parse "X/Y Z/W" format: current era / total eras, metadata blocks
    std::istringstream iss(status);
    uint64_t current_era, total_eras;
    iss >> current_era;
    char sep;
    iss >> sep;
    iss >> total_eras;
    if (current_era > 0) {
        ranges.push_back({0, static_cast<uint32_t>(current_era * 4096), true});
    }
    return !ranges.empty();
}

} // namespace vovqa::incremental
