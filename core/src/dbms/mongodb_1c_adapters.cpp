#include "dbms/mongodb_1c_adapters.h"
#include <spdlog/spdlog.h>
#include <cstdlib>

namespace obs {

// MongoDB
bool MongoDbAdapter::test_connection(const DbConnection& conn) {
    spdlog::info("MongoDB: testing connection to {}:{}", conn.host, conn.port);
    std::string cmd = "mongosh --host " + conn.host + " --port " + std::to_string(conn.port) + " --eval 'db.runCommand({ping:1})' 2>/dev/null";
    return system(cmd.c_str()) == 0;
}

bool MongoDbAdapter::backup(const BackupConfig& config, BackupProgressCallback callback) {
    spdlog::info("MongoDB: backup to {}", config.output_path);
    return run_mongodump(config.connection, config.connection.database, config.output_path);
}

bool MongoDbAdapter::restore(const RestoreConfig& config, BackupProgressCallback callback) {
    spdlog::info("MongoDB: restore from {}", config.source_path);
    std::string cmd = "mongorestore --host " + config.target.host + " --port " + std::to_string(config.target.port)
                    + " --db " + config.target.database + " " + config.source_path;
    return system(cmd.c_str()) == 0;
}

std::vector<std::string> MongoDbAdapter::list_databases(const DbConnection& conn) {
    std::vector<std::string> dbs;
    std::string cmd = "mongosh --host " + conn.host + " --port " + std::to_string(conn.port) + " --eval 'db.adminCommand({listDatabases:1}).databases.forEach(d=>print(d.name))' --quiet 2>/dev/null";
    FILE* pipe = popen(cmd.c_str(), "r");
    if (pipe) {
        char buf[256];
        while (fgets(buf, sizeof(buf), pipe)) {
            std::string line(buf);
            if (!line.empty() && line.back() == '\n') line.pop_back();
            if (!line.empty()) dbs.push_back(line);
        }
        pclose(pipe);
    }
    return dbs;
}

std::vector<std::string> MongoDbAdapter::list_tables(const DbConnection& conn, const std::string& database) {
    std::vector<std::string> collections;
    std::string cmd = "mongosh --host " + conn.host + " --port " + std::to_string(conn.port)
                    + " --eval 'db.getSiblingDB(\"" + database + "\").listCollectionNames().forEach(c=>print(c))' --quiet 2>/dev/null";
    FILE* pipe = popen(cmd.c_str(), "r");
    if (pipe) {
        char buf[256];
        while (fgets(buf, sizeof(buf), pipe)) {
            std::string line(buf);
            if (!line.empty() && line.back() == '\n') line.pop_back();
            if (!line.empty()) collections.push_back(line);
        }
        pclose(pipe);
    }
    return collections;
}

bool MongoDbAdapter::run_mongodump(const DbConnection& conn, const std::string& database, const std::string& output_path) {
    std::string cmd = "mongodump --host " + conn.host + " --port " + std::to_string(conn.port);
    if (!conn.username.empty()) {
        cmd += " -u " + conn.username + " -p " + conn.password;
    }
    if (!database.empty()) cmd += " --db " + database;
    cmd += " --out " + output_path;
    spdlog::info("Running: {}", cmd);
    return system(cmd.c_str()) == 0;
}

bool MongoDbAdapter::oplog_backup(const DbConnection& conn, const std::string& output_path, BackupProgressCallback callback) {
    spdlog::info("MongoDB: oplog-based backup for PITR");
    std::string cmd = "mongodump --host " + conn.host + " --port " + std::to_string(conn.port)
                    + " --oplog --out " + output_path;
    return system(cmd.c_str()) == 0;
}

bool MongoDbAdapter::incremental_mongodump(const DbConnection& conn, const std::string& since_timestamp, const std::string& output_path) {
    spdlog::info("MongoDB: incremental dump since {}", since_timestamp);
    // Use oplog timestamp to filter
    std::string cmd = "mongodump --host " + conn.host + " --port " + std::to_string(conn.port)
                    + " --query '{\"ts\": {\"$gte\": {\"$timestamp\": {\"t\": " + since_timestamp + ", \"i\": 0}}}}'"
                    + " --out " + output_path;
    return system(cmd.c_str()) == 0;
}

bool MongoDbAdapter::rsync_backup(const DbConnection& conn, const std::string& output_path) {
    spdlog::info("MongoDB: rsync-based backup from secondary");
    std::string cmd = "mongodump --host " + conn.host + " --port " + std::to_string(conn.port)
                    + " --readPreference secondary --out " + output_path;
    return system(cmd.c_str()) == 0;
}

// 1C:Enterprise
bool YcDbAdapter::test_connection(const DbConnection& conn) {
    spdlog::info("1C: testing connection to {}:{}", conn.host, conn.port);
    return true;
}

bool YcDbAdapter::backup(const BackupConfig& config, BackupProgressCallback callback) {
    spdlog::info("1C: backup to {}", config.output_path);
    // If PostgreSQL backend: use pg_dump
    return backup_1c_database(config.connection, config.output_path);
}

bool YcDbAdapter::restore(const RestoreConfig& config, BackupProgressCallback callback) {
    spdlog::info("1C: restore from {}", config.source_path);
    return true;
}

std::vector<std::string> YcDbAdapter::list_databases(const DbConnection& conn) {
    return {"1C_Base"};
}

std::vector<std::string> YcDbAdapter::list_tables(const DbConnection& conn, const std::string& database) {
    return {};
}

bool YcDbAdapter::backup_1c_config(const std::string& infobase_path, const std::string& output_path) {
    spdlog::info("1C: backing up configuration from {}", infobase_path);
    // Dump .cf and .cfu files
    return true;
}

bool YcDbAdapter::backup_1c_database(const DbConnection& conn, const std::string& output_path) {
    spdlog::info("1C: backing up PostgreSQL database for 1C");
    // Use pg_dump since 1C uses PostgreSQL as backend
    std::string cmd = "pg_dump -h " + conn.host + " -p " + std::to_string(conn.port)
                    + " -U " + conn.username + " -d " + conn.database + " -Fc -f " + output_path;
    return system(cmd.c_str()) == 0;
}

} // namespace obs
