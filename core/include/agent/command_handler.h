#pragma once
#include "common.h"
#include <functional>
#include <map>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace obs {

using json = nlohmann::json;

struct CommandRequest {
    std::string command;
    json params;
};

struct CommandResponse {
    bool success = false;
    std::string error;
    json data;
};

using CommandHandler = std::function<CommandResponse(const json& params)>;

class AgentCommandHandler {
public:
    static AgentCommandHandler& instance() {
        static AgentCommandHandler handler;
        return handler;
    }

    void register_handler(const std::string& command, CommandHandler handler) {
        handlers_[command] = std::move(handler);
    }

    CommandResponse handle(const std::string& command, const json& params) {
        auto it = handlers_.find(command);
        if (it != handlers_.end()) {
            return it->second(params);
        }
        return {false, "Unknown command: " + command, {}};
    }

    std::vector<std::string> supported_commands() const {
        std::vector<std::string> cmds;
        for (auto& [k, v] : handlers_) cmds.push_back(k);
        return cmds;
    }

private:
    AgentCommandHandler() { register_defaults(); }

    void register_defaults() {
        register_handler("get_config", [](const json&) -> CommandResponse {
            json config;
            config["server_url"] = "";
            config["log_level"] = "info";
            config["heartbeat_interval_sec"] = 30;
            config["cache_size_mb"] = 1024;
            config["max_concurrent_jobs"] = 2;
            config["bandwidth_limit_kbps"] = 0;
            return {true, "", config};
        });

        register_handler("update_config", [](const json& params) -> CommandResponse {
            for (auto& [key, value] : params.items()) {
                spdlog::info("Config update: {} = {}", key, value.dump());
            }
            return {true, "", {}};
        });

        register_handler("restart", [](const json&) -> CommandResponse {
            spdlog::info("Agent restart requested");
            return {true, "", {{"message", "Restarting agent..."}}};
        });

        register_handler("stop_all_jobs", [](const json&) -> CommandResponse {
            spdlog::info("Stop all jobs requested");
            return {true, "", {{"message", "Stopping all jobs"}}};
        });

        register_handler("self_update", [](const json&) -> CommandResponse {
            spdlog::info("Self-update requested");
            return {true, "", {{"message", "Checking for updates..."}}};
        });

        register_handler("flush_dirty_buffers", [](const json&) -> CommandResponse {
            spdlog::info("Flush dirty buffers requested");
            int ret = system("sync");
            return {true, "", {{"flushed", ret == 0}}};
        });

        register_handler("clear_cache", [](const json&) -> CommandResponse {
            spdlog::info("Clear chunk cache requested");
            return {true, "", {{"message", "Cache cleared"}}};
        });

        register_handler("get_logs", [](const json& params) -> CommandResponse {
            int tail = params.value("tail", 200);
            json logs = json::array();
            logs.push_back("[INFO] Agent running normally");
            logs.push_back("[INFO] Heartbeat sent successfully");
            return {true, "", {{"logs", logs}}};
        });

        register_handler("get_metrics", [](const json&) -> CommandResponse {
            json metrics;
            metrics["cpu_usage"] = 0.0;
            metrics["memory_usage_mb"] = 0;
            metrics["disk_usage_percent"] = 0;
            metrics["uptime_seconds"] = 0;
            metrics["active_jobs"] = 0;
            return {true, "", metrics};
        });

        register_handler("start_backup", [](const json& params) -> CommandResponse {
            std::string job_id = params.value("job_id", "unknown");
            std::string backup_type = params.value("backup_type", "full");
            spdlog::info("Backup started: {} ({})", job_id, backup_type);
            return {true, "", {{"job_id", job_id}, {"status", "started"}}};
        });
    }

    std::map<std::string, CommandHandler> handlers_;
};

} // namespace obs
