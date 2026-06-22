#pragma once
#include "common.h"
#include "framework/component.h"
#include <thread>

namespace obs {

struct CdpConfig {
    std::string session_id;
    std::string source_path;
    int interval_ms = 5000;
    int retention_minutes = 60;
};

struct CdpRecoveryPoint {
    std::string id;
    std::string timestamp;
    uint64_t size_bytes = 0;
    bool is_consistent = false;
};

class ICdpEngine : public IComponent {
public:
    virtual std::string start_session(const CdpConfig& config) = 0;
    virtual void stop_session(const std::string& session_id) = 0;
    virtual std::vector<CdpRecoveryPoint> get_recovery_points(const std::string& session_id) = 0;
    virtual bool restore_to_point(const std::string& session_id, const std::string& point_id) = 0;
};

} // namespace obs
