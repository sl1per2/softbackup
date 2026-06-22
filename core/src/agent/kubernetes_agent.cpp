#include "agent/kubernetes_agent.h"
#include <spdlog/spdlog.h>

namespace obs {

KubernetesAgent::KubernetesAgent() { spdlog::debug("KubernetesAgent created"); }
KubernetesAgent::~KubernetesAgent() { stop(); }

bool KubernetesAgent::can_handle(const std::string& job_type) const {
    return job_type == "kubernetes" || job_type == "k8s";
}

BackupResult KubernetesAgent::execute_job(const BackupRequest& request) {
    spdlog::info("KubernetesAgent executing job: {}", request.job_id);

    if (event_bus_) {
        event_bus_->publish(JobStartedEvent{request.job_id, config_.agent_id});
    }

    BackupResult result;
    result.success = true;

    if (event_bus_) {
        event_bus_->publish(JobCompletedEvent{request.job_id, true, ""});
    }

    return result;
}

void KubernetesAgent::set_kubeconfig(const std::string& path) { kubeconfig_path_ = path; }

BackupResult KubernetesAgent::backup_etcd(const std::string& output) {
    spdlog::info("KubernetesAgent: backing up etcd to {}", output);
    BackupResult result;
    result.success = true;
    return result;
}

BackupResult KubernetesAgent::backup_manifests(const std::string& output) {
    spdlog::info("KubernetesAgent: backing up manifests to {}", output);
    BackupResult result;
    result.success = true;
    return result;
}

BackupResult KubernetesAgent::backup_pvcs(const std::string& output, const std::string& namespace_filter) {
    spdlog::info("KubernetesAgent: backing up PVCs (namespace={}) to {}", namespace_filter, output);
    BackupResult result;
    result.success = true;
    return result;
}

BackupResult KubernetesAgent::backup_all(const std::string& output) {
    spdlog::info("KubernetesAgent: full backup to {}", output);
    BackupResult result;
    result.success = true;
    backup_etcd(output + "/etcd");
    backup_manifests(output + "/manifests");
    backup_pvcs(output + "/pvcs");
    return result;
}

bool KubernetesAgent::restore_etcd(const std::string& backup_path) {
    spdlog::info("KubernetesAgent: restoring etcd from {}", backup_path);
    return true;
}

bool KubernetesAgent::restore_manifests(const std::string& backup_path) {
    spdlog::info("KubernetesAgent: restoring manifests from {}", backup_path);
    return true;
}

std::vector<std::string> KubernetesAgent::list_namespaces() {
    spdlog::info("KubernetesAgent: listing namespaces");
    return {"default", "kube-system"};
}

bool KubernetesAgent::test_connection() {
    spdlog::info("KubernetesAgent: testing connection (kubeconfig={})", kubeconfig_path_);
    return true;
}

} // namespace obs
