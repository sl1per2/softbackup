#include "backup_engine/replication.h"
#include <spdlog/spdlog.h>

namespace obs {

void ReplicationEngine::start(const ReplicationConfig& config) {
    config_ = config;
    running_ = true;
    spdlog::info("ReplicationEngine: started for {} -> {}", config.source_storage, config.dest_storage);
}

void ReplicationEngine::stop() {
    running_ = false;
    spdlog::info("ReplicationEngine: stopped");
}

void ReplicationEngine::replication_loop() {
    while (running_) {
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
}

bool ReplicationEngine::replicate_chunk(const std::string& chunk_hash) {
    spdlog::debug("ReplicationEngine: replicating chunk {}", chunk_hash);
    return true;
}

} // namespace obs
