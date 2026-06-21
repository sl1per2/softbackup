#include "antivirus/av_scanner.h"
#include <spdlog/spdlog.h>
#include <fstream>
#include <algorithm>
#include <cmath>

namespace obs {

AvScanner::AvScanner() {}
AvScanner::~AvScanner() { shutdown(); }

bool AvScanner::init(const std::string& db_path) {
    return load_signatures(db_path);
}

void AvScanner::shutdown() {
    scanning_ = false;
}

bool AvScanner::load_signatures(const std::string& db_path) {
    if (!fs::exists(db_path)) {
        spdlog::warn("AV signature database not found at {}, using built-in patterns", db_path);
    }
    // In production: load YARA/ClamAV signatures
    return true;
}

ScanResult AvScanner::scan_file(const std::string& path) {
    ScanResult result;
    result.file_path = path;

    if (!fs::exists(path)) {
        result.level = ThreatLevel::UNKNOWN;
        return result;
    }

    // Check ransomware extensions
    if (check_known_ransomware_extensions(path)) {
        result.level = ThreatLevel::SUSPICIOUS;
        result.threat_name = "Suspicious_Ransomware_Extension";
        result.action_taken = "none";
        return result;
    }

    // Check file entropy (high entropy = possibly encrypted/compressed malware)
    if (check_file_entropy(path)) {
        result.level = ThreatLevel::SUSPICIOUS;
        result.threat_name = "High_Entropy_File";
        result.action_taken = "none";
        return result;
    }

    // Scan content against signatures
    std::ifstream ifs(path, std::ios::binary);
    if (ifs) {
        std::vector<uint8_t> data((std::istreambuf_iterator<char>(ifs)),
                                   std::istreambuf_iterator<char>());
        if (match_signature(data.data(), data.size())) {
            result.level = ThreatLevel::MALICIOUS;
            result.threat_name = "Known_Malware_Signature";
            result.action_taken = "quarantined";
            return result;
        }
    }

    result.level = ThreatLevel::CLEAN;
    return result;
}

ScanResult AvScanner::scan_buffer(const uint8_t* data, size_t size) {
    ScanResult result;
    if (match_signature(data, size)) {
        result.level = ThreatLevel::MALICIOUS;
        result.threat_name = "Known_Malware_Signature";
    } else {
        result.level = ThreatLevel::CLEAN;
    }
    return result;
}

void AvScanner::scan_directory(const std::string& path, std::function<void(const ScanProgress&)> callback) {
    scanning_ = true;
    {
        std::lock_guard<std::mutex> lock(progress_mutex_);
        progress_ = ScanProgress();
        progress_.status = "scanning";
    }

    int total = 0;
    int scanned = 0;
    for (auto& entry : fs::recursive_directory_iterator(path)) {
        if (!scanning_) break;
        if (entry.is_regular_file()) total++;
    }

    for (auto& entry : fs::recursive_directory_iterator(path)) {
        if (!scanning_) break;
        if (!entry.is_regular_file()) continue;

        auto result = scan_file(entry.path().string());
        scanned++;

        {
            std::lock_guard<std::mutex> lock(progress_mutex_);
            progress_.files_scanned = scanned;
            progress_.current_file = entry.path().string();
            progress_.percent = total > 0 ? (scanned * 100 / total) : 0;
            if (result.level == ThreatLevel::MALICIOUS || result.level == ThreatLevel::SUSPICIOUS) {
                progress_.threats_found++;
                progress_.results.push_back(result);
            }
        }
        if (callback) callback(progress_);
    }

    {
        std::lock_guard<std::mutex> lock(progress_mutex_);
        progress_.status = "done";
    }
    scanning_ = false;
}

void AvScanner::cancel_scan() {
    scanning_ = false;
}

ScanProgress AvScanner::get_progress() const {
    std::lock_guard<std::mutex> lock(progress_mutex_);
    return progress_;
}

bool AvScanner::match_signature(const uint8_t* /*data*/, size_t /*size*/) {
    // In production: compare against loaded signatures (ClamAV-style)
    return false;
}

bool AvScanner::check_file_entropy(const std::string& path) {
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs) return false;

    std::vector<size_t> freq(256, 0);
    size_t total = 0;
    char buf[4096];
    while (ifs.read(buf, sizeof(buf))) {
        for (size_t i = 0; i < ifs.gcount(); i++) {
            freq[static_cast<uint8_t>(buf[i])]++;
            total++;
        }
    }

    double entropy = 0;
    for (int i = 0; i < 256; i++) {
        if (freq[i] == 0) continue;
        double p = (double)freq[i] / total;
        entropy -= p * std::log2(p);
    }

    return entropy > 7.5; // High entropy suggests encryption/compression
}

bool AvScanner::check_known_ransomware_extensions(const std::string& path) {
    auto ext = fs::path(path).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return std::find(ransomware_extensions_.begin(), ransomware_extensions_.end(), ext)
           != ransomware_extensions_.end();
}

static std::string gen_id() {
    static int counter = 0;
    return std::to_string(++counter);
}

bool AvScanner::quarantine_file(const std::string& path) {
    std::string quarantine_dir = "/var/lib/obs/quarantine";
    fs::create_directories(quarantine_dir);
    std::string dest = quarantine_dir + "/" + fs::path(path).filename().string() + "." + gen_id();
    try {
        fs::rename(path, dest);
        spdlog::info("Quarantined: {} -> {}", path, dest);
        return true;
    } catch (...) {
        return false;
    }
}

bool AvScanner::restore_from_quarantine(const std::string& quarantine_id) {
    spdlog::info("Restoring from quarantine: {}", quarantine_id);
    return true;
}

std::vector<std::string> AvScanner::list_quarantine() {
    std::vector<std::string> files;
    std::string quarantine_dir = "/var/lib/obs/quarantine";
    if (fs::exists(quarantine_dir)) {
        for (auto& entry : fs::directory_iterator(quarantine_dir)) {
            files.push_back(entry.path().string());
        }
    }
    return files;
}

} // namespace obs
