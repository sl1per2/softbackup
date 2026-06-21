#pragma once
#include "dbms/database_adapter.h"

namespace obs {

class SapHanaAdapter : public DatabaseAdapter {
public:
    DbmsType type() const override { return DbmsType::SAP_HANA; }
    std::string name() const override { return "SAP HANA"; }
    bool test_connection(const DbConnection& conn) override;
    bool backup(const BackupConfig& config, BackupProgressCallback callback) override;
    bool restore(const RestoreConfig& config, BackupProgressCallback callback) override;
    std::vector<std::string> list_databases(const DbConnection& conn) override;
    std::vector<std::string> list_tables(const DbConnection& conn, const std::string& database) override;

    bool supports_incr_backup() const override { return true; }
    bool supports_continuous_backup() const override { return true; }
    bool supports_hot_backup() const override { return true; }
    std::vector<std::string> supported_versions() const override {
        return {"SAP HANA 2.0 SPS01-SPS08"};
    }

    bool backup_system_database(const DbConnection& conn, const std::string& backup_path);
    bool backup_tenant(const DbConnection& conn, const std::string& tenant,
                      const std::string& backup_path, BackupProgressCallback callback);
    bool setup_log_shipping(const DbConnection& conn, const std::string& target_host);

private:
    bool run_hdbsql(const DbConnection& conn, const std::string& sql);
};

class YandexDbAdapter : public DatabaseAdapter {
public:
    DbmsType type() const override { return DbmsType::YANDEXDB; }
    std::string name() const override { return "YandexDB"; }
    bool test_connection(const DbConnection& conn) override;
    bool backup(const BackupConfig& config, BackupProgressCallback callback) override;
    bool restore(const RestoreConfig& config, BackupProgressCallback callback) override;
    std::vector<std::string> list_databases(const DbConnection& conn) override;
    std::vector<std::string> list_tables(const DbConnection& conn, const std::string& database) override;
    std::vector<std::string> supported_versions() const override { return {"YandexDB 1.0-2.x"}; }
};

class RedDatabaseAdapter : public DatabaseAdapter {
public:
    DbmsType type() const override { return DbmsType::RED_DATABASE; }
    std::string name() const override { return "РЕД База Данных"; }
    bool test_connection(const DbConnection& conn) override;
    bool backup(const BackupConfig& config, BackupProgressCallback callback) override;
    bool restore(const RestoreConfig& config, BackupProgressCallback callback) override;
    std::vector<std::string> list_databases(const DbConnection& conn) override;
    std::vector<std::string> list_tables(const DbConnection& conn, const std::string& database) override;
    std::vector<std::string> supported_versions() const override { return {"РЕД БД 7.x-8.x"}; }
};

class ArenadataAdapter : public DatabaseAdapter {
public:
    DbmsType type() const override { return DbmsType::ARENADATA; }
    std::string name() const override { return "Arenadata"; }
    bool test_connection(const DbConnection& conn) override;
    bool backup(const BackupConfig& config, BackupProgressCallback callback) override;
    bool restore(const RestoreConfig& config, BackupProgressCallback callback) override;
    std::vector<std::string> list_databases(const DbConnection& conn) override;
    std::vector<std::string> list_tables(const DbConnection& conn, const std::string& database) override;
    std::vector<std::string> supported_versions() const override { return {"Arenadata DB 6.2-6.5"}; }
};

class GreenplumAdapter : public DatabaseAdapter {
public:
    DbmsType type() const override { return DbmsType::GREENPLUM; }
    std::string name() const override { return "Greenplum"; }
    bool test_connection(const DbConnection& conn) override;
    bool backup(const BackupConfig& config, BackupProgressCallback callback) override;
    bool restore(const RestoreConfig& config, BackupProgressCallback callback) override;
    std::vector<std::string> list_databases(const DbConnection& conn) override;
    std::vector<std::string> list_tables(const DbConnection& conn, const std::string& database) override;
    bool supports_parallel() const override { return true; }
    std::vector<std::string> supported_versions() const override { return {"Greenplum 6.x-7.x"}; }
};

class BrestAdapter : public DatabaseAdapter {
public:
    DbmsType type() const override { return DbmsType::BREST_DB; }
    std::string name() const override { return "ПК СВ «Брест»"; }
    bool test_connection(const DbConnection& conn) override;
    bool backup(const BackupConfig& config, BackupProgressCallback callback) override;
    bool restore(const RestoreConfig& config, BackupProgressCallback callback) override;
    std::vector<std::string> list_databases(const DbConnection& conn) override;
    std::vector<std::string> list_tables(const DbConnection& conn, const std::string& database) override;
    std::vector<std::string> supported_versions() const override { return {"ПК СВ Брест 3.x-5.x"}; }
};

} // namespace obs
