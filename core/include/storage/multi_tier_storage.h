#pragma once
#include "common.h"

namespace obs {

enum class StorageTier { HOT, WARM, COLD, ARCHIVE };

struct TierPolicy {
    StorageTier source_tier;
    StorageTier dest_tier;
    int days_after_creation = 30;
    bool auto_migrate = true;
};

struct StorageTierInfo {
    StorageTier tier;
    std::string name;
    std::string storage_path;
    uint64_t total_bytes = 0;
    uint64_t used_bytes = 0;
    int retention_days = 365;
    bool encrypted = false;
};

class MultiTierStorage {
public:
    void add_tier(const StorageTierInfo& tier);
    void add_policy(const TierPolicy& policy);
    StorageTierInfo get_tier_info(StorageTier tier);
    std::vector<StorageTierInfo> get_all_tiers();
    void evaluate_and_migrate();

private:
    std::string tier_to_path(StorageTier tier);
    bool migrate_file(const std::string& src, StorageTier src_tier, StorageTier dst_tier);

    std::vector<StorageTierInfo> tiers_;
    std::vector<TierPolicy> policies_;
};

} // namespace obs
