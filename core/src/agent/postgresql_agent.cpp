#include "agent/postgresql_agent.h"
#include <spdlog/spdlog.h>

namespace obs {

PostgresqlAgent::PostgresqlAgent() {
    adapter_ = std::make_shared<PostgresAdapter>();
    spdlog::debug("PostgresqlAgent created");
}

PostgresqlAgent::~PostgresqlAgent() { stop(); }

bool PostgresqlAgent::can_handle(const std::string& job_type) const {
    return job_type == "database_backup" || job_type == "postgresql" || job_type == "postgres";
}

BackupResult PostgresqlAgent::execute_job(const BackupRequest& request) {
    spdlog::info("PostgresqlAgent executing job: {}", request.job_id);

    if (event_bus_) {
        event_bus_->publish(JobStartedEvent{request.job_id, config_.agent_id});
    }

    BackupConfig cfg{request.job_id, connection_, DbmsType::POSTGRESQL,
        "full", request.target_path, true, false, 1};

    BackupResult result;
    adapter_->backup(cfg, [this, &request](const BackupProgress& p) {
        if (event_bus_) {
            event_bus_->publish(JobProgressEvent{request.job_id,
                p.total_bytes > 0 ? double(p.transferred_bytes) / p.total_bytes * 100.0 : 0.0,
                p.transferred_bytes});
        }
    });

    if (event_bus_) {
        event_bus_->publish(JobCompletedEvent{request.job_id, result.success, result.error});
    }

    return result;
}

void PostgresqlAgent::set_connection(const DbConnection& conn) { connection_ = conn; }

BackupResult PostgresqlAgent::backup_full(const std::string& output) {
    spdlog::info("PostgresqlAgent: full backup to {}", output);
    BackupConfig cfg{"pg_full_" + current_time_string(), connection_, DbmsType::POSTGRESQL,
        "full", output, true, false, 1};
    BackupResult result;
    adapter_->backup(cfg, [](const BackupProgress&) {});
    return result;
}

BackupResult PostgresqlAgent::backup_wal(const std::string& output) {
    spdlog::info("PostgresqlAgent: WAL backup to {}", output);
    BackupConfig cfg{"pg_wal_" + current_time_string(), connection_, DbmsType::POSTGRESQL,
        "wal", output, true, false, 1};
    BackupResult result;
    adapter_->backup(cfg, [](const BackupProgress&) {});
    return result;
}

bool PostgresqlAgent::restore(const std::string& backup_path, const std::string& target_time) {
    spdlog::info("PostgresqlAgent: restoring from {} to time {}", backup_path, target_time);
    RestoreConfig cfg{"pg_restore", connection_, backup_path};
    cfg.point_in_time = !target_time.empty();
    cfg.recovery_time = target_time;
    return adapter_->restore(cfg, [](const BackupProgress&) {});
}

bool PostgresqlAgent::archive_wal(const std::string& wal_path) {
    spdlog::info("PostgresqlAgent: archiving WAL {}", wal_path);
    return true;
}

std::vector<std::string> PostgresqlAgent::list_databases() {
    return adapter_->list_databases(connection_);
}

bool PostgresqlAgent::test_connection() {
    return adapter_->test_connection(connection_);
}

bool PostgresqlAgent::enable_continuous_archiving(const std::string& archive_dir) {
    spdlog::info("PostgresqlAgent: enabling continuous archiving to {}", archive_dir);
    continuous_archiving_ = true;
    return true;
}

} // namespace obs
