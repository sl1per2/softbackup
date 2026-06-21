#pragma once
#include "common.h"
#include <functional>
#include <thread>
#include <atomic>

namespace obs {

enum class TransportMode { AUTO, DIRECT_SAN, HOT_ADD, NETWORK, NBD };

struct TransferConfig {
    std::string job_id;
    std::string source_path;
    int dest_storage_id = 0;
    TransportMode transport_mode = TransportMode::AUTO;
    int compression_level = 1;
    bool dedup_enabled = true;
    std::string encryption_key_id;
    int bandwidth_limit_kbps = 0;
    int streams_count = 5;
    int checkpoint_interval_bytes = 100 * 1024 * 1024; // 100MB
};

struct TransferProgress {
    uint64_t total_bytes = 0;
    uint64_t transferred_bytes = 0;
    double speed_bps = 0;
    int64_t eta_seconds = 0;
    int32_t cache_hits = 0;
    int32_t zero_blocks = 0;
};

using ProgressCallback = std::function<void(const TransferProgress&)>;

class DataMover {
public:
    DataMover();
    ~DataMover();

    void configure(const TransferConfig& config);
    void start_transfer();
    void cancel_transfer();
    TransferProgress get_progress() const;
    void set_progress_callback(ProgressCallback cb) { callback_ = std::move(cb); }

    static TransportMode detect_transport_mode();

private:
    void do_transfer();
    bool is_zero_block(const uint8_t* data, size_t size) const;
    std::string get_checkpoint_path() const;
    void save_checkpoint(uint64_t offset);
    uint64_t load_checkpoint() const;

    TransferConfig config_;
    std::atomic<bool> cancelled_{false};
    std::atomic<bool> running_{false};
    TransferProgress progress_;
    mutable std::mutex progress_mutex_;
    ProgressCallback callback_;
    std::thread worker_;
};

} // namespace obs
