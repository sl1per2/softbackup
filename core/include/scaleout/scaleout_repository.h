#pragma once
#include "common.h"
#include <functional>
#include <vector>
#include <string>

namespace obs {

enum class PlacementPolicy { ROUND_ROBIN, DATA_LOCALITY, PERFORMANCE_TIER, CAPACITY_TIER };
enum class ExtentStatus { ONLINE, OFFLINE, MAINTENANCE, EVACUATING };

struct ScaleOutExtent {
    std::string extent_id;
    std::string storage_id;
    std::string name;
    uint64_t total_bytes = 0;
    uint64_t used_bytes = 0;
    ExtentStatus status = ExtentStatus::ONLINE;
    bool is_performance_tier = false;
    std::string added_at;
};

struct ScaleOutRepo {
    std::string repo_id;
    std::string name;
    PlacementPolicy policy = PlacementPolicy::ROUND_ROBIN;
    std::vector<ScaleOutExtent> extents;
    bool encrypted = false;
    std::string encryption_key_id;
    uint64_t total_capacity = 0;
    uint64_t used_capacity = 0;
};

struct ScaleOutWriteResult {
    bool success = false;
    std::string extent_id;
    std::string path;
    uint64_t bytes_written = 0;
    std::string error;
};

class ScaleOutRepository {
public:
    ScaleOutRepository();
    ~ScaleOutRepository();

    std::string create_repo(const ScaleOutRepo& repo);
    bool update_repo(const std::string& repo_id, const ScaleOutRepo& repo);
    bool delete_repo(const std::string& repo_id);
    ScaleOutRepo get_repo(const std::string& repo_id);
    std::vector<ScaleOutRepo> list_repos();

    bool add_extent(const std::string& repo_id, const ScaleOutExtent& extent);
    bool remove_extent(const std::string& repo_id, const std::string& extent_id);
    bool evacuate_extent(const std::string& repo_id, const std::string& extent_id,
                        std::function<void(int)> progress_callback = nullptr);
    bool set_maintenance_mode(const std::string& repo_id, const std::string& extent_id, bool enable);

    ScaleOutWriteResult write(const std::string& repo_id, const std::string& key,
                             const uint8_t* data, size_t size);
    std::vector<uint8_t> read(const std::string& repo_id, const std::string& key);
    bool delete_object(const std::string& repo_id, const std::string& key);

    std::vector<ScaleOutExtent> get_extents(const std::string& repo_id);
    ScaleOutExtent get_best_extent(const std::string& repo_id, uint64_t data_size);
    bool rebalance(const std::string& repo_id);

private:
    std::string select_extent_round_robin(const std::string& repo_id);
    std::string select_extent_performance(const std::string& repo_id, uint64_t data_size);
    std::string select_extent_capacity(const std::string& repo_id, uint64_t data_size);
    bool evacuate_to_extent(const std::string& from_extent_id, const std::string& to_extent_id);

    std::map<std::string, ScaleOutRepo> repos_;
    mutable std::mutex repos_mutex_;
};

} // namespace obs
