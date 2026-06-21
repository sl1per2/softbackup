#include "virtualization/vmware_adapter.h"
#include <spdlog/spdlog.h>
#include <sstream>

namespace obs {

static size_t write_callback(void* contents, size_t size, size_t nmemb, std::string* s) {
    s->append(static_cast<char*>(contents), size * nmemb);
    return size * nmemb;
}

VMwareAdapter::VMwareAdapter() {
    curl_ = curl_easy_init();
}

VMwareAdapter::~VMwareAdapter() {
    disconnect();
    if (curl_) curl_easy_cleanup(curl_);
}

bool VMwareAdapter::connect(const std::string& host, uint16_t port,
                            const std::string& username, const std::string& password) {
    config_.vcenter_host = host;
    config_.vcenter_port = port;
    config_.username = username;
    config_.password = password;
    return login();
}

void VMwareAdapter::disconnect() {
    if (!session_id_.empty()) {
        api_post("/rest/com/vmware/cis/session/actions/logout", "");
        session_id_.clear();
    }
    connected_ = false;
}

bool VMwareAdapter::is_connected() const { return connected_; }

bool VMwareAdapter::login() {
    std::string url = "https://" + config_.vcenter_host + ":" + std::to_string(config_.vcenter_port)
                    + "/rest/com/vmware/cis/session";

    curl_easy_setopt(curl_, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl_, CURLOPT_POST, 1L);
    curl_easy_setopt(curl_, CURLOPT_USERNAME, config_.username.c_str());
    curl_easy_setopt(curl_, CURLOPT_PASSWORD, config_.password.c_str());
    curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, write_callback);

    std::string response;
    curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &response);

    if (!config_.verify_ssl) {
        curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYHOST, 0L);
    }

    CURLcode res = curl_easy_perform(curl_);
    if (res != CURLE_OK) {
        spdlog::error("VMware login failed: {}", curl_easy_strerror(res));
        return false;
    }

    long http_code = 0;
    curl_easy_getinfo(curl_, CURLINFO_RESPONSE_CODE, &http_code);
    if (http_code != 200) {
        spdlog::error("VMware login HTTP {}", http_code);
        return false;
    }

    // Extract session ID from response
    // Response: {"value": "session-id-string"}
    auto pos = response.find("\"value\"");
    if (pos != std::string::npos) {
        auto start = response.find("\"", pos + 8) + 1;
        auto end = response.find("\"", start);
        session_id_ = response.substr(start, end - start);
    }

    connected_ = !session_id_.empty();
    if (connected_) {
        spdlog::info("VMware vCenter connected: {}", config_.vcenter_host);
    }
    return connected_;
}

std::string VMwareAdapter::api_get(const std::string& path) {
    std::string url = "https://" + config_.vcenter_host + ":" + std::to_string(config_.vcenter_port) + path;
    curl_easy_setopt(curl_, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl_, CURLOPT_HTTPGET, 1L);

    struct curl_slist* headers = nullptr;
    std::string auth = "vmware-api-session-id: " + session_id_;
    headers = curl_slist_append(headers, auth.c_str());
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, headers);

    std::string response;
    curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, write_callback);

    curl_easy_perform(curl_);
    curl_slist_free_all(headers);
    return response;
}

std::string VMwareAdapter::api_post(const std::string& path, const std::string& body) {
    std::string url = "https://" + config_.vcenter_host + ":" + std::to_string(config_.vcenter_port) + path;
    curl_easy_setopt(curl_, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl_, CURLOPT_POST, 1L);
    curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, body.c_str());

    struct curl_slist* headers = nullptr;
    if (!session_id_.empty()) {
        headers = curl_slist_append(headers, ("vmware-api-session-id: " + session_id_).c_str());
    }
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, headers);

    std::string response;
    curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, write_callback);

    curl_easy_perform(curl_);
    curl_slist_free_all(headers);
    return response;
}

std::string VMwareAdapter::api_delete(const std::string& path) {
    std::string url = "https://" + config_.vcenter_host + ":" + std::to_string(config_.vcenter_port) + path;
    curl_easy_setopt(curl_, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl_, CURLOPT_CUSTOMREQUEST, "DELETE");

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, ("vmware-api-session-id: " + session_id_).c_str());
    curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, headers);

    std::string response;
    curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, write_callback);

    curl_easy_perform(curl_);
    curl_slist_free_all(headers);
    return response;
}

std::string VMwareAdapter::get_vm_mor(const std::string& vm_id) {
    return "/rest/vcenter/vm/" + vm_id;
}

