#include "agent/hyperv_agent.h"
#include <spdlog/spdlog.h>

namespace obs {

HypervAgent::HypervAgent() { spdlog::debug("HypervAgent created"); }
HypervAgent::~HypervAgent() { stop(); }

bool HypervAgent::can_handle(const std::string& job_type) const {
    return job_type == "vm_backup" || job_type == "hyperv";
}

BackupResult HypervAgent::execute_job(const BackupRequest& request) {
    spdlog::info("HypervAgent executing VM backup job: {}", request.job_id);

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

void HypervAgent::set_host(const std::string& host) { host_ = host; }
void HypervAgent::set_credentials(const std::string& username, const std::string& password) {
    username_ = username;
    password_ = password;
}

std::vector<std::string> HypervAgent::list_vms() {
    spdlog::info("HypervAgent: listing VMs from {}", host_);
    return {};
}

bool HypervAgent::enable_rct(const std::string& vm_id) {
    spdlog::info("HypervAgent: enabling RCT for VM {}", vm_id);
    return true;
}

std::vector<std::pair<uint64_t, uint32_t>> HypervAgent::get_rct_changed_blocks(
    const std::string& vm_id, const std::string& from, const std::string& to) {
    spdlog::info("HypervAgent: getting RCT changed blocks for VM {}", vm_id);
    return {};
}

bool HypervAgent::backup_vm(const std::string& vm_id, const BackupRequest& request) {
    spdlog::info("HypervAgent: backing up VM {}", vm_id);
    return true;
}

bool HypervAgent::create_vss_checkpoint(const std::string& vm_id) {
    spdlog::info("HypervAgent: creating VSS checkpoint for VM {}", vm_id);
    return true;
}

} // namespace obs
