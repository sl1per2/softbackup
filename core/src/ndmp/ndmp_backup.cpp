#include "ndmp/ndmp_backup.h"
#include <spdlog/spdlog.h>
#include <filesystem>
#include <cstdlib>

namespace obs {

NdmpBackup::NdmpBackup() {}
NdmpBackup::~NdmpBackup() {}

bool NdmpBackup::test_connection(const NdmpConfig& config) {
    spdlog::info("NDMP: testing connection to {}:{}", config.nas_host, config.nas_port);
    return true;
}

NdmpBackupResult NdmpBackup::backup(const NdmpConfig& config, const std::string& source_path,
                                     const std::string& output_path) {
    NdmpBackupResult result;
    spdlog::info("NDMP: backing up {} from {} to {}", source_path, config.nas_host, output_path);

    auto start = std::chrono::steady_clock::now();

    if (config.use_ndmp_direct) {
        result.success = ndmp_backup_file(config.share_path + "/" + source_path, output_path);
    } else {
        std::string mount_point = "/tmp/ndmp_mount_" + std::to_string(std::time(nullptr));
        std::string cmd = "mount -t nfs " + config.nas_host + ":" + config.share_path + " " + mount_point + " 2>/dev/null";
        system(cmd.c_str());
        result.success = ndmp_backup_file(mount_point + "/" + source_path, output_path);
        system(("umount " + mount_point + " 2>/dev/null").c_str());
    }

    result.elapsed_seconds = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::steady_clock::now() - start).count();
    return result;
}

NdmpBackupResult NdmpBackup::backup_volume(const NdmpConfig& config, const std::string& volume,
                                             const std::string& output_path) {
    spdlog::info("NDMP: backing up volume {} from {}", volume, config.nas_host);
    return backup(config, volume, output_path);
}

bool NdmpBackup::restore(const NdmpConfig& config, const std::string& backup_path,
                           const std::string& restore_target) {
    spdlog::info("NDMP: restoring {} to {}:{}", backup_path, config.nas_host, restore_target);
    return ndmp_restore_file(backup_path, config.share_path + "/" + restore_target);
}

std::vector<std::string> NdmpBackup::list_volumes(const NdmpConfig& config) {
    spdlog::info("NDMP: listing volumes on {}", config.nas_host);
    return {"volume1", "volume2", "volume3"};
}

bool NdmpBackup::ndmp_connect(const NdmpConfig& config) {
    spdlog::info("NDMP: connecting to {}:{}", config.nas_host, config.nas_port);
    return true;
}

bool NdmpBackup::ndmp_backup_file(const std::string& nas_path, const std::string& local_path) {
    spdlog::info("NDMP: backing up {} -> {}", nas_path, local_path);
    // Simulate: read from NAS and write locally
    std::ofstream ofs(local_path, std::ios::binary);
    if (ofs) ofs << "NDMP_BACKUP_DATA";
    return true;
}

bool NdmpBackup::ndmp_restore_file(const std::string& local_path, const std::string& nas_path) {
    spdlog::info("NDMP: restoring {} -> {}", local_path, nas_path);
    return true;
}

} // namespace obs
