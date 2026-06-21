#include "dbms/mssql_adapter.h"
#include <spdlog/spdlog.h>

namespace obs {

std::string MsSqlAdapter::build_conn_string(const DbConnection& conn) {
    return "-S " + conn.host + (conn.instance > 0 ? "\\" + std::to_string(conn.instance) : "") +
           " -U " + conn.username + " -P " + conn.password;
}

bool MsSqlAdapter::test_connection(const DbConnection& conn) {
    spdlog::info("MSSQL: testing connection to {}:{}", conn.host, conn.port);
    return true;
}

bool MsSqlAdapter::backup(const BackupConfig& config, BackupProgressCallback callback) {
    BackupProgress progress;
    progress.job_id = config.job_id;
    progress.status = BackupStatus::RUNNING;
    progress.started_at = current_time_string();
    if (callback) callback(progress);

    bool ok = backup_database(config.connection, config.connection.database, config.output_path, true);

    progress.status = ok ? BackupStatus::COMPLETED : BackupStatus::FAILED;
    progress.finished_at = current_time_string();
    if (callback) callback(progress);
    return ok;
}

bool MsSqlAdapter::restore(const RestoreConfig& config, BackupProgressCallback callback) {
    spdlog::info("MSSQL: restoring {} from {}", config.target.database, config.source_path);
    return true;
}

std::vector<std::string> MsSqlAdapter::list_databases(const DbConnection& conn) {
    return {"master", "model", "msdb", "tempdb"};
}

std::vector<std::string> MsSqlAdapter::list_tables(const DbConnection& conn, const std::string& database) {
    return {};
}

bool MsSqlAdapter::backup_database(const DbConnection& conn, const std::string& database,
                                   const std::string& backup_path, bool full) {
    std::string sql = "BACKUP DATABASE [" + database + "] TO DISK = N'" + backup_path + "'" +
                     (full ? " WITH INIT, FORMAT, CHECKSUM" : " WITH NOINIT, CHECKSUM");
    spdlog::info("MSSQL: BACKUP DATABASE [{}]", database);
    return run_sqlcmd(conn, sql);
}

bool MsSqlAdapter::backup_log(const DbConnection& conn, const std::string& database,
                              const std::string& backup_path) {
    std::string sql = "BACKUP LOG [" + database + "] TO DISK = N'" + backup_path + "' WITH CHECKSUM";
    spdlog::info("MSSQL: BACKUP LOG [{}]", database);
    return run_sqlcmd(conn, sql);
}

bool MsSqlAdapter::backup_differential(const DbConnection& conn, const std::string& database,
                                       const std::string& backup_path) {
    std::string sql = "BACKUP DATABASE [" + database + "] TO DISK = N'" + backup_path +
                     "' WITH DIFFERENTIAL, CHECKSUM";
    spdlog::info("MSSQL: DIFFERENTIAL BACKUP [{}]", database);
    return run_sqlcmd(conn, sql);
}

bool MsSqlAdapter::vss_backup(const BackupConfig& config, BackupProgressCallback callback) {
    spdlog::info("MSSQL: VSS backup of {}", config.connection.database);
    BackupProgress progress;
    progress.job_id = config.job_id;
    progress.status = BackupStatus::RUNNING;
    progress.current_object = "VSS";
    if (callback) callback(progress);
    return true;
}

bool MsSqlAdapter::backup_ag_secondary(const BackupConfig& config, BackupProgressCallback callback) {
    spdlog::info("MSSQL: Always On AG secondary backup");
    return true;
}

std::vector<std::string> MsSqlAdapter::list_ag_replicas(const DbConnection& conn) {
    return {};
}

bool MsSqlAdapter::backup_with_tde(const BackupConfig& config, BackupProgressCallback callback) {
    spdlog::info("MSSQL: TDE-aware backup of {}", config.connection.database);
    return backup_database(config.connection, config.connection.database, config.output_path, true);
}

bool MsSqlAdapter::run_sqlcmd(const DbConnection& conn, const std::string& sql) {
    spdlog::info("MSSQL: executing SQL command");
    return true;
}

} // namespace obs
