#pragma once
#include "common.h"
#include <boost/asio.hpp>
#include <functional>
#include <unordered_map>

namespace obs {

class MessageHandler;
using MessageHandlerFunc = std::function<std::vector<uint8_t>(const std::vector<uint8_t>&)>;

class UnixSocketServer {
public:
    UnixSocketServer(boost::asio::io_context& io_ctx, const std::string& socket_path);
    ~UnixSocketServer();

    void start();
    void stop();
    void register_handler(uint32_t message_type, MessageHandlerFunc handler);

private:
    void do_accept();
    void handle_session(std::shared_ptr<boost::asio::local::stream_protocol::socket> socket);

    boost::asio::io_context& io_ctx_;
    boost::asio::local::stream_protocol::acceptor acceptor_;
    std::string socket_path_;
    std::atomic<bool> running_{false};
    std::unordered_map<uint32_t, MessageHandlerFunc> handlers_;
    mutable std::mutex handlers_mutex_;
};

} // namespace obs
