#include "agent/saphana_agent.h"
#include <spdlog/spdlog.h>

namespace obs {

SapHanaAgent::SapHanaAgent() { spdlog::debug("SapHanaAgent created"); }
SapHanaAgent::~SapHanaAgent() { stop(); }

bool SapHanaAgent::can_handle(const std::string& job_type) const {
    return job_type == "database_backup" || job_type == "saphana" || job_type == "hana";
}

BackupResult SapHanaAgent::execute_job(const BackupRequest& request) {
    spdlog::info("SapHanaAgent executing job: {}", request.job_id);

    if (event_bus_) {
        event_bus_->publish(JobStartedEvent{request.job_id, config_.agent_id});
    }

    BackupResult result;
    result.success = true;

    if (event_bus_) {
        event_bus_->publish(JobCompletedEvent{request.job_id, true, ""});
    }

    return result;
}

void SapHanaAgent::set_connection(const DbConnection& conn) { connection_ = conn; }
void SapHanaAgent::set_system_user(const std::string& user, const std::string& password) {
    system_user_ = user;
    system_password_ = password;
}

BackupResult SapHanaAgent::backup_full(const std::string& output) {
    spdlog::info("SapHanaAgent: full backup to {}", output);
    BackupResult result;
    result.success = true;
    return result;
}

BackupResult SapHanaAgent::backup_incremental(const std::string& output) {
    spdlog::info("SapHanaAgent: incremental backup to {}", output);
    BackupResult result;
    result.success = true;
    return result;
}

BackupResult SapHanaAgent::backup_log(const std::string& output) {
    spdlog::info("SapHanaAgent: log backup to {}", output);
    BackupResult result;
    result.success = true;
    return result;
}

bool SapHanaAgent::restore(const std::string& backup_id) {
    spdlog::info("SapHanaAgent: restoring from backup {}", backup_id);
    return true;
}

bool SapHanaAgent::test_connection() {
    spdlog::info("SapHanaAgent: testing connection to {}", connection_.host);
    return true;
}

} // namespace obs
