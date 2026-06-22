#pragma once
#include "common.h"
#include "framework/component.h"

namespace obs {

enum class CompressionAlgorithm { NONE, ZSTD, LZ4, DEFLATE, BZIP2 };

class ICompressionEngine : public IComponent {
public:
    virtual std::vector<uint8_t> compress(const uint8_t* data, size_t size, int level = 1) = 0;
    virtual std::vector<uint8_t> decompress(const uint8_t* data, size_t size) = 0;
    virtual CompressionAlgorithm algorithm() const = 0;
    virtual double estimate_ratio(const uint8_t* data, size_t size) = 0;
};

} // namespace obs