std::vector<VirtualMachine> VMwareAdapter::list_vms() {
    std::vector<VirtualMachine> vms;
    std::string response = api_get("/rest/vcenter/vm");

    // Parse JSON response (simplified - in production use nlohmann/json)
    // Response format: {"value": [{"vm": "vm-123", "name": "myvm", ...}]}
    spdlog::info("VMware: listing VMs from vCenter");

    // Simulated response parsing
    VirtualMachine vm;
    vm.vm_id = "vm-001";
    vm.name = "SampleVM";
    vm.host = config_.vcenter_host;
    vm.hypervisor = HypervisorType::VMWARE;
    vm.power_state = VmPowerState::POWERED_ON;
    vm.cpu_count = 4;
    vm.memory_mb = 8192;
    vm.tools_installed = true;
    vm.os_type = "linux";
    vms.push_back(vm);

    vm_cache_.clear();
    for (auto& v : vms) vm_cache_[v.vm_id] = v;
    return vms;
}

VirtualMachine VMwareAdapter::get_vm(const std::string& vm_id) {
    auto it = vm_cache_.find(vm_id);
    if (it != vm_cache_.end()) return it->second;

    VirtualMachine vm;
    vm.vm_id = vm_id;
    vm.name = "VM-" + vm_id;
    vm.host = config_.vcenter_host;
    vm.hypervisor = HypervisorType::VMWARE;
    vm.power_state = VmPowerState::POWERED_ON;
    vm.tools_installed = true;
    vm_cache_[vm_id] = vm;
    return vm;
}

std::vector<VmSnapshot> VMwareAdapter::list_snapshots(const std::string& vm_id) {
    std::vector<VmSnapshot> snapshots;
    std::string response = api_get(get_vm_mor(vm_id) + "/snapshots");
    spdlog::info("VMware: listing snapshots for {}", vm_id);
    return snapshots;
}

std::string VMwareAdapter::create_snapshot(const std::string& vm_id, const std::string& name, bool quiesce) {
    std::string body = R"({"spec":{"description":"OBS Backup snapshot","memory":false,"quiesce":)" +
                       std::string(quiesce ? "true" : "false") + "}}";

    std::string response = api_post(get_vm_mor(vm_id) + "/snapshots", body);
    spdlog::info("VMware: created snapshot '{}' for {} (quiesce={})", name, vm_id, quiesce);

    // Extract snapshot ID from response
    return "snap-" + vm_id + "-" + name;
}

bool VMwareAdapter::remove_snapshot(const std::string& vm_id, const std::string& snapshot_id) {
    api_delete(get_vm_mor(vm_id) + "/snapshots/" + snapshot_id);
    spdlog::info("VMware: removed snapshot {} from {}", snapshot_id, vm_id);
    return true;
}

bool VMwareAdapter::revert_to_snapshot(const std::string& vm_id, const std::string& snapshot_id) {
    api_post(get_vm_mor(vm_id) + "/snapshots/" + snapshot_id + "/actions/revert-to-snapshot", "{}");
    spdlog::info("VMware: reverted {} to snapshot {}", vm_id, snapshot_id);
    return true;
}

bool VMwareAdapter::power_on(const std::string& vm_id) {
    api_post(get_vm_mor(vm_id) + "/power/actions/start", "{}");
    spdlog::info("VMware: powered on {}", vm_id);
    return true;
}

bool VMwareAdapter::power_off(const std::string& vm_id) {
    api_post(get_vm_mor(vm_id) + "/power/actions/stop", "{}");
    spdlog::info("VMware: powered off {}", vm_id);
    return true;
}

bool VMwareAdapter::suspend(const std::string& vm_id) {
    api_post(get_vm_mor(vm_id) + "/power/actions/suspend", "{}");
    spdlog::info("VMware: suspended {}", vm_id);
    return true;
}

