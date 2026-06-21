#pragma once
#include "common.h"

namespace obs {

struct FilesystemInfo {
    std::string type; // "xfs", "btrfs", "ext4", etc.
    bool supports_reflink;
};

class FastClone {
public:
    static FilesystemInfo detect_filesystem(const std::string& path);
    static bool clone_file(const std::string& source, const std::string& dest);
};

class SyntheticFull {
public:
    static bool create(const std::string& full_path,
                       const std::vector<std::string>& increment_paths,
                       const std::string& output_path);
};

} // namespace obs
