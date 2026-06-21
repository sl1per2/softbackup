#include "ipc/unix_socket_server.h"
#include <spdlog/spdlog.h>

namespace obs {

UnixSocketServer::UnixSocketServer(boost::asio::io_context& io_ctx, const std::string& socket_path)
    : io_ctx_(io_ctx),
      acceptor_(io_ctx, boost::asio::local::stream_protocol::endpoint(socket_path)),
      socket_path_(socket_path) {
    std::filesystem::remove(socket_path);
}

UnixSocketServer::~UnixSocketServer() { stop(); }

void UnixSocketServer::start() {
    running_ = true;
    do_accept();
    spdlog::info("IPC server started on {}", socket_path_);
}

void UnixSocketServer::stop() {
    running_ = false;
    boost::system::error_code ec;
    acceptor_.close(ec);
    std::filesystem::remove(socket_path_);
}

void UnixSocketServer::register_handler(uint32_t message_type, MessageHandlerFunc handler) {
    std::lock_guard<std::mutex> lock(handlers_mutex_);
    handlers_[message_type] = std::move(handler);
}

void UnixSocketServer::do_accept() {
    if (!running_) return;
    acceptor_.async_accept([this](boost::system::error_code ec,
                                  boost::asio::local::stream_protocol::socket socket) {
        if (!ec && running_) {
            auto sock = std::make_shared<boost::asio::local::stream_protocol::socket>(std::move(socket));
            handle_session(sock);
        }
        do_accept();
    });
}

void UnixSocketServer::handle_session(std::shared_ptr<boost::asio::local::stream_protocol::socket> socket) {
    std::thread([this, socket]() {
        try {
            while (socket->is_open()) {
                uint32_t msg_len = 0;
                boost::asio::read(*socket, boost::asio::buffer(&msg_len, 4));

                if (msg_len == 0 || msg_len > 64 * 1024 * 1024) break;

                std::vector<uint8_t> msg_data(msg_len);
                boost::asio::read(*socket, boost::asio::buffer(msg_data.data(), msg_len));

                uint32_t msg_type = 0;
                if (msg_data.size() >= 4) {
                    msg_type = msg_data[0];
                }

                std::vector<uint8_t> response;
                {
                    std::lock_guard<std::mutex> lock(handlers_mutex_);
                    auto it = handlers_.find(msg_type);
                    if (it != handlers_.end()) {
                        response = it->second(msg_data);
                    } else {
                        response.resize(4, 0);
                    }
                }

                uint32_t resp_len = response.size();
                boost::asio::write(*socket, boost::asio::buffer(&resp_len, 4));
                if (resp_len > 0) {
                    boost::asio::write(*socket, boost::asio::buffer(response.data(), resp_len));
                }
            }
        } catch (const std::exception& e) {
            spdlog::debug("IPC session ended: {}", e.what());
        }
    }).detach();
}

} // namespace obs
