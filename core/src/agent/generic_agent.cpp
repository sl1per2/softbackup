#include "agent/generic_agent.h"
#include <spdlog/spdlog.h>
#include <curl/curl.h>

namespace obs {

static size_t curl_write_callback(void* contents, size_t size, size_t nmemb, std::string* s) {
    size_t newsize = size * nmemb;
    s->append(static_cast<char*>(contents), newsize);
    return newsize;
}

GenericAgent::GenericAgent() {
    spdlog::debug("GenericAgent created");
}

GenericAgent::~GenericAgent() {
    stop();
}

void GenericAgent::start(const AgentConfig& config) {
    config_ = config;
    running_ = true;

    status_.agent_id = config.agent_id;
    status_.os_type =
#ifdef _WIN32
        "windows";
#elif __APPLE__
        "macos";
#else
        "linux";
#endif
    status_.version = "1.0.0";
    status_.is_connected = false;

    spdlog::info("Agent {} starting (type={})", config.agent_id, type_name());

    heartbeat_thread_ = std::thread(&GenericAgent::heartbeat_loop, this);
    mark_initialized();
}

void GenericAgent::stop() {
    if (!running_.exchange(false)) return;

    spdlog::info("Agent {} stopping", config_.agent_id);

    if (heartbeat_thread_.joinable()) {
        heartbeat_thread_.join();
    }

    if (event_bus_) {
        event_bus_->publish(AgentDisconnectedEvent{config_.agent_id});
    }
}

AgentStatus GenericAgent::get_status() const {
    std::lock_guard<std::mutex> lock(status_mutex_);
    return status_;
}

void GenericAgent::heartbeat() {
    spdlog::debug("Agent {} heartbeat", config_.agent_id);

    std::string body = R"({"agent_id":")" + config_.agent_id + R"(","status":"ok","timestamp":")" +
                       current_time_string() + R"("})";

    std::string response = http_post(config_.server_url + "/api/heartbeat", body);

    std::lock_guard<std::mutex> lock(status_mutex_);
    status_.last_heartbeat = current_time_string();
    status_.is_connected = !response.empty();
}

bool GenericAgent::can_handle(const std::string& job_type) const {
    return job_type == "file_backup" || job_type == "generic";
}

BackupResult GenericAgent::execute_job(const BackupRequest& request) {
    spdlog::info("Agent {} executing job {}", config_.agent_id, request.job_id);

    if (event_bus_) {
        event_bus_->publish(JobStartedEvent{request.job_id, config_.agent_id});
    }

    BackupResult result;
    if (pipeline_) {
        result = pipeline_->backup_engine()->backup(request,
            [this, &request](double progress, uint64_t bytes) {
                if (event_bus_) {
                    event_bus_->publish(JobProgressEvent{request.job_id, progress, bytes});
                }
            });
    }

    if (event_bus_) {
        event_bus_->publish(JobCompletedEvent{request.job_id, result.success, result.error});
    }

    {
        std::lock_guard<std::mutex> lock(status_mutex_);
        status_.active_jobs = std::max(0, status_.active_jobs - 1);
    }

    return result;
}

void GenericAgent::heartbeat_loop() {
    while (running_) {
        heartbeat();
        for (int i = 0; i < config_.heartbeat_interval_sec * 10 && running_; i++) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}

bool GenericAgent::register_with_server() {
    std::string body = R"({"agent_id":")" + config_.agent_id + R"(","version":")" + status_.version +
                       R"(","os":")" + status_.os_type + R"(","type":")" + type_name() + R"("})";
    return !http_post(config_.server_url + "/api/register", body).empty();
}

std::string GenericAgent::http_post(const std::string& url, const std::string& body) {
    CURL* curl = curl_easy_init();
    if (!curl) return "";

    std::string response;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    if (!config_.auth_token.empty()) {
        std::string auth_header = "Authorization: Bearer " + config_.auth_token;
        headers = curl_slist_append(headers, auth_header.c_str());
    }
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    return (res == CURLE_OK) ? response : "";
}

} // namespace obs
