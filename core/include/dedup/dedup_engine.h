#pragma once
#include "common.h"
#include <sqlite3.h>

namespace obs {

struct ChunkInfo {
    std::string hash;
    uint32_t size;
    int32_t ref_count;
    std::string first_seen;
};

struct DedupStats {
    size_t total_chunks = 0;
    size_t unique_chunks = 0;
    uint64_t saved_bytes = 0;
};

class DedupEngine {
public:
    DedupEngine(const std::string& db_path = "/tmp/obs_chunks.db");
    ~DedupEngine();

    std::string hash_chunk(const uint8_t* data, size_t size);
    std::optional<ChunkInfo> lookup(const std::string& hash);
    void add_chunk(const std::string& hash, uint32_t size);
    DedupStats get_stats();
    void process_file(const std::string& path,
                      std::function<void(const uint8_t*, size_t, const std::string&)> callback);

    static constexpr size_t MIN_CHUNK = 64 * 1024;    // 64KB
    static constexpr size_t MAX_CHUNK = 4 * 1024 * 1024; // 4MB
    static constexpr size_t AVG_CHUNK = 256 * 1024;   // 256KB

private:
    uint32_t fastcdc_boundary(const uint8_t* data, size_t size);
    void init_db();

    sqlite3* db_ = nullptr;
    mutable std::mutex db_mutex_;
};

} // namespace obs
