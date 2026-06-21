#include "agent/agent.h"
#include "common.h"
#include <boost/asio.hpp>
#include <spdlog/spdlog.h>
#include <curl/curl.h>
#include <fstream>
#include <sstream>

namespace obs {

static size_t write_callback(void* contents, size_t size, size_t nmemb, std::string* s) {
    s->append(static_cast<char*>(contents), size * nmemb);
    return size * nmemb;
}

Agent::Agent() {
    info_.os_type =
#ifdef __linux__
        "linux";
#elif defined(_WIN32)
        "windows";
#else
        "unknown";
#endif
    info_.core_version = "1.0.0";
    info_.version = "1.0.0";

    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) == 0) {
        info_.hostname = hostname;
    }
}

Agent::~Agent() { stop(); }

void Agent::start(const std::string& server_url, const std::string& socket_path) {
    server_url_ = server_url;
    socket_path_ = socket_path;
    running_ = true;

    if (!register_with_server()) {
        spdlog::warn("Failed to register with server, will retry in heartbeat loop");
    }

    heartbeat_thread_ = std::thread(&Agent::heartbeat_loop, this);
    spdlog::info("Agent started, server={}", server_url_);
}

void Agent::stop() {
    running_ = false;
    if (heartbeat_thread_.joinable()) {
        heartbeat_thread_.join();
    }
}

bool Agent::register_with_server() {
    std::ostringstream json;
    json << "{\"hostname\":\"" << info_.hostname
         << "\",\"os_type\":\"" << info_.os_type
         << "\",\"version\":\"" << info_.version
         << "\",\"core_version\":\"" << info_.core_version << "\"}";

    std::string response = do_http_post(server_url_ + "/api/agent/register", json.str());
    if (response.empty()) return false;
    spdlog::info("Registered with server: {}", response);
    return true;
}

void Agent::heartbeat_loop() {
    while (running_) {
        std::ostringstream json;
        json << "{\"hostname\":\"" << info_.hostname << "\"}";
        do_http_post(server_url_ + "/api/agent/heartbeat", json.str());
        fetch_tasks();
        std::this_thread::sleep_for(std::chrono::seconds(30));
    }
}

void Agent::fetch_tasks() {
    // GET /api/agent/tasks would be implemented with curl GET
}

std::string Agent::do_http_post(const std::string& url, const std::string& body) {
    CURL* curl = curl_easy_init();
    if (!curl) return "";

    std::string response;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, body.size());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        spdlog::error("HTTP POST failed for {}: {}", url, curl_easy_strerror(res));
        return "";
    }
    return response;
}

AgentInfo Agent::get_info() const { return info_; }

} // namespace obs
