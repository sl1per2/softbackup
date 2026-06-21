#pragma once
#include "common.h"

namespace obs {

struct TapeInfo {
    std::string device_path;   // /dev/nst0
    std::string media_type;    // LTO-7, LTO-8, LTO-9
    uint64_t capacity_bytes = 0;
    uint64_t used_bytes = 0;
    std::string label;
    std::string pool;
    int slot = -1;
    bool write_protected = false;
};

class TapeLibrary {
public:
    std::vector<TapeInfo> scan_devices();
    bool mount(const std::string& device);
    bool unmount(const std::string& device);
    bool write(const std::string& device, const std::string& file_path);
    bool read(const std::string& device, const std::string& file_path);
    bool rewind(const std::string& device);
    bool erase(const std::string& device);
    TapeInfo get_info(const std::string& device);

private:
    std::string run_mt_cmd(const std::string& device, const std::string& cmd);
};

} // namespace obs
