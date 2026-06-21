#pragma once
#include "common.h"
#include <functional>

namespace obs {

enum class DbmsType {
    POSTGRESQL, MYSQL, MARIADB, MSSQL, ORACLE, SAP_HANA,
    POSTGRES_PRO, TANTOR, ARENADATA, GREENPLUM, YANDEXDB,
    RED_DATABASE, MONGODB, BREST_DB
};

struct DbConnection {
    std::string host;
    uint16_t port;
    std::string username;
    std::string password;
    std::string database;
    bool ssl = false;
    std::string ssl_ca;
    std::string service_name; // Oracle SID
    int instance = 1; // MSSQL instance
};

struct BackupConfig {
    std::string job_id;
    DbConnection connection;
    DbmsType type;
    std::string backup_type; // full, incremental, wal, binlog
    std::string output_path;
    bool compress = true;
    bool encrypt = false;
    int compression_level = 1;
    bool include_globals = true;
    bool include_roles = true;
    bool include_tablespaces = true;
    std::string exclude_tables;
    std::vector<std::string> include_databases;
    bool streaming = true;
    bool checksum = true;
};

enum class BackupStatus { PENDING, RUNNING, COMPLETED, FAILED, CANCELLED };

struct BackupProgress {
    std::string job_id;
    BackupStatus status = BackupStatus::PENDING;
    uint64_t total_bytes = 0;
    uint64_t transferred_bytes = 0;
    double speed_mbps = 0;
    int32_t objects_completed = 0;
    int32_t objects_total = 0;
    std::string current_object;
    std::string error;
    std::string started_at;
    std::string finished_at;
    double compression_ratio = 0;
    uint64_t dedup_saved = 0;
};

struct RestoreConfig {
    std::string backup_id;
    DbConnection target;
    std::string source_path;
    bool drop_existing = false;
    bool create_database = true;
    std::vector<std::string> include_databases;
    bool point_in_time = false;
    std::string recovery_time;
    bool verify = true;
};

struct WALEntry {
    uint64_t lsn;
    std::string timestamp;
    std::string action;
    uint64_t size;
};

struct BinlogEntry {
    std::string filename;
    uint64_t position;
    std::string timestamp;
    std::string event_type;
};

using BackupProgressCallback = std::function<void(const BackupProgress&)>;

class DatabaseAdapter {
public:
    virtual ~DatabaseAdapter() = default;
    virtual DbmsType type() const = 0;
    virtual std::string name() const = 0;

    virtual bool test_connection(const DbConnection& conn) = 0;
    virtual bool backup(const BackupConfig& config, BackupProgressCallback callback) = 0;
    virtual bool restore(const RestoreConfig& config, BackupProgressCallback callback) = 0;
    virtual std::vector<std::string> list_databases(const DbConnection& conn) = 0;
    virtual std::vector<std::string> list_tables(const DbConnection& conn, const std::string& database) = 0;

    virtual bool supports_incr_backup() const { return false; }
    virtual bool supports_wal_archive() const { return false; }
    virtual bool supports_continuous_backup() const { return false; }
    virtual bool supports_point_in_time() const { return false; }
    virtual bool supports_cluster() const { return false; }
    virtual bool supports_compression() const { return true; }
    virtual bool supports_encryption() const { return false; }
    virtual bool supports_checksum() const { return true; }
    virtual bool supports_streaming() const { return true; }
    virtual bool supports_hot_backup() const { return true; }
    virtual bool supports_parallel() const { return false; }

    virtual std::vector<std::string> supported_versions() const = 0;
};

} // namespace obs
