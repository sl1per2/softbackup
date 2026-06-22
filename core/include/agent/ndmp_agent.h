#pragma once
#include "agent/generic_agent.h"
#include "ndmp/ndmp_backup.h"

namespace obs {

class NdmpAgent : public GenericAgent {
public:
    NdmpAgent();
    ~NdmpAgent() override;

    AgentType type() const override { return AgentType::NDMP; }
    std::string type_name() const override { return "ndmp"; }
    std::string component_name() const override { return "NdmpAgent"; }

    bool can_handle(const std::string& job_type) const override;
    BackupResult execute_job(const BackupRequest& request) override;

    void set_nas_host(const std::string& host, uint16_t port = 10000);
    void set_credentials(const std::string& username, const std::string& password);

    bool test_connection();
    std::vector<std::string> list_volumes();
    BackupResult backup_volume(const std::string& volume, const std::string& output);
    BackupResult backup_all(const std::string& output);
    bool restore_volume(const std::string& volume, const std::string& backup_path);

private:
    std::string nas_host_;
    uint16_t nas_port_ = 10000;
    std::string username_;
    std::string password_;
};

} // namespace obs
