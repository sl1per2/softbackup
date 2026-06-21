#include "data_mover/tcp_stream.h"
#include <spdlog/spdlog.h>

namespace obs {

TcpStream::TcpStream(boost::asio::io_context& io_ctx) : socket_(io_ctx) {}

TcpStream::~TcpStream() { close(); }

bool TcpStream::connect(const std::string& host, uint16_t port) {
    try {
        boost::asio::ip::tcp::resolver resolver(socket_.get_executor());
        auto endpoints = resolver.resolve(host, std::to_string(port));
        boost::asio::connect(socket_, endpoints);
        socket_.set_option(boost::asio::ip::tcp::no_delay(true));
        connected_ = true;
        return true;
    } catch (const std::exception& e) {
        spdlog::error("TcpStream connect failed: {}", e.what());
        connected_ = false;
        return false;
    }
}

bool TcpStream::send(const uint8_t* data, size_t size) {
    try {
        size_t total_sent = 0;
        while (total_sent < size) {
            auto sent = socket_.send(
                boost::asio::buffer(data + total_sent, size - total_sent));
            total_sent += sent;
        }
        return true;
    } catch (const std::exception& e) {
        spdlog::error("TcpStream send failed: {}", e.what());
        connected_ = false;
        return false;
    }
}

bool TcpStream::receive(uint8_t* data, size_t size) {
    try {
        boost::asio::read(socket_, boost::asio::buffer(data, size));
        return true;
    } catch (const std::exception& e) {
        spdlog::error("TcpStream receive failed: {}", e.what());
        connected_ = false;
        return false;
    }
}

void TcpStream::close() {
    if (connected_) {
        boost::system::error_code ec;
        socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
        socket_.close(ec);
        connected_ = false;
    }
}

bool TcpStream::is_connected() const { return connected_; }

// TcpMultiStream

TcpMultiStream::TcpMultiStream(boost::asio::io_context& io_ctx, int stream_count) {
    for (int i = 0; i < stream_count; i++) {
        streams_.push_back(std::make_unique<TcpStream>(io_ctx));
    }
}

TcpMultiStream::~TcpMultiStream() { close_all(); }

bool TcpMultiStream::connect_all(const std::string& host, uint16_t port) {
    for (auto& s : streams_) {
        if (!s->connect(host, port)) return false;
    }
    return true;
}

bool TcpMultiStream::send_chunk(const uint8_t* data, size_t size) {
    size_t idx = next_stream_.fetch_add(1) % streams_.size();
    return streams_[idx]->send(data, size);
}

void TcpMultiStream::close_all() {
    for (auto& s : streams_) s->close();
}

void TcpMultiStream::set_bandwidth_limit(int kbps) {
    bandwidth_limit_kbps_ = kbps;
}

} // namespace obs
