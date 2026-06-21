#pragma once
#include "common.h"
#include <thread>
#include <atomic>

namespace obs {

struct VmVerifyConfig {
    std::string backup_path;
    std::string vm_name;
    std::string vm_type; // "kvm", "vmware", "hyperv"
    int timeout_seconds = 300;
    bool check_boot = true;
    bool check_network = true;
    bool check_services = true;
    std::string expected_ip;
    std::vector<std::string> expected_services; // ssh, http, etc.
};

enum class VerifyStatus { PENDING, RUNNING, BOOTING, CHECKING, PASSED, FAILED, TIMEOUT };

struct VmVerifyResult {
    VerifyStatus status = VerifyStatus::PENDING;
    bool boot_ok = false;
    bool network_ok = false;
    bool services_ok = false;
    bool checksum_ok = false;
    std::string log;
    std::string error;
    int64_t boot_time_ms = 0;
    std::string verified_at;
};

class VmVerification {
public:
    VmVerification();
    ~VmVerification();

    VmVerifyResult verify(const VmVerifyConfig& config);
    bool cancel();
    VmVerifyResult get_result() const;

private:
    void do_verify(VmVerifyConfig config);
    bool boot_vm(const std::string& vm_name, const std::string& vm_type);
    bool check_vm_booted(const std::string& ip, int timeout_ms);
    bool check_network(const std::string& ip);
    bool check_services(const std::string& ip, const std::vector<std::string>& services);
    bool verify_checksums(const std::string& backup_path);
    bool stop_vm(const std::string& vm_name, const std::string& vm_type);

    std::atomic<bool> cancelled_{false};
    VmVerifyResult result_;
    std::thread worker_;
    mutable std::mutex result_mutex_;
};

} // namespace obs
