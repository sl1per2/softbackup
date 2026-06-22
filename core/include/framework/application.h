#pragma once
#include "common.h"
#include "framework/service_registry.h"
#include "framework/event_bus.h"
#include "framework/command.h"
#include "agent/i_agent.h"
#include "agent/agent_factory.h"
#include "engine/backup_pipeline.h"
#include "engine/engine_factory.h"
#include "ipc/i_ipc_server.h"
#include "ipc/message_router.h"
#include "ipc/unix_socket_server.h"
#ifdef _WIN32
#include "ipc/named_pipe_server.h"
#endif
#include "cdp/cdp_engine.h"
#include <boost/asio.hpp>
#include <memory>
#include <atomic>

namespace obs {

struct ApplicationConfig {
    std::string config_file = "/etc/obs/core.conf";
    std::string socket_path = "/tmp/obs-core.sock";
    std::string log_level = "info";
    std::string log_dir = "/var/log/obs";
    std::string data_dir = "/var/lib/obs";
    std::string cache_dir = "/var/cache/obs";
    std::string server_url = "http://localhost:8000";
    std::string agent_type = "generic";
    bool daemon = false;
};

class Application {
public:
    Application();
    ~Application();

    void initialize(const ApplicationConfig& config);
    void run();
    void shutdown();

    std::shared_ptr<ServiceRegistry> services() const { return service_registry_; }
    std::shared_ptr<EventBus> event_bus() const { return event_bus_; }
    std::shared_ptr<IAgent> agent() const { return agent_; }
    std::shared_ptr<BackupPipeline> pipeline() const { return pipeline_; }

    bool is_running() const { return running_.load(); }

private:
    void setup_logging(const std::string& log_level, const std::string& log_dir);
    void setup_signal_handlers();
    void initialize_subsystems();
    void initialize_agents();
    void initialize_ipc();
    void event_loop();

    ApplicationConfig config_;
    std::atomic<bool> running_{false};

    std::shared_ptr<ServiceRegistry> service_registry_;
    std::shared_ptr<EventBus> event_bus_;
    std::shared_ptr<CommandQueue> command_queue_;
    std::shared_ptr<BackupPipeline> pipeline_;
    std::shared_ptr<IAgent> agent_;
    std::shared_ptr<IIpcServer> ipc_server_;
    std::shared_ptr<MessageRouter> message_router_;
    std::shared_ptr<CdpEngine> cdp_engine_;

    boost::asio::io_context io_ctx_;
};

} // namespace obs
