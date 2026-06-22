#pragma once
#include "common.h"
#include "ipc/i_ipc_server.h"

#ifdef _WIN32
#include <windows.h>

namespace obs {

class NamedPipeServer : public IIpcServer {
public:
    NamedPipeServer(const std::string& pipe_name);
    ~NamedPipeServer() override;

    void initialize() override;
    void shutdown() override;
    void start() override;
    void stop() override;
    void register_handler(uint32_t message_type, IpcHandler handler) override;
    bool is_running() const override { return running_.load(); }
    std::string component_name() const override { return "NamedPipeServer"; }

private:
    void do_accept();
    void handle_session(HANDLE pipe);

    std::string pipe_name_;
    HANDLE pipe_handle_ = INVALID_HANDLE_VALUE;
    std::atomic<bool> running_{false};
    std::thread accept_thread_;
    std::unordered_map<uint32_t, IpcHandler> handlers_;
    mutable std::mutex handlers_mutex_;
};

} // namespace obs

#endif
