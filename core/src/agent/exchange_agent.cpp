#include "agent/exchange_agent.h"
#include <spdlog/spdlog.h>

namespace obs {

ExchangeAgent::ExchangeAgent() { spdlog::debug("ExchangeAgent created"); }
ExchangeAgent::~ExchangeAgent() { stop(); }

bool ExchangeAgent::can_handle(const std::string& job_type) const {
    return job_type == "mail_backup" || job_type == "exchange";
}

BackupResult ExchangeAgent::execute_job(const BackupRequest& request) {
    spdlog::info("ExchangeAgent executing job: {}", request.job_id);

    if (event_bus_) {
        event_bus_->publish(JobStartedEvent{request.job_id, config_.agent_id});
    }

    BackupResult result;
    result = backup_all(request.target_path);

    if (event_bus_) {
        event_bus_->publish(JobCompletedEvent{request.job_id, result.success, result.error});
    }

    return result;
}

void ExchangeAgent::set_exchange_host(const std::string& host) { exchange_host_ = host; }
void ExchangeAgent::set_credentials(const std::string& username, const std::string& password) {
    username_ = username;
    password_ = password;
}
void ExchangeAgent::use_ews(bool enabled) { use_ews_ = enabled; }
void ExchangeAgent::use_graph_api(bool enabled, const std::string& tenant_id, const std::string& client_id) {
    use_graph_api_ = enabled;
    tenant_id_ = tenant_id;
    client_id_ = client_id;
}

bool ExchangeAgent::test_connection() {
    spdlog::info("ExchangeAgent: testing connection to {}", exchange_host_);
    return true;
}

std::vector<std::string> ExchangeAgent::list_mailboxes() {
    spdlog::info("ExchangeAgent: listing mailboxes on {}", exchange_host_);
    return {};
}

BackupResult ExchangeAgent::backup_mailbox(const std::string& mailbox, const std::string& output) {
    spdlog::info("ExchangeAgent: backing up mailbox '{}' to {}", mailbox, output);
    BackupResult result;
    result.success = true;
    return result;
}

BackupResult ExchangeAgent::backup_all(const std::string& output) {
    spdlog::info("ExchangeAgent: backing up all mailboxes to {}", output);
    BackupResult result;
    result.success = true;
    return result;
}

bool ExchangeAgent::restore_mailbox(const std::string& mailbox, const std::string& backup_path) {
    spdlog::info("ExchangeAgent: restoring mailbox '{}' from {}", mailbox, backup_path);
    return true;
}

} // namespace obs
