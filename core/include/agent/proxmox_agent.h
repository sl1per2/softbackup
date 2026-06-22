#pragma once
#include "agent/generic_agent.h"
#include "incremental/proxmox/proxmox_backup.h"

namespace obs {

class ProxmoxAgent : public GenericAgent {
public:
    ProxmoxAgent();
    ~ProxmoxAgent() override;

    AgentType type() const override { return AgentType::PROXMOX; }
    std::string type_name() const override { return "proxmox"; }
    std::string component_name() const override { return "ProxmoxAgent"; }

    bool can_handle(const std::string& job_type) const override;
    BackupResult execute_job(const BackupRequest& request) override;

    void set_api_host(const std::string& host, uint16_t port = 8006);
    void set_credentials(const std::string& username, const std::string& password, const std::string& realm = "pam");

    std::vector<std::string> list_vms();
    std::vector<std::string> list_lxc_containers();
    bool backup_vm(const std::string& vm_id, const BackupRequest& request);
    bool backup_container(const std::string& ct_id, const BackupRequest& request);
    bool restore_vm(const std::string& vm_id, const std::string& backup_path);
    std::string create_qemu_bitmap(const std::string& vm_id, const std::string& name);
    bool dump_qemu_bitmap(const std::string& vm_id, const std::string& bitmap_name);

private:
    std::string api_host_;
    uint16_t api_port_ = 8006;
    std::string username_;
    std::string password_;
    std::string realm_;
    std::string api_token_;
};

} // namespace obs
