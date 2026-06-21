#include "ha/ha_cluster.h"
#include <spdlog/spdlog.h>
#include <chrono>

namespace obs {

HaCluster::HaCluster() {}
HaCluster::~HaCluster() { stop(); }

bool HaCluster::configure(const HaConfig& config) {
    config_ = config;
    spdlog::info("HA Cluster: configured mode={}", config.mode);
    return true;
}

bool HaCluster::start() {
    running_ = true;
    health_thread_ = std::thread(&HaCluster::health_check_loop, this);
    spdlog::info("HA Cluster: started");
    return true;
}

void HaCluster::stop() {
    running_ = false;
    if (health_thread_.joinable()) health_thread_.join();
}

HaNodeStatus HaCluster::get_local_status() {
    std::lock_guard<std::mutex> lock(nodes_mutex_);
    return local_status_;
}

std::vector<HaNodeStatus> HaCluster::get_cluster_status() {
    std::lock_guard<std::mutex> lock(nodes_mutex_);
    return cluster_nodes_;
}

bool HaCluster::failover() {
    spdlog::info("HA Cluster: initiating failover");
    return true;
}

bool HaCluster::failback() {
    spdlog::info("HA Cluster: initiating failback");
    return true;
}

bool HaCluster::is_primary() {
    std::lock_guard<std::mutex> lock(nodes_mutex_);
    return local_status_.is_active;
}

void HaCluster::health_check_loop() {
    while (running_) {
        bool pg_ok = check_postgres_health();
        bool redis_ok = check_redis_health();
        bool backend_ok = check_backend_health();

        std::lock_guard<std::mutex> lock(nodes_mutex_);
        local_status_.is_healthy = pg_ok && redis_ok && backend_ok;
        local_status_.last_health_check = current_time_string();

        if (!local_status_.is_healthy && local_status_.is_active) {
            spdlog::error("HA: local node unhealthy, initiating failover");
            promote_standby();
        }

        std::this_thread::sleep_for(std::chrono::seconds(config_.health_check_interval));
    }
}

bool HaCluster::check_postgres_health() {
    std::string cmd = "pg_isready -h " + config_.postgres_host + " -p " + std::to_string(config_.postgres_port) + " 2>/dev/null";
    return system(cmd.c_str()) == 0;
}

bool HaCluster::check_redis_health() {
    std::string cmd = "redis-cli -h " + config_.redis_sentinel_host + " -p " + std::to_string(config_.redis_sentinel_port) + " ping 2>/dev/null";
    return system(cmd.c_str()) == 0;
}

bool HaCluster::check_backend_health() {
    return true;
}

void HaCluster::assign_roles() {
    local_status_.is_active = true;
    local_status_.role = "primary";
}

void HaCluster::promote_standby() {
    spdlog::error("HA: promoting standby node");
    local_status_.is_active = false;
    local_status_.role = "standby";
}

} // namespace obs
