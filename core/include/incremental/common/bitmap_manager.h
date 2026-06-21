#pragma once
#include "common.h"
#include "incremental/common/incremental_engine.h"
#include <sqlite3.h>
#include <vector>
#include <string>
#include <functional>

namespace vovqa::incremental {

struct IncrementalMetrics {
    int64_t changed_blocks_total = 0;
    int64_t changed_blocks_read = 0;
    int64_t changed_blocks_skipped = 0;
    int64_t cbt_query_time_ms = 0;
    int64_t data_read_time_ms = 0;
    double change_percent = 0.0;
    int32_t incremental_chain_length = 0;
    uint64_t incremental_savings_bytes = 0;
};

struct ChainInfo {
    std::string base_job_id;
    std::string parent_job_id;
    std::string job_type; // full, incremental, differential
    std::string bitmap_hash;
    int32_t chain_length;
    bool is_broken;
};

class BitmapManager {
public:
    BitmapManager();
    ~BitmapManager();

    bool save_bitmap(const std::string& object_id, const std::string& job_id,
                    const std::vector<uint8_t>& bitmap, uint32_t block_size);
    bool load_bitmap(const std::string& object_id, std::vector<uint8_t>& bitmap, uint32_t& block_size);
    bool sync_bitmap_to_server(const std::string& object_id, const std::string& server_url);

    ChainInfo get_chain_info(const std::string& job_id);
    bool verify_chain_integrity(const std::string& base_job_id);
    void record_job(const std::string& job_id, const std::string& parent_id,
                   const std::string& type, const std::string& bitmap_hash);

    IncrementalMetrics get_metrics(const std::string& job_id);
    void record_metrics(const std::string& job_id, const IncrementalMetrics& metrics);

    double estimate_change_percent(const std::string& object_id, uint64_t total_blocks);
    bool should_do_full(const std::string& object_id, uint64_t total_blocks, double threshold = 0.4);

private:
    void init_db();

    sqlite3* db_ = nullptr;
    mutable std::mutex db_mutex_;
};

class ChangeJournal {
public:
    ChangeJournal();
    ~ChangeJournal();

    void log_change(const std::string& session_id, uint64_t lba, uint32_t size, const std::string& operation);
    std::vector<BlockRange> get_changes_since(const std::string& session_id, const std::string& since);
    void cleanup_old_entries(int retention_minutes);

private:
    void init_db();
    sqlite3* db_ = nullptr;
    mutable std::mutex db_mutex_;
};

std::vector<uint8_t> compress_bitmap_rle(const std::vector<uint8_t>& bitmap);
std::vector<uint8_t> decompress_bitmap_rle(const std::vector<uint8_t>& data, size_t expected_size);

} // namespace vovqa::incremental
