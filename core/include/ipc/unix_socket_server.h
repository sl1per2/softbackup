#pragma once
#include "common.h"
#include "ipc/i_ipc_server.h"
#include <boost/asio.hpp>
#include <thread>

namespace obs {

class UnixSocketServer : public IIpcServer {
public:
    UnixSocketServer(boost::asio::io_context& io_ctx, const std::string& socket_path);
    ~UnixSocketServer() override;

    void initialize() override;
    void shutdown() override;
    void start() override;
    void stop() override;
    void register_handler(uint32_t message_type, IpcHandler handler) override;
    bool is_running() const override { return running_.load(); }
    std::string component_name() const override { return "UnixSocketServer"; }

private:
    void do_accept();
    void handle_session(std::shared_ptr<boost::asio::local::stream_protocol::socket> socket);

    boost::asio::io_context& io_ctx_;
    std::string socket_path_;
    std::unique_ptr<boost::asio::local::stream_protocol::acceptor> acceptor_;
    std::atomic<bool> running_{false};
    std::unordered_map<uint32_t, IpcHandler> handlers_;
    mutable std::mutex handlers_mutex_;
};

} // namespace obs
