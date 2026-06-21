#pragma once
#include "common.h"
#include <functional>
#include <vector>
#include <string>

namespace obs {

enum class SapSystem { HANA, ORACLE_RMAN };

struct SapBackupConfig {
    SapSystem system = SapSystem::HANA;
    std::string sid;
    std::string host;
    int port = 30015;
    std::string username;
    std::string password;
    std::string backup_dest;
    bool compressed = true;
    bool include_logs = true;
    bool include_catalog = true;
};

struct SapBackupResult {
    bool success = false;
    uint64_t bytes_backed_up = 0;
    int32_t elapsed_seconds = 0;
    std::string backup_id;
    std::string error;
};

class SapBackup {
public:
    SapBackup();
    ~SapBackup();

    SapBackupResult backup_hana(const SapBackupConfig& config);
    SapBackupResult backup_hana_incremental(const SapBackupConfig& config);
    SapBackupResult backup_hana_log(const SapBackupConfig& config);
    bool restore_hana(const SapBackupConfig& config, const std::string& backup_id);
    bool enable_backint(const SapBackupConfig& config);

    SapBackupResult backup_oracle_rman(const SapBackupConfig& config);
    SapBackupResult backup_oracle_incremental(const SapBackupConfig& config, int level);
    SapBackupResult backup_oracle_archivelog(const SapBackupConfig& config);
    bool restore_oracle_rman(const SapBackupConfig& config, const std::string& target_time = "");

private:
    bool run_hdbsql(const SapBackupConfig& config, const std::string& sql);
    bool run_rman(const SapBackupConfig& config, const std::string& script);
    SapBackupResult parse_hana_output(const std::string& output);
    SapBackupResult parse_rman_output(const std::string& output);
};

} // namespace obs
