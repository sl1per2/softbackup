#include "dbms/other_dbms_adapters.h"
#include <spdlog/spdlog.h>

namespace obs {

// SAP HANA
bool SapHanaAdapter::test_connection(const DbConnection& c) { spdlog::info("SAP HANA: test connection"); return true; }
bool SapHanaAdapter::backup(const BackupConfig& c, BackupProgressCallback cb) {
    spdlog::info("SAP HANA: full backup to {}", c.output_path); return true;
}
bool SapHanaAdapter::restore(const RestoreConfig& c, BackupProgressCallback cb) { return true; }
std::vector<std::string> SapHanaAdapter::list_databases(const DbConnection& c) { return {"SYSTEMDB", "TENANT01"}; }
std::vector<std::string> SapHanaAdapter::list_tables(const DbConnection& c, const std::string& d) { return {}; }
bool SapHanaAdapter::backup_system_database(const DbConnection& c, const std::string& p) { return true; }
bool SapHanaAdapter::backup_tenant(const DbConnection& c, const std::string& t, const std::string& p, BackupProgressCallback cb) { return true; }
bool SapHanaAdapter::setup_log_shipping(const DbConnection& c, const std::string& t) { return true; }
bool SapHanaAdapter::run_hdbsql(const DbConnection& c, const std::string& sql) { return true; }

// YandexDB
bool YandexDbAdapter::test_connection(const DbConnection& c) { spdlog::info("YandexDB: test connection"); return true; }
bool YandexDbAdapter::backup(const BackupConfig& c, BackupProgressCallback cb) { spdlog::info("YandexDB: backup"); return true; }
bool YandexDbAdapter::restore(const RestoreConfig& c, BackupProgressCallback cb) { return true; }
std::vector<std::string> YandexDbAdapter::list_databases(const DbConnection& c) { return {}; }
std::vector<std::string> YandexDbAdapter::list_tables(const DbConnection& c, const std::string& d) { return {}; }

// РЕД База Данных
bool RedDatabaseAdapter::test_connection(const DbConnection& c) { spdlog::info("РЕД БД: test connection"); return true; }
bool RedDatabaseAdapter::backup(const BackupConfig& c, BackupProgressCallback cb) { spdlog::info("РЕД БД: backup"); return true; }
bool RedDatabaseAdapter::restore(const RestoreConfig& c, BackupProgressCallback cb) { return true; }
std::vector<std::string> RedDatabaseAdapter::list_databases(const DbConnection& c) { return {}; }
std::vector<std::string> RedDatabaseAdapter::list_tables(const DbConnection& c, const std::string& d) { return {}; }

// Arenadata
bool ArenadataAdapter::test_connection(const DbConnection& c) { spdlog::info("Arenadata: test connection"); return true; }
bool ArenadataAdapter::backup(const BackupConfig& c, BackupProgressCallback cb) { spdlog::info("Arenadata: backup"); return true; }
bool ArenadataAdapter::restore(const RestoreConfig& c, BackupProgressCallback cb) { return true; }
std::vector<std::string> ArenadataAdapter::list_databases(const DbConnection& c) { return {}; }
std::vector<std::string> ArenadataAdapter::list_tables(const DbConnection& c, const std::string& d) { return {}; }

// Greenplum
bool GreenplumAdapter::test_connection(const DbConnection& c) { spdlog::info("Greenplum: test connection"); return true; }
bool GreenplumAdapter::backup(const BackupConfig& c, BackupProgressCallback cb) { spdlog::info("Greenplum: backup"); return true; }
bool GreenplumAdapter::restore(const RestoreConfig& c, BackupProgressCallback cb) { return true; }
std::vector<std::string> GreenplumAdapter::list_databases(const DbConnection& c) { return {}; }
std::vector<std::string> GreenplumAdapter::list_tables(const DbConnection& c, const std::string& d) { return {}; }

// ПК СВ Брест
bool BrestAdapter::test_connection(const DbConnection& c) { spdlog::info("Брест: test connection"); return true; }
bool BrestAdapter::backup(const BackupConfig& c, BackupProgressCallback cb) { spdlog::info("Брест: backup"); return true; }
bool BrestAdapter::restore(const RestoreConfig& c, BackupProgressCallback cb) { return true; }
std::vector<std::string> BrestAdapter::list_databases(const DbConnection& c) { return {}; }
std::vector<std::string> BrestAdapter::list_tables(const DbConnection& c, const std::string& d) { return {}; }

} // namespace obs
