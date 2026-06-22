#pragma once
#include "agent/generic_agent.h"
#include "dbms/postgres_adapter.h"

namespace obs {

class PostgresqlAgent : public GenericAgent {
public:
    PostgresqlAgent();
    ~PostgresqlAgent() override;

    AgentType type() const override { return AgentType::POSTGRESQL; }
    std::string type_name() const override { return "postgresql"; }
    std::string component_name() const override { return "PostgresqlAgent"; }

    bool can_handle(const std::string& job_type) const override;
    BackupResult execute_job(const BackupRequest& request) override;

    void set_connection(const DbConnection& conn);
    BackupResult backup_full(const std::string& output);
    BackupResult backup_wal(const std::string& output);
    bool restore(const std::string& backup_path, const std::string& target_time = "");
    bool archive_wal(const std::string& wal_path);
    std::vector<std::string> list_databases();
    bool test_connection();
    bool enable_continuous_archiving(const std::string& archive_dir);

private:
    DbConnection connection_;
    std::shared_ptr<PostgresAdapter> adapter_;
    bool continuous_archiving_ = false;
};

} // namespace obs
