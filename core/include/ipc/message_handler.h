#pragma once
#include "common.h"
#include <unordered_map>
#include <functional>

namespace obs {

class BackupJob;
class CdpEngine;

using IpcHandler = std::function<std::vector<uint8_t>(const std::vector<uint8_t>&)>;

class MessageHandler {
public:
    MessageHandler();
    void set_backup_job_factory(std::function<std::shared_ptr<BackupJob>()> factory);
    void set_cdp_engine(CdpEngine* engine);
    std::vector<uint8_t> handle(uint32_t type, const std::vector<uint8_t>& data);

private:
    std::vector<uint8_t> handle_start_job(const std::vector<uint8_t>& data);
    std::vector<uint8_t> handle_stop_job(const std::vector<uint8_t>& data);
    std::vector<uint8_t> handle_get_status(const std::vector<uint8_t>& data);
    std::vector<uint8_t> handle_get_metrics(const std::vector<uint8_t>& data);
    std::vector<uint8_t> handle_restore(const std::vector<uint8_t>& data);

    std::unordered_map<uint32_t, IpcHandler> handlers_;
    std::function<std::shared_ptr<BackupJob>()> job_factory_;
    CdpEngine* cdp_engine_ = nullptr;
    std::map<std::string, std::shared_ptr<BackupJob>> active_jobs_;
    mutable std::mutex jobs_mutex_;
};

} // namespace obs
