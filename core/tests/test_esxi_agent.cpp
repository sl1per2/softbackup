#include "agent/esxi_agent.h"
#include "agent/agent_factory.h"
#include <cassert>
#include <iostream>

void test_esxi_agent_creation() {
    auto agent = std::make_shared<obs::EsxiAgent>();
    assert(agent->type() == obs::AgentType::ESXi);
    assert(agent->type_name() == "esxi");
    assert(agent->component_name() == "EsxiAgent");
    std::cout << "[PASS] test_esxi_agent_creation" << std::endl;
}

void test_esxi_can_handle() {
    obs::EsxiAgent agent;
    assert(agent.can_handle("vm_backup"));
    assert(agent.can_handle("vmware"));
    assert(agent.can_handle("esxi"));
    assert(!agent.can_handle("database_backup"));
    assert(!agent.can_handle("mssql"));
    std::cout << "[PASS] test_esxi_can_handle" << std::endl;
}

void test_esxi_config() {
    obs::EsxiAgent agent;
    agent.set_vcenter_host("vcenter.example.com");
    agent.set_credentials("admin", "password123");
    agent.set_transport(obs::TransportMode::DIRECT_SAN);
    std::cout << "[PASS] test_esxi_config" << std::endl;
}

void test_esxi_list_vms() {
    obs::EsxiAgent agent;
    auto vms = agent.list_vms();
    assert(vms.empty());
    std::cout << "[PASS] test_esxi_list_vms" << std::endl;
}

void test_esxi_snapshot() {
    obs::EsxiAgent agent;
    auto snap_id = agent.create_snapshot("vm-123", "test_snap");
    assert(!snap_id.empty());
    assert(agent.remove_snapshot("vm-123", snap_id));
    std::cout << "[PASS] test_esxi_snapshot" << std::endl;
}

void test_esxi_changed_blocks() {
    obs::EsxiAgent agent;
    auto blocks = agent.get_changed_blocks("vm-123", "snap1", "snap2");
    assert(blocks.empty());
    std::cout << "[PASS] test_esxi_changed_blocks" << std::endl;
}

void test_esxi_factory() {
    auto agent = obs::AgentFactory::instance().create(obs::AgentType::ESXi);
    assert(agent != nullptr);
    assert(agent->type() == obs::AgentType::ESXi);
    assert(agent->can_handle("esxi"));
    std::cout << "[PASS] test_esxi_factory" << std::endl;
}

int main() {
    test_esxi_agent_creation();
    test_esxi_can_handle();
    test_esxi_config();
    test_esxi_list_vms();
    test_esxi_snapshot();
    test_esxi_changed_blocks();
    test_esxi_factory();
    std::cout << "\nAll ESXi agent tests passed!" << std::endl;
    return 0;
}
