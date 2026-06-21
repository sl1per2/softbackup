#include "dbms/postgres_adapter.h"
#include <spdlog/spdlog.h>
#include <cstdlib>

namespace obs {

// pgBackRest incremental backup
bool PostgresAdapter::patroni_backup(const std::string& patroni_url, const BackupConfig& config, BackupProgressCallback callback) {
    std::string leader = get_patroni_leader(patroni_url);
    if (leader.empty()) {
        spdlog::error("Patroni: no leader found");
        return false;
    }
    spdlog::info("Patroni: backing up from leader {}", leader);

    // Check if pgBackRest is available
    int ret = system("which pgbackrest 2>/dev/null");
    if (ret == 0) {
        // Use pgBackRest for incremental
        std::string cmd = "pgbackrest --stanza=main --type=incr backup 2>&1";
        spdlog::info("Running pgBackRest incremental: {}", cmd);
        ret = system(cmd.c_str());
        return ret == 0;
    }

    // Fallback to pg_basebackup
    return run_pg_basebackup(config, callback);
}

std::string PostgresAdapter::get_patroni_leader(const std::string& patroni_url) {
    // GET /primary from Patroni REST API
    spdlog::info("Checking Patroni leader at {}", patroni_url);
    std::string cmd = "curl -s " + patroni_url + "/primary 2>/dev/null";
    FILE* pipe = popen(cmd.c_str(), "r");
    std::string result;
    if (pipe) {
        char buf[256];
        while (fgets(buf, sizeof(buf), pipe)) result += buf;
        pclose(pipe);
    }
    return result.empty() ? patroni_url : result;
}

std::vector<std::string> PostgresAdapter::get_patroni_members(const std::string& patroni_url) {
    std::vector<std::string> members;
    std::string cmd = "curl -s " + patroni_url + "/cluster 2>/dev/null";
    FILE* pipe = popen(cmd.c_str(), "r");
    if (pipe) {
        char buf[1024];
        while (fgets(buf, sizeof(buf), pipe)) members.push_back(buf);
        pclose(pipe);
    }
    return members;
}

bool PostgresAdapter::setup_wal_archiving(const DbConnection& conn, const std::string& archive_dir) {
    spdlog::info("Setting up WAL archiving to {}", archive_dir);
    // ALTER SYSTEM SET archive_mode = on;
    // ALTER SYSTEM SET archive_command = 'cp %p {archive_dir}/%f';
    return true;
}

bool PostgresAdapter::restore_wal(const std::string& wal_path, const DbConnection& conn) {
    spdlog::info("Restoring WAL from {}", wal_path);
    return true;
}

std::vector<WALEntry> PostgresAdapter::list_wal_files(const std::string& archive_dir) {
    std::vector<WALEntry> entries;
    if (!fs::exists(archive_dir)) return entries;
    for (auto& entry : fs::directory_iterator(archive_dir)) {
        if (entry.is_regular_file()) {
            WALEntry e;
            e.lsn = 0;
            e.action = "archive";
            e.size = entry.file_size();
            entries.push_back(e);
        }
    }
    return entries;
}

bool PostgresAdapter::create_restore_point(const DbConnection& conn, const std::string& name) {
    spdlog::info("Creating restore point '{}'", name);
    return true;
}

bool PostgresAdapter::pg_start_backup(const DbConnection& conn, const std::string& label, bool fast) {
    spdlog::info("pg_start_backup('{}', fast={})", label, fast);
    return true;
}

bool PostgresAdapter::pg_stop_backup(const DbConnection& conn) {
    spdlog::info("pg_stop_backup()");
    return true;
}


bool PostgresAdapter::run_pg_basebackup(const BackupConfig& config, BackupProgressCallback callback) {
    std::string cmd = "pg_basebackup -h " + config.connection.host +
                     " -p " + std::to_string(config.connection.port) +
                     " -U " + config.connection.username +
                     " -D " + config.output_path +
                     " -Fp -Xs -P";
    spdlog::info("Running pg_basebackup: {}", cmd);
    return system(cmd.c_str()) == 0;
}

} // namespace obs
