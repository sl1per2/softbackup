#pragma once
#include "dbms/database_adapter.h"

namespace obs {

class MsSqlAdapter : public DatabaseAdapter {
public:
    DbmsType type() const override { return DbmsType::MSSQL; }
    std::string name() const override { return "Microsoft SQL Server"; }
    bool test_connection(const DbConnection& conn) override;
    bool backup(const BackupConfig& config, BackupProgressCallback callback) override;
    bool restore(const RestoreConfig& config, BackupProgressCallback callback) override;
    std::vector<std::string> list_databases(const DbConnection& conn) override;
    std::vector<std::string> list_tables(const DbConnection& conn, const std::string& database) override;

    bool supports_incr_backup() const override { return true; }
    bool supports_continuous_backup() const override { return true; }
    bool supports_point_in_time() const override { return true; }
    bool supports_hot_backup() const override { return true; }
    bool supports_checksum() const override { return true; }
    std::vector<std::string> supported_versions() const override {
        return {"SQL Server 2012", "SQL Server 2014", "SQL Server 2016",
                "SQL Server 2017", "SQL Server 2019", "SQL Server 2022"};
    }

    // VSS (Volume Shadow Copy) support
    bool vss_backup(const BackupConfig& config, BackupProgressCallback callback);
    bool backup_database(const DbConnection& conn, const std::string& database,
                        const std::string& backup_path, bool full);

    // Transaction log backup
    bool backup_log(const DbConnection& conn, const std::string& database,
                   const std::string& backup_path);

    // Differential backup
    bool backup_differential(const DbConnection& conn, const std::string& database,
                            const std::string& backup_path);

    // Always On AG awareness
    bool backup_ag_secondary(const BackupConfig& config, BackupProgressCallback callback);
    std::vector<std::string> list_ag_replicas(const DbConnection& conn);

    // TDE (Transparent Data Encryption) awareness
    bool backup_with_tde(const BackupConfig& config, BackupProgressCallback callback);

private:
    bool run_sqlcmd(const DbConnection& conn, const std::string& sql);
    std::string build_conn_string(const DbConnection& conn);
};

} // namespace obs