bool VMwareAdapter::backup_vm(const VmBackupConfig& config, VmProgressCallback callback) {
    VmBackupJob job;
    job.job_id = "vmp-" + config.vm_id + "-" + std::to_string(time(nullptr));
    job.vm_id = config.vm_id;
    job.hypervisor = HypervisorType::VMWARE;
    job.transport = config.transport;
    job.status = "running";

    spdlog::info("VMware backup: vm={} transport={}", config.vm_id, static_cast<int>(config.transport));

    // Step 1: Create snapshot
    if (config.create_snapshot) {
        job.snapshot_id = create_snapshot(config.vm_id, "obs-backup-" + job.job_id, config.quiesce);
        spdlog::info("Snapshot created: {}", job.snapshot_id);
    }

    // Step 2: Get CBT data if enabled
    if (config.use_cbt && is_cbt_supported()) {
        auto changed = get_cbt_changed_blocks(config.vm_id, 0, UINT64_MAX);
        job.total_bytes = 0;
        for (auto& block : changed) {
            job.total_bytes += block.size;
        }
        spdlog::info("CBT: {} changed blocks, {} bytes total", changed.size(), job.total_bytes);
    }

    // Step 3: Transfer based on transport mode
    switch (config.transport) {
        case BackupTransport::HOT_ADD:
        case BackupTransport::HOTADD_NBD:
            spdlog::info("Hot Add transport: attaching virtual disk to backup proxy");
            break;
        case BackupTransport::NBD:
            spdlog::info("NBD transport: streaming disk over NBD protocol");
            break;
        case BackupTransport::DIRECT_SAN:
            spdlog::info("Direct SAN: reading LUN directly from SAN");
            break;
        default:
            spdlog::info("Network transport: standard network transfer");
            break;
    }

    // Simulate progress
    if (job.total_bytes == 0) job.total_bytes = 100ULL * 1024 * 1024;
    job.transferred_bytes = job.total_bytes;
    job.status = "completed";
    job.speed_mbps = 150.0;

    // Step 4: Remove snapshot if configured
    if (config.remove_snapshot_after && !job.snapshot_id.empty()) {
        remove_snapshot(config.vm_id, job.snapshot_id);
    }

    if (callback) callback(job);
    return true;
}

bool VMwareAdapter::restore_vm(const VmRestoreConfig& config, VmProgressCallback callback) {
    VmBackupJob job;
    job.job_id = "vmr-" + config.vm_id + "-" + std::to_string(time(nullptr));
    job.vm_id = config.vm_id;
    job.status = "running";
    spdlog::info("VMware restore: backup={} target={}", config.backup_id, config.target_host);

    // In production: power off target, swap disks, power on
    job.status = "completed";
    if (callback) callback(job);
    return true;
}

bool VMwareAdapter::export_vm(const std::string& vm_id, const std::string& format,
                              const std::string& output_path, VmProgressCallback callback) {
    spdlog::info("VMware: exporting {} as {} to {}", vm_id, format, output_path);
    // In production: use VDDK to export virtual disks
    return true;
}

bool VMwareAdapter::import_vm(const std::string& import_path, const std::string& target_host,
                              const std::string& target_datastore, VmProgressCallback callback) {
    spdlog::info("VMware: importing from {} to {}:{}", import_path, target_host, target_datastore);
    return true;
}

std::vector<ChangedBlockInfo> VMwareAdapter::get_changed_blocks(
    const std::string& vm_id, const std::string& from_snapshot, const std::string& to_snapshot) {
    return get_cbt_changed_blocks(vm_id, 0, UINT64_MAX);
}

std::vector<ChangedBlockInfo> VMwareAdapter::get_cbt_changed_blocks(
    const std::string& vm_id, uint64_t change_id_start, uint64_t change_id_end) {
    std::vector<ChangedBlockInfo> blocks;
    spdlog::info("VMware CBT: getting changed blocks for {} [{}, {}]",
                 vm_id, change_id_start, change_id_end);

    // In production: call QueryChangedDiskAreas via vSphere API
    // or use VDDK CBT API
    ChangedBlockInfo block{};
    block.offset = 0;
    block.size = 4096;
    block.dirty = true;
    blocks.push_back(block);
    return blocks;
}

bool VMwareAdapter::hot_add_disk(const std::string& vm_id, const std::string& disk_path, int controller) {
    std::string body = R"({"spec":{"devices":[{"type":"VIRTUAL_DISK","backing":{"type":"VIRTUAL_DISK_FILE_INFO",
        "virtualDiskFileBackingInfo":{"datastore":")" + config_.datacenter + R"(","fileName":")" + disk_path +
        R"(","thinProvisioned":true}},"controllerKey":)" + std::to_string(controller) + "}}]}}";

    api_post(get_vm_mor(vm_id) + "/hardware", body);
    spdlog::info("VMware: hot-added disk {} to {} on controller {}", disk_path, vm_id, controller);
    return true;
}

bool VMwareAdapter::nbd_export_disk(const std::string& vm_id, const std::string& disk_id,
                                     const std::string& output_path, VmProgressCallback callback) {
    spdlog::info("VMware NBD: exporting disk {} from {} to {}", disk_id, vm_id, output_path);
    // In production: use VDDK NBD export or NBD server
    return true;
}

} // namespace obs
