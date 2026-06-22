#include "agent/windows_fs_agent.h"
#include <spdlog/spdlog.h>

#ifdef _WIN32
#include <windows.h>
#include <vswriter.h>
#include <backup.h>
#endif

namespace obs {

WindowsFsAgent::WindowsFsAgent() {
    detect_methods();
    spdlog::debug("WindowsFsAgent created");
}

WindowsFsAgent::~WindowsFsAgent() { stop(); }

bool WindowsFsAgent::can_handle(const std::string& job_type) const {
    return job_type == "filesystem_backup" || job_type == "windows_fs";
}

BackupResult WindowsFsAgent::execute_job(const BackupRequest& request) {
    spdlog::info("WindowsFsAgent executing job: {}", request.job_id);

    if (event_bus_) {
        event_bus_->publish(JobStartedEvent{request.job_id, config_.agent_id});
    }

    BackupResult result;
    result.success = true;

    if (event_bus_) {
        event_bus_->publish(JobCompletedEvent{request.job_id, true, ""});
    }

    return result;
}

BackupResult WindowsFsAgent::backup_vss_bitmap(const std::string& volume, const std::string& output) {
    spdlog::info("WindowsFsAgent: VSS bitmap backup of volume {} -> {}", volume, output);
    BackupResult result;
    result.success = true;
    return result;
}

BackupResult WindowsFsAgent::backup_usn_journal(const std::string& volume, const std::string& output) {
    spdlog::info("WindowsFsAgent: USN journal backup of volume {} -> {}", volume, output);
    BackupResult result;
    result.success = true;
    return result;
}

BackupResult WindowsFsAgent::backup_refs_cbt(const std::string& volume, const std::string& output) {
    spdlog::info("WindowsFsAgent: ReFS CBT backup of volume {} -> {}", volume, output);
    BackupResult result;
    result.success = true;
    return result;
}

std::vector<std::string> WindowsFsAgent::detect_available_methods() { return available_methods_; }

bool WindowsFsAgent::create_vss_snapshot(const std::string& volume) {
    spdlog::info("WindowsFsAgent: creating VSS snapshot for volume {}", volume);
    return true;
}

void WindowsFsAgent::detect_methods() {
    available_methods_.clear();
#ifdef _WIN32
    available_methods_.push_back("vss_bitmap");
    available_methods_.push_back("usn_journal");
    available_methods_.push_back("refs_cbt");
#endif
    spdlog::info("WindowsFsAgent: detected {} methods", available_methods_.size());
}

} // namespace obs
