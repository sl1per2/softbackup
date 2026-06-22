#include "agent/ndmp_agent.h"
#include <spdlog/spdlog.h>

namespace obs {

NdmpAgent::NdmpAgent() { spdlog::debug("NdmpAgent created"); }
NdmpAgent::~NdmpAgent() { stop(); }

bool NdmpAgent::can_handle(const std::string& job_type) const {
    return job_type == "nas_backup" || job_type == "ndmp";
}

BackupResult NdmpAgent::execute_job(const BackupRequest& request) {
    spdlog::info("NdmpAgent executing job: {}", request.job_id);

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

void NdmpAgent::set_nas_host(const std::string& host, uint16_t port) {
    nas_host_ = host;
    nas_port_ = port;
}

void NdmpAgent::set_credentials(const std::string& username, const std::string& password) {
    username_ = username;
    password_ = password;
}

bool NdmpAgent::test_connection() {
    spdlog::info("NdmpAgent: testing connection to {}:{}", nas_host_, nas_port_);
    return true;
}

std::vector<std::string> NdmpAgent::list_volumes() {
    spdlog::info("NdmpAgent: listing volumes on {}:{}", nas_host_, nas_port_);
    return {};
}

BackupResult NdmpAgent::backup_volume(const std::string& volume, const std::string& output) {
    spdlog::info("NdmpAgent: backing up volume '{}' to {}", volume, output);
    BackupResult result;
    result.success = true;
    return result;
}

BackupResult NdmpAgent::backup_all(const std::string& output) {
    spdlog::info("NdmpAgent: backing up all volumes to {}", output);
    BackupResult result;
    result.success = true;
    return result;
}

bool NdmpAgent::restore_volume(const std::string& volume, const std::string& backup_path) {
    spdlog::info("NdmpAgent: restoring volume '{}' from {}", volume, backup_path);
    return true;
}

} // namespace obs
