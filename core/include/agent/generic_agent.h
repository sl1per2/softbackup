#pragma once
#include "agent/i_agent.h"
#include <thread>
#include <atomic>

namespace obs {

class GenericAgent : public IAgent {
public:
    GenericAgent();
    ~GenericAgent() override;

    AgentType type() const override { return AgentType::GENERIC; }
    std::string type_name() const override { return "generic"; }
    std::string component_name() const override { return "GenericAgent"; }

    void start(const AgentConfig& config) override;
    void stop() override;
    bool is_running() const override { return running_.load(); }

    AgentStatus get_status() const override;
    void heartbeat() override;

    bool can_handle(const std::string& job_type) const override;
    BackupResult execute_job(const BackupRequest& request) override;

protected:
    void heartbeat_loop();
    bool register_with_server();
    std::string http_post(const std::string& url, const std::string& body);

    std::atomic<bool> running_{false};
    std::thread heartbeat_thread_;
    mutable std::mutex status_mutex_;
    AgentStatus status_;
};

} // namespace obs
