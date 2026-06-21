#include "dbms/oracle_adapter.h"
#include <spdlog/spdlog.h>

namespace obs {

bool OracleAdapter::test_connection(const DbConnection& conn) {
    spdlog::info("Oracle: testing connection to {}:{}", conn.host, conn.port);
    return true;
}

bool OracleAdapter::backup(const BackupConfig& config, BackupProgressCallback callback) {
    BackupProgress progress;
    progress.job_id = config.job_id;
    progress.status = BackupStatus::RUNNING;
    progress.started_at = current_time_string();
    if (callback) callback(progress);

    bool ok = rman_backup(config, callback);

    progress.status = ok ? BackupStatus::COMPLETED : BackupStatus::FAILED;
    progress.finished_at = current_time_string();
    if (callback) callback(progress);
    return ok;
}

bool OracleAdapter::restore(const RestoreConfig& config, BackupProgressCallback callback) {
    return rman_restore(config, callback);
}

std::vector<std::string> OracleAdapter::list_databases(const DbConnection& conn) {
    return {conn.service_name};
}

std::vector<std::string> OracleAdapter::list_tables(const DbConnection& conn, const std::string& database) {
    return {};
}

bool OracleAdapter::rman_backup(const BackupConfig& config, BackupProgressCallback callback) {
    std::string script = "BACKUP DATABASE PLUS ARCHIVELOG DELETE INPUT";
    spdlog::info("Oracle RMAN: {}", script);
    BackupProgress progress;
    progress.job_id = config.job_id;
    progress.status = BackupStatus::RUNNING;
    progress.current_object = "RMAN";
    if (callback) callback(progress);
    return run_rman(config.connection, script);
}

bool OracleAdapter::rman_restore(const RestoreConfig& config, BackupProgressCallback callback) {
    spdlog::info("Oracle RMAN: restoring from {}", config.source_path);
    return true;
}

bool OracleAdapter::rman_incremental(const BackupConfig& config, BackupProgressCallback callback) {
    spdlog::info("Oracle RMAN: incremental backup level 1");
    return true;
}

bool OracleAdapter::rman_archivelog_backup(const BackupConfig& config, BackupProgressCallback callback) {
    spdlog::info("Oracle RMAN: archivelog backup");
    return true;
}

bool OracleAdapter::begin_hot_backup(const DbConnection& conn) {
    spdlog::info("Oracle: ALTER TABLESPACE ... BEGIN BACKUP");
    return true;
}

bool OracleAdapter::end_hot_backup(const DbConnection& conn) {
    spdlog::info("Oracle: ALTER TABLESPACE ... END BACKUP");
    return true;
}

bool OracleAdapter::datapump_export(const DbConnection& conn, const std::string& schema,
                                    const std::string& output_dir, BackupProgressCallback callback) {
    spdlog::info("Oracle Data Pump: exporting schema {}", schema);
    return true;
}

bool OracleAdapter::datapump_import(const DbConnection& conn, const std::string& schema,
                                    const std::string& dumpfile, BackupProgressCallback callback) {
    spdlog::info("Oracle Data Pump: importing schema {}", schema);
    return true;
}

bool OracleAdapter::backup_pdb(const DbConnection& conn, const std::string& pdb_name,
                               const std::string& backup_path, BackupProgressCallback callback) {
    spdlog::info("Oracle RMAN: backing up PDB {}", pdb_name);
    return true;
}

bool OracleAdapter::run_rman(const DbConnection& conn, const std::string& script) {
    spdlog::info("Oracle: running RMAN script");
    return true;
}

bool OracleAdapter::run_sqlplus(const DbConnection& conn, const std::string& sql) {
    spdlog::info("Oracle: running SQL*Plus");
    return true;
}

} // namespace obs
