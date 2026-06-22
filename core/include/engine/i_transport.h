#pragma once
#include "common.h"
#include "framework/component.h"

namespace obs {

enum class TransportMode { AUTO, DIRECT_SAN, HOT_ADD, NETWORK, NBD };

struct TransferRequest {
    std::string job_id;
    std::string source_path;
    std::string destination;
    TransportMode mode = TransportMode::AUTO;
    int bandwidth_limit_kbps = 0;
    int stream_count = 1;
};

struct TransferProgress {
    uint64_t total_bytes = 0;
    uint64_t transferred_bytes = 0;
    double speed_mbps = 0.0;
    int eta_seconds = 0;
};

using TransferProgressCallback = std::function<void(const TransferProgress&)>;

class ITransportEngine : public IComponent {
public:
    virtual bool transfer(const TransferRequest& request, TransferProgressCallback callback = nullptr) = 0;
    virtual bool cancel(const std::string& job_id) = 0;
    virtual TransportMode detect_best_mode(const std::string& source, const std::string& dest) = 0;
};

} // namespace obs
