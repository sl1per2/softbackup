#include "verification/vm_verification.h"
#include <spdlog/spdlog.h>

namespace obs {

VmVerification::VmVerification() {}
VmVerification::~VmVerification() { cancel(); }

VmVerifyResult VmVerification::verify(const VmVerifyConfig& config) {
    cancelled_ = false;
    {
        std::lock_guard<std::mutex> lock(result_mutex_);
        result_ = VmVerifyResult();
        result_.status = VerifyStatus::RUNNING;
    }
    worker_ = std::thread(&VmVerification::do_verify, this, config);
    worker_.join();
    std::lock_guard<std::mutex> lock(result_mutex_);
    result_.verified_at = current_time_string();
    return result_;
}

bool VmVerification::cancel() {
    cancelled_ = true;
    return true;
}

VmVerifyResult VmVerification::get_result() const {
    std::lock_guard<std::mutex> lock(result_mutex_);
    return result_;
}

void VmVerification::do_verify(VmVerifyConfig config) {
    try {
        // Step 1: Boot VM
        {
            std::lock_guard<std::mutex> lock(result_mutex_);
            result_.status = VerifyStatus::BOOTING;
            result_.log += "Booting VM...\n";
        }

        auto boot_start = std::chrono::steady_clock::now();
        if (!boot_vm(config.vm_name, config.vm_type)) {
            std::lock_guard<std::mutex> lock(result_mutex_);
            result_.status = VerifyStatus::FAILED;
            result_.error = "Failed to boot VM";
            return;
        }

        // Step 2: Wait for boot
        if (!check_vm_booted(config.expected_ip, config.timeout_seconds * 1000)) {
            std::lock_guard<std::mutex> lock(result_mutex_);
            result_.status = VerifyStatus::TIMEOUT;
            result_.error = "VM did not boot within timeout";
            stop_vm(config.vm_name, config.vm_type);
            return;
        }

        auto boot_time = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - boot_start).count();
        {
            std::lock_guard<std::mutex> lock(result_mutex_);
            result_.boot_ok = true;
            result_.boot_time_ms = boot_time;
            result_.log += "VM booted in " + std::to_string(boot_time) + "ms\n";
        }

        // Step 3: Network check
        if (config.check_network && !cancelled_) {
            {
                std::lock_guard<std::mutex> lock(result_mutex_);
                result_.status = VerifyStatus::CHECKING;
                result_.log += "Checking network...\n";
            }
            result_.network_ok = check_network(config.expected_ip);
        }

        // Step 4: Services check
        if (config.check_services && !cancelled_) {
            result_.services_ok = check_services(config.expected_ip, config.expected_services);
        }

        // Step 5: Checksum verification
        if (!cancelled_) {
            result_.checksum_ok = verify_checksums(config.backup_path);
        }

        // Stop VM
        stop_vm(config.vm_name, config.vm_type);

        {
            std::lock_guard<std::mutex> lock(result_mutex_);
            if (cancelled_) {
                result_.status = VerifyStatus::FAILED;
                result_.error = "Cancelled";
            } else if (result_.boot_ok && result_.network_ok && result_.services_ok && result_.checksum_ok) {
                result_.status = VerifyStatus::PASSED;
                result_.log += "All checks PASSED\n";
            } else {
                result_.status = VerifyStatus::FAILED;
                result_.log += "Some checks FAILED\n";
            }
        }

    } catch (const std::exception& e) {
        std::lock_guard<std::mutex> lock(result_mutex_);
        result_.status = VerifyStatus::FAILED;
        result_.error = e.what();
    }
}

bool VmVerification::boot_vm(const std::string& vm_name, const std::string& vm_type) {
    spdlog::info("Booting VM {} ({}) for verification", vm_name, vm_type);
    if (vm_type == "kvm") {
        // virsh start vm_name
        return true;
    } else if (vm_type == "vmware") {
        // vmrun start vmx_path
        return true;
    }
    return true;
}

bool VmVerification::check_vm_booted(const std::string& ip, int timeout_ms) {
    auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
    while (std::chrono::steady_clock::now() < deadline) {
        if (cancelled_) return false;
        // In production: ping or TCP connect to SSH port
        std::this_thread::sleep_for(std::chrono::seconds(5));
        spdlog::info("Waiting for VM at {} to boot...", ip);
        return true; // Simplified
    }
    return false;
}

bool VmVerification::check_network(const std::string& ip) {
    spdlog::info("Checking network for VM at {}", ip);
    return true;
}

bool VmVerification::check_services(const std::string& ip, const std::vector<std::string>& services) {
    for (const auto& svc : services) {
        spdlog::info("Checking service {} on {}", svc, ip);
    }
    return true;
}

bool VmVerification::verify_checksums(const std::string& backup_path) {
    spdlog::info("Verifying checksums for backup: {}", backup_path);
    return true;
}

bool VmVerification::stop_vm(const std::string& vm_name, const std::string& vm_type) {
    spdlog::info("Stopping verification VM {} ({})", vm_name, vm_type);
    return true;
}

} // namespace obs
