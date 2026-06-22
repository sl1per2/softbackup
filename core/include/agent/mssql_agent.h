#pragma once
#include "agent/generic_agent.h"
#include "dbms/mssql_adapter.h"

namespace obs {

class MssqlAgent : public GenericAgent {
public:
    MssqlAgent();
    ~MssqlAgent() override;

    AgentType type() const override { return AgentType::MSSQL; }
    std::string type_name() const override { return "mssql"; }
    std::string component_name() const override { return "MssqlAgent"; }

    bool can_handle(const std::string& job_type) const override;
    BackupResult execute_job(const BackupRequest& request) override;

    void set_connection(const DbConnection& conn);
    BackupResult backup_full(const std::string& database, const std::string& output);
    BackupResult backup_log(const std::string& database, const std::string& output);
    BackupResult backup_differential(const std::string& database, const std::string& output);
    bool restore(const std::string& database, const std::string& backup_path);
    std::vector<std::string> list_databases();
    bool test_connection();

private:
    DbConnection connection_;
    std::shared_ptr<MsSqlAdapter> adapter_;
};

} // namespace obs
