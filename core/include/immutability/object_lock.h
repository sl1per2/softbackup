#pragma once
#include "common.h"
#include <string>
#include <vector>

namespace obs {

enum class ImmutabilityMode { NONE, GOVERNANCE, COMPLIANCE };
enum class LockType { S3_OBJECT_LOCK, FILESYSTEM_CHATTR, WORM, READ_ONLY_MOUNT };

struct ImmutabilityConfig {
    bool enabled = false;
    ImmutabilityMode mode = ImmutabilityMode::NONE;
    int retention_days = 30;
    bool multi_factor_delete = false;
    bool require_mfa_for_delete = false;
};

struct ObjectLockConfig {
    std::string bucket;
    ImmutabilityMode mode = ImmutabilityMode::GOVERNANCE;
    int retention_days = 30;
    std::string object_key;
};

class ImmutabilityManager {
public:
    ImmutabilityManager();
    ~ImmutabilityManager();

    bool enable_s3_object_lock(const ObjectLockConfig& config);
    bool disable_s3_object_lock(const std::string& bucket, const std::string& key);
    bool check_s3_lock_status(const std::string& bucket, const std::string& key);

    bool set_filesystem_immutable(const std::string& path);
    bool remove_filesystem_immutable(const std::string& path);
    bool is_filesystem_immutable(const std::string& path);

    bool enable_worm_policy(const std::string& storage_path, int retention_days);
    bool check_worm_status(const std::string& storage_path);

    bool verify_mfa_code(const std::string& user_id, const std::string& mfa_code);
    bool request_mfa_delete(const std::string& user_id, const std::string& job_id);
    bool approve_mfa_delete(const std::string& approval_id, const std::string& approver_id);

    bool can_delete(const std::string& job_id, const std::string& user_id);
    bool apply_policy(const std::string& job_id, const ImmutabilityConfig& config);
    bool check_policy_expired(const std::string& job_id);

private:
    std::map<std::string, ImmutabilityConfig> policies_;
    mutable std::mutex policies_mutex_;
};

} // namespace obs
