#pragma once
#include "common.h"

namespace obs {

struct NetworkQuality {
    double rtt_ms = 0;
    double jitter_ms = 0;
    double packet_loss_percent = 0;
};

enum class QosPriority { CDP = 0, BACKUP = 1, LOW = 2 };

struct TrafficStats {
    uint64_t bytes_sent = 0;
    double throughput_mbps = 0;
    double efficiency_ratio = 0;
    int32_t active_streams = 0;
};

class TrafficOptimizer {
public:
    TrafficOptimizer();

    NetworkQuality measure_network_quality(const std::string& host, uint16_t port);
    size_t get_adaptive_chunk_size(double rtt_ms) const;
    int get_adaptive_stream_count(double rtt_ms, double loss_percent) const;
    void set_qos_priority(QosPriority priority);
    QosPriority get_qos_priority() const { return current_priority_; }
    TrafficStats get_stats() const;

    static constexpr size_t LAN_CHUNK = 4 * 1024 * 1024;    // 4MB
    static constexpr size_t WAN_CHUNK = 1 * 1024 * 1024;     // 1MB
    static constexpr size_t HIGH_LATENCY_CHUNK = 256 * 1024; // 256KB

private:
    QosPriority current_priority_ = QosPriority::BACKUP;
    mutable std::mutex stats_mutex_;
    TrafficStats stats_;
};

} // namespace obs
