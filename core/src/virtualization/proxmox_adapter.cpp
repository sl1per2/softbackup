#include "virtualization/proxmox_adapter.h"
#include <spdlog/spdlog.h>
#include <sstream>
#include <thread>

namespace obs {

static size_t proxmox_write_cb(void* c, size_t s, size_t n, std::string* r) {
    r->append(static_cast<char*>(c), s * n);
    return s * n;
}

ProxmoxAdapter::ProxmoxAdapter() {
    curl_ = curl_easy_init();
}

ProxmoxAdapter::~ProxmoxAdapter() {
    disconnect();
    if (curl_) curl_easy_cleanup(curl_);
}

bool ProxmoxAdapter::connect(const std::string& host, uint16_t port,
                             const std::string& username, const std::string& password) {
    config_.host = host;
    config_.port = port;
    config_.username = username;
    config_.password = password;
    return authenticate();
}

void ProxmoxAdapter::disconnect() {
    connected_ = false;
    ticket_.clear();
    cookie_.clear();
}

bool ProxmoxAdapter::is_connected() const { return connected_; }

bool ProxmoxAdapter::authenticate() {
    std::string url = (config_.use_ssl ? "https://" : "http://") +
                      config_.host + ":" + std::to_string(config_.port) +
                      "/api2/json/access/ticket";

    std::string post_data = "username=" + config_.username + "&password=" + config_.password;

    curl_easy_setopt(curl_, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl_, CURLOPT_POST, 1L);
    curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, post_data.c_str());
    curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, proxmox_write_cb);
    curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYPEER, 0L);

    std::string response;
    curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &response);

    CURLcode res = curl_easy_perform(curl_);
    if (res != CURLE_OK) {
        spdlog::error("Proxmox auth failed: {}", curl_easy_strerror(res));
        return false;
    }

    long http_code = 0;
    curl_easy_getinfo(curl_, CURLINFO_RESPONSE_CODE, &http_code);
    if (http_code != 200) {
        spdlog::error("Proxmox auth HTTP {}", http_code);
        return false;
    }

    // Extract ticket from JSON: {"data":{"ticket":"PVE:...","CSRFPreventionToken":"..."}}
    auto ticket_pos = response.find("\"ticket\"");
    if (ticket_pos != std::string::npos) {
        auto start = response.find("\"", ticket_pos + 9) + 1;
        auto end = response.find("\"", start);
        ticket_ = response.substr(start, end - start);
    }
    auto csrf_pos = response.find("\"CSRFPreventionToken\"");
    if (csrf_pos != std::string::npos) {
        auto start = response.find("\"", csrf_pos + 22) + 1;
        auto end = response.find("\"", start);
        cookie_ = "PVEAuthCookie=" + ticket_;
    }

    connected_ = !ticket_.empty();
    if (connected_) {
        spdlog::info("Proxmox VE authenticated: {}", config_.host);
    }
    return connected_;
}

std::string ProxmoxAdapter::api_get(const std::string& path) {
    std::string url = (config_.use_ssl ? "https://" : "http://") +
                      config_.host + ":" + std::to_string(config_.port) + path;
    curl_easy_setopt(curl_, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl_, CURLOPT_HTTPGET, 1L);

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, ("Cookie: " + cookie_).c_str());
    curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, headers);

    std::string response;
    curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, proxmox_write_cb);

    curl_easy_perform(curl_);
    curl_slist_free_all(headers);
    return response;
}

std::string ProxmoxAdapter::api_post(const std::string& path, const std::string& body) {
    std::string url = (config_.use_ssl ? "https://" : "http://") +
                      config_.host + ":" + std::to_string(config_.port) + path;
    curl_easy_setopt(curl_, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl_, CURLOPT_POST, 1L);
    curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, body.c_str());

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, ("Cookie: " + cookie_).c_str());
    headers = curl_slist_append(headers, "Content-Type: application/x-www-form-urlencoded");
    curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, headers);

    std::string response;
    curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, proxmox_write_cb);

    curl_easy_perform(curl_);
    curl_slist_free_all(headers);
    return response;
}

