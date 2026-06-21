#pragma once
#include "common.h"
#include <vector>

namespace obs {

struct BlockRange {
    uint64_t offset;
    uint32_t size;
};

class ChangedBlockTracking {
public:
    ChangedBlockTracking(uint64_t total_blocks, uint32_t block_size = 4096);

    void mark_changed(uint64_t block_index);
    void mark_range(uint64_t start, uint64_t end);
    std::vector<BlockRange> get_changed_blocks() const;
    std::vector<uint8_t> compress_bitmap_rle() const;
    void decompress_bitmap_rle(const std::vector<uint8_t>& data);
    bool save(const std::string& path) const;
    bool load(const std::string& path);
    void reset();
    size_t changed_count() const { return changed_count_; }

private:
    uint64_t total_blocks_;
    uint32_t block_size_;
    std::vector<bool> bitmap_;
    size_t changed_count_ = 0;
};

} // namespace obs
