#pragma once
#include "dbms/database_adapter.h"
#include <spdlog/spdlog.h>
#include <cstdlib>

namespace obs {

class MongoDbAdapter : public DatabaseAdapter {
public:
    DbmsType type() const override { return DbmsType::MONGODB; }
    std::string name() const override { return "MongoDB"; }
    bool test_connection(const DbConnection& conn) override;
    bool backup(const BackupConfig& config, BackupProgressCallback callback) override;
    bool restore(const RestoreConfig& config, BackupProgressCallback callback) override;
    std::vector<std::string> list_databases(const DbConnection& conn) override;
    std::vector<std::string> list_tables(const DbConnection& conn, const std::string& database) override;

    bool supports_incr_backup() const override { return true; }
    bool supports_hot_backup() const override { return true; }
    std::vector<std::string> supported_versions() const override {
        return {"MongoDB 4.4-7.0"};
    }

    // MongoDB-specific
    bool oplog_backup(const DbConnection& conn, const std::string& output_path, BackupProgressCallback callback);
    bool incremental_mongodump(const DbConnection& conn, const std::string& since_timestamp, const std::string& output_path);
    bool rsync_backup(const DbConnection& conn, const std::string& output_path);

private:
    bool run_mongodump(const DbConnection& conn, const std::string& database, const std::string& output_path);
};

class YcDbAdapter : public DatabaseAdapter {
public:
    DbmsType type() const override { return DbmsType::YANDEXDB; }
    std::string name() const override { return "YandexDB (1C)"; }
    bool test_connection(const DbConnection& conn) override;
    bool backup(const BackupConfig& config, BackupProgressCallback callback) override;
    bool restore(const RestoreConfig& config, BackupProgressCallback callback) override;
    std::vector<std::string> list_databases(const DbConnection& conn) override;
    std::vector<std::string> list_tables(const DbConnection& conn, const std::string& database) override;
    std::vector<std::string> supported_versions() const override {
        return {"1C:Enterprise 8.3 (PostgreSQL backend)", "1C:Enterprise 8.3 (file-based)"};
    }

    // 1C-specific
    bool backup_1c_config(const std::string& infobase_path, const std::string& output_path);
    bool backup_1c_database(const DbConnection& conn, const std::string& output_path);
};

} // namespace obs
