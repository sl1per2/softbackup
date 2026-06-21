#pragma once
#include "common.h"
#include <string>
#include <vector>
#include <map>
#include <thread>

namespace obs {

struct HaConfig {
    bool enabled = false;
    std::string mode = "active-passive"; // active-passive or active-active
    std::string virtual_ip;
    std::string postgres_host;
    int postgres_port = 5432;
    std::string postgres_standby_host;
    std::string redis_sentinel_host;
    int redis_sentinel_port = 26379;
    int health_check_interval = 10;
    int failover_timeout = 60;
};

struct HaNodeStatus {
    std::string node_id;
    std::string ip;
    bool is_active = false;
    bool is_healthy = false;
    std::string last_health_check;
    std::string role; // primary, standby
};

class HaCluster {
public:
    HaCluster();
    ~HaCluster();

    bool configure(const HaConfig& config);
    bool start();
    void stop();

    HaNodeStatus get_local_status();
    std::vector<HaNodeStatus> get_cluster_status();
    bool failover();
    bool failback();
    bool is_primary();

private:
    void health_check_loop();
    bool check_postgres_health();
    bool check_redis_health();
    bool check_backend_health();
    void assign_roles();
    void promote_standby();

    HaConfig config_;
    HaNodeStatus local_status_;
    std::vector<HaNodeStatus> cluster_nodes_;
    std::atomic<bool> running_{false};
    std::thread health_thread_;
    mutable std::mutex nodes_mutex_;
};

} // namespace obs
