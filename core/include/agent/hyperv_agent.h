#pragma once
#include "agent/generic_agent.h"

namespace obs {

class HypervAgent : public GenericAgent {
public:
    HypervAgent();
    ~HypervAgent() override;

    AgentType type() const override { return AgentType::HYPERV; }
    std::string type_name() const override { return "hyperv"; }
    std::string component_name() const override { return "HypervAgent"; }

    bool can_handle(const std::string& job_type) const override;
    BackupResult execute_job(const BackupRequest& request) override;

    void set_host(const std::string& host);
    void set_credentials(const std::string& username, const std::string& password);

    std::vector<std::string> list_vms();
    bool enable_rct(const std::string& vm_id);
    std::vector<std::pair<uint64_t, uint32_t>> get_rct_changed_blocks(
        const std::string& vm_id, const std::string& from, const std::string& to);
    bool backup_vm(const std::string& vm_id, const BackupRequest& request);
    bool create_vss_checkpoint(const std::string& vm_id);

private:
    std::string host_;
    std::string username_;
    std::string password_;
};

} // namespace obs
