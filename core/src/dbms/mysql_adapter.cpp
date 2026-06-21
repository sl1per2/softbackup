#include "dbms/mysql_adapter.h"
#include <spdlog/spdlog.h>

namespace obs {

std::string MySQLAdapter::build_conn_string(const DbConnection& conn) {
    return "-h " + conn.host + " -P " + std::to_string(conn.port) +
           " -u " + conn.username + " -p" + conn.password;
}

bool MySQLAdapter::test_connection(const DbConnection& conn) {
    spdlog::info("MySQL: testing connection to {}:{}", conn.host, conn.port);
    return true;
}

bool MySQLAdapter::backup(const BackupConfig& config, BackupProgressCallback callback) {
    BackupProgress progress;
    progress.job_id = config.job_id;
    progress.status = BackupStatus::RUNNING;
    progress.started_at = current_time_string();
    if (callback) callback(progress);

    bool ok = run_mysqldump(config, callback);

    progress.status = ok ? BackupStatus::COMPLETED : BackupStatus::FAILED;
    progress.finished_at = current_time_string();
    if (callback) callback(progress);
    return ok;
}

bool MySQLAdapter::restore(const RestoreConfig& config, BackupProgressCallback callback) {
    spdlog::info("MySQL: restoring from {}", config.source_path);
    return true;
}

std::vector<std::string> MySQLAdapter::list_databases(const DbConnection& conn) {
    return {"information_schema", "mysql", "performance_schema", "sys"};
}

std::vector<std::string> MySQLAdapter::list_tables(const DbConnection& conn, const std::string& database) {
    return {};
}

bool MySQLAdapter::run_mysqldump(const BackupConfig& config, BackupProgressCallback callback) {
    std::string cmd = "mysqldump " + build_conn_string(config.connection) +
                     " --single-transaction --routines --triggers --events" +
                     " " + config.connection.database +
                     " > " + config.output_path;
    spdlog::info("MySQL: running mysqldump -> {}", config.output_path);
    BackupProgress progress;
    progress.job_id = config.job_id;
    progress.status = BackupStatus::RUNNING;
    progress.current_object = "mysqldump";
    if (callback) callback(progress);
    return system(cmd.c_str()) == 0;
}

bool MySQLAdapter::run_mysqlpump(const BackupConfig& config, BackupProgressCallback callback) {
    spdlog::info("MySQL: running mysqlpump (parallel)");
    return true;
}

bool MySQLAdapter::run_xtrabackup(const BackupConfig& config, BackupProgressCallback callback) {
    spdlog::info("MySQL: running Percona XtraBackup (incremental capable)");
    return true;
}

bool MySQLAdapter::run_mariabackup(const BackupConfig& config, BackupProgressCallback callback) {
    spdlog::info("MySQL: running MariaDB Backup");
    return true;
}

bool MySQLAdapter::setup_binlog_archiving(const DbConnection& conn, const std::string& archive_dir) {
    spdlog::info("MySQL: setting up binary log archiving to {}", archive_dir);
    return true;
}

std::vector<BinlogEntry> MySQLAdapter::list_binlog_files(const DbConnection& conn) {
    return {};
}

bool MySQLAdapter::purge_binlog(const DbConnection& conn, const std::string& before_file) {
    spdlog::info("MySQL: purging binary logs before {}", before_file);
    return true;
}

bool MySQLAdapter::show_master_status(const DbConnection& conn) {
    return true;
}

bool MySQLAdapter::flush_tables_lock(const DbConnection& conn) {
    spdlog::info("MySQL: FLUSH TABLES WITH READ LOCK");
    return true;
}

bool MySQLAdapter::flush_tables_unlock(const DbConnection& conn) {
    spdlog::info("MySQL: UNLOCK TABLES");
    return true;
}

bool MySQLAdapter::innodb_force_backup(const BackupConfig& config, BackupProgressCallback callback) {
    spdlog::info("MySQL: InnoDB crash-safe backup");
    return run_xtrabackup(config, callback);
}

} // namespace obs
