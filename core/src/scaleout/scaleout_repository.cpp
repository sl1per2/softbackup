#include "scaleout/scaleout_repository.h"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <numeric>

namespace obs {

ScaleOutRepository::ScaleOutRepository() {}
ScaleOutRepository::~ScaleOutRepository() = default;

std::string ScaleOutRepository::create_repo(const ScaleOutRepo& repo) {
    std::lock_guard<std::mutex> lock(repos_mutex_);
    std::string id = "repo-" + std::to_string(std::time(nullptr));
    ScaleOutRepo r = repo;
    r.repo_id = id;
    repos_[id] = r;
    spdlog::info("Scale-Out Repo created: {} with {} extents", id, r.extents.size());
    return id;
}

bool ScaleOutRepository::update_repo(const std::string& repo_id, const ScaleOutRepo& repo) {
    std::lock_guard<std::mutex> lock(repos_mutex_);
    auto it = repos_.find(repo_id);
    if (it == repos_.end()) return false;
    it->second.name = repo.name;
    it->second.policy = repo.policy;
    it->second.encrypted = repo.encrypted;
    return true;
}

bool ScaleOutRepository::delete_repo(const std::string& repo_id) {
    std::lock_guard<std::mutex> lock(repos_mutex_);
    return repos_.erase(repo_id) > 0;
}

ScaleOutRepo ScaleOutRepository::get_repo(const std::string& repo_id) {
    std::lock_guard<std::mutex> lock(repos_mutex_);
    auto it = repos_.find(repo_id);
    return it != repos_.end() ? it->second : ScaleOutRepo{};
}

std::vector<ScaleOutRepo> ScaleOutRepository::list_repos() {
    std::vector<ScaleOutRepo> result;
    std::lock_guard<std::mutex> lock(repos_mutex_);
    for (auto& [id, repo] : repos_) result.push_back(repo);
    return result;
}

bool ScaleOutRepository::add_extent(const std::string& repo_id, const ScaleOutExtent& extent) {
    std::lock_guard<std::mutex> lock(repos_mutex_);
    auto it = repos_.find(repo_id);
    if (it == repos_.end()) return false;
    it->second.extents.push_back(extent);
    it->second.total_capacity += extent.total_bytes;
    spdlog::info("Scale-Out: added extent {} to repo {}", extent.extent_id, repo_id);
    return true;
}

bool ScaleOutRepository::remove_extent(const std::string& repo_id, const std::string& extent_id) {
    std::lock_guard<std::mutex> lock(repos_mutex_);
    auto it = repos_.find(repo_id);
    if (it == repos_.end()) return false;
    auto& extents = it->second.extents;
    for (auto eit = extents.begin(); eit != extents.end(); ++eit) {
        if (eit->extent_id == extent_id) {
            it->second.total_capacity -= eit->total_bytes;
            extents.erase(eit);
            return true;
        }
    }
    return false;
}

bool ScaleOutRepository::evacuate_extent(const std::string& repo_id, const std::string& extent_id,
                                           std::function<void(int)> progress_callback) {
    spdlog::info("Scale-Out: evacuating extent {} from repo {}", extent_id, repo_id);
    std::lock_guard<std::mutex> lock(repos_mutex_);
    auto it = repos_.find(repo_id);
    if (it == repos_.end()) return false;

    // Find another extent to move data to
    std::string target_extent;
    for (auto& ext : it->second.extents) {
        if (ext.extent_id != extent_id && ext.status == ExtentStatus::ONLINE) {
            target_extent = ext.extent_id;
            break;
        }
    }
    if (target_extent.empty()) return false;

    // Simulate evacuation
    for (int i = 0; i <= 100; i += 10) {
        if (progress_callback) progress_callback(i);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Mark as offline
    for (auto& ext : it->second.extents) {
        if (ext.extent_id == extent_id) {
            ext.status = ExtentStatus::OFFLINE;
            break;
        }
    }

    spdlog::info("Scale-Out: extent {} evacuated to {}", extent_id, target_extent);
    return true;
}

bool ScaleOutRepository::set_maintenance_mode(const std::string& repo_id, const std::string& extent_id, bool enable) {
    std::lock_guard<std::mutex> lock(repos_mutex_);
    auto it = repos_.find(repo_id);
    if (it == repos_.end()) return false;
    for (auto& ext : it->second.extents) {
        if (ext.extent_id == extent_id) {
            ext.status = enable ? ExtentStatus::MAINTENANCE : ExtentStatus::ONLINE;
            return true;
        }
    }
    return false;
}

ScaleOutWriteResult ScaleOutRepository::write(const std::string& repo_id, const std::string& key,
                                                const uint8_t* data, size_t size) {
    ScaleOutWriteResult result;
    std::lock_guard<std::mutex> lock(repos_mutex_);
    auto it = repos_.find(repo_id);
    if (it == repos_.end()) { result.error = "Repo not found"; return result; }

    std::string extent_id;
    switch (it->second.policy) {
        case PlacementPolicy::ROUND_ROBIN: extent_id = select_extent_round_robin(repo_id); break;
        case PlacementPolicy::DATA_LOCALITY: extent_id = select_extent_performance(repo_id, size); break;
        case PlacementPolicy::PERFORMANCE_TIER: extent_id = select_extent_performance(repo_id, size); break;
        case PlacementPolicy::CAPACITY_TIER: extent_id = select_extent_capacity(repo_id, size); break;
    }

    result.success = true;
    result.extent_id = extent_id;
    result.bytes_written = size;
    result.path = extent_id + "/" + key;
    return result;
}

std::vector<uint8_t> ScaleOutRepository::read(const std::string& repo_id, const std::string& key) {
    // In production: find which extent has the key and read from it
    return std::vector<uint8_t>();
}

bool ScaleOutRepository::delete_object(const std::string& repo_id, const std::string& key) {
    spdlog::info("Scale-Out: deleting {} from repo {}", key, repo_id);
    return true;
}

std::vector<ScaleOutExtent> ScaleOutRepository::get_extents(const std::string& repo_id) {
    std::lock_guard<std::mutex> lock(repos_mutex_);
    auto it = repos_.find(repo_id);
    return it != repos_.end() ? it->second.extents : std::vector<ScaleOutExtent>{};
}

ScaleOutExtent ScaleOutRepository::get_best_extent(const std::string& repo_id, uint64_t data_size) {
    std::lock_guard<std::mutex> lock(repos_mutex_);
    auto it = repos_.find(repo_id);
    if (it == repos_.end()) return {};

    for (auto& ext : it->second.extents) {
        if (ext.status == ExtentStatus::ONLINE && (ext.total_bytes - ext.used_bytes) >= data_size) {
            return ext;
        }
    }
    return {};
}

bool ScaleOutRepository::rebalance(const std::string& repo_id) {
    spdlog::info("Scale-Out: rebalancing repo {}", repo_id);
    return true;
}

std::string ScaleOutRepository::select_extent_round_robin(const std::string& repo_id) {
    auto it = repos_.find(repo_id);
    if (it == repos_.end()) return "";
    static size_t counter = 0;
    auto online = std::count_if(it->second.extents.begin(), it->second.extents.end(),
                                 [](const ScaleOutExtent& e) { return e.status == ExtentStatus::ONLINE; });
    if (online == 0) return "";
    size_t idx = counter++ % online;
    size_t current = 0;
    for (auto& ext : it->second.extents) {
        if (ext.status == ExtentStatus::ONLINE) {
            if (current == idx) return ext.extent_id;
            current++;
        }
    }
    return "";
}

std::string ScaleOutRepository::select_extent_performance(const std::string& repo_id, uint64_t data_size) {
    auto it = repos_.find(repo_id);
    if (it == repos_.end()) return "";
    for (auto& ext : it->second.extents) {
        if (ext.status == ExtentStatus::ONLINE && ext.is_performance_tier) return ext.extent_id;
    }
    return select_extent_round_robin(repo_id);
}

std::string ScaleOutRepository::select_extent_capacity(const std::string& repo_id, uint64_t data_size) {
    auto it = repos_.find(repo_id);
    if (it == repos_.end()) return "";
    ScaleOutExtent best;
    uint64_t best_free = 0;
    for (auto& ext : it->second.extents) {
        if (ext.status == ExtentStatus::ONLINE) {
            uint64_t free = ext.total_bytes - ext.used_bytes;
            if (free > best_free) { best_free = free; best = ext; }
        }
    }
    return best.extent_id;
}

bool ScaleOutRepository::evacuate_to_extent(const std::string& from_extent_id, const std::string& to_extent_id) {
    spdlog::info("Scale-Out: evacuating from {} to {}", from_extent_id, to_extent_id);
    return true;
}

} // namespace obs
