#pragma once
#include "dbms/database_adapter.h"

namespace obs {

class MySQLAdapter : public DatabaseAdapter {
public:
    DbmsType type() const override { return DbmsType::MYSQL; }
    std::string name() const override { return "MySQL/MariaDB"; }
    bool test_connection(const DbConnection& conn) override;
    bool backup(const BackupConfig& config, BackupProgressCallback callback) override;
    bool restore(const RestoreConfig& config, BackupProgressCallback callback) override;
    std::vector<std::string> list_databases(const DbConnection& conn) override;
    std::vector<std::string> list_tables(const DbConnection& conn, const std::string& database) override;

    bool supports_incr_backup() const override { return true; }
    bool supports_wal_archive() const override { return true; }
    bool supports_continuous_backup() const override { return true; }
    bool supports_hot_backup() const override { return true; }
    bool supports_parallel() const override { return true; }
    std::vector<std::string> supported_versions() const override {
        return {"MySQL 5.7-8.4", "MariaDB 10.3-11.4"};
    }

    // Binary log support
    bool setup_binlog_archiving(const DbConnection& conn, const std::string& archive_dir);
    std::vector<BinlogEntry> list_binlog_files(const DbConnection& conn);
    bool purge_binlog(const DbConnection& conn, const std::string& before_file);
    bool show_master_status(const DbConnection& conn);

    // FLUSH TABLES WITH READ LOCK
    bool flush_tables_lock(const DbConnection& conn);
    bool flush_tables_unlock(const DbConnection& conn);

    // InnoDB crash-safe backup
    bool innodb_force_backup(const BackupConfig& config, BackupProgressCallback callback);

private:
    bool run_mysqldump(const BackupConfig& config, BackupProgressCallback callback);
    bool run_mysqlpump(const BackupConfig& config, BackupProgressCallback callback);
    bool run_xtrabackup(const BackupConfig& config, BackupProgressCallback callback);
    bool run_mariabackup(const BackupConfig& config, BackupProgressCallback callback);
    std::string build_conn_string(const DbConnection& conn);
};

} // namespace obs
