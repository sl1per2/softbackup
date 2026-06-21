#pragma once
#include "common.h"
#include <boost/asio.hpp>
#include <functional>

namespace obs {

class TcpStream {
public:
    TcpStream(boost::asio::io_context& io_ctx);
    ~TcpStream();

    bool connect(const std::string& host, uint16_t port);
    bool send(const uint8_t* data, size_t size);
    bool receive(uint8_t* data, size_t size);
    void close();
    bool is_connected() const;

private:
    boost::asio::ip::tcp::socket socket_;
    bool connected_ = false;
};

class TcpMultiStream {
public:
    TcpMultiStream(boost::asio::io_context& io_ctx, int stream_count = 5);
    ~TcpMultiStream();

    bool connect_all(const std::string& host, uint16_t port);
    bool send_chunk(const uint8_t* data, size_t size);
    void close_all();
    void set_bandwidth_limit(int kbps);
    size_t get_stream_count() const { return streams_.size(); }

private:
    std::vector<std::unique_ptr<TcpStream>> streams_;
    std::atomic<size_t> next_stream_{0};
    int bandwidth_limit_kbps_ = 0;
    std::chrono::steady_clock::time_point last_send_;
};

} // namespace obs
