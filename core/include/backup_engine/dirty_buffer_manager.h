#pragma once
#include "common.h"
#include <sqlite3.h>
#include <vector>
#include <functional>

namespace obs {

enum class DirtyReason {
    WRITE,
    DELETE,
    MODIFY,
    METADATA_CHANGE,
    TRUNCATE,
    EXTEND
};

struct DirtyBlock {
    int64_t id = 0;
    std::string job_id;
    std::string source_path;
    uint64_t block_offset = 0;
    uint32_t block_size = 0;
    uint64_t lba = 0;
    DirtyReason reason = DirtyReason::WRITE;
    std::string dirty_at;
    std::string checksum;
    std::string original_hash;
    bool is_backed_up = false;
};

struct DirtyBufferStats {
    std::string job_id;
    int64_t total_blocks_scanned = 0;
    int64_t dirty_blocks_count = 0;
    uint64_t dirty_bytes = 0;
    uint64_t clean_bytes = 0;
    int64_t scan_time_ms = 0;
    std::string recorded_at;
    double dirty_ratio = 0.0;
};

struct HotspotEntry {
    std::string source_path;
    uint64_t block_offset;
    int32_t change_count;
    std::string first_seen;
    std::string last_seen;
};

struct ChangeFrequency {
    std::string time_bucket;
    int64_t change_count;
    uint64_t bytes_changed;
};

class DirtyBufferManager {
public:
    DirtyBufferManager(const std::string& db_path = "/tmp/obs_dirty_buffers.db");
    ~DirtyBufferManager();

    void record_dirty_block(const DirtyBlock& block);
    void record_dirty_blocks_batch(const std::vector<DirtyBlock>& blocks);
    void record_from_bitmap(const std::string& job_id, const std::string& source_path,
                           const std::vector<bool>& bitmap, uint32_t block_size);

    std::vector<DirtyBlock> get_dirty_blocks(const std::string& job_id) const;
    std::vector<DirtyBlock> get_dirty_blocks_since(const std::string& source_path,
                                                    const std::string& since) const;
    std::vector<DirtyBlock> get_unbacked_blocks(const std::string& job_id) const;

    DirtyBufferStats get_stats(const std::string& job_id) const;
    void record_stats(const DirtyBufferStats& stats);

    void mark_backed_up(const std::string& job_id);
    void mark_block_backed_up(int64_t block_id);

    std::vector<HotspotEntry> get_hotspots(int min_change_count = 3) const;
    std::vector<ChangeFrequency> get_change_frequency(int hours = 24) const;
    int64_t get_total_dirty_blocks(const std::string& source_path) const;

    void cleanup_old_records(int retention_days = 30);
    void vacuum();

private:
    void init_db();
    void ensure_table(const char* name, const char* create_sql);

    sqlite3* db_ = nullptr;
    mutable std::mutex db_mutex_;
};

} // namespace obs
