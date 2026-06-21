#include "tape_lib/tape_engine.h"
#include <spdlog/spdlog.h>
#include <cstdlib>
#include <chrono>

namespace obs {

TapeEngine::TapeEngine() {}
TapeEngine::~TapeEngine() { disconnect(); }

bool TapeEngine::connect(const std::string& device_path) {
    device_path_ = device_path;
    spdlog::info("TapeEngine: connecting to {}", device_path);
    connected_ = true;
    simulate_inventory();
    return true;
}

void TapeEngine::disconnect() { connected_ = false; }

TapeLibraryInfo TapeEngine::get_library_info() {
    TapeLibraryInfo info;
    info.vendor = "Simulated";
    info.model = "LTO Library";
    info.serial = "VOL001";
    info.slots_count = 24;
    info.drives_count = 2;
    info.device_path = device_path_;
    info.online = connected_;
    return info;
}

std::vector<TapeInfo> TapeEngine::inventory() {
    std::lock_guard<std::mutex> lock(tapes_mutex_);
    simulate_inventory();
    return tapes_;
}

void TapeEngine::simulate_inventory() {
    tapes_.clear();
    for (int i = 0; i < 12; i++) {
        TapeInfo tape;
        tape.barcode = "LTO9-" + std::to_string(i + 1);
        tape.pool = (i < 4) ? TapePool::ACTIVE : (i < 8) ? TapePool::ARCHIVE : TapePool::FREE;
        tape.capacity_bytes = 12ULL * 1024 * 1024 * 1024;
        tape.used_bytes = (i < 8) ? tape.capacity_bytes * 0.6 : 0;
        tape.write_count = (i < 8) ? 15 + i * 2 : 0;
        tape.location = (i < 2) ? "drive_" + std::to_string(i) : "slot_" + std::to_string(i);
        tapes_.push_back(tape);
    }
}

bool TapeEngine::write_to_tape(const std::vector<std::string>& backup_paths,
                                const std::vector<std::string>& barcodes,
                                TapeJobCallback callback) {
    TapeJobRecord job;
    job.job_id = "tape-write-" + std::to_string(std::time(nullptr));
    job.type = TapeJobType::WRITE;
    job.tape_barcodes = barcodes;
    job.status = TapeJobStatus::RUNNING;
    job.started_at = current_time_string();
    if (callback) callback(job);

    spdlog::info("TapeEngine: writing {} files to {} tapes", backup_paths.size(), barcodes.size());

    for (size_t i = 0; i < backup_paths.size(); i++) {
        if (i < barcodes.size()) {
            spdlog::info("TapeEngine: writing {} to tape {}", backup_paths[i], barcodes[i]);
        }
    }

    job.status = TapeJobStatus::COMPLETED;
    job.completed_at = current_time_string();
    if (callback) callback(job);
    return true;
}

bool TapeEngine::read_from_tape(const std::string& barcode, const std::string& output_dir,
                                 TapeJobCallback callback) {
    spdlog::info("TapeEngine: reading from tape {} to {}", barcode, output_dir);
    return true;
}

bool TapeEngine::format_tape(const std::string& barcode) {
    spdlog::info("TapeEngine: formatting tape {}", barcode);
    return true;
}

bool TapeEngine::clean_drive(int drive_index) {
    spdlog::info("TapeEngine: cleaning drive {}", drive_index);
    return true;
}

bool TapeEngine::mount_tape(const std::string& barcode, int drive_index) {
    spdlog::info("TapeEngine: mounting tape {} to drive {}", barcode, drive_index);
    return execute_mtx("load " + barcode + " " + std::to_string(drive_index));
}

bool TapeEngine::unmount_tape(int drive_index) {
    spdlog::info("TapeEngine: unmounting drive {}", drive_index);
    return execute_mtx("unload " + std::to_string(drive_index));
}

bool TapeEngine::move_tape(const std::string& barcode, TapePool target_pool) {
    std::lock_guard<std::mutex> lock(tapes_mutex_);
    for (auto& tape : tapes_) {
        if (tape.barcode == barcode) {
            tape.pool = target_pool;
            spdlog::info("TapeEngine: moved {} to pool {}", barcode, static_cast<int>(target_pool));
            return true;
        }
    }
    return false;
}

std::vector<TapeInfo> TapeEngine::get_tapes_by_pool(TapePool pool) {
    std::vector<TapeInfo> result;
    std::lock_guard<std::mutex> lock(tapes_mutex_);
    for (auto& tape : tapes_) {
        if (tape.pool == pool) result.push_back(tape);
    }
    return result;
}

TapeInfo TapeEngine::get_tape(const std::string& barcode) {
    std::lock_guard<std::mutex> lock(tapes_mutex_);
    for (auto& tape : tapes_) {
        if (tape.barcode == barcode) return tape;
    }
    return {};
}

bool TapeEngine::write_multi_stream(const std::string& barcode,
                                     const std::vector<std::vector<uint8_t>>& streams) {
    spdlog::info("TapeEngine: multi-stream write {} streams to tape {}", streams.size(), barcode);
    return true;
}

bool TapeEngine::write_spanning(const std::vector<std::string>& data_chunks,
                                 std::vector<std::string>& output_barcodes) {
    spdlog::info("TapeEngine: spanning {} chunks", data_chunks.size());
    output_barcodes.push_back("LTO9-SPAN-1");
    return true;
}

bool TapeEngine::execute_mtx(const std::string& command) {
    std::string cmd = "mtx -f " + device_path_ + " " + command + " 2>/dev/null";
    spdlog::info("TapeEngine: mtx {}", command);
    return system(cmd.c_str()) == 0;
}

} // namespace obs
