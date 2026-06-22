#include "ipc/message_router.h"
#include <spdlog/spdlog.h>

namespace obs {

MessageRouter::MessageRouter(
    std::shared_ptr<IIpcServer> ipc_server,
    std::shared_ptr<EventBus> event_bus)
    : ipc_server_(std::move(ipc_server))
    , event_bus_(std::move(event_bus)) {}

void MessageRouter::initialize() {
    running_ = true;

    ipc_server_->register_handler(1,
        [this](const IpcMessage& msg) { return handle_start_job(msg); });
    ipc_server_->register_handler(2,
        [this](const IpcMessage& msg) { return handle_stop_job(msg); });
    ipc_server_->register_handler(3,
        [this](const IpcMessage& msg) { return handle_get_status(msg); });
    ipc_server_->register_handler(4,
        [this](const IpcMessage& msg) { return handle_get_metrics(msg); });
    ipc_server_->register_handler(5,
        [this](const IpcMessage& msg) { return handle_restore(msg); });
    ipc_server_->register_handler(6,
        [this](const IpcMessage& msg) { return handle_cdp_start(msg); });
    ipc_server_->register_handler(7,
        [this](const IpcMessage& msg) { return handle_cdp_stop(msg); });
    ipc_server_->register_handler(8,
        [this](const IpcMessage& msg) { return handle_cdp_status(msg); });

    spdlog::info("MessageRouter: initialized with 8 handlers");
}

void MessageRouter::register_agent(std::shared_ptr<IAgent> agent) {
    agents_.push_back(std::move(agent));
    spdlog::info("MessageRouter: registered agent {}", agents_.back()->type_name());
}

void MessageRouter::register_cdp_engine(std::shared_ptr<ICdpEngine> engine) {
    cdp_engine_ = std::move(engine);
}

void MessageRouter::register_backup_engine(std::shared_ptr<IBackupEngine> engine) {
    backup_engine_ = std::move(engine);
}

IpcMessage MessageRouter::handle_start_job(const IpcMessage& msg) {
    spdlog::info("MessageRouter: handling START_JOB");
    IpcMessage response;
    response.type = 1;

    BackupRequest request;
    request.job_id = "job_" + current_time_string();
    request.source_path = "/tmp/source";
    request.target_path = "/tmp/backup";

    auto agent = find_agent_for_job("file_backup");
    if (agent) {
        agent->execute_job(request);
        response.payload = {1, 0, 0, 0}; // success
    } else {
        response.payload = {0, 0, 0, 0}; // no agent
    }

    return response;
}

IpcMessage MessageRouter::handle_stop_job(const IpcMessage& msg) {
    spdlog::info("MessageRouter: handling STOP_JOB");
    IpcMessage response;
    response.type = 2;
    response.payload = {1, 0, 0, 0};
    return response;
}

IpcMessage MessageRouter::handle_get_status(const IpcMessage& msg) {
    spdlog::debug("MessageRouter: handling GET_STATUS");
    IpcMessage response;
    response.type = 3;
    response.payload = {1, 0, 0, 0};
    return response;
}

IpcMessage MessageRouter::handle_get_metrics(const IpcMessage& msg) {
    spdlog::debug("MessageRouter: handling GET_METRICS");
    IpcMessage response;
    response.type = 4;
    response.payload = {1, 0, 0, 0};
    return response;
}

IpcMessage MessageRouter::handle_restore(const IpcMessage& msg) {
    spdlog::info("MessageRouter: handling RESTORE");
    IpcMessage response;
    response.type = 5;
    response.payload = {1, 0, 0, 0};
    return response;
}

IpcMessage MessageRouter::handle_cdp_start(const IpcMessage& msg) {
    spdlog::info("MessageRouter: handling CDP_START");
    IpcMessage response;
    response.type = 6;

    if (cdp_engine_) {
        CdpConfig config;
        config.session_id = "cdp_" + current_time_string();
        cdp_engine_->start_session(config);
        response.payload = {1, 0, 0, 0};
    } else {
        response.payload = {0, 0, 0, 0};
    }

    return response;
}

IpcMessage MessageRouter::handle_cdp_stop(const IpcMessage& msg) {
    spdlog::info("MessageRouter: handling CDP_STOP");
    IpcMessage response;
    response.type = 7;
    response.payload = {1, 0, 0, 0};
    return response;
}

IpcMessage MessageRouter::handle_cdp_status(const IpcMessage& msg) {
    spdlog::debug("MessageRouter: handling CDP_STATUS");
    IpcMessage response;
    response.type = 8;
    response.payload = {1, 0, 0, 0};
    return response;
}

std::shared_ptr<IAgent> MessageRouter::find_agent_for_job(const std::string& job_type) {
    for (auto& agent : agents_) {
        if (agent->can_handle(job_type)) {
            return agent;
        }
    }
    if (!agents_.empty()) return agents_.front();
    return nullptr;
}

} // namespace obs
