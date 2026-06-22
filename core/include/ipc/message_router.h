#pragma once
#include "common.h"
#include "ipc/i_ipc_server.h"
#include "agent/i_agent.h"
#include "engine/i_cdp_engine.h"
#include "engine/i_backup_engine.h"
#include "framework/event_bus.h"

namespace obs {

class MessageRouter {
public:
    MessageRouter(
        std::shared_ptr<IIpcServer> ipc_server,
        std::shared_ptr<EventBus> event_bus
    );

    void initialize();
    void register_agent(std::shared_ptr<IAgent> agent);
    void register_cdp_engine(std::shared_ptr<ICdpEngine> engine);
    void register_backup_engine(std::shared_ptr<IBackupEngine> engine);

    IpcMessage handle_start_job(const IpcMessage& msg);
    IpcMessage handle_stop_job(const IpcMessage& msg);
    IpcMessage handle_get_status(const IpcMessage& msg);
    IpcMessage handle_get_metrics(const IpcMessage& msg);
    IpcMessage handle_restore(const IpcMessage& msg);
    IpcMessage handle_cdp_start(const IpcMessage& msg);
    IpcMessage handle_cdp_stop(const IpcMessage& msg);
    IpcMessage handle_cdp_status(const IpcMessage& msg);

private:
    std::shared_ptr<IIpcServer> ipc_server_;
    std::shared_ptr<EventBus> event_bus_;
    std::vector<std::shared_ptr<IAgent>> agents_;
    std::shared_ptr<ICdpEngine> cdp_engine_;
    std::shared_ptr<IBackupEngine> backup_engine_;
    std::atomic<bool> running_{false};

    std::shared_ptr<IAgent> find_agent_for_job(const std::string& job_type);
};

} // namespace obs
