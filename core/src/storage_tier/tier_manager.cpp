#include "storage_tier/tier_manager.h"
#include <spdlog/spdlog.h>
#include <sys/stat.h>
#include <algorithm>

namespace obs {

TierManager::TierManager() {}
TierManager::~TierManager() {}

void TierManager::add_tier(const TierConfig& config) {
    std::lock_guard<std::mutex> lock(tiers_mutex_);
    fs::create_directories(config.storage_path);
    tiers_.push_back(config);
    spdlog::info("Storage tier added: type={} path={}", static_cast<int>(config.type), config.storage_path);
}

void TierManager::remove_tier(TierType type) {
    std::lock_guard<std::mutex> lock(tiers_mutex_);
    tiers_.erase(std::remove_if(tiers_.begin(), tiers_.end(),
        [type](const TierConfig& t) { return t.type == type; }), tiers_.end());
}

std::vector<TierStats> TierManager::get_stats() const {
    std::vector<TierStats> stats;
    std::lock_guard<std::mutex> lock(tiers_mutex_);
    for (const auto& tier : tiers_) {
        TierStats s;
        s.type = tier.type;
        s.total_bytes = tier.max_size_bytes;
        s.used_bytes = 0;
        s.file_count = 0;
        if (fs::exists(tier.storage_path)) {
            for (auto& entry : fs::recursive_directory_iterator(tier.storage_path)) {
                if (entry.is_regular_file()) {
                    s.used_bytes += entry.file_size();
                    s.file_count++;
                }
            }
        }
        s.utilization_percent = s.total_bytes > 0 ? (double)s.used_bytes / s.total_bytes * 100 : 0;
        stats.push_back(s);
    }
    return stats;
}

void TierManager::run_tiering(const std::string& source_path) {
    std::lock_guard<std::mutex> lock(tiers_mutex_);
    spdlog::info("Running storage tiering from {}", source_path);

    for (auto& entry : fs::recursive_directory_iterator(source_path)) {
        if (!entry.is_regular_file()) continue;
        int days = get_days_since_access(entry.path().string());

        // Find appropriate tier
        for (const auto& tier : tiers_) {
            if (days >= tier.min_access_days) {
                move_to_tier(entry.path().string(), tier);
                break;
            }
        }
    }
}

TierType TierManager::select_tier(uint64_t data_size) const {
    std::lock_guard<std::mutex> lock(tiers_mutex_);
    // Hot tier for recent/small data, cold for old/large
    for (const auto& tier : tiers_) {
        if (tier.type == TierType::HOT) return TierType::HOT;
    }
    return TierType::WARM;
}

bool TierManager::migrate(const std::string& path, TierType target_tier) {
    std::lock_guard<std::mutex> lock(tiers_mutex_);
    for (const auto& tier : tiers_) {
        if (tier.type == target_tier) {
            return move_to_tier(path, tier);
        }
    }
    return false;
}

int TierManager::get_days_since_access(const std::string& path) {
    struct stat st;
    if (stat(path.c_str(), &st) != 0) return 0;
    auto now = std::chrono::system_clock::now();
    auto mtime = std::chrono::system_clock::from_time_t(st.st_mtime);
    return std::chrono::duration_cast<std::chrono::hours>(now - mtime).count() / 24;
}

bool TierManager::move_to_tier(const std::string& source, const TierConfig& tier) {
    fs::path src(source);
    fs::path dest_dir(tier.storage_path);
    fs::path dest = dest_dir / src.filename();

    if (fs::exists(dest)) return false;

    try {
        fs::create_directories(dest_dir);
        fs::rename(src, dest);
        spdlog::debug("Moved {} to tier {}", source, static_cast<int>(tier.type));
        return true;
    } catch (const std::exception& e) {
        spdlog::error("Failed to move {} to tier {}: {}", source, static_cast<int>(tier.type), e.what());
        return false;
    }
}

} // namespace obs
