#pragma once
#include "agent/generic_agent.h"
#include "k8s/k8s_backup.h"

namespace obs {

class KubernetesAgent : public GenericAgent {
public:
    KubernetesAgent();
    ~KubernetesAgent() override;

    AgentType type() const override { return AgentType::KUBERNETES; }
    std::string type_name() const override { return "kubernetes"; }
    std::string component_name() const override { return "KubernetesAgent"; }

    bool can_handle(const std::string& job_type) const override;
    BackupResult execute_job(const BackupRequest& request) override;

    void set_kubeconfig(const std::string& path);
    BackupResult backup_etcd(const std::string& output);
    BackupResult backup_manifests(const std::string& output);
    BackupResult backup_pvcs(const std::string& output, const std::string& namespace_filter = "");
    BackupResult backup_all(const std::string& output);
    bool restore_etcd(const std::string& backup_path);
    bool restore_manifests(const std::string& backup_path);
    std::vector<std::string> list_namespaces();
    bool test_connection();

private:
    std::string kubeconfig_path_;
};

} // namespace obs
