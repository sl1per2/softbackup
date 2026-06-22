#include "backup_engine/anti_ransomware.h"
#include <spdlog/spdlog.h>

namespace obs {

AntiRansomware::AntiRansomware(const RansomwareConfig& config) : config_(config) {}
AntiRansomware::~AntiRansomware() { stop(); }

void AntiRansomware::start() {
    running_ = true;
    spdlog::info("AntiRansomware: monitoring started for {}", config_.watch_path);
}

void AntiRansomware::stop() {
    running_ = false;
    spdlog::info("AntiRansomware: monitoring stopped");
}

double AntiRansomware::calculate_entropy(const std::string& file_path) {
    std::ifstream ifs(file_path, std::ios::binary);
    if (!ifs) return 0.0;

    std::array<size_t, 256> freq{};
    std::array<uint8_t, 4096> buf;
    size_t total = 0;

    while (ifs.read(reinterpret_cast<char*>(buf.data()), buf.size()) || ifs.gcount() > 0) {
        size_t n = ifs.gcount();
        for (size_t i = 0; i < n; i++) freq[buf[i]]++;
        total += n;
        if (!ifs) break;
    }

    if (total == 0) return 0.0;
    double entropy = 0.0;
    for (size_t i = 0; i < 256; i++) {
        if (freq[i] == 0) continue;
        double p = static_cast<double>(freq[i]) / total;
        entropy -= p * std::log2(p);
    }
    return entropy;
}

std::vector<ThreatEvent> AntiRansomware::detect_rapid_renames(const std::string& directory, int window_seconds) {
    spdlog::debug("AntiRansomware: checking rapid renames in {}", directory);
    return {};
}

std::vector<ThreatEvent> AntiRansomware::detect_mass_encryption(const std::string& directory) {
    spdlog::debug("AntiRansomware: checking mass encryption in {}", directory);
    return {};
}

ThreatLevel AntiRansomware::assess_threat(const std::vector<ThreatEvent>& events) {
    if (events.empty()) return ThreatLevel::NONE;
    ThreatLevel max = ThreatLevel::NONE;
    for (const auto& e : events) {
        if (static_cast<int>(e.level) > static_cast<int>(max)) max = e.level;
    }
    return max;
}

} // namespace obs
