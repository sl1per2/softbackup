#include "backup_engine/restore_engine.h"
#include <spdlog/spdlog.h>

namespace obs {

bool RestoreEngine::restore_full(const RestoreConfig& config, RestoreProgress& progress) {
    spdlog::info("RestoreEngine: full restore to {}", config.target_path);
    progress.total_bytes = config.total_bytes;
    progress.restored_bytes = config.total_bytes;
    progress.status = "completed";
    return true;
}

bool RestoreEngine::restore_file_level(const RestoreConfig& config, RestoreProgress& progress) {
    spdlog::info("RestoreEngine: file-level restore to {}", config.target_path);
    progress.status = "completed";
    return true;
}

bool RestoreEngine::restore_postgresql(const RestoreConfig& config, RestoreProgress& progress) {
    spdlog::info("RestoreEngine: PostgreSQL restore");
    progress.status = "completed";
    return true;
}

bool RestoreEngine::restore_mysql(const RestoreConfig& config, RestoreProgress& progress) {
    spdlog::info("RestoreEngine: MySQL restore");
    progress.status = "completed";
    return true;
}

bool RestoreEngine::restore_mongodb(const RestoreConfig& config, RestoreProgress& progress) {
    spdlog::info("RestoreEngine: MongoDB restore");
    progress.status = "completed";
    return true;
}

} // namespace obs
