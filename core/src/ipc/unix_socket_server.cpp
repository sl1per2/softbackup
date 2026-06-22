#include "ipc/unix_socket_server.h"
#include <spdlog/spdlog.h>

namespace obs {

UnixSocketServer::UnixSocketServer(boost::asio::io_context& io_ctx, const std::string& socket_path)
    : io_ctx_(io_ctx), socket_path_(socket_path) {}

UnixSocketServer::~UnixSocketServer() { shutdown(); }

void UnixSocketServer::initialize() { mark_initialized(); }

void UnixSocketServer::shutdown() { stop(); }

void UnixSocketServer::start() {
    namespace fs = std::filesystem;
    if (fs::exists(socket_path_)) {
        fs::remove(socket_path_);
    }

    acceptor_ = std::make_unique<boost::asio::local::stream_protocol::acceptor>(
        io_ctx_, boost::asio::local::stream_protocol::endpoint(socket_path_));

    running_ = true;
    do_accept();
    spdlog::info("UnixSocketServer started on {}", socket_path_);
}

void UnixSocketServer::stop() {
    running_ = false;
    if (acceptor_ && acceptor_->is_open()) {
        acceptor_->close();
    }
    spdlog::info("UnixSocketServer stopped");
}

void UnixSocketServer::register_handler(uint32_t message_type, IpcHandler handler) {
    std::lock_guard<std::mutex> lock(handlers_mutex_);
    handlers_[message_type] = std::move(handler);
    spdlog::debug("UnixSocketServer: registered handler for type {}", message_type);
}

void UnixSocketServer::do_accept() {
    if (!running_) return;

    auto socket = std::make_shared<boost::asio::local::stream_protocol::socket>(io_ctx_);
    acceptor_->async_accept(*socket, [this, socket](const boost::system::error_code& ec) {
        if (!ec && running_) {
            handle_session(socket);
        }
        do_accept();
    });
}

void UnixSocketServer::handle_session(
    std::shared_ptr<boost::asio::local::stream_protocol::socket> socket) {
    try {
        // Read length prefix (4 bytes)
        uint32_t msg_len = 0;
        boost::asio::read(*socket, boost::asio::buffer(&msg_len, 4));

        // Read message body
        std::vector<uint8_t> data(msg_len);
        boost::asio::read(*socket, boost::asio::buffer(data.data(), msg_len));

        // Extract message type (first 4 bytes of payload)
        if (msg_len < 4) {
            spdlog::warn("UnixSocketServer: message too short");
            return;
        }
        uint32_t msg_type;
        std::memcpy(&msg_type, data.data(), 4);

        IpcMessage request;
        request.type = msg_type;
        request.payload.assign(data.begin() + 4, data.end());

        // Dispatch to handler
        IpcMessage response;
        {
            std::lock_guard<std::mutex> lock(handlers_mutex_);
            auto it = handlers_.find(msg_type);
            if (it != handlers_.end()) {
                response = it->second(request);
            } else {
                response.type = 0;
                response.payload = {0, 0, 0, 0}; // error
            }
        }

        // Send response: 4-byte length + payload
        uint32_t resp_len = response.payload.size() + 4;
        std::vector<uint8_t> resp_buf(4 + resp_len);
        std::memcpy(resp_buf.data(), &resp_len, 4);
        std::memcpy(resp_buf.data() + 4, &response.type, 4);
        std::memcpy(resp_buf.data() + 8, response.payload.data(), response.payload.size());
        boost::asio::write(*socket, boost::asio::buffer(resp_buf.data(), resp_buf.size()));

    } catch (const std::exception& e) {
        spdlog::error("UnixSocketServer session error: {}", e.what());
    }
}

} // namespace obs
