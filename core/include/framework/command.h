#pragma once
#include "common.h"

namespace obs {

struct CommandResult {
    bool success = false;
    std::string error_message;
    std::map<std::string, std::string> metadata;
};

class ICommand {
public:
    virtual ~ICommand() = default;
    virtual CommandResult execute() = 0;
    virtual std::string command_name() const = 0;
    virtual bool can_undo() const { return false; }
    virtual CommandResult undo() { return {}; }
};

class CommandQueue {
public:
    void enqueue(std::shared_ptr<ICommand> cmd) {
        std::lock_guard<std::mutex> lock(mutex_);
        commands_.push_back(std::move(cmd));
    }

    std::shared_ptr<ICommand> dequeue() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (commands_.empty()) return nullptr;
        auto cmd = std::move(commands_.front());
        commands_.erase(commands_.begin());
        return cmd;
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return commands_.size();
    }

    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        commands_.clear();
    }

private:
    mutable std::mutex mutex_;
    std::vector<std::shared_ptr<ICommand>> commands_;
};

} // namespace obs
