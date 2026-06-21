#pragma once
#include "common.h"
#include <functional>
#include <vector>
#include <string>

namespace obs {

struct OracleBackupConfig {
    std::string host;
    std::string port = "1521";
    std::string sid;
    std::string service_name;
    std::string username;
    std::string password;
    std::string rman_path = "rman";
    std::string backup_dest;
    int level = 1;
    bool enable_bct = true;
    bool compressed = true;
    bool include_archivelog = true;
    bool include_spfile = true;
    bool include_controlfile = true;
};

struct OracleBackupResult {
    bool success = false;
    int64_t scn_from = 0;
    int64_t scn_to = 0;
    bool bct_used = false;
    uint64_t bytes_backed_up = 0;
    uint64_t bytes_input = 0;
    double compression_ratio = 0;
    int32_t files_backed_up = 0;
    int32_t elapsed_seconds = 0;
    std::string output_log;
    std::string error;
};

class OracleBackup {
public:
    OracleBackup();
    ~OracleBackup();

    bool enable_bct(const OracleBackupConfig& config);
    bool is_bct_enabled(const OracleBackupConfig& config);
    std::string get_current_scn(const OracleBackupConfig& config);

    OracleBackupResult full_backup(const OracleBackupConfig& config);
    OracleBackupResult incremental_backup(const OracleBackupConfig& config, int level = 1);
    OracleBackupResult archivelog_backup(const OracleBackupConfig& config);
    OracleBackupResult spfile_backup(const OracleBackupConfig& config);
    OracleBackupResult controlfile_backup(const OracleBackupConfig& config);

    OracleBackupResult backup_all(const OracleBackupConfig& config,
                                  std::function<void(const OracleBackupResult&)> callback = nullptr);

    bool restore(const OracleBackupConfig& config, const std::string& backup_path,
                 int64_t target_scn = 0, const std::string& target_time = "");

private:
    std::string build_rman_script(const OracleBackupConfig& config, const std::string& action);
    std::string run_rman(const std::string& script, const OracleBackupConfig& config);
    OracleBackupResult parse_rman_output(const std::string& output);
    std::string get_connection_string(const OracleBackupConfig& config);
};

} // namespace obs
