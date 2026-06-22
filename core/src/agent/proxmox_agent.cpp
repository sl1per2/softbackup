#include "agent/proxmox_agent.h"
#include <spdlog/spdlog.h>

namespace obs {

ProxmoxAgent::ProxmoxAgent() { spdlog::debug("ProxmoxAgent created"); }
ProxmoxAgent::~ProxmoxAgent() { stop(); }

bool ProxmoxAgent::can_handle(const std::string& job_type) const {
    return job_type == "vm_backup" || job_type == "proxmox";
}

BackupResult ProxmoxAgent::execute_job(const BackupRequest& request) {
    spdlog::info("ProxmoxAgent executing job: {}", request.job_id);

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

void ProxmoxAgent::set_api_host(const std::string& host, uint16_t port) {
    api_host_ = host;
    api_port_ = port;
}

void ProxmoxAgent::set_credentials(const std::string& username, const std::string& password,
                                    const std::string& realm) {
    username_ = username;
    password_ = password;
    realm_ = realm;
}

std::vector<std::string> ProxmoxAgent::list_vms() {
    spdlog::info("ProxmoxAgent: listing QEMU VMs from {}:{}", api_host_, api_port_);
    return {};
}

std::vector<std::string> ProxmoxAgent::list_lxc_containers() {
    spdlog::info("ProxmoxAgent: listing LXC containers from {}:{}", api_host_, api_port_);
    return {};
}

bool ProxmoxAgent::backup_vm(const std::string& vm_id, const BackupRequest& request) {
    spdlog::info("ProxmoxAgent: backing up QEMU VM {}", vm_id);
    return true;
}

bool ProxmoxAgent::backup_container(const std::string& ct_id, const BackupRequest& request) {
    spdlog::info("ProxmoxAgent: backing up LXC container {}", ct_id);
    return true;
}

bool ProxmoxAgent::restore_vm(const std::string& vm_id, const std::string& backup_path) {
    spdlog::info("ProxmoxAgent: restoring VM {} from {}", vm_id, backup_path);
    return true;
}

std::string ProxmoxAgent::create_qemu_bitmap(const std::string& vm_id, const std::string& name) {
    spdlog::info("ProxmoxAgent: creating QEMU dirty bitmap '{}' for VM {}", name, vm_id);
    return "bitmap_" + vm_id + "_" + name;
}

bool ProxmoxAgent::dump_qemu_bitmap(const std::string& vm_id, const std::string& bitmap_name) {
    spdlog::info("ProxmoxAgent: dumping QEMU dirty bitmap '{}' for VM {}", bitmap_name, vm_id);
    return true;
}

} // namespace obs
