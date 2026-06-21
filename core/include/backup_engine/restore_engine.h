#pragma once
#include "common.h"
#include <functional>

namespace obs {

enum class RestoreType { FULL, FILE_LEVEL, GRANULAR, VM_INSTANT, BLOCK_LEVEL };

struct RestoreConfig {
    std::string backup_path;
    std::string target_path;
    RestoreType type = RestoreType::FULL;
    std::vector<std::string> selected_files;
    bool overwrite = false;
    bool preserve_permissions = true;
    int db_port = 0;
    std::string db_password;
};

struct RestoreProgress {
    uint64_t restored_bytes = 0;
    uint64_t total_bytes = 0;
    std::string current_file;
    int percent = 0;
};

using RestoreProgressCallback = std::function<void(const RestoreProgress&)>;

class RestoreEngine {
public:
    bool restore(const RestoreConfig& config);
    void set_progress_callback(RestoreProgressCallback cb) { callback_ = std::move(cb); }
    void cancel() { cancelled_ = true; }

private:
    bool restore_full(const RestoreConfig& config);
    bool restore_file_level(const RestoreConfig& config);
    bool restore_postgresql(const RestoreConfig& config);
    bool restore_mysql(const RestoreConfig& config);
    bool restore_mongodb(const RestoreConfig& config);
    void report_progress(uint64_t restored, uint64_t total, const std::string& file);

    RestoreProgressCallback callback_;
    std::atomic<bool> cancelled_{false};
};

} // namespace obs
