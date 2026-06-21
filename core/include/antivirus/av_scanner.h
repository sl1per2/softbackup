#pragma once
#include "common.h"
#include <thread>
#include <atomic>

namespace obs {

enum class ThreatLevel { CLEAN, SUSPICIOUS, MALICIOUS, UNKNOWN };

struct ScanResult {
    ThreatLevel level = ThreatLevel::CLEAN;
    std::string threat_name;
    std::string file_path;
    std::string action_taken; // "cleaned", "quarantined", "deleted", "none"
};

struct ScanProgress {
    std::string status;
    int percent = 0;
    int files_scanned = 0;
    int threats_found = 0;
    std::string current_file;
    std::vector<ScanResult> results;
};

class AvScanner {
public:
    AvScanner();
    ~AvScanner();

    bool init(const std::string& db_path = "/var/lib/obs/signatures");
    void shutdown();
    ScanResult scan_file(const std::string& path);
    ScanResult scan_buffer(const uint8_t* data, size_t size);
    void scan_directory(const std::string& path, std::function<void(const ScanProgress&)> callback);
    void cancel_scan();
    ScanProgress get_progress() const;

    // Quarantine management
    bool quarantine_file(const std::string& path);
    bool restore_from_quarantine(const std::string& quarantine_id);
    std::vector<std::string> list_quarantine();

private:
    bool load_signatures(const std::string& db_path);
    bool match_signature(const uint8_t* data, size_t size);
    bool check_file_entropy(const std::string& path);
    bool check_known_ransomware_extensions(const std::string& path);

    std::atomic<bool> scanning_{false};
    ScanProgress progress_;
    mutable std::mutex progress_mutex_;
    std::vector<std::vector<uint8_t>> signatures_;
    std::vector<std::string> ransomware_extensions_ = {
        ".encrypted", ".locked", ".crypto", ".crypt", ".crypted",
        ".wannacry", ".petya", ".ryuk", ".lockbit", ".conti",
        ".revil", ".maze", ".darkside", ".blackcat"
    };
};

} // namespace obs
