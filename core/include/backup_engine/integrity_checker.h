#pragma once
#include "common.h"
#include <vector>

namespace obs {

struct IntegrityResult {
    bool passed = true;
    int total_chunks = 0;
    int verified_chunks = 0;
    int failed_chunks = 0;
    std::vector<std::string> failed_hashes;
    std::string error;
};

class IntegrityChecker {
public:
    IntegrityResult verify_backup(const std::string& backup_path);
    IntegrityResult verify_chunk(const std::string& chunk_path, const std::string& expected_hash);
    bool verify_restore(const std::string& original_path, const std::string& restored_path);

private:
    std::string compute_hash(const std::string& path);
};

} // namespace obs
