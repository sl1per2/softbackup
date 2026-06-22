#pragma once
#include "common.h"
#include <typeindex>

namespace obs {

class IComponent {
public:
    virtual ~IComponent() = default;
    virtual std::string component_name() const = 0;
    virtual void initialize() {}
    virtual void shutdown() {}
    virtual bool is_initialized() const { return initialized_; }

protected:
    void mark_initialized() { initialized_ = true; }

private:
    bool initialized_ = false;
};

template<typename T>
class ComponentBase : public IComponent {
public:
    static std::type_index type_id() { return std::type_index(typeid(T)); }
    std::string component_name() const override { return typeid(T).name(); }
};

} // namespace obs
