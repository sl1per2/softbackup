#pragma once
#include "agent/generic_agent.h"

namespace obs {

class EsxiAgent : public GenericAgent {
public:
    EsxiAgent();
    ~EsxiAgent() override;

    AgentType type() const override { return AgentType::ESXi; }
    std::string type_name() const override { return "esxi"; }
    std::string component_name() const override { return "EsxiAgent"; }

    bool can_handle(const std::string& job_type) const override;
    BackupResult execute_job(const BackupRequest& request) override;

    void set_vcenter_host(const std::string& host);
    void set_credentials(const std::string& username, const std::string& password);
    void set_transport(TransportMode mode);

    std::vector<std::string> list_vms();
    std::string create_snapshot(const std::string& vm_id, const std::string& name);
    bool remove_snapshot(const std::string& vm_id, const std::string& snapshot_id);
    bool backup_vm(const std::string& vm_id, const BackupRequest& request);
    std::vector<std::pair<uint64_t, uint32_t>> get_changed_blocks(
        const std::string& vm_id, const std::string& from, const std::string& to);

private:
    std::string vcenter_host_;
    std::string username_;
    std::string password_;
    TransportMode transport_ = TransportMode::AUTO;
    std::string session_token_;
};

} // namespace obs
