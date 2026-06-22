#include "agent/mssql_agent.h"
#include <spdlog/spdlog.h>

namespace obs {

MssqlAgent::MssqlAgent() {
    adapter_ = std::make_shared<MsSqlAdapter>();
    spdlog::debug("MssqlAgent created");
}

MssqlAgent::~MssqlAgent() { stop(); }

bool MssqlAgent::can_handle(const std::string& job_type) const {
    return job_type == "database_backup" || job_type == "mssql" || job_type == "sqlserver";
}

BackupResult MssqlAgent::execute_job(const BackupRequest& request) {
    spdlog::info("MssqlAgent executing database backup job: {}", request.job_id);

    if (event_bus_) {
        event_bus_->publish(JobStartedEvent{request.job_id, config_.agent_id});
    }

    BackupResult result;
    bool ok = adapter_->backup({request.job_id, connection_, DbmsType::MSSQL,
        "full", request.target_path, true, false, 1},
        [](const BackupProgress&) {});
    result.success = ok;

    if (event_bus_) {
        event_bus_->publish(JobCompletedEvent{request.job_id, result.success, result.error});
    }

    return result;
}

void MssqlAgent::set_connection(const DbConnection& conn) { connection_ = conn; }

BackupResult MssqlAgent::backup_full(const std::string& database, const std::string& output) {
    spdlog::info("MssqlAgent: full backup of database '{}' to {}", database, output);
    BackupConfig cfg{"mssql_full_" + current_time_string(), connection_, DbmsType::MSSQL,
        "full", output, true, false, 1};
    BackupResult result;
    adapter_->backup(cfg, [](const BackupProgress&) {});
    return result;
}

BackupResult MssqlAgent::backup_log(const std::string& database, const std::string& output) {
    spdlog::info("MssqlAgent: log backup of database '{}' to {}", database, output);
    BackupConfig cfg{"mssql_log_" + current_time_string(), connection_, DbmsType::MSSQL,
        "log", output, true, false, 1};
    BackupResult result;
    adapter_->backup(cfg, [](const BackupProgress&) {});
    return result;
}

BackupResult MssqlAgent::backup_differential(const std::string& database, const std::string& output) {
    spdlog::info("MssqlAgent: differential backup of database '{}' to {}", database, output);
    BackupConfig cfg{"mssql_diff_" + current_time_string(), connection_, DbmsType::MSSQL,
        "differential", output, true, false, 1};
    BackupResult result;
    adapter_->backup(cfg, [](const BackupProgress&) {});
    return result;
}

bool MssqlAgent::restore(const std::string& database, const std::string& backup_path) {
    spdlog::info("MssqlAgent: restoring database '{}' from {}", database, backup_path);
    RestoreConfig cfg{"mssql_restore", connection_, backup_path};
    return adapter_->restore(cfg, [](const BackupProgress&) {});
}

std::vector<std::string> MssqlAgent::list_databases() {
    return adapter_->list_databases(connection_);
}

bool MssqlAgent::test_connection() {
    return adapter_->test_connection(connection_);
}

} // namespace obs
