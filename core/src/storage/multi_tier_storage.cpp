#include "storage/multi_tier_storage.h"
#include <spdlog/spdlog.h>

namespace obs {

void MultiTierStorage::add_tier(const std::string& name, StorageTier tier) {
    tiers_[name] = std::move(tier);
    spdlog::info("MultiTierStorage: added tier '{}'", name);
}

void MultiTierStorage::add_policy(const TierPolicy& policy) {
    policies_.push_back(policy);
    spdlog::info("MultiTierStorage: added policy for tier '{}'", policy.tier_name);
}

void MultiTierStorage::evaluate_and_migrate() {
    spdlog::info("MultiTierStorage: evaluating {} policies", policies_.size());
    for (const auto& policy : policies_) {
        spdlog::debug("MultiTierStorage: checking policy for '{}'", policy.tier_name);
    }
}

} // namespace obs
