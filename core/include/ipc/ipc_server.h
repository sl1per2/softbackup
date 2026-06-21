#pragma once
#include "common.h"
#include "ipc/unix_socket_server.h"
#include <boost/asio.hpp>
#include <functional>
#include <unordered_map>

#ifdef _WIN32
#include <boost/asio/windows/object_handle.hpp>
#endif

namespace obs {

class MessageHandler;
using MessageHandlerFunc = std::function<std::vector<uint8_t>(const std::vector<uint8_t>&)>;

#ifdef _WIN32

class NamedPipeServer {
public:
    NamedPipeServer(boost::asio::io_context& io_ctx, const std::string& pipe_name);
    ~NamedPipeServer();

    void start();
    void stop();
    void register_handler(uint32_t message_type, MessageHandlerFunc handler);

private:
    void do_accept();
    void handle_session(HANDLE pipe);

    boost::asio::io_context& io_ctx_;
    std::string pipe_name_;
    std::atomic<bool> running_{false};
    std::unordered_map<uint32_t, MessageHandlerFunc> handlers_;
    mutable std::mutex handlers_mutex_;
};

using IpcServer = NamedPipeServer;

#else

using IpcServer = UnixSocketServer;

#endif

} // namespace obs
