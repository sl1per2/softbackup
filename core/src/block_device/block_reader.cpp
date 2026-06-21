#include "block_device/block_reader.h"
#include <spdlog/spdlog.h>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#define O_RDONLY _O_RDONLY
#define O_DIRECT 0
inline int pread(int fd, void* buf, size_t count, off_t offset) {
    _lseeki64(fd, offset, SEEK_SET);
    return _read(fd, buf, count);
}
inline void* aligned_alloc(size_t alignment, size_t size) {
    return _aligned_malloc(size, alignment);
}
inline void aligned_free(void* ptr) { _aligned_free(ptr); }
#else
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
inline void* aligned_alloc(size_t alignment, size_t size) {
    void* ptr = nullptr;
    posix_memalign(&ptr, alignment, size);
    return ptr;
}
inline void aligned_free(void* ptr) { free(ptr); }
#endif

namespace obs {

BlockReader::BlockReader() {}
BlockReader::~BlockReader() { close(); }

bool BlockReader::open(const std::string& path) {
    fd_ = ::open(path.c_str(), O_RDONLY | O_DIRECT);
    if (fd_ < 0) {
        fd_ = ::open(path.c_str(), O_RDONLY);
        if (fd_ < 0) {
            spdlog::error("Failed to open block device: {}", path);
            return false;
        }
        spdlog::warn("Opened {} without O_DIRECT", path);
    }
    return true;
}

void BlockReader::close() {
    if (fd_ >= 0) { ::close(fd_); fd_ = -1; }
}

std::vector<std::vector<uint8_t>> BlockReader::read_blocks(uint64_t offset, size_t count, size_t block_size) {
    std::vector<std::vector<uint8_t>> blocks;
    if (fd_ < 0) return blocks;

    size_t aligned_size = ((block_size + 4095) / 4096) * 4096;
    uint8_t* aligned_buf = reinterpret_cast<uint8_t*>(aligned_alloc(4096, aligned_size));

    for (size_t i = 0; i < count; i++) {
        uint64_t pos = offset + i * block_size;
        ssize_t n = pread(fd_, aligned_buf, block_size, pos);
        if (n <= 0) break;
        blocks.push_back(std::vector<uint8_t>(aligned_buf, aligned_buf + n));
    }
    aligned_free(aligned_buf);
    return blocks;
}

PartitionInfo BlockReader::detect_partition_table(const std::string& path) {
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs) return {};

    char sig[8] = {};
    ifs.seekg(512);
    ifs.read(sig, 8);
    if (std::string(sig, 8) == "EFI PART") {
        return read_gpt(path);
    }

    ifs.seekg(510);
    uint16_t mbr_sig = 0;
    ifs.read(reinterpret_cast<char*>(&mbr_sig), 2);
    if (mbr_sig == 0xAA55) {
        return read_mbr(path);
    }

    return {};
}

PartitionInfo BlockReader::get_partition_info(const std::string& path, size_t partition_index) {
    auto info = detect_partition_table(path);
    if (partition_index < info.partitions.size()) {
        PartitionInfo result;
        result.type = info.type;
        result.partitions.push_back(info.partitions[partition_index]);
        return result;
    }
    return {};
}

PartitionInfo BlockReader::read_mbr(const std::string& path) {
    PartitionInfo info;
    info.type = "MBR";

    std::ifstream ifs(path, std::ios::binary);
    if (!ifs) return info;

    ifs.seekg(446);
    for (int i = 0; i < 4; i++) {
        char entry[16];
        ifs.read(entry, 16);
        if (entry[4] == 0) continue;

        PartitionInfo::Partition p;
        p.type_guid = static_cast<uint32_t>(static_cast<uint8_t>(entry[4]));
        p.start_lba = static_cast<uint64_t>(static_cast<uint8_t>(entry[8])) |
                      (static_cast<uint64_t>(static_cast<uint8_t>(entry[9])) << 8) |
                      (static_cast<uint64_t>(static_cast<uint8_t>(entry[10])) << 16) |
                      (static_cast<uint64_t>(static_cast<uint8_t>(entry[11])) << 24);
        p.size_lba = static_cast<uint64_t>(static_cast<uint8_t>(entry[12])) |
                     (static_cast<uint64_t>(static_cast<uint8_t>(entry[13])) << 8) |
                     (static_cast<uint64_t>(static_cast<uint8_t>(entry[14])) << 16) |
                     (static_cast<uint64_t>(static_cast<uint8_t>(entry[15])) << 24);
        p.name = "partition_" + std::to_string(i + 1);
        info.partitions.push_back(p);
    }
    return info;
}

PartitionInfo BlockReader::read_gpt(const std::string& path) {
    PartitionInfo info;
    info.type = "GPT";

    std::ifstream ifs(path, std::ios::binary);
    if (!ifs) return info;

    ifs.seekg(512 + 72);
    uint64_t entries_lba = 0;
    ifs.read(reinterpret_cast<char*>(&entries_lba), 8);

    uint32_t num_entries = 0;
    uint32_t entry_size = 0;
    ifs.seekg(512 + 80);
    ifs.read(reinterpret_cast<char*>(&num_entries), 4);
    ifs.read(reinterpret_cast<char*>(&entry_size), 4);

    ifs.seekg(entries_lba * 512);
    for (uint32_t i = 0; i < num_entries && i < 128; i++) {
        std::vector<char> entry(entry_size);
        ifs.read(entry.data(), entry_size);

        bool nonzero = false;
        for (int j = 0; j < 16; j++) {
            if (entry[j] != 0) { nonzero = true; break; }
        }
        if (!nonzero) continue;

        PartitionInfo::Partition p;
        p.start_lba = *reinterpret_cast<uint64_t*>(entry.data() + 32);
        p.size_lba = *reinterpret_cast<uint64_t*>(entry.data() + 40);

        std::ostringstream guid;
        for (int j = 0; j < 16; j++) {
            guid << std::hex << std::setw(2) << std::setfill('0')
                 << static_cast<int>(static_cast<uint8_t>(entry[j]));
            if (j == 3 || j == 5 || j == 7 || j == 9) guid << '-';
        }
        p.name = "partition_" + std::to_string(i + 1);
        info.partitions.push_back(p);
    }
    return info;
}

} // namespace obs
