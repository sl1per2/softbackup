#pragma once
#include "common.h"
#include "framework/component.h"
#include "framework/event_bus.h"
#include "engine/backup_pipeline.h"
#include <thread>
#include <atomic>

namespace obs {

enum class AgentType {
    GENERIC, ESXi, HYPERV, MSSQL, POSTGRESQL, ORACLE, PROXMOX,
    SAPHANA, KUBERNETES, LINUX_FS, WINDOWS_FS, NDMP, EXCHANGE
};

struct AgentConfig {
    std::string agent_id;
    std::string server_url;
    std::string auth_token;
    std::string socket_path;
    std::string data_dir = "/var/lib/obs";
    std::string log_dir = "/var/log/obs";
    std::string cache_dir = "/var/cache/obs";
    int heartbeat_interval_sec = 30;
    int max_concurrent_jobs = 2;
    int chunk_size = 256 * 1024;
    int bandwidth_limit_kbps = 0;
};

struct AgentStatus {
    std::string agent_id;
    std::string hostname;
    std::string ip_address;
    std::string os_type;
    std::string version;
    int active_jobs = 0;
    double cpu_usage = 0.0;
    double memory_usage_mb = 0.0;
    bool is_connected = false;
    std::string last_heartbeat;
};

class IAgent : public IComponent {
public:
    virtual ~IAgent() = default;

    virtual AgentType type() const = 0;
    virtual std::string type_name() const = 0;

    virtual void start(const AgentConfig& config) = 0;
    virtual void stop() = 0;
    virtual bool is_running() const = 0;

    virtual AgentStatus get_status() const = 0;
    virtual void heartbeat() = 0;

    virtual bool can_handle(const std::string& job_type) const = 0;
    virtual BackupResult execute_job(const BackupRequest& request) = 0;

    virtual void set_pipeline(std::shared_ptr<BackupPipeline> pipeline) { pipeline_ = pipeline; }
    virtual void set_event_bus(std::shared_ptr<EventBus> bus) { event_bus_ = bus; }

protected:
    std::shared_ptr<BackupPipeline> pipeline_;
    std::shared_ptr<EventBus> event_bus_;
    AgentConfig config_;
};

} // namespace obs
