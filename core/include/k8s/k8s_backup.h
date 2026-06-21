#pragma once
#include "common.h"
#include <functional>
#include <vector>
#include <string>

namespace obs {

struct K8sConfig {
    std::string kubeconfig_path;
    std::string api_server;
    std::string namespace_filter;
    bool backup_etcd = true;
    bool backup_manifests = true;
    bool backup_pvcs = true;
    bool use_velero = false;
};

struct K8sBackupResult {
    bool success = false;
    uint64_t etcd_size = 0;
    int32_t manifest_count = 0;
    int32_t pvc_count = 0;
    int32_t elapsed_seconds = 0;
    std::string backup_path;
    std::string error;
};

class K8sBackup {
public:
    K8sBackup();
    ~K8sBackup();

    K8sBackupResult backup(const K8sConfig& config, const std::string& output_path);
    bool restore(const K8sConfig& config, const std::string& backup_path);
    bool backup_etcd(const K8sConfig& config, const std::string& output_path);
    bool backup_manifests(const K8sConfig& config, const std::string& output_path);
    bool backup_pvcs(const K8sConfig& config, const std::string& output_path);
    std::vector<std::string> list_namespaces(const K8sConfig& config);

private:
    bool run_kubectl(const std::string& command, const K8sConfig& config);
    bool backup_etcd_snapshot(const std::string& api_server, const std::string& output_path);
    bool backup_k8s_resources(const K8sConfig& config, const std::string& output_path);
    bool backup_pv_data(const K8sConfig& config, const std::string& output_path);
};

} // namespace obs
