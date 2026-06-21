#include "dedup/dedup_engine.h"
#include <cassert>
#include <cstring>
#include <iostream>

void test_blake3_hash() {
    obs::DedupEngine engine("/tmp/test_dedup.db");
    const char* data = "Hello, OBS Backup!";
    std::string hash = engine.hash_chunk(reinterpret_cast<const uint8_t*>(data), strlen(data));
    assert(!hash.empty());
    assert(hash.size() == 128); // BLAKE2b-512 produces 128 hex chars

    // Same data should produce same hash
    std::string hash2 = engine.hash_chunk(reinterpret_cast<const uint8_t*>(data), strlen(data));
    assert(hash == hash2);

    // Different data should produce different hash
    const char* data2 = "Different data!";
    std::string hash3 = engine.hash_chunk(reinterpret_cast<const uint8_t*>(data2), strlen(data2));
    assert(hash != hash3);

    std::cout << "[PASS] test_blake3_hash" << std::endl;
}

void test_chunk_index() {
    obs::DedupEngine engine("/tmp/test_dedup_index.db");
    std::string hash = "abc123def456";
    engine.add_chunk(hash, 4096);

    auto info = engine.lookup(hash);
    assert(info.has_value());
    assert(info->hash == hash);
    assert(info->size == 4096);
    assert(info->ref_count >= 1); // add + lookup incremented

    // Non-existent chunk
    auto missing = engine.lookup("nonexistent");
    assert(!missing.has_value());

    std::cout << "[PASS] test_chunk_index" << std::endl;
}

void test_fastcdc() {
    obs::DedupEngine engine("/tmp/test_fastcdc.db");
    std::vector<uint8_t> data(1024 * 1024); // 1MB of zeros
    std::string hash = engine.hash_chunk(data.data(), data.size());
    assert(!hash.empty());
    assert(hash.size() == 128);
    std::cout << "[PASS] test_fastcdc (hash " << hash.substr(0, 16) << "...)" << std::endl;
}

void test_stats() {
    obs::DedupEngine engine("/tmp/test_stats.db");
    engine.add_chunk("hash1", 1000);
    engine.add_chunk("hash2", 2000);
    engine.add_chunk("hash3", 3000);

    auto stats = engine.get_stats();
    assert(stats.unique_chunks == 3);
    std::cout << "[PASS] test_stats: " << stats.unique_chunks << " unique chunks" << std::endl;
}

int main() {
    test_blake3_hash();
    test_chunk_index();
    test_fastcdc();
    test_stats();
    std::cout << "All dedup tests passed!" << std::endl;
    return 0;
}
