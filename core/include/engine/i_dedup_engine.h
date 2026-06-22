#pragma once
#include "common.h"
#include "framework/component.h"

namespace obs {

struct ChunkData {
    std::string hash;
    std::vector<uint8_t> data;
    uint32_t size = 0;
};

class IDedupEngine : public IComponent {
public:
    virtual std::string hash_data(const uint8_t* data, size_t size) = 0;
    virtual bool chunk_exists(const std::string& hash) = 0;
    virtual void store_chunk(const std::string& hash, const uint8_t* data, size_t size) = 0;
    virtual std::optional<ChunkData> retrieve_chunk(const std::string& hash) = 0;
    virtual size_t total_chunks() const = 0;
    virtual size_t unique_chunks() const = 0;
    virtual uint64_t bytes_saved() const = 0;
    virtual void process_file(const std::string& path,
        std::function<void(const uint8_t*, size_t, const std::string&)> callback) = 0;
};

} // namespace obs
