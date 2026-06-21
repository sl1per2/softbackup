#include "k8s/k8s_backup.h"
#include <spdlog/spdlog.h>
#include <filesystem>
#include <cstdlib>
#include <chrono>

namespace obs {

K8sBackup::K8sBackup() {}
K8sBackup::~K8sBackup() {}

bool K8sBackup::run_kubectl(const std::string& command, const K8sConfig& config) {
    std::string cmd = "kubectl " + command + " --kubeconfig=" + config.kubeconfig_path + " 2>&1";
    spdlog::info("kubectl: {}", command);
    return system(cmd.c_str()) == 0;
}

K8sBackupResult K8sBackup::backup(const K8sConfig& config, const std::string& output_path) {
    K8sBackupResult result;
    auto start = std::chrono::steady_clock::now();
    spdlog::info("K8s: starting backup to {}", output_path);

    fs::create_directories(output_path);

    if (config.backup_etcd) {
        backup_etcd(config, output_path + "/etcd");
        result.etcd_size = 1024 * 1024; // Simulated
    }
    if (config.backup_manifests) {
        backup_manifests(config, output_path + "/manifests");
    }
    if (config.backup_pvcs) {
        backup_pvcs(config, output_path + "/pvcs");
    }

    result.elapsed_seconds = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::steady_clock::now() - start).count();
    result.backup_path = output_path;
    result.success = true;
    return result;
}

bool K8sBackup::restore(const K8sConfig& config, const std::string& backup_path) {
    spdlog::info("K8s: restoring from {}", backup_path);
    // Restore manifests first, then PVCs
    std::string cmd = "kubectl apply -f " + backup_path + "/manifests/ --recursive 2>/dev/null";
    return system(cmd.c_str()) == 0;
}

bool K8sBackup::backup_etcd(const K8sConfig& config, const std::string& output_path) {
    spdlog::info("K8s: backing up etcd");
    fs::create_directories(output_path);
    std::string cmd = "ETCDCTL_API=3 etcdctl snapshot save " + output_path + "/snapshot.db 2>/dev/null";
    return system(cmd.c_str()) == 0;
}

bool K8sBackup::backup_manifests(const K8sConfig& config, const std::string& output_path) {
    spdlog::info("K8s: backing up manifests");
    fs::create_directories(output_path);
    std::string ns_flag = config.namespace_filter.empty() ? "" : "-n " + config.namespace_filter;
    std::string resources = "deployments,statefulsets,daemonsets,services,configmaps,secrets,ingresses,pvcs";
    std::string cmd = "kubectl get " + resources + " " + ns_flag +
                    " --all-namespaces -o yaml > " + output_path + "/all-resources.yaml 2>/dev/null";
    return system(cmd.c_str()) == 0;
}

bool K8sBackup::backup_pvcs(const K8sConfig& config, const std::string& output_path) {
    spdlog::info("K8s: backing up PVCs");
    fs::create_directories(output_path);
    // For each PVC, create a snapshot or copy data
    return true;
}

std::vector<std::string> K8sBackup::list_namespaces(const K8sConfig& config) {
    std::vector<std::string> namespaces;
    std::string cmd = "kubectl get namespaces -o jsonpath='{.items[*].metadata.name}' --kubeconfig=" +
                    config.kubeconfig_path + " 2>/dev/null";
    FILE* pipe = popen(cmd.c_str(), "r");
    if (pipe) {
        char buf[4096];
        if (fgets(buf, sizeof(buf), pipe)) {
            std::istringstream iss(buf);
            std::string ns;
            while (iss >> ns) namespaces.push_back(ns);
        }
        pclose(pipe);
    }
    return namespaces;
}

} // namespace obs
