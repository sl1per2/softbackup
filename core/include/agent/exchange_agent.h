#pragma once
#include "agent/generic_agent.h"
#include "mail/mail_adapter.h"

namespace obs {

class ExchangeAgent : public GenericAgent {
public:
    ExchangeAgent();
    ~ExchangeAgent() override;

    AgentType type() const override { return AgentType::EXCHANGE; }
    std::string type_name() const override { return "exchange"; }
    std::string component_name() const override { return "ExchangeAgent"; }

    bool can_handle(const std::string& job_type) const override;
    BackupResult execute_job(const BackupRequest& request) override;

    void set_exchange_host(const std::string& host);
    void set_credentials(const std::string& username, const std::string& password);
    void use_ews(bool enabled);
    void use_graph_api(bool enabled, const std::string& tenant_id = "", const std::string& client_id = "");

    bool test_connection();
    std::vector<std::string> list_mailboxes();
    BackupResult backup_mailbox(const std::string& mailbox, const std::string& output);
    BackupResult backup_all(const std::string& output);
    bool restore_mailbox(const std::string& mailbox, const std::string& backup_path);

private:
    std::string exchange_host_;
    std::string username_;
    std::string password_;
    bool use_ews_ = true;
    bool use_graph_api_ = false;
    std::string tenant_id_;
    std::string client_id_;
};

} // namespace obs
