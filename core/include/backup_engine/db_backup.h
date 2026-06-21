#pragma once
#include "common.h"
#include <functional>

namespace obs {

struct DbBackupConfig {
    std::string db_type;     // "postgresql", "mysql", "mongodb"
    std::string host;
    int port = 0;
    std::string username;
    std::string password;
    std::string database;
    std::string backup_path;
    bool include_globals = true;
    bool consistent = true;
};

struct DbBackupResult {
    bool success = false;
    std::string error;
    uint64_t size_bytes = 0;
    std::string backup_file;
    std::string wal_segment;    // PostgreSQL WAL
    double duration_sec = 0;
};

class DatabaseBackup {
public:
    DbBackupResult backup(const DbBackupConfig& config);
    DbBackupResult backup_postgresql(const DbBackupConfig& config);
    DbBackupResult backup_mysql(const DbBackupConfig& config);
    DbBackupResult backup_mongodb(const DbBackupConfig& config);

private:
    DbBackupResult run_command(const std::string& cmd);
    bool check_connection(const DbBackupConfig& config);
    std::string build_pg_dump_cmd(const DbBackupConfig& config);
    std::string build_mysqldump_cmd(const DbBackupConfig& config);
    std::string build_mongodump_cmd(const DbBackupConfig& config);
};

} // namespace obs
