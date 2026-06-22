#pragma once
#include "common.h"
#include "framework/component.h"
#include <functional>

namespace obs {

struct IpcMessage {
    uint32_t type = 0;
    std::vector<uint8_t> payload;
};

using IpcHandler = std::function<IpcMessage(const IpcMessage&)>;

class IIpcServer : public IComponent {
public:
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual void register_handler(uint32_t message_type, IpcHandler handler) = 0;
    virtual bool is_running() const = 0;
};

} // namespace obs
