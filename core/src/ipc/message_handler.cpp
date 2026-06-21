#include "ipc/message_handler.h"
#include "backup_engine/backup_job.h"
#include "cdp/cdp_engine.h"
#include <spdlog/spdlog.h>

namespace obs {

MessageHandler::MessageHandler() {
    handlers_[1] = [this](const std::vector<uint8_t>& d) { return handle_start_job(d); };
    handlers_[2] = [this](const std::vector<uint8_t>& d) { return handle_stop_job(d); };
    handlers_[3] = [this](const std::vector<uint8_t>& d) { return handle_get_status(d); };
    handlers_[4] = [this](const std::vector<uint8_t>& d) { return handle_get_metrics(d); };
    handlers_[5] = [this](const std::vector<uint8_t>& d) { return handle_restore(d); };
}

void MessageHandler::set_backup_job_factory(std::function<std::shared_ptr<BackupJob>()> factory) {
    job_factory_ = std::move(factory);
}

void MessageHandler::set_cdp_engine(CdpEngine* engine) {
    cdp_engine_ = engine;
}

std::vector<uint8_t> MessageHandler::handle(uint32_t type, const std::vector<uint8_t>& data) {
    auto it = handlers_.find(type);
    if (it != handlers_.end()) {
        return it->second(data);
    }
    return {};
}

std::vector<uint8_t> MessageHandler::handle_start_job(const std::vector<uint8_t>& data) {
    spdlog::info("IPC: START_JOB received");
    // In production: parse protobuf, create BackupJob from config, start it
    if (job_factory_) {
        auto job = job_factory_();
        if (job) {
            std::lock_guard<std::mutex> lock(jobs_mutex_);
            active_jobs_[job->get_metrics().transport_mode_used.empty() ? "new" : "job"] = job;
            job->run();
        }
    }
    return {0, 0, 0, 0}; // OK response
}

std::vector<uint8_t> MessageHandler::handle_stop_job(const std::vector<uint8_t>& data) {
    spdlog::info("IPC: STOP_JOB received");
    std::lock_guard<std::mutex> lock(jobs_mutex_);
    for (auto& [id, job] : active_jobs_) {
        if (job->get_status() == JobStatus::RUNNING) {
            job->cancel();
        }
    }
    return {0, 0, 0, 0};
}

std::vector<uint8_t> MessageHandler::handle_get_status(const std::vector<uint8_t>& data) {
    spdlog::info("IPC: GET_STATUS received");
    // Return status of requested job
    return {0, 0, 0, 0};
}

std::vector<uint8_t> MessageHandler::handle_get_metrics(const std::vector<uint8_t>& data) {
    spdlog::info("IPC: GET_METRICS received");
    // Return system metrics
    return {0, 0, 0, 0};
}

std::vector<uint8_t> MessageHandler::handle_restore(const std::vector<uint8_t>& data) {
    spdlog::info("IPC: RESTORE received");
    return {0, 0, 0, 0};
}

} // namespace obs
