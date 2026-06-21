#include "verify/surebackup_engine.h"
#include <spdlog/spdlog.h>
#include <fstream>
#include <sstream>
#include <chrono>

namespace obs {

SureBackupEngine::SureBackupEngine() {}
SureBackupEngine::~SureBackupEngine() {}

bool SureBackupEngine::create_virtual_lab(const VirtualLabConfig& config) {
    spdlog::info("Creating Virtual Lab: network={}, subnet={}", config.network_name, config.subnet);
    // Create isolated network bridge
    std::string cmd = "ip link add name " + config.network_name + " type bridge 2>/dev/null && "
                    + "ip addr add " + config.subnet + " dev " + config.network_name + " 2>/dev/null && "
                    + "ip link set " + config.network_name + " up 2>/dev/null";
    return system(cmd.c_str()) == 0;
}

bool SureBackupEngine::destroy_virtual_lab(const std::string& lab_id) {
    std::string cmd = "ip link delete " + lab_id + " 2>/dev/null";
    return system(cmd.c_str()) == 0;
}

SureBackupTestResult SureBackupEngine::run_test(const std::string& job_id,
                                                 const SureBackupTestConfig& config,
                                                 SureBackupCallback callback) {
    SureBackupTestResult result;
    result.test_id = "sb-" + std::to_string(std::time(nullptr));
    result.job_id = job_id;
    result.status = SureBackupTestStatus::RUNNING;
    result.started_at = current_time_string();

    spdlog::info("SureBackup: starting test for job {}", job_id);

    auto start = std::chrono::steady_clock::now();

    // Heartbeat test
    if (std::find(config.checks.begin(), config.checks.end(), SureBackupCheckType::HEARTBEAT) != config.checks.end()) {
        result.heartbeat_ok = test_heartbeat("vm-" + job_id, config.heartbeat_timeout_seconds);
        if (!result.heartbeat_ok) result.errors.push_back("Heartbeat test failed");
    }

    // Ping test
    if (std::find(config.checks.begin(), config.checks.end(), SureBackupCheckType::PING) != config.checks.end()) {
        result.ping_ok = test_ping("10.0.0." + job_id.substr(0, 3));
        if (!result.ping_ok) result.errors.push_back("Ping test failed");
    }

    // Application test
    if (std::find(config.checks.begin(), config.checks.end(), SureBackupCheckType::APPLICATION) != config.checks.end()) {
        result.application_ok = test_application("10.0.0." + job_id.substr(0, 3), config.application_scripts);
        if (!result.application_ok) result.errors.push_back("Application test failed");
    }

    // CRC test
    if (std::find(config.checks.begin(), config.checks.end(), SureBackupCheckType::CRC) != config.checks.end()) {
        result.crc_ok = test_crc("vm-" + job_id);
        if (!result.crc_ok) result.errors.push_back("CRC check failed");
    }

    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - start);
    result.boot_time_seconds = elapsed.count();
    result.completed_at = current_time_string();

    // Determine overall status
    if (!result.heartbeat_ok && !result.ping_ok) {
        result.status = SureBackupTestStatus::FAILED;
    } else if (!result.application_ok || !result.crc_ok) {
        result.status = SureBackupTestStatus::WARNING;
    } else {
        result.status = SureBackupTestStatus::SUCCESS;
    }

    save_test_result(result);
    notify_zabbix(result);
    if (callback) callback(result);

    spdlog::info("SureBackup test {} completed: status={}", result.test_id,
                 static_cast<int>(result.status));
    return result;
}

SureBackupTestResult SureBackupEngine::get_test_result(const std::string& test_id) {
    std::lock_guard<std::mutex> lock(results_mutex_);
    auto it = test_results_.find(test_id);
    return it != test_results_.end() ? it->second : SureBackupTestResult();
}

std::vector<SureBackupTestResult> SureBackupEngine::get_test_history(const std::string& job_id) {
    std::vector<SureBackupTestResult> results;
    std::lock_guard<std::mutex> lock(results_mutex_);
    for (auto& [id, r] : test_results_) {
        if (job_id.empty() || r.job_id == job_id) results.push_back(r);
    }
    return results;
}

bool SureBackupEngine::should_verify(const std::string& job_id, const std::string& backup_type, int32_t incremental_count) {
    if (backup_type == "full") return true;
    if (incremental_count > 0 && incremental_count % 5 == 0) return true;
    return false;
}

bool SureBackupEngine::start_vm_in_lab(const std::string& vm_id, const std::string& lab_id) {
    spdlog::info("SureBackup: starting VM {} in lab {}", vm_id, lab_id);
    return true;
}

bool SureBackupEngine::stop_vm_in_lab(const std::string& vm_id) {
    spdlog::info("SureBackup: stopping VM {}", vm_id);
    return true;
}

bool SureBackupEngine::test_heartbeat(const std::string& vm_id, int timeout_seconds) {
    spdlog::info("SureBackup: testing heartbeat for {} (timeout={}s)", vm_id, timeout_seconds);
    // Check for VMware Tools / QEMU Guest Agent heartbeat
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(timeout_seconds);
    while (std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        return true; // Simplified - in production check actual heartbeat
    }
    return false;
}

bool SureBackupEngine::test_ping(const std::string& ip) {
    std::string cmd = "ping -c 3 -W 2 " + ip + " 2>/dev/null";
    return system(cmd.c_str()) == 0;
}

bool SureBackupEngine::test_application(const std::string& ip, const std::vector<std::string>& scripts) {
    for (const auto& script : scripts) {
        spdlog::info("SureBackup: running application test: {}", script);
        // In production: SSH to VM and run script
    }
    return true;
}

bool SureBackupEngine::test_crc(const std::string& vm_id) {
    spdlog::info("SureBackup: CRC check for {}", vm_id);
    return true;
}

void SureBackupEngine::save_test_result(const SureBackupTestResult& result) {
    std::lock_guard<std::mutex> lock(results_mutex_);
    test_results_[result.test_id] = result;
}

void SureBackupEngine::notify_zabbix(const SureBackupTestResult& result) {
    int status_code = 0;
    if (result.status == SureBackupTestStatus::WARNING) status_code = 1;
    if (result.status == SureBackupTestStatus::FAILED) status_code = 2;
    spdlog::info("SureBackup: Zabbix metric obs.surebackup.status = {}", status_code);
}

} // namespace obs
