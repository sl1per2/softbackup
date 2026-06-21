#include "traffic_optimizer/traffic_optimizer.h"
#include <spdlog/spdlog.h>
#include <boost/asio.hpp>
#include <cmath>

namespace obs {

TrafficOptimizer::TrafficOptimizer() {}

NetworkQuality TrafficOptimizer::measure_network_quality(const std::string& host, uint16_t port) {
    NetworkQuality quality;
    try {
        auto start = std::chrono::steady_clock::now();
        boost::asio::io_context io_ctx;
        boost::asio::ip::tcp::socket socket(io_ctx);
        boost::asio::ip::tcp::resolver resolver(io_ctx);
        auto endpoints = resolver.resolve(host, std::to_string(port));
        socket.connect(*endpoints.begin());
        auto end = std::chrono::steady_clock::now();
        quality.rtt_ms = std::chrono::duration<double, std::milli>(end - start).count();
        quality.jitter_ms = quality.rtt_ms * 0.1;
        quality.packet_loss_percent = 0;
        socket.close();
    } catch (...) {
        quality.rtt_ms = 100;
        quality.packet_loss_percent = 5;
    }
    return quality;
}

size_t TrafficOptimizer::get_adaptive_chunk_size(double rtt_ms) const {
    if (rtt_ms < 5.0) return LAN_CHUNK;
    if (rtt_ms < 50.0) return WAN_CHUNK;
    return HIGH_LATENCY_CHUNK;
}

int TrafficOptimizer::get_adaptive_stream_count(double rtt_ms, double loss_percent) const {
    if (rtt_ms < 5.0 && loss_percent < 0.1) return 8;
    if (rtt_ms < 50.0) return 5;
    return 3;
}

void TrafficOptimizer::set_qos_priority(QosPriority priority) {
    current_priority_ = priority;
    spdlog::info("TrafficOptimizer: QoS priority set to {}", static_cast<int>(priority));
}

TrafficStats TrafficOptimizer::get_stats() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return stats_;
}

} // namespace obs
