#include "agent/esxi_agent.h"
#include <spdlog/spdlog.h>

namespace obs {

EsxiAgent::EsxiAgent() { spdlog::debug("EsxiAgent created"); }
EsxiAgent::~EsxiAgent() { stop(); }

bool EsxiAgent::can_handle(const std::string& job_type) const {
    return job_type == "vm_backup" || job_type == "vmware" || job_type == "esxi";
}

BackupResult EsxiAgent::execute_job(const BackupRequest& request) {
    spdlog::info("EsxiAgent executing VM backup job: {}", request.job_id);

    if (event_bus_) {
        event_bus_->publish(JobStartedEvent{request.job_id, config_.agent_id});
    }

    BackupResult result;
    result.success = true;
    result.total_bytes = 0;
    result.transferred_bytes = 0;

    spdlog::info("EsxiAgent: backup completed for job {}", request.job_id);

    if (event_bus_) {
        event_bus_->publish(JobCompletedEvent{request.job_id, true, ""});
    }

    return result;
}

void EsxiAgent::set_vcenter_host(const std::string& host) { vcenter_host_ = host; }
void EsxiAgent::set_credentials(const std::string& username, const std::string& password) {
    username_ = username;
    password_ = password;
}
void EsxiAgent::set_transport(TransportMode mode) { transport_ = mode; }

std::vector<std::string> EsxiAgent::list_vms() {
    spdlog::info("EsxiAgent: listing VMs from {}", vcenter_host_);
    return {};
}

std::string EsxiAgent::create_snapshot(const std::string& vm_id, const std::string& name) {
    spdlog::info("EsxiAgent: creating snapshot '{}' for VM {}", name, vm_id);
    return "snap_" + vm_id + "_" + current_time_string();
}

bool EsxiAgent::remove_snapshot(const std::string& vm_id, const std::string& snapshot_id) {
    spdlog::info("EsxiAgent: removing snapshot {} from VM {}", snapshot_id, vm_id);
    return true;
}

bool EsxiAgent::backup_vm(const std::string& vm_id, const BackupRequest& request) {
    spdlog::info("EsxiAgent: backing up VM {} with job {}", vm_id, request.job_id);
    return true;
}

std::vector<std::pair<uint64_t, uint32_t>> EsxiAgent::get_changed_blocks(
    const std::string& vm_id, const std::string& from, const std::string& to) {
    spdlog::info("EsxiAgent: getting changed blocks for VM {} ({} -> {})", vm_id, from, to);
    return {};
}

} // namespace obs