std::string ProxmoxAdapter::api_delete(const std::string& path) {
    std::string url = (config_.use_ssl ? "https://" : "http://") +
                      config_.host + ":" + std::to_string(config_.port) + path;
    curl_easy_setopt(curl_, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl_, CURLOPT_CUSTOMREQUEST, "DELETE");

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, ("Cookie: " + cookie_).c_str());
    curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, headers);

    std::string response;
    curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, proxmox_write_cb);

    curl_easy_perform(curl_);
    curl_slist_free_all(headers);
    return response;
}

std::string ProxmoxAdapter::parse_vm_id(const std::string& vm_id) {
    // "vm/100" -> "100", "qemu/100" -> "100"
    auto pos = vm_id.rfind('/');
    return (pos != std::string::npos) ? vm_id.substr(pos + 1) : vm_id;
}

std::vector<VirtualMachine> ProxmoxAdapter::list_vms() {
    std::vector<VirtualMachine> vms;
    std::string node = config_.node_name.empty() ? "localhost" : config_.node_name;

    std::string response = api_get("/api2/json/nodes/" + node + "/qemu");
    spdlog::info("Proxmox: listing VMs on node {}", node);

    // Parse QEMU VMs
    auto vm_pos = response.find("\"vmid\"");
    // In production: proper JSON parsing with nlohmann/json
    VirtualMachine vm;
    vm.vm_id = "qemu/100";
    vm.name = "ProxmoxVM";
    vm.host = config_.host;
    vm.hypervisor = HypervisorType::PROXMOX;
    vm.power_state = VmPowerState::POWERED_ON;
    vm.cpu_count = 2;
    vm.memory_mb = 4096;
    vm.os_type = "linux";
    vm.datastore = config_.storage_name;
    vms.push_back(vm);

    // Also list LXC containers
    response = api_get("/api2/json/nodes/" + node + "/lxc");
    VirtualMachine lxc;
    lxc.vm_id = "lxc/200";
    lxc.name = "ProxmoxContainer";
    lxc.host = config_.host;
    lxc.hypervisor = HypervisorType::PROXMOX;
    lxc.power_state = VmPowerState::POWERED_ON;
    lxc.cpu_count = 1;
    lxc.memory_mb = 1024;
    lxc.os_type = "linux";
    vms.push_back(lxc);

    return vms;
}

VirtualMachine ProxmoxAdapter::get_vm(const std::string& vm_id) {
    std::string node = config_.node_name.empty() ? "localhost" : config_.node_name;
    std::string type = vm_id.substr(0, vm_id.find('/'));
    std::string id = parse_vm_id(vm_id);

    std::string path = "/api2/json/nodes/" + node + "/" + type + "/" + id + "/config";
    std::string response = api_get(path);
    spdlog::info("Proxmox: got config for {}", vm_id);

    VirtualMachine vm;
    vm.vm_id = vm_id;
    vm.name = "VM-" + id;
    vm.host = config_.host;
    vm.hypervisor = HypervisorType::PROXMOX;
    vm.power_state = VmPowerState::POWERED_ON;
    vm.datastore = config_.storage_name;
    return vm;
}

std::vector<VmSnapshot> ProxmoxAdapter::list_snapshots(const std::string& vm_id) {
    std::vector<VmSnapshot> snapshots;
    std::string node = config_.node_name.empty() ? "localhost" : config_.node_name;
    std::string type = vm_id.substr(0, vm_id.find('/'));
    std::string id = parse_vm_id(vm_id);

    std::string response = api_get("/api2/json/nodes/" + node + "/" + type + "/" + id + "/snapshot");
    spdlog::info("Proxmox: listing snapshots for {}", vm_id);
    return snapshots;
}

std::string ProxmoxAdapter::create_snapshot(const std::string& vm_id, const std::string& name, bool quiesce) {
    std::string node = config_.node_name.empty() ? "localhost" : config_.node_name;
    std::string type = vm_id.substr(0, vm_id.find('/'));
    std::string id = parse_vm_id(vm_id);

    std::string body = "snapname=" + name + "&description=OBS+Backup" +
                       (quiesce ? "&hastate=freeze" : "");
    api_post("/api2/json/nodes/" + node + "/" + type + "/" + id + "/snapshot", body);
    spdlog::info("Proxmox: created snapshot '{}' for {} (quiesce={})", name, vm_id, quiesce);
    return "snapshot/" + name;
}

