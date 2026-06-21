#include "backup_engine/cbt.h"
#include <fstream>
#include <cstring>

namespace obs {

ChangedBlockTracking::ChangedBlockTracking(uint64_t total_blocks, uint32_t block_size)
    : total_blocks_(total_blocks), block_size_(block_size), bitmap_(total_blocks, false) {}

void ChangedBlockTracking::mark_changed(uint64_t block_index) {
    if (block_index < total_blocks_ && !bitmap_[block_index]) {
        bitmap_[block_index] = true;
        changed_count_++;
    }
}

void ChangedBlockTracking::mark_range(uint64_t start, uint64_t end) {
    for (uint64_t i = start; i < std::min(end, total_blocks_) && i < total_blocks_; i++) {
        mark_changed(i);
    }
}

std::vector<BlockRange> ChangedBlockTracking::get_changed_blocks() const {
    std::vector<BlockRange> ranges;
    bool in_range = false;
    uint64_t range_start = 0;

    for (uint64_t i = 0; i < total_blocks_; i++) {
        if (bitmap_[i]) {
            if (!in_range) {
                range_start = i;
                in_range = true;
            }
        } else {
            if (in_range) {
                ranges.push_back({range_start * block_size_,
                                  static_cast<uint32_t>((i - range_start) * block_size_)});
                in_range = false;
            }
        }
    }
    if (in_range) {
        ranges.push_back({range_start * block_size_,
                          static_cast<uint32_t>((total_blocks_ - range_start) * block_size_)});
    }
    return ranges;
}

std::vector<uint8_t> ChangedBlockTracking::compress_bitmap_rle() const {
    std::vector<uint8_t> result;
    if (bitmap_.empty()) return result;

    bool current = bitmap_[0];
    uint16_t count = 1;

    for (size_t i = 1; i < bitmap_.size(); i++) {
        if (bitmap_[i] == current && count < 65535) {
            count++;
        } else {
            result.push_back(current ? 1 : 0);
            result.push_back(static_cast<uint8_t>((count >> 8) & 0xFF));
            result.push_back(static_cast<uint8_t>(count & 0xFF));
            current = bitmap_[i];
            count = 1;
        }
    }
    result.push_back(current ? 1 : 0);
    result.push_back(static_cast<uint8_t>((count >> 8) & 0xFF));
    result.push_back(static_cast<uint8_t>(count & 0xFF));
    return result;
}

void ChangedBlockTracking::decompress_bitmap_rle(const std::vector<uint8_t>& data) {
    bitmap_.clear();
    changed_count_ = 0;
    for (size_t i = 0; i + 2 < data.size(); i += 3) {
        bool val = data[i] != 0;
        uint16_t count = (static_cast<uint16_t>(data[i + 1]) << 8) | data[i + 2];
        for (uint16_t j = 0; j < count && bitmap_.size() < total_blocks_; j++) {
            bitmap_.push_back(val);
            if (val) changed_count_++;
        }
    }
}

bool ChangedBlockTracking::save(const std::string& path) const {
    std::ofstream ofs(path, std::ios::binary);
    if (!ofs) return false;
    auto compressed = compress_bitmap_rle();
    uint32_t size = static_cast<uint32_t>(compressed.size());
    ofs.write(reinterpret_cast<const char*>(&size), sizeof(size));
    ofs.write(reinterpret_cast<const char*>(compressed.data()), compressed.size());
    return ofs.good();
}

bool ChangedBlockTracking::load(const std::string& path) {
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs) return false;
    uint32_t size = 0;
    ifs.read(reinterpret_cast<char*>(&size), sizeof(size));
    std::vector<uint8_t> data(size);
    ifs.read(reinterpret_cast<char*>(data.data()), size);
    decompress_bitmap_rle(data);
    return true;
}

void ChangedBlockTracking::reset() {
    std::fill(bitmap_.begin(), bitmap_.end(), false);
    changed_count_ = 0;
}

} // namespace obs
