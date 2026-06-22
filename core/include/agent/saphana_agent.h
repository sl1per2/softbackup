#pragma once
#include "agent/generic_agent.h"
#include "dbms/mongodb_1c_adapters.h"

namespace obs {

class SapHanaAgent : public GenericAgent {
public:
    SapHanaAgent();
    ~SapHanaAgent() override;

    AgentType type() const override { return AgentType::SAPHANA; }
    std::string type_name() const override { return "saphana"; }
    std::string component_name() const override { return "SapHanaAgent"; }

    bool can_handle(const std::string& job_type) const override;
    BackupResult execute_job(const BackupRequest& request) override;

    void set_connection(const DbConnection& conn);
    void set_system_user(const std::string& user, const std::string& password);
    BackupResult backup_full(const std::string& output);
    BackupResult backup_incremental(const std::string& output);
    BackupResult backup_log(const std::string& output);
    bool restore(const std::string& backup_id);
    bool test_connection();

private:
    DbConnection connection_;
    std::string system_user_;
    std::string system_password_;
};

} // namespace obs
