#include "data_mover/data_mover.h"
#include "data_mover/tcp_stream.h"
#include <fstream>
#include <filesystem>
#include <spdlog/spdlog.h>
#include <cstring>

namespace obs {

DataMover::DataMover() {}
DataMover::~DataMover() { cancel_transfer(); }

void DataMover::configure(const TransferConfig& config) { config_ = config; }

TransportMode DataMover::detect_transport_mode() {
    if (fs::exists("/sys/class/fc_host")) {
        for (auto& entry : fs::directory_iterator("/sys/class/fc_host")) {
            return TransportMode::DIRECT_SAN;
        }
    }
    if (fs::exists("/sys/class/block/vd*") || fs::exists("/dev/vda")) {
        return TransportMode::HOT_ADD;
    }
    return TransportMode::NETWORK;
}

void DataMover::start_transfer() {
    cancelled_ = false;
    running_ = true;
    worker_ = std::thread(&DataMover::do_transfer, this);
}

void DataMover::cancel_transfer() {
    cancelled_ = true;
    if (worker_.joinable()) worker_.join();
    running_ = false;
}

TransferProgress DataMover::get_progress() const {
    std::lock_guard<std::mutex> lock(progress_mutex_);
    return progress_;
}

bool DataMover::is_zero_block(const uint8_t* data, size_t size) const {
    const uint64_t* ptr = reinterpret_cast<const uint64_t*>(data);
    size_t count = size / sizeof(uint64_t);
    for (size_t i = 0; i < count; i++) {
        if (ptr[i] != 0) return false;
    }
    return true;
}

std::string DataMover::get_checkpoint_path() const {
    return "/tmp/obs_checkpoint_" + config_.job_id;
}

void DataMover::save_checkpoint(uint64_t offset) {
    std::ofstream ofs(get_checkpoint_path(), std::ios::binary);
    ofs.write(reinterpret_cast<const char*>(&offset), sizeof(offset));
}

uint64_t DataMover::load_checkpoint() const {
    std::ifstream ifs(get_checkpoint_path(), std::ios::binary);
    uint64_t offset = 0;
    if (ifs) ifs.read(reinterpret_cast<char*>(&offset), sizeof(offset));
    return offset;
}

void DataMover::do_transfer() {
    try {
        if (config_.transport_mode == TransportMode::AUTO) {
            config_.transport_mode = detect_transport_mode();
        }

        std::string mode_str;
        switch (config_.transport_mode) {
            case TransportMode::DIRECT_SAN: mode_str = "Direct SAN"; break;
            case TransportMode::HOT_ADD:    mode_str = "Hot Add"; break;
            case TransportMode::NETWORK:    mode_str = "Network"; break;
            case TransportMode::NBD:        mode_str = "NBD"; break;
            default: mode_str = "Auto"; break;
        }

        spdlog::info("DataMover: starting transfer job {} mode={}", config_.job_id, mode_str);

        if (!fs::exists(config_.source_path)) {
            spdlog::error("Source path not found: {}", config_.source_path);
            running_ = false;
            return;
        }

        uint64_t file_size = fs::is_regular_file(config_.source_path)
            ? fs::file_size(config_.source_path) : 0;

        uint64_t start_offset = load_checkpoint();

        {
            std::lock_guard<std::mutex> lock(progress_mutex_);
            progress_.total_bytes = file_size;
            progress_.transferred_bytes = start_offset;
        }

        const size_t CHUNK_SIZE = 4 * 1024 * 1024;
        std::vector<uint8_t> buffer(CHUNK_SIZE);
        uint64_t transferred = start_offset;

        std::ifstream ifs(config_.source_path, std::ios::binary);
        if (start_offset > 0) {
            ifs.seekg(start_offset);
        }

        while (ifs && !cancelled_) {
            ifs.read(reinterpret_cast<char*>(buffer.data()), CHUNK_SIZE);
            size_t bytes_read = ifs.gcount();
            if (bytes_read == 0) break;

            if (is_zero_block(buffer.data(), bytes_read)) {
                std::lock_guard<std::mutex> lock(progress_mutex_);
                progress_.zero_blocks++;
                transferred += bytes_read;
                progress_.transferred_bytes = transferred;
                if (callback_) callback_(progress_);
                continue;
            }

            if (config_.bandwidth_limit_kbps > 0) {
                double chunk_time_sec = (double)(bytes_read) / (config_.bandwidth_limit_kbps * 1024.0);
                auto delay = std::chrono::microseconds(static_cast<int64_t>(chunk_time_sec * 1000000));
                std::this_thread::sleep_for(delay);
            } else {
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }

            transferred += bytes_read;
            {
                std::lock_guard<std::mutex> lock(progress_mutex_);
                progress_.transferred_bytes = transferred;
                if (progress_.total_bytes > 0) {
                    progress_.speed_bps = (double)transferred / 1.0;
                    int64_t remaining = progress_.total_bytes - transferred;
                    progress_.eta_seconds = progress_.speed_bps > 0
                        ? remaining / (int64_t)progress_.speed_bps : 0;
                }
                if (callback_) callback_(progress_);
            }

            if (transferred % config_.checkpoint_interval_bytes < CHUNK_SIZE) {
                save_checkpoint(transferred);
            }
        }

        save_checkpoint(0);
        std::remove(get_checkpoint_path().c_str());
        running_ = false;
        spdlog::info("DataMover: transfer complete job {}", config_.job_id);

    } catch (const std::exception& e) {
        spdlog::error("DataMover error: {}", e.what());
        running_ = false;
    }
}

} // namespace obs