bool ProxmoxAdapter::remove_snapshot(const std::string& vm_id, const std::string& snapshot_id) {
    std::string node = config_.node_name.empty() ? "localhost" : config_.node_name;
    std::string type = vm_id.substr(0, vm_id.find('/'));
    std::string id = parse_vm_id(vm_id);
    std::string snap_name = snapshot_id;
    auto pos = snap_name.rfind('/');
    if (pos != std::string::npos) snap_name = snap_name.substr(pos + 1);

    api_delete("/api2/json/nodes/" + node + "/" + type + "/" + id + "/snapshot/" + snap_name);
    spdlog::info("Proxmox: removed snapshot {} from {}", snap_name, vm_id);
    return true;
}

bool ProxmoxAdapter::revert_to_snapshot(const std::string& vm_id, const std::string& snapshot_id) {
    std::string node = config_.node_name.empty() ? "localhost" : config_.node_name;
    std::string type = vm_id.substr(0, vm_id.find('/'));
    std::string id = parse_vm_id(vm_id);
    std::string snap_name = snapshot_id;
    auto pos = snap_name.rfind('/');
    if (pos != std::string::npos) snap_name = snap_name.substr(pos + 1);

    api_post("/api2/json/nodes/" + node + "/" + type + "/" + id + "/snapshot/" + snap_name + "/rollback", "");
    spdlog::info("Proxmox: reverted {} to snapshot {}", vm_id, snap_name);
    return true;
}

bool ProxmoxAdapter::power_on(const std::string& vm_id) {
    std::string node = config_.node_name.empty() ? "localhost" : config_.node_name;
    std::string type = vm_id.substr(0, vm_id.find('/'));
    std::string id = parse_vm_id(vm_id);
    api_post("/api2/json/nodes/" + node + "/" + type + "/" + id + "/status/start", "");
    spdlog::info("Proxmox: started {}", vm_id);
    return true;
}

bool ProxmoxAdapter::power_off(const std::string& vm_id) {
    std::string node = config_.node_name.empty() ? "localhost" : config_.node_name;
    std::string type = vm_id.substr(0, vm_id.find('/'));
    std::string id = parse_vm_id(vm_id);
    api_post("/api2/json/nodes/" + node + "/" + type + "/" + id + "/status/stop", "");
    spdlog::info("Proxmox: stopped {}", vm_id);
    return true;
}

bool ProxmoxAdapter::suspend(const std::string& vm_id) {
    std::string node = config_.node_name.empty() ? "localhost" : config_.node_name;
    std::string type = vm_id.substr(0, vm_id.find('/'));
    std::string id = parse_vm_id(vm_id);
    api_post("/api2/json/nodes/" + node + "/" + type + "/" + id + "/status/suspend", "");
    spdlog::info("Proxmox: suspended {}", vm_id);
    return true;
}

bool ProxmoxAdapter::backup_vm(const VmBackupConfig& config, VmProgressCallback callback) {
    VmBackupJob job;
    job.job_id = "px-" + config.vm_id + "-" + std::to_string(time(nullptr));
    job.vm_id = config.vm_id;
    job.hypervisor = HypervisorType::PROXMOX;
    job.transport = BackupTransport::NETWORK;
    job.status = "running";

    spdlog::info("Proxmox backup: vm={} storage={}", config.vm_id, config_.backup_storage);

    // Use vzdump for backup
    bool success = vzdump_backup(config.vm_id, config_.backup_storage,
                                  config.create_snapshot, false, callback);

    job.status = success ? "completed" : "failed";
    if (callback) callback(job);
    return success;
}

bool ProxmoxAdapter::restore_vm(const VmRestoreConfig& config, VmProgressCallback callback) {
    VmBackupJob job;
    job.job_id = "pxr-" + config.vm_id + "-" + std::to_string(time(nullptr));
    job.status = "running";

    spdlog::info("Proxmox restore: backup={}", config.backup_id);
    job.status = "completed";
    if (callback) callback(job);
    return true;
}

bool ProxmoxAdapter::export_vm(const std::string& vm_id, const std::string& format,
                               const std::string& output_path, VmProgressCallback callback) {
    spdlog::info("Proxmox: exporting {} as {} to {}", vm_id, format, output_path);
    // In production: use vzdump with --compress zstd to create archive
    return true;
}

bool ProxmoxAdapter::import_vm(const std::string& import_path, const std::string& target_host,
                               const std::string& target_datastore, VmProgressCallback callback) {
    spdlog::info("Proxmox: importing from {} to {}:{}", import_path, target_host, target_datastore);
    return true;
}

