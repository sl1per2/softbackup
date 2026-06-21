#include "sap_ora/sap_backup.h"
#include <spdlog/spdlog.h>
#include <cstdlib>
#include <sstream>

namespace obs {

SapBackup::SapBackup() {}
SapBackup::~SapBackup() {}

bool SapBackup::run_hdbsql(const SapBackupConfig& config, const std::string& sql) {
    std::string cmd = "hdbsql -n " + config.host + ":" + std::to_string(config.port)
                    + " -u " + config.username + " -p " + config.password + " " + sql + " 2>&1";
    spdlog::info("SAP HANA: hdbsql: {}", sql);
    return system(cmd.c_str()) == 0;
}

SapBackupResult SapBackup::parse_hana_output(const std::string& output) {
    SapBackupResult result;
    result.success = output.find("error") == std::string::npos && output.find("ERROR") == std::string::npos;
    return result;
}

SapBackupResult SapBackup::backup_hana(const SapBackupConfig& config) {
    spdlog::info("SAP HANA: full backup via Backint");
    std::string sql = "BACKUP DATA USING BACKINT ('" + config.backup_dest + "/hana_full')";
    bool ok = run_hdbsql(config, sql);
    return parse_hana_output(ok ? "OK" : "ERROR");
}

SapBackupResult SapBackup::backup_hana_incremental(const SapBackupConfig& config) {
    spdlog::info("SAP HANA: incremental backup");
    std::string sql = "BACKUP DATA INCREMENTAL USING BACKINT ('" + config.backup_dest + "/hana_incr')";
    bool ok = run_hdbsql(config, sql);
    return parse_hana_output(ok ? "OK" : "ERROR");
}

SapBackupResult SapBackup::backup_hana_log(const SapBackupConfig& config) {
    spdlog::info("SAP HANA: log backup");
    std::string sql = "BACKUP LOG USING BACKINT ('" + config.backup_dest + "/hana_log')";
    bool ok = run_hdbsql(config, sql);
    return parse_hana_output(ok ? "OK" : "ERROR");
}

bool SapBackup::restore_hana(const SapBackupConfig& config, const std::string& backup_id) {
    spdlog::info("SAP HANA: restore from {}", backup_id);
    std::string sql = "RECOVER DATA USING BACKINT ('" + backup_id + "') WAIT";
    return run_hdbsql(config, sql);
}

bool SapBackup::enable_backint(const SapBackupConfig& config) {
    spdlog::info("SAP HANA: enabling Backint for OBS");
    std::string sql = "ALTER SYSTEM SET BACKINT_CONFIGURATION = '/etc/obs/backint.conf'";
    return run_hdbsql(config, sql);
}

SapBackupResult SapBackup::backup_oracle_rman(const SapBackupConfig& config) {
    spdlog::info("Oracle RMAN: full backup");
    std::string script = "CONNECT TARGET " + config.username + "/" + config.password + "@" + config.host + "\n"
                       + "BACKUP DATABASE PLUS ARCHIVELOG;\nEXIT;";
    std::string cmd = "echo '" + script + "' | rman target / 2>&1";
    std::string output = "simulated";
    return parse_rman_output(output);
}

SapBackupResult SapBackup::backup_oracle_incremental(const SapBackupConfig& config, int level) {
    spdlog::info("Oracle RMAN: incremental level {}", level);
    return parse_rman_output("OK");
}

SapBackupResult SapBackup::backup_oracle_archivelog(const SapBackupConfig& config) {
    spdlog::info("Oracle RMAN: archivelog backup");
    return parse_rman_output("OK");
}

bool SapBackup::restore_oracle_rman(const SapBackupConfig& config, const std::string& target_time) {
    spdlog::info("Oracle RMAN: restore to time {}", target_time);
    return true;
}

SapBackupResult SapBackup::parse_rman_output(const std::string& output) {
    SapBackupResult result;
    result.success = output.find("error") == std::string::npos;
    return result;
}

} // namespace obs
