#pragma once
#include "dbms/database_adapter.h"
#include <thread>

namespace obs {

// PostgreSQL, Postgres Pro, Tantor, Arenadata, Greenplum, Patroni
class PostgresAdapter : public DatabaseAdapter {
public:
    DbmsType type() const override { return DbmsType::POSTGRESQL; }
    std::string name() const override { return "PostgreSQL"; }
    bool test_connection(const DbConnection& conn) override;
    bool backup(const BackupConfig& config, BackupProgressCallback callback) override;
    bool restore(const RestoreConfig& config, BackupProgressCallback callback) override;
    std::vector<std::string> list_databases(const DbConnection& conn) override;
    std::vector<std::string> list_tables(const DbConnection& conn, const std::string& database) override;

    bool supports_incr_backup() const override { return true; }
    bool supports_wal_archive() const override { return true; }
    bool supports_continuous_backup() const override { return true; }
    bool supports_point_in_time() const override { return true; }
    bool supports_cluster() const override { return true; }
    bool supports_parallel() const override { return true; }
    bool supports_hot_backup() const override { return true; }
    std::vector<std::string> supported_versions() const override {
        return {"PostgreSQL 9.6-17", "Postgres Pro 11-16", "Tantor SE 12-16",
                "Arenadata DB 6.2-6.5", "Greenplum 6-7"};
    }

    // Patroni cluster support
    bool patroni_backup(const std::string& patroni_url, const BackupConfig& config, BackupProgressCallback callback);
    std::string get_patroni_leader(const std::string& patroni_url);
    std::vector<std::string> get_patroni_members(const std::string& patroni_url);

    // WAL archiving
    bool setup_wal_archiving(const DbConnection& conn, const std::string& archive_dir);
    bool restore_wal(const std::string& wal_path, const DbConnection& conn);
    std::vector<WALEntry> list_wal_files(const std::string& archive_dir);
    bool create_restore_point(const DbConnection& conn, const std::string& name);

    // pg_start_backup / pg_stop_backup
    bool pg_start_backup(const DbConnection& conn, const std::string& label, bool fast);
    bool pg_stop_backup(const DbConnection& conn);

private:
    bool run_pg_dump(const BackupConfig& config, BackupProgressCallback callback);
    bool run_pg_basebackup(const BackupConfig& config, BackupProgressCallback callback);
    bool run_pg_restore(const RestoreConfig& config, BackupProgressCallback callback);
    std::string build_conn_string(const DbConnection& conn);
};

} // namespace obs
