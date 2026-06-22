#pragma once
#include "common.h"
#include "framework/component.h"
#include <typeindex>
#include <any>

namespace obs {

class ServiceRegistry {
public:
    template<typename Interface, typename Implementation, typename... Args>
    void register_service(Args&&... args) {
        auto impl = std::make_shared<Implementation>(std::forward<Args>(args)...);
        impl->initialize();
        services_[std::type_index(typeid(Interface))] = impl;
    }

    template<typename Interface>
    std::shared_ptr<Interface> resolve() {
        auto it = services_.find(std::type_index(typeid(Interface)));
        if (it != services_.end()) {
            return std::any_cast<std::shared_ptr<Interface>>(it->second);
        }
        return nullptr;
    }

    template<typename Interface>
    void register_instance(std::shared_ptr<Interface> instance) {
        services_[std::type_index(typeid(Interface))] = std::move(instance);
    }

    void shutdown_all() {
        for (auto& [key, value] : services_) {
            try {
                auto comp = std::any_cast<std::shared_ptr<IComponent>>(value);
                if (comp) comp->shutdown();
            } catch (...) {}
        }
        services_.clear();
    }

private:
    std::unordered_map<std::type_index, std::any> services_;
};

} // namespace obs
