#include "virtualization/vm_manager.h"
#include <spdlog/spdlog.h>

namespace obs {

VirtualizationManager::VirtualizationManager() {}
VirtualizationManager::~VirtualizationManager() = default;

std::string VirtualizationManager::add_connection(HypervisorType type, const std::string& host,
                                                   uint16_t port, const std::string& username,
                                                   const std::string& password) {
    std::string id = "conn-" + std::to_string(connections_.size() + 1) + "-" + host;

    std::unique_ptr<VirtualizationAdapter> adapter;
    switch (type) {
        case HypervisorType::VMWARE:
            adapter = std::make_unique<VMwareAdapter>();
            break;
        case HypervisorType::HYPERV:
            adapter = std::make_unique<HyperVAdapter>();
            break;
        case HypervisorType::PROXMOX:
            adapter = std::make_unique<ProxmoxAdapter>();
            break;
        default:
            spdlog::error("Unsupported hypervisor type");
            return "";
    }

    if (!adapter->connect(host, port, username, password)) {
        spdlog::error("Failed to connect to {} at {}:{}", adapter->name(), host, port);
        return "";
    }

    Connection conn;
    conn.info.id = id;
    conn.info.name = adapter->name();
    conn.info.type = type;
    conn.info.host = host;
    conn.info.port = port;
    conn.info.connected = true;
    conn.info.username = username;
    conn.adapter = std::move(adapter);

    {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        connections_[id] = std::move(conn);
    }

    spdlog::info("Hypervisor connection added: {} ({})", id, host);
    return id;
}

bool VirtualizationManager::remove_connection(const std::string& conn_id) {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    auto it = connections_.find(conn_id);
    if (it == connections_.end()) return false;
    it->second.adapter->disconnect();
    connections_.erase(it);
    return true;
}

std::vector<HypervisorConnection> VirtualizationManager::list_connections() const {
    std::vector<HypervisorConnection> result;
    std::lock_guard<std::mutex> lock(connections_mutex_);
    for (const auto& [id, conn] : connections_) {
        result.push_back(conn.info);
    }
    return result;
}

VirtualizationAdapter* VirtualizationManager::get_adapter(const std::string& conn_id) {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    auto it = connections_.find(conn_id);
    return (it != connections_.end()) ? it->second.adapter.get() : nullptr;
}

VirtualizationAdapter* VirtualizationManager::get_adapter_by_type(HypervisorType type) {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    for (auto& [id, conn] : connections_) {
        if (conn.info.type == type && conn.info.connected) {
            return conn.adapter.get();
        }
    }
    return nullptr;
}

std::vector<VirtualMachine> VirtualizationManager::list_all_vms() const {
    std::vector<VirtualMachine> all_vms;
    std::lock_guard<std::mutex> lock(connections_mutex_);
    for (const auto& [id, conn] : connections_) {
        if (conn.info.connected) {
            auto vms = conn.adapter->list_vms();
            all_vms.insert(all_vms.end(), vms.begin(), vms.end());
        }
    }
    return all_vms;
}

std::vector<VirtualMachine> VirtualizationManager::list_vms_by_hypervisor(HypervisorType type) const {
    std::vector<VirtualMachine> vms;
    std::lock_guard<std::mutex> lock(connections_mutex_);
    for (const auto& [id, conn] : connections_) {
        if (conn.info.type == type && conn.info.connected) {
            auto vm_list = conn.adapter->list_vms();
            vms.insert(vms.end(), vm_list.begin(), vm_list.end());
        }
    }
    return vms;
}

bool VirtualizationManager::backup_vm(const std::string& conn_id, const std::string& vm_id,
                                       const VmBackupConfig& config, VmProgressCallback callback) {
    auto* adapter = get_adapter(conn_id);
    if (!adapter) {
        spdlog::error("Connection not found: {}", conn_id);
        return false;
    }
    return adapter->backup_vm(config, callback);
}

bool VirtualizationManager::restore_vm(const std::string& conn_id, const std::string& vm_id,
                                        const VmRestoreConfig& config, VmProgressCallback callback) {
    auto* adapter = get_adapter(conn_id);
    if (!adapter) return false;
    return adapter->restore_vm(config, callback);
}

HypervisorType VirtualizationManager::detect_hypervisor(const std::string& host) {
    // Try Proxmox API first (port 8006)
    // Then VMware vCenter (port 443)
    // Then Hyper-V (port 5985/5986 WSMAN)
    spdlog::info("Detecting hypervisor at {}", host);
    return HypervisorType::UNKNOWN;
}

} // namespace obs
