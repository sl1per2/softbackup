#pragma once
#include "common.h"
#include "framework/component.h"
#include <functional>

namespace obs {

struct BackupRequest {
    std::string job_id;
    std::string source_path;
    std::string target_path;
    std::vector<std::string> exclude_patterns;
    bool compress = true;
    bool encrypt = false;
    std::string encryption_key;
    int compression_level = 1;
};

struct BackupResult {
    bool success = false;
    uint64_t total_bytes = 0;
    uint64_t transferred_bytes = 0;
    uint64_t chunks_created = 0;
    uint64_t dedup_savings = 0;
    double compression_ratio = 0.0;
    double elapsed_seconds = 0.0;
    std::string error;
};

using EngineProgressCallback = std::function<void(double progress, uint64_t bytes)>;

class IBackupEngine : public IComponent {
public:
    virtual BackupResult backup(const BackupRequest& request, EngineProgressCallback progress = nullptr) = 0;
    virtual bool cancel(const std::string& job_id) = 0;
    virtual std::map<std::string, std::string> get_metrics() const = 0;
};

class IIncrementalEngine : public IBackupEngine {
public:
    virtual std::vector<std::pair<uint64_t, uint32_t>> get_changed_blocks(
        const std::string& vm_id, const std::string& from_snapshot, const std::string& to_snapshot) = 0;
    virtual bool supports_cbt() const = 0;
};

} // namespace obs
