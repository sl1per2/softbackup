#pragma once
#include "common.h"
#include <thread>
#include <atomic>

namespace obs {

struct RansomwareConfig {
    std::string watch_path;
    double entropy_threshold = 7.5;    // High entropy = encrypted data
    int rapid_change_threshold = 10;   // Files changed per second
    std::string alert_command;          // Command to run on detection
};

enum class ThreatLevel { NONE, LOW, MEDIUM, HIGH, CRITICAL };

struct ThreatEvent {
    ThreatLevel level;
    std::string description;
    std::string file_path;
    std::string timestamp;
    double entropy;
    int files_changed_per_sec;
};

using ThreatCallback = std::function<void(const ThreatEvent&)>;

class AntiRansomware {
public:
    AntiRansomware();
    ~AntiRansomware();

    void start_monitoring(const RansomwareConfig& config);
    void stop_monitoring();
    void set_callback(ThreatCallback cb) { callback_ = std::move(cb); }

private:
    void monitor_loop();
    double calculate_entropy(const std::string& file_path);
    bool detect_rapid_renames();
    bool detect_mass_encryption();
    ThreatLevel assess_threat(double entropy, int changes_per_sec);

    RansomwareConfig config_;
    std::atomic<bool> running_{false};
    std::thread monitor_thread_;
    ThreatCallback callback_;

    // Tracking state
    std::map<std::string, TimePoint> recent_changes_;
    std::map<std::string, double> file_entropies_;
    mutable std::mutex state_mutex_;
};

} // namespace obs
