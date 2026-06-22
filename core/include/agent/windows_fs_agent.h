#pragma once
#include "agent/generic_agent.h"
#include "incremental/window/windows_fs_backup.h"

namespace obs {

class WindowsFsAgent : public GenericAgent {
public:
    WindowsFsAgent();
    ~WindowsFsAgent() override;

    AgentType type() const override { return AgentType::WINDOWS_FS; }
    std::string type_name() const override { return "windows_fs"; }
    std::string component_name() const override { return "WindowsFsAgent"; }

    bool can_handle(const std::string& job_type) const override;
    BackupResult execute_job(const BackupRequest& request) override;

    BackupResult backup_vss_bitmap(const std::string& volume, const std::string& output);
    BackupResult backup_usn_journal(const std::string& volume, const std::string& output);
    BackupResult backup_refs_cbt(const std::string& volume, const std::string& output);
    std::vector<std::string> detect_available_methods();
    bool create_vss_snapshot(const std::string& volume);

private:
    std::vector<std::string> available_methods_;
    void detect_methods();
};

} // namespace obs
