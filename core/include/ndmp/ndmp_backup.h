#pragma once
#include "common.h"
#include <functional>
#include <vector>
#include <string>

namespace obs {

struct NdmpConfig {
    std::string nas_host;
    uint16_t nas_port = 10000;
    std::string username;
    std::string password;
    std::string share_path;
    std::string storage_path;
    bool use_ndmp_direct = true;
};

struct NdmpBackupResult {
    bool success = false;
    uint64_t bytes_backed_up = 0;
    int32_t files_count = 0;
    int32_t elapsed_seconds = 0;
    std::string error;
};

class NdmpBackup {
public:
    NdmpBackup();
    ~NdmpBackup();

    bool test_connection(const NdmpConfig& config);
    NdmpBackupResult backup(const NdmpConfig& config, const std::string& source_path,
                           const std::string& output_path);
    NdmpBackupResult backup_volume(const NdmpConfig& config, const std::string& volume,
                                   const std::string& output_path);
    bool restore(const NdmpConfig& config, const std::string& backup_path,
                const std::string& restore_target);
    std::vector<std::string> list_volumes(const NdmpConfig& config);

private:
    bool ndmp_connect(const NdmpConfig& config);
    bool ndmp_backup_file(const std::string& nas_path, const std::string& local_path);
    bool ndmp_restore_file(const std::string& local_path, const std::string& nas_path);
};

} // namespace obs
