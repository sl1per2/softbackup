#pragma once
#include "common.h"
#include "agent/i_agent.h"
#include "agent/generic_agent.h"
#include "agent/esxi_agent.h"
#include "agent/hyperv_agent.h"
#include "agent/mssql_agent.h"
#include "agent/postgresql_agent.h"
#include "agent/oracle_agent.h"
#include "agent/proxmox_agent.h"
#include "agent/saphana_agent.h"
#include "agent/kubernetes_agent.h"
#include "agent/linux_fs_agent.h"
#include "agent/windows_fs_agent.h"
#include "agent/ndmp_agent.h"
#include "agent/exchange_agent.h"
#include <functional>
#include <memory>

namespace obs {

class AgentFactory {
public:
    using Creator = std::function<std::shared_ptr<IAgent>()>;

    static AgentFactory& instance() {
        static AgentFactory factory;
        return factory;
    }

    void register_agent(AgentType type, Creator creator) {
        creators_[type] = std::move(creator);
    }

    std::shared_ptr<IAgent> create(AgentType type) {
        auto it = creators_.find(type);
        if (it != creators_.end()) {
            return it->second();
        }
        return nullptr;
    }

    std::shared_ptr<IAgent> create_by_name(const std::string& name) {
        auto type = name_to_type(name);
        if (type.has_value()) {
            return create(type.value());
        }
        return nullptr;
    }

    std::vector<AgentType> supported_types() const {
        std::vector<AgentType> types;
        for (auto& [type, creator] : creators_) {
            types.push_back(type);
        }
        return types;
    }

    static std::optional<AgentType> name_to_type(const std::string& name) {
        static const std::map<std::string, AgentType> mapping = {
            {"generic", AgentType::GENERIC}, {"esxi", AgentType::ESXi},
            {"hyperv", AgentType::HYPERV}, {"mssql", AgentType::MSSQL},
            {"postgresql", AgentType::POSTGRESQL}, {"oracle", AgentType::ORACLE},
            {"proxmox", AgentType::PROXMOX}, {"saphana", AgentType::SAPHANA},
            {"kubernetes", AgentType::KUBERNETES}, {"linux_fs", AgentType::LINUX_FS},
            {"windows_fs", AgentType::WINDOWS_FS}, {"ndmp", AgentType::NDMP},
            {"exchange", AgentType::EXCHANGE}
        };
        auto it = mapping.find(name);
        if (it != mapping.end()) return it->second;
        return std::nullopt;
    }

private:
    AgentFactory() { register_defaults(); }

    void register_defaults() {
        register_agent(AgentType::GENERIC, []() -> std::shared_ptr<IAgent> {
            return std::make_shared<GenericAgent>();
        });
        register_agent(AgentType::ESXi, []() -> std::shared_ptr<IAgent> {
            return std::make_shared<EsxiAgent>();
        });
        register_agent(AgentType::HYPERV, []() -> std::shared_ptr<IAgent> {
            return std::make_shared<HypervAgent>();
        });
        register_agent(AgentType::MSSQL, []() -> std::shared_ptr<IAgent> {
            return std::make_shared<MssqlAgent>();
        });
        register_agent(AgentType::POSTGRESQL, []() -> std::shared_ptr<IAgent> {
            return std::make_shared<PostgresqlAgent>();
        });
        register_agent(AgentType::ORACLE, []() -> std::shared_ptr<IAgent> {
            return std::make_shared<OracleAgent>();
        });
        register_agent(AgentType::PROXMOX, []() -> std::shared_ptr<IAgent> {
            return std::make_shared<ProxmoxAgent>();
        });
        register_agent(AgentType::SAPHANA, []() -> std::shared_ptr<IAgent> {
            return std::make_shared<SapHanaAgent>();
        });
        register_agent(AgentType::KUBERNETES, []() -> std::shared_ptr<IAgent> {
            return std::make_shared<KubernetesAgent>();
        });
        register_agent(AgentType::LINUX_FS, []() -> std::shared_ptr<IAgent> {
            return std::make_shared<LinuxFsAgent>();
        });
        register_agent(AgentType::WINDOWS_FS, []() -> std::shared_ptr<IAgent> {
            return std::make_shared<WindowsFsAgent>();
        });
        register_agent(AgentType::NDMP, []() -> std::shared_ptr<IAgent> {
            return std::make_shared<NdmpAgent>();
        });
        register_agent(AgentType::EXCHANGE, []() -> std::shared_ptr<IAgent> {
            return std::make_shared<ExchangeAgent>();
        });
    }

    std::unordered_map<AgentType, Creator> creators_;
};

} // namespace obs
