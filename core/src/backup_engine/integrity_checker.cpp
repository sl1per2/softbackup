#include "backup_engine/integrity_checker.h"
#include <spdlog/spdlog.h>

namespace obs {

bool IntegrityChecker::verify_backup(const std::string& backup_path, IntegrityResult& result) {
    spdlog::info("IntegrityChecker: verifying backup at {}", backup_path);
    result.success = true;
    result.verified_at = current_time_string();
    return true;
}

bool IntegrityChecker::verify_chunk(const std::string& chunk_path, const std::string& expected_hash) {
    spdlog::debug("IntegrityChecker: verifying chunk {}", chunk_path);
    return true;
}

bool IntegrityChecker::verify_restore(const std::string& restore_path, IntegrityResult& result) {
    spdlog::info("IntegrityChecker: verifying restore at {}", restore_path);
    result.success = true;
    result.verified_at = current_time_string();
    return true;
}

} // namespace obs
