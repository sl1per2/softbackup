#include "incremental/common/bitmap_manager.h"
#include <spdlog/spdlog.h>

namespace vovqa::incremental {

// Chain Management
struct ChainBreakInfo {
    std::string broken_job_id;
    std::string missing_parent;
    std::string recovery_action; // "differential", "full"
};

class ChainManager {
public:
    ChainManager(BitmapManager& bm) : bitmap_manager_(bm) {}

    ChainBreakInfo check_chain(const std::string& current_job_id) {
        ChainBreakInfo info;
        std::string current = current_job_id;
        int depth = 0;

        while (!current.empty() && depth < 1000) {
            auto chain = bitmap_manager_.get_chain_info(current);
            if (chain.job_type == "full") {
                info.recovery_action = "ok";
                return info;
            }
            if (chain.parent_job_id.empty()) {
                // No parent but not full → broken
                info.broken_job_id = current;
                info.recovery_action = "full";
                spdlog::warn("Chain broken at {}: no parent, not full", current);
                return info;
            }
            // Verify parent exists
            auto parent = bitmap_manager_.get_chain_info(chain.parent_job_id);
            if (parent.job_type.empty()) {
                info.broken_job_id = current;
                info.missing_parent = chain.parent_job_id;
                info.recovery_action = "differential";
                spdlog::warn("Chain broken: parent {} missing for {}", chain.parent_job_id, current);
                return info;
            }
            current = chain.parent_job_id;
            depth++;
        }

        info.recovery_action = "ok";
        return info;
    }

    void handle_chain_break(const ChainBreakInfo& info) {
        if (info.recovery_action == "full") {
            spdlog::info("Chain recovery: scheduling forced full backup (broken at {})", info.broken_job_id);
        } else if (info.recovery_action == "differential") {
            spdlog::info("Chain recovery: switching to differential from last known good (missing {})", info.missing_parent);
        }
    }

    int get_chain_length(const std::string& job_id) {
        auto chain = bitmap_manager_.get_chain_info(job_id);
        return chain.chain_length;
    }

    bool should_force_full(const std::string& job_id, int max_chain_length = 7) {
        int length = get_chain_length(job_id);
        if (length >= max_chain_length) {
            spdlog::info("Chain length {} >= max {}, forcing full backup", length, max_chain_length);
            return true;
        }
        return false;
    }

private:
    BitmapManager& bitmap_manager_;
};

} // namespace vovqa::incremental
