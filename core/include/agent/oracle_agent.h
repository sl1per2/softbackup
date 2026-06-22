#pragma once
#include "agent/generic_agent.h"
#include "dbms/oracle_adapter.h"

namespace obs {

class OracleAgent : public GenericAgent {
public:
    OracleAgent();
    ~OracleAgent() override;

    AgentType type() const override { return AgentType::ORACLE; }
    std::string type_name() const override { return "oracle"; }
    std::string component_name() const override { return "OracleAgent"; }

    bool can_handle(const std::string& job_type) const override;
    BackupResult execute_job(const BackupRequest& request) override;

    void set_connection(const DbConnection& conn);
    void set_oracle_home(const std::string& home);
    BackupResult backup_full(const std::string& output);
    BackupResult backup_incremental(const std::string& output);
    BackupResult backup_archivelog(const std::string& output);
    bool restore_to_scn(uint64_t scn);
    bool restore_to_time(const std::string& time);
    bool enable_bct();
    bool test_connection();
    std::vector<std::string> list_databases();

private:
    DbConnection connection_;
    std::shared_ptr<OracleAdapter> adapter_;
    std::string oracle_home_;
};

} // namespace obs
