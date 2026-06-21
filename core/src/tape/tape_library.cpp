#include "tape/tape_library.h"
#include <spdlog/spdlog.h>
#include <fstream>

namespace obs {

TapeLibrary::TapeLibrary() {}
TapeLibrary::~TapeLibrary() { disconnect(); }

bool TapeLibrary::connect(const std::string& device_path) {
    device_path_ = device_path;
    // Check if device exists
    if (!fs::exists(device_path)) {
        spdlog::warn("Tape device not found: {}, using simulation mode", device_path);
    }
    connected_ = true;
    simulate_inventory();
    spdlog::info("Tape library connected: {}", device_path);
    return true;
}

void TapeLibrary::disconnect() {
    connected_ = false;
}

TapeLibraryInfo TapeLibrary::get_info() const {
    return info_;
}

bool TapeLibrary::mount(const std::string& barcode, int drive_index) {
    if (drive_index < 0 || drive_index >= (int)info_.drives.size()) return false;
    info_.drives[drive_index].current_barcode = barcode;
    info_.drives[drive_index].online = true;
    spdlog::info("Tape {} mounted to drive {}", barcode, drive_index);
    return true;
}

bool TapeLibrary::unmount(int drive_index) {
    if (drive_index < 0 || drive_index >= (int)info_.drives.size()) return false;
    info_.drives[drive_index].current_barcode.clear();
    spdlog::info("Tape unmounted from drive {}", drive_index);
    return true;
}

bool TapeLibrary::write(const std::string& drive_path, const uint8_t* data, size_t size) {
    // In production: open /dev/nst0, write with MTIOCBSF (filemark)
    spdlog::info("Writing {} bytes to tape drive {}", size, drive_path);
    return true;
}

bool TapeLibrary::read(const std::string& drive_path, uint8_t* data, size_t size, size_t offset) {
    spdlog::info("Reading {} bytes from tape drive {} offset {}", size, drive_path, offset);
    return true;
}

bool TapeLibrary::move_tape(int source_slot, int dest_slot) {
    spdlog::info("Moving tape from slot {} to slot {}", source_slot, dest_slot);
    return true;
}

bool TapeLibrary::eject(const std::string& barcode) {
    spdlog::info("Ejecting tape {}", barcode);
    return true;
}

bool TapeLibrary::inventory() {
    simulate_inventory();
    return true;
}

bool TapeLibrary::write_multi(const std::string& barcode, const std::vector<std::vector<uint8_t>>& streams) {
    spdlog::info("Multi-stream write: {} streams to tape {}", streams.size(), barcode);
    for (size_t i = 0; i < streams.size(); i++) {
        spdlog::info("  Stream {}: {} bytes", i, streams[i].size());
    }
    return true;
}

void TapeLibrary::simulate_inventory() {
    info_.name = "Simulated Tape Library";
    info_.type = "tape_library";
    info_.total_slots = 24;
    info_.used_slots = 8;
    info_.total_drives = 2;
    info_.online_drives = 2;

    for (int i = 0; i < info_.total_slots; i++) {
        TapeInfo tape;
        tape.barcode = "TAPE" + std::to_string(i + 1);
        tape.pool = (i < 4) ? "daily" : (i < 8) ? "weekly" : "monthly";
        tape.capacity_bytes = 12ULL * 1024 * 1024 * 1024; // 12TB LTO-9
        tape.used_bytes = (i < info_.used_slots) ? tape.capacity_bytes * 0.7 : 0;
        tape.status = (i < info_.used_slots) ? TapeStatus::ONLINE : TapeStatus::OFFLINE;
        tape.slot = i;
        info_.tapes.push_back(tape);
    }

    for (int i = 0; i < info_.total_drives; i++) {
        TapeDriveInfo drive;
        drive.device_path = "/dev/nst" + std::to_string(i);
        drive.online = true;
        drive.status = "idle";
        info_.drives.push_back(drive);
    }
}

} // namespace obs
