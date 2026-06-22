#include "agent/mssql_agent.h"
#include "agent/agent_factory.h"
#include <cassert>
#include <iostream>

void test_mssql_creation() {
    auto agent = std::make_shared<obs::MssqlAgent>();
    assert(agent->type() == obs::AgentType::MSSQL);
    assert(agent->type_name() == "mssql");
    assert(agent->component_name() == "MssqlAgent");
    std::cout << "[PASS] test_mssql_creation" << std::endl;
}

void test_mssql_can_handle() {
    obs::MssqlAgent agent;
    assert(agent.can_handle("database_backup"));
    assert(agent.can_handle("mssql"));
    assert(agent.can_handle("sqlserver"));
    assert(!agent.can_handle("vm_backup"));
    assert(!agent.can_handle("postgresql"));
    std::cout << "[PASS] test_mssql_can_handle" << std::endl;
}

void test_mssql_connection() {
    obs::MssqlAgent agent;
    obs::DbConnection conn;
    conn.host = "sqlserver.example.com";
    conn.port = 1433;
    conn.username = "sa";
    conn.password = "password";
    conn.database = "master";
    agent.set_connection(conn);
    std::cout << "[PASS] test_mssql_connection" << std::endl;
}

void test_mssql_factory() {
    auto agent = obs::AgentFactory::instance().create(obs::AgentType::MSSQL);
    assert(agent != nullptr);
    assert(agent->type() == obs::AgentType::MSSQL);
    assert(agent->can_handle("mssql"));
    std::cout << "[PASS] test_mssql_factory" << std::endl;
}

void test_mssql_types() {
    assert(obs::AgentFactory::instance().create_by_name("mssql") != nullptr);
    assert(obs::AgentFactory::instance().create_by_name("postgresql") != nullptr);
    assert(obs::AgentFactory::instance().create_by_name("oracle") != nullptr);
    assert(obs::AgentFactory::instance().create_by_name("unknown") == nullptr);
    std::cout << "[PASS] test_mssql_types" << std::endl;
}

int main() {
    test_mssql_creation();
    test_mssql_can_handle();
    test_mssql_connection();
    test_mssql_factory();
    test_mssql_types();
    std::cout << "\nAll MSSQL agent tests passed!" << std::endl;
    return 0;
}
