#pragma once
#include "common.h"
#include <thread>
#include <atomic>

namespace obs {

enum class TapeStatus { ONLINE, OFFLINE, ERROR, READ_ONLY };

struct TapeInfo {
    std::string barcode;
    std::string pool;
    uint64_t capacity_bytes;
    uint64_t used_bytes;
    TapeStatus status;
    int slot;
    std::string last_write;
    std::string mount_point;
};

struct TapeDriveInfo {
    std::string device_path;
    bool online;
    std::string current_barcode;
    std::string status;
};

struct TapeLibraryInfo {
    std::string name;
    std::string type; // "tape", "autoload", "tape_library"
    int total_slots;
    int used_slots;
    int total_drives;
    int online_drives;
    std::vector<TapeInfo> tapes;
    std::vector<TapeDriveInfo> drives;
};

class TapeLibrary {
public:
    TapeLibrary();
    ~TapeLibrary();

    bool connect(const std::string& device_path);
    void disconnect();
    TapeLibraryInfo get_info() const;

    bool mount(const std::string& barcode, int drive_index);
    bool unmount(int drive_index);
    bool write(const std::string& drive_path, const uint8_t* data, size_t size);
    bool read(const std::string& drive_path, uint8_t* data, size_t size, size_t offset);

    bool move_tape(int source_slot, int dest_slot);
    bool eject(const std::string& barcode);
    bool inventory();

    // Multi-streaming
    bool write_multi(const std::string& barcode, const std::vector<std::vector<uint8_t>>& streams);

private:
    bool send_sg_command(const std::string& device, const uint8_t* cdb, size_t cdb_len);
    void simulate_inventory();

    std::string device_path_;
    TapeLibraryInfo info_;
    bool connected_ = false;
};

} // namespace obs
