#pragma once
#include "common.h"

namespace obs {

enum class TierType { HOT, WARM, COLD, ARCHIVE };

struct TierConfig {
    TierType type;
    std::string storage_path;
    uint64_t max_size_bytes;
    int min_access_days; // minimum days since last access for this tier
    bool encrypted = false;
    int compression_level = 1;
};

struct TierStats {
    TierType type;
    uint64_t total_bytes;
    uint64_t used_bytes;
    int32_t file_count;
    double utilization_percent;
};

class TierManager {
public:
    TierManager();
    ~TierManager();

    void add_tier(const TierConfig& config);
    void remove_tier(TierType type);
    std::vector<TierStats> get_stats() const;

    // Move data between tiers based on age
    void run_tiering(const std::string& source_path);

    // Get optimal tier for new data
    TierType select_tier(uint64_t data_size) const;

    // Migrate specific file to target tier
    bool migrate(const std::string& path, TierType target_tier);

private:
    TierConfig get_tier_config(TierType type) const;
    bool move_to_tier(const std::string& source, const TierConfig& tier);
    int get_days_since_access(const std::string& path);

    std::vector<TierConfig> tiers_;
    mutable std::mutex tiers_mutex_;
};

} // namespace obs
