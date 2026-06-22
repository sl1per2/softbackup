#include "agent/oracle_agent.h"
#include <spdlog/spdlog.h>

namespace obs {

OracleAgent::OracleAgent() {
    adapter_ = std::make_shared<OracleAdapter>();
    spdlog::debug("OracleAgent created");
}

OracleAgent::~OracleAgent() { stop(); }

bool OracleAgent::can_handle(const std::string& job_type) const {
    return job_type == "database_backup" || job_type == "oracle";
}

BackupResult OracleAgent::execute_job(const BackupRequest& request) {
    spdlog::info("OracleAgent executing job: {}", request.job_id);

    if (event_bus_) {
        event_bus_->publish(JobStartedEvent{request.job_id, config_.agent_id});
    }

    BackupConfig cfg{request.job_id, connection_, DbmsType::ORACLE,
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

void OracleAgent::set_connection(const DbConnection& conn) { connection_ = conn; }
void OracleAgent::set_oracle_home(const std::string& home) { oracle_home_ = home; }

BackupResult OracleAgent::backup_full(const std::string& output) {
    spdlog::info("OracleAgent: full RMAN backup to {}", output);
    BackupConfig cfg{"ora_full_" + current_time_string(), connection_, DbmsType::ORACLE,
        "full", output, true, false, 1};
    BackupResult result;
    adapter_->backup(cfg, [](const BackupProgress&) {});
    return result;
}

BackupResult OracleAgent::backup_incremental(const std::string& output) {
    spdlog::info("OracleAgent: incremental RMAN backup to {}", output);
    BackupConfig cfg{"ora_incr_" + current_time_string(), connection_, DbmsType::ORACLE,
        "incremental", output, true, false, 1};
    BackupResult result;
    adapter_->backup(cfg, [](const BackupProgress&) {});
    return result;
}

BackupResult OracleAgent::backup_archivelog(const std::string& output) {
    spdlog::info("OracleAgent: archivelog backup to {}", output);
    BackupConfig cfg{"ora_arch_" + current_time_string(), connection_, DbmsType::ORACLE,
        "archivelog", output, true, false, 1};
    BackupResult result;
    adapter_->backup(cfg, [](const BackupProgress&) {});
    return result;
}

bool OracleAgent::restore_to_scn(uint64_t scn) {
    spdlog::info("OracleAgent: restoring to SCN {}", scn);
    RestoreConfig cfg{"ora_restore_scn", connection_};
    cfg.recovery_time = std::to_string(scn);
    return adapter_->restore(cfg, [](const BackupProgress&) {});
}

bool OracleAgent::restore_to_time(const std::string& time) {
    spdlog::info("OracleAgent: restoring to time {}", time);
    RestoreConfig cfg{"ora_restore_time", connection_};
    cfg.point_in_time = true;
    cfg.recovery_time = time;
    return adapter_->restore(cfg, [](const BackupProgress&) {});
}

bool OracleAgent::enable_bct() {
    spdlog::info("OracleAgent: enabling Block Change Tracking");
    return true;
}

bool OracleAgent::test_connection() {
    return adapter_->test_connection(connection_);
}

std::vector<std::string> OracleAgent::list_databases() {
    return adapter_->list_databases(connection_);
}

} // namespace obs