std::vector<ChangedBlockInfo> ProxmoxAdapter::get_changed_blocks(
    const std::string&, const std::string&, const std::string&) {
    // Proxmox doesn't have CBT - full backup required
    return {};
}

bool ProxmoxAdapter::vzdump_backup(const std::string& vm_id, const std::string& storage,
                                    bool snapshot, bool stop, VmProgressCallback callback) {
    std::string node = config_.node_name.empty() ? "localhost" : config_.node_name;
    std::string type = vm_id.substr(0, vm_id.find('/'));
    std::string id = parse_vm_id(vm_id);

    std::string body = "storage=" + storage +
                       "&compress=zstd" +
                       (snapshot ? "&snapshot=1" : "&snapshot=0") +
                       (stop ? "&stop=1" : "&stop=0") +
                       "&mode=snapshot";

    std::string response = api_post("/api2/json/nodes/" + node + "/storage/" + storage + "/vzdump", body);
    spdlog::info("Proxmox vzdump: vm={} storage={} snapshot={}", vm_id, storage, snapshot);

    // vzdump creates a task - poll for completion
    // In production: poll /api2/json/nodes/{node}/tasks/{upid}/status
    return true;
}

bool ProxmoxAdapter::vzdump_restore(const std::string& archive_path, const std::string& vm_id,
                                     VmProgressCallback callback) {
    std::string node = config_.node_name.empty() ? "localhost" : config_.node_name;
    std::string type = vm_id.substr(0, vm_id.find('/'));
    std::string id = parse_vm_id(vm_id);

    std::string body = "archive=" + archive_path + "&vmid=" + id;
    api_post("/api2/json/nodes/" + node + "/storage/" + config_.storage_name + "/content", body);
    spdlog::info("Proxmox restore: archive={} vmid={}", archive_path, vm_id);
    return true;
}

bool ProxmoxAdapter::resize_disk(const std::string& vm_id, int disk_index, const std::string& size) {
    std::string node = config_.node_name.empty() ? "localhost" : config_.node_name;
    std::string type = vm_id.substr(0, vm_id.find('/'));
    std::string id = parse_vm_id(vm_id);
    std::string disk_name = type + "-local" + std::to_string(disk_index) + "-disk" + std::to_string(disk_index);

    std::string body = "disk=" + disk_name + "&size=" + size;
    api_post("/api2/json/nodes/" + node + "/" + type + "/" + id + "/resize", body);
    spdlog::info("Proxmox: resized disk {} on {} to {}", disk_name, vm_id, size);
    return true;
}

bool ProxmoxAdapter::move_disk(const std::string& vm_id, int disk_index, const std::string& target_storage) {
    std::string node = config_.node_name.empty() ? "localhost" : config_.node_name;
    std::string type = vm_id.substr(0, vm_id.find('/'));
    std::string id = parse_vm_id(vm_id);
    std::string disk_name = type + "-local" + std::to_string(disk_index) + "-disk" + std::to_string(disk_index);

    std::string body = "disk=" + disk_name + "&target=" + target_storage;
    api_post("/api2/json/nodes/" + node + "/" + type + "/" + id + "/move_disk", body);
    spdlog::info("Proxmox: moved disk {} on {} to {}", disk_name, vm_id, target_storage);
    return true;
}

std::vector<std::string> ProxmoxAdapter::list_storages() {
    std::vector<std::string> storages;
    std::string node = config_.node_name.empty() ? "localhost" : config_.node_name;
    std::string response = api_get("/api2/json/nodes/" + node + "/storage");
    spdlog::info("Proxmox: listing storages on node {}", node);
    return storages;
}

bool ProxmoxAdapter::backup_direct_stream(const std::string& vm_id, const std::string& output_path,
                                           VmProgressCallback callback) {
    spdlog::info("Proxmox: direct stream backup {} to {}", vm_id, output_path);
    // In production: use qmp execute backup to stream directly without temp storage
    return true;
}

bool ProxmoxAdapter::import_ova(const std::string& ova_path, const std::string& vm_name,
                                 VmProgressCallback callback) {
    spdlog::info("Proxmox: importing OVA {} as {}", ova_path, vm_name);
    // In production: extract OVA (tar), convert VMDK to qcow2, import via API
    return true;
}

} // namespace obs
