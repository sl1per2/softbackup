#pragma once
#include "common.h"
#include <functional>
#include <typeindex>
#include <vector>
#include <any>

namespace obs {

struct JobStartedEvent { std::string job_id; std::string agent_id; };
struct JobProgressEvent { std::string job_id; double progress; uint64_t bytes_transferred; };
struct JobCompletedEvent { std::string job_id; bool success; std::string error; };
struct AgentConnectedEvent { std::string agent_id; std::string hostname; };
struct AgentDisconnectedEvent { std::string agent_id; };
struct CdpSessionStartedEvent { std::string session_id; };
struct CdpSessionStoppedEvent { std::string session_id; };
struct ThreatDetectedEvent { std::string file_path; int severity; std::string description; };
struct ReplicationCompletedEvent { std::string source_id; std::string dest_id; uint64_t bytes_replicated; };

class EventBus {
public:
    template<typename Event>
    using Handler = std::function<void(const Event&)>;

    template<typename Event>
    void subscribe(Handler<Event> handler) {
        auto wrapper = [h = std::move(handler)](const std::any& e) {
            h(std::any_cast<const Event&>(e));
        };
        handlers_[std::type_index(typeid(Event))].push_back(std::move(wrapper));
    }

    template<typename Event>
    void publish(const Event& event) {
        auto it = handlers_.find(std::type_index(typeid(Event)));
        if (it != handlers_.end()) {
            for (auto& handler : it->second) {
                handler(event);
            }
        }
    }

    void clear() { handlers_.clear(); }

private:
    using GenericHandler = std::function<void(const std::any&)>;
    std::unordered_map<std::type_index, std::vector<GenericHandler>> handlers_;
};

} // namespace obs
