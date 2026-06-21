#pragma once
#include "common.h"
#include <functional>
#include <memory>
#include <sqlite3.h>

namespace vovqa::incremental {

struct BlockRange {
    uint64_t offset;
    uint32_t size;
    bool dirty;
};

struct BitmapState {
    std::string object_id;
    uint32_t block_size;
    std::vector<uint8_t> bitmap;
    uint64_t total_blocks;
    std::string last_job_id;
    std::string last_success_at;
};

class IncrementalBackupEngine {
public:
    virtual ~IncrementalBackupEngine() = default;
    virtual std::string name() const = 0;

    virtual std::vector<BlockRange> detect_changes(const std::string& path) = 0;
    virtual bool read_changed_blocks(const std::string& path, const std::vector<BlockRange>& ranges,
                                     std::function<void(const uint8_t*, size_t, uint64_t)> callback) = 0;
    virtual std::vector<BlockRange> merge_bitmaps(const std::vector<BlockRange>& a, const std::vector<BlockRange>& b) = 0;

    BitmapState load_bitmap(const std::string& object_id);
    void save_bitmap(const std::string& object_id, const BitmapState& state);
    std::vector<uint8_t> compress_rle(const std::vector<uint8_t>& data);
    std::vector<uint8_t> decompress_rle(const std::vector<uint8_t>& data);

private:
    void init_db();
    sqlite3* db_ = nullptr;
    mutable std::mutex db_mutex_;
};

} // namespace vovqa::incremental
