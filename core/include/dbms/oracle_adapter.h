#pragma once
#include "dbms/database_adapter.h"

namespace obs {

class OracleAdapter : public DatabaseAdapter {
public:
    DbmsType type() const override { return DbmsType::ORACLE; }
    std::string name() const override { return "Oracle Database"; }
    bool test_connection(const DbConnection& conn) override;
    bool backup(const BackupConfig& config, BackupProgressCallback callback) override;
    bool restore(const RestoreConfig& config, BackupProgressCallback callback) override;
    std::vector<std::string> list_databases(const DbConnection& conn) override;
    std::vector<std::string> list_tables(const DbConnection& conn, const std::string& database) override;

    bool supports_incr_backup() const override { return true; }
    bool supports_continuous_backup() const override { return true; }
    bool supports_point_in_time() const override { return true; }
    bool supports_hot_backup() const override { return true; }
    bool supports_parallel() const override { return true; }
    std::vector<std::string> supported_versions() const override {
        return {"Oracle 11g", "Oracle 12c", "Oracle 18c", "Oracle 19c", "Oracle 21c", "Oracle 23ai"};
    }

    // RMAN (Recovery Manager) support
    bool rman_backup(const BackupConfig& config, BackupProgressCallback callback);
    bool rman_restore(const RestoreConfig& config, BackupProgressCallback callback);
    bool rman_incremental(const BackupConfig& config, BackupProgressCallback callback);
    bool rman_archivelog_backup(const BackupConfig& config, BackupProgressCallback callback);

    // Hot backup mode
    bool begin_hot_backup(const DbConnection& conn);
    bool end_hot_backup(const DbConnection& conn);

    // Data Pump export
    bool datapump_export(const DbConnection& conn, const std::string& schema,
                        const std::string& output_dir, BackupProgressCallback callback);
    bool datapump_import(const DbConnection& conn, const std::string& schema,
                        const std::string& dumpfile, BackupProgressCallback callback);

    // Pluggable Database (PDB) support
    bool backup_pdb(const DbConnection& conn, const std::string& pdb_name,
                   const std::string& backup_path, BackupProgressCallback callback);

private:
    bool run_rman(const DbConnection& conn, const std::string& script);
    bool run_sqlplus(const DbConnection& conn, const std::string& sql);
};

} // namespace obs
