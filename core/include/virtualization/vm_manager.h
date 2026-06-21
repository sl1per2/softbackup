#pragma once
#include "virtualization/virtual_machine.h"
#include "virtualization/vmware_adapter.h"
#include "virtualization/hyperv_adapter.h"
#include "virtualization/proxmox_adapter.h"
#include <map>
#include <memory>

namespace obs {

struct HypervisorConnection {
    std::string id;
    std::string name;
    HypervisorType type;
    std::string host;
    uint16_t port;
    bool connected;
    std::string username;
};

class VirtualizationManager {
public:
    VirtualizationManager();
    ~VirtualizationManager();

    std::string add_connection(HypervisorType type, const std::string& host, uint16_t port,
                              const std::string& username, const std::string& password);
    bool remove_connection(const std::string& conn_id);
    std::vector<HypervisorConnection> list_connections() const;

    VirtualizationAdapter* get_adapter(const std::string& conn_id);
    VirtualizationAdapter* get_adapter_by_type(HypervisorType type);

    std::vector<VirtualMachine> list_all_vms() const;
    std::vector<VirtualMachine> list_vms_by_hypervisor(HypervisorType type) const;

    bool backup_vm(const std::string& conn_id, const std::string& vm_id,
                  const VmBackupConfig& config, VmProgressCallback callback);
    bool restore_vm(const std::string& conn_id, const std::string& vm_id,
                   const VmRestoreConfig& config, VmProgressCallback callback);

    // Auto-detect hypervisor from host
    HypervisorType detect_hypervisor(const std::string& host);

private:
    struct Connection {
        HypervisorConnection info;
        std::unique_ptr<VirtualizationAdapter> adapter;
    };

    std::map<std::string, Connection> connections_;
    mutable std::mutex connections_mutex_;
};

} // namespace obs
