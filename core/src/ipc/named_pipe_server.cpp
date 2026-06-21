#ifdef _WIN32

#include "ipc/ipc_server.h"
#include <spdlog/spdlog.h>
#include <windows.h>

namespace obs {

NamedPipeServer::NamedPipeServer(boost::asio::io_context& io_ctx, const std::string& pipe_name)
    : io_ctx_(io_ctx), pipe_name_(pipe_name) {}

NamedPipeServer::~NamedPipeServer() { stop(); }

void NamedPipeServer::start() {
    running_ = true;
    spdlog::info("IPC server (named pipe) starting on \\\\.\\pipe\\{}", pipe_name_);
    do_accept();
}

void NamedPipeServer::stop() {
    running_ = false;
}

void NamedPipeServer::register_handler(uint32_t message_type, MessageHandlerFunc handler) {
    std::lock_guard<std::mutex> lock(handlers_mutex_);
    handlers_[message_type] = std::move(handler);
}

void NamedPipeServer::do_accept() {
    if (!running_) return;

    std::string full_pipe_name = "\\\\.\\pipe\\" + pipe_name_;
    HANDLE hPipe = CreateNamedPipeA(
        full_pipe_name.c_str(),
        PIPE_ACCESS_DUPLEX,
        PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
        PIPE_UNLIMITED_INSTANCES,
        65536,
        65536,
        0,
        NULL);

    if (hPipe == INVALID_HANDLE_VALUE) {
        spdlog::error("Failed to create named pipe: {}", GetLastError());
        return;
    }

    spdlog::info("Waiting for client connection on pipe...");
    
    OVERLAPPED overlapped = {};
    overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    
    if (ConnectNamedPipe(hPipe, &overlapped)) {
        CloseHandle(overlapped.hEvent);
        handle_session(hPipe);
    } else {
        DWORD err = GetLastError();
        if (err == ERROR_IO_PENDING) {
            WaitForSingleObject(overlapped.hEvent, INFINITE);
            DWORD bytes_transferred;
            if (GetOverlappedResult(hPipe, &overlapped, &bytes_transferred, FALSE)) {
                CloseHandle(overlapped.hEvent);
                handle_session(hPipe);
            } else {
                CloseHandle(overlapped.hEvent);
                CloseHandle(hPipe);
            }
        } else if (err == ERROR_PIPE_CONNECTED) {
            CloseHandle(overlapped.hEvent);
            handle_session(hPipe);
        } else {
            spdlog::error("ConnectNamedPipe failed: {}", err);
            CloseHandle(overlapped.hEvent);
            CloseHandle(hPipe);
        }
    }
}

void NamedPipeServer::handle_session(HANDLE hPipe) {
    std::thread([this, hPipe]() {
        try {
            while (running_) {
                uint32_t msg_len = 0;
                DWORD bytes_read;
                
                if (!ReadFile(hPipe, &msg_len, 4, &bytes_read, NULL) || bytes_read != 4) {
                    break;
                }

                if (msg_len == 0 || msg_len > 64 * 1024 * 1024) break;

                std::vector<uint8_t> msg_data(msg_len);
                if (!ReadFile(hPipe, msg_data.data(), msg_len, &bytes_read, NULL) || bytes_read != msg_len) {
                    break;
                }

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
                DWORD bytes_written;
                WriteFile(hPipe, &resp_len, 4, &bytes_written, NULL);
                if (resp_len > 0) {
                    WriteFile(hPipe, response.data(), resp_len, &bytes_written, NULL);
                }
            }
        } catch (const std::exception& e) {
            spdlog::debug("IPC session ended: {}", e.what());
        }
        DisconnectNamedPipe(hPipe);
        CloseHandle(hPipe);
    }).detach();
    
    do_accept();
}

} // namespace obs

#endif
