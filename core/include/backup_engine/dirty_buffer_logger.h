#pragma once
#include "common.h"
#include <sqlite3.h>
#include <memory>
#include <vector>
#include <functional>
#include <atomic>

namespace obs {

enum class ConsistencyLevel {
    APPLICATION_CONSISTENT,
    CRASH_CONSISTENT,
    FILE_CONSISTENT
};

enum class FlushStatus {
    FLUSHED,
    NOT_FLUSHED,
    TIMEOUT,
    FAILED
};

inline const char* to_string(ConsistencyLevel level) {
    switch (level) {
        case ConsistencyLevel::APPLICATION_CONSISTENT: return "application_consistent";
        case ConsistencyLevel::CRASH_CONSISTENT:        return "crash_consistent";
        case ConsistencyLevel::FILE_CONSISTENT:         return "file_consistent";
    }
    return "unknown";
}

inline const char* to_string(FlushStatus status) {
    switch (status) {
        case FlushStatus::FLUSHED:     return "flushed";
        case FlushStatus::NOT_FLUSHED: return "not_flushed";
        case FlushStatus::TIMEOUT:     return "timeout";
        case FlushStatus::FAILED:      return "failed";
    }
    return "unknown";
}

struct BufferState {
    int64_t page_count = 0;
    uint64_t size_bytes = 0;
    double percent_of_ram = 0.0;
};

struct ComponentDetail {
    std::string name;
    int64_t dirty_pages = 0;
    uint64_t size_bytes = 0;
};

struct FlushResult {
    std::string job_id;
    std::string agent_id;
    std::string plugin_name;

    BufferState before;
    BufferState after;

    FlushStatus status = FlushStatus::NOT_FLUSHED;
    int64_t flush_duration_ms = 0;
    std::string error_message;

    std::vector<ComponentDetail> component_details;
    ConsistencyLevel consistency = ConsistencyLevel::CRASH_CONSISTENT;
    bool is_consistent = false;

    uint64_t total_ram_bytes = 0;
    uint64_t buffer_pool_size = 0;

    std::string flush_started_at;
    std::string flush_finished_at;
    std::string created_at;
};

struct DirtyBufferLogEntry {
    int64_t id = 0;
    std::string backup_job_id;
    std::string agent_id;
    std::string plugin_name;

    int64_t before_page_count = 0;
    uint64_t before_size_bytes = 0;
    double before_percent_ram = 0.0;

    int64_t after_page_count = 0;
    uint64_t after_size_bytes = 0;
    double after_percent_ram = 0.0;

    std::string flush_status;
    int64_t flush_duration_ms = 0;
    std::string error_message;
    std::string component_details_json;
    std::string consistency_level;
    bool is_consistent = false;

    uint64_t total_ram_bytes = 0;
    uint64_t buffer_pool_size = 0;

    std::string flush_started_at;
    std::string flush_finished_at;
    std::string created_at;
};

class IBufferFlusher {
public:
    virtual ~IBufferFlusher() = default;
    virtual std::string plugin_name() const = 0;
    virtual BufferState query_dirty_state() = 0;
    virtual bool flush_dirty_buffers() = 0;
    virtual std::vector<ComponentDetail> get_component_details() = 0;
    virtual uint64_t get_total_ram() = 0;
    virtual uint64_t get_buffer_pool_size() { return 0; }
    virtual bool quiesce_application() { return true; }
    virtual void unquiesce_application() {}
};

class DirtyBufferLogger {
public:
    DirtyBufferLogger(const std::string& db_path = "/tmp/obs_dirty_buffers.db",
                      const std::string& server_url = "");
    ~DirtyBufferLogger();

    FlushResult flush_and_log(const std::string& job_id,
                              const std::string& agent_id,
                              IBufferFlusher* flusher);

    void save_to_db(const FlushResult& result);
    std::vector<DirtyBufferLogEntry> get_logs(const std::string& job_id) const;
    std::vector<DirtyBufferLogEntry> get_all_logs(int limit = 100) const;
    DirtyBufferLogEntry get_latest_log(const std::string& plugin_name) const;

    std::vector<DirtyBufferLogEntry> get_failed_flushes() const;
    std::vector<DirtyBufferLogEntry> get_inconsistent_backups() const;

    void set_server_url(const std::string& url) { server_url_ = url; }

private:
    void init_db();
    void log_before(const std::string& job_id, const BufferState& state);
    void log_flush_start(const std::string& job_id, const std::string& plugin);
    void log_flush_complete(const FlushResult& result);
    void log_consistency(const std::string& job_id, ConsistencyLevel level, bool consistent);

    bool send_to_server(const FlushResult& result);
    std::string build_json(const FlushResult& result) const;
    std::string components_to_json(const std::vector<ComponentDetail>& details) const;

    static std::string bytes_to_human(uint64_t bytes);

    sqlite3* db_ = nullptr;
    mutable std::mutex db_mutex_;
    std::string server_url_;
    std::atomic<bool> initialized_{false};
};

} // namespace obs
