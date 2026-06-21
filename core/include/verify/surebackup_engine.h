#pragma once
#include "common.h"
#include <functional>
#include <vector>
#include <string>
#include <thread>

namespace obs {

enum class SureBackupTestStatus { PENDING, RUNNING, SUCCESS, WARNING, FAILED, SKIPPED };
enum class SureBackupCheckType { HEARTBEAT, PING, APPLICATION, CRC, BOOT_TIME, CUSTOM_SCRIPT };

struct VirtualLabConfig {
    std::string network_name;
    std::string subnet;
    std::string gateway;
    std::string dns_server;
    std::string dhcp_range_start;
    std::string dhcp_range_end;
};

struct SureBackupTestConfig {
    std::vector<SureBackupCheckType> checks;
    int boot_timeout_seconds = 300;
    int heartbeat_timeout_seconds = 120;
    std::vector<std::string> application_scripts;
    bool verify_after_full = true;
    bool verify_after_n_incrementals = 5;
    bool verify_weekly = true;
};

struct SureBackupTestResult {
    std::string test_id;
    std::string job_id;
    std::string vm_id;
    SureBackupTestStatus status = SureBackupTestStatus::PENDING;
    std::string started_at;
    std::string completed_at;
    int boot_time_seconds = 0;
    bool heartbeat_ok = false;
    bool ping_ok = false;
    bool application_ok = false;
    bool crc_ok = false;
    std::vector<std::string> errors;
    std::string error;
};

using SureBackupCallback = std::function<void(const SureBackupTestResult&)>;

class SureBackupEngine {
public:
    SureBackupEngine();
    ~SureBackupEngine();

    bool create_virtual_lab(const VirtualLabConfig& config);
    bool destroy_virtual_lab(const std::string& lab_id);

    SureBackupTestResult run_test(const std::string& job_id, const SureBackupTestConfig& config,
                                  SureBackupCallback callback = nullptr);
    SureBackupTestResult get_test_result(const std::string& test_id);
    std::vector<SureBackupTestResult> get_test_history(const std::string& job_id = "");

    bool should_verify(const std::string& job_id, const std::string& backup_type, int32_t incremental_count);

private:
    bool start_vm_in_lab(const std::string& vm_id, const std::string& lab_id);
    bool stop_vm_in_lab(const std::string& vm_id);
    bool test_heartbeat(const std::string& vm_id, int timeout_seconds);
    bool test_ping(const std::string& ip);
    bool test_application(const std::string& ip, const std::vector<std::string>& scripts);
    bool test_crc(const std::string& vm_id);
    void save_test_result(const SureBackupTestResult& result);
    void notify_zabbix(const SureBackupTestResult& result);

    std::map<std::string, SureBackupTestResult> test_results_;
    mutable std::mutex results_mutex_;
};

} // namespace obs
