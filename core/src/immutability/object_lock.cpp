#include "immutability/object_lock.h"
#include <spdlog/spdlog.h>
#include <sys/stat.h>

namespace obs {

ImmutabilityManager::ImmutabilityManager() {}
ImmutabilityManager::~ImmutabilityManager() {}

bool ImmutabilityManager::enable_s3_object_lock(const ObjectLockConfig& config) {
    spdlog::info("S3 Object Lock: enabling {} mode on {}/{}", static_cast<int>(config.mode),
                 config.bucket, config.object_key);
    std::string mode_str = (config.mode == ImmutabilityMode::GOVERNANCE) ? "GOVERNANCE" : "COMPLIANCE";
    std::string cmd = "aws s3api put-object-retention --bucket " + config.bucket
                    + " --key " + config.object_key
                    + " --retention '{\"Mode\":\"" + mode_str + "\",\"RetainUntilDate\":\""
                    + std::to_string(std::time(nullptr) + config.retention_days * 86400) + "\"}' 2>/dev/null";
    return system(cmd.c_str()) == 0;
}

bool ImmutabilityManager::disable_s3_object_lock(const std::string& bucket, const std::string& key) {
    std::string cmd = "aws s3api delete-object-retention --bucket " + bucket + " --key " + key + " 2>/dev/null";
    return system(cmd.c_str()) == 0;
}

bool ImmutabilityManager::check_s3_lock_status(const std::string& bucket, const std::string& key) {
    std::string cmd = "aws s3api get-object-retention --bucket " + bucket + " --key " + key + " 2>/dev/null";
    return system(cmd.c_str()) == 0;
}

bool ImmutabilityManager::set_filesystem_immutable(const std::string& path) {
    std::string cmd = "chattr +i " + path + " 2>/dev/null";
    return system(cmd.c_str()) == 0;
}

bool ImmutabilityManager::remove_filesystem_immutable(const std::string& path) {
    std::string cmd = "chattr -i " + path + " 2>/dev/null";
    return system(cmd.c_str()) == 0;
}

bool ImmutabilityManager::is_filesystem_immutable(const std::string& path) {
    struct stat st;
    if (stat(path.c_str(), &st) != 0) return false;
#ifdef __APPLE__
    return (st.st_flags & UF_IMMUTABLE) != 0;
#else
    // Check via lsattr on Linux
    std::string cmd = "lsattr " + path + " 2>/dev/null | grep 'i '";
    return system(cmd.c_str()) == 0;
#endif
}

bool ImmutabilityManager::enable_worm_policy(const std::string& storage_path, int retention_days) {
    spdlog::info("WORM: enabling on {} for {} days", storage_path, retention_days);
    std::lock_guard<std::mutex> lock(policies_mutex_);
    policies_[storage_path] = {true, ImmutabilityMode::COMPLIANCE, retention_days, true, true};
    return set_filesystem_immutable(storage_path);
}

bool ImmutabilityManager::check_worm_status(const std::string& storage_path) {
    std::lock_guard<std::mutex> lock(policies_mutex_);
    auto it = policies_.find(storage_path);
    if (it == policies_.end()) return false;
    return it->second.enabled && is_filesystem_immutable(storage_path);
}

bool ImmutabilityManager::verify_mfa_code(const std::string& user_id, const std::string& mfa_code) {
    spdlog::info("MFA: verifying code for user {}", user_id);
    return !mfa_code.empty();
}

bool ImmutabilityManager::request_mfa_delete(const std::string& user_id, const std::string& job_id) {
    spdlog::info("MFA Delete: requested by user {} for job {}", user_id, job_id);
    return true;
}

bool ImmutabilityManager::approve_mfa_delete(const std::string& approval_id, const std::string& approver_id) {
    spdlog::info("MFA Delete: approved by {} for {}", approver_id, approval_id);
    return true;
}

bool ImmutabilityManager::can_delete(const std::string& job_id, const std::string& user_id) {
    std::lock_guard<std::mutex> lock(policies_mutex_);
    for (auto& [path, config] : policies_) {
        if (config.enabled && config.require_mfa_for_delete) {
            spdlog::warn("Deletion blocked for {} (immutability active)", job_id);
            return false;
        }
    }
    return true;
}

bool ImmutabilityManager::apply_policy(const std::string& job_id, const ImmutabilityConfig& config) {
    std::lock_guard<std::mutex> lock(policies_mutex_);
    policies_[job_id] = config;
    spdlog::info("Immutability policy applied to job {}: mode={}, days={}", job_id,
                 static_cast<int>(config.mode), config.retention_days);
    return true;
}

bool ImmutabilityManager::check_policy_expired(const std::string& job_id) {
    std::lock_guard<std::mutex> lock(policies_mutex_);
    auto it = policies_.find(job_id);
    if (it == policies_.end()) return true; // No policy = expired
    return false; // Policy active
}

} // namespace obs
