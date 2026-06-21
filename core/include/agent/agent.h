#pragma once
#include <string>
#include <atomic>
#include <thread>
#include <vector>
#include <functional>

namespace obs {

struct AgentInfo {
    std::string hostname;
    std::string ip;
    std::string os_type;
    std::string version;
    std::string core_version;
};

class Agent {
public:
    Agent();
    ~Agent();

    void start(const std::string& server_url, const std::string& socket_path);
    void stop();
    bool is_running() const { return running_.load(); }
    AgentInfo get_info() const;

private:
    void heartbeat_loop();
    void fetch_tasks();
    bool register_with_server();
    std::string do_http_post(const std::string& url, const std::string& body);

    std::string server_url_;
    std::string socket_path_;
    std::atomic<bool> running_{false};
    std::thread heartbeat_thread_;
    AgentInfo info_;
};

} // namespace obs
