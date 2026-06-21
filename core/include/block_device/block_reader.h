#pragma once
#include "common.h"

namespace obs {

struct PartitionInfo {
    std::string type; // "MBR" or "GPT"
    struct Partition {
        std::string name;
        uint64_t start_lba;
        uint64_t size_lba;
        uint32_t type_guid;
    };
    std::vector<Partition> partitions;
};

class BlockReader {
public:
    BlockReader();
    ~BlockReader();

    bool open(const std::string& path);
    void close();
    std::vector<std::vector<uint8_t>> read_blocks(uint64_t offset, size_t count, size_t block_size = 512);
    PartitionInfo detect_partition_table(const std::string& path);
    PartitionInfo get_partition_info(const std::string& path, size_t partition_index);

private:
    PartitionInfo read_mbr(const std::string& path);
    PartitionInfo read_gpt(const std::string& path);
    int fd_ = -1;
};

} // namespace obs
