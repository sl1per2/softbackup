#include "agent/postgresql_agent.h"
#include "agent/agent_factory.h"
#include <cassert>
#include <iostream>

void test_pgsql_creation() {
    auto agent = std::make_shared<obs::PostgresqlAgent>();
    assert(agent->type() == obs::AgentType::POSTGRESQL);
    assert(agent->type_name() == "postgresql");
    assert(agent->component_name() == "PostgresqlAgent");
    std::cout << "[PASS] test_pgsql_creation" << std::endl;
}

void test_pgsql_can_handle() {
    obs::PostgresqlAgent agent;
    assert(agent.can_handle("database_backup"));
    assert(agent.can_handle("postgresql"));
    assert(agent.can_handle("postgres"));
    assert(!agent.can_handle("vm_backup"));
    assert(!agent.can_handle("mssql"));
    std::cout << "[PASS] test_pgsql_can_handle" << std::endl;
}

void test_pgsql_connection() {
    obs::PostgresqlAgent agent;
    obs::DbConnection conn;
    conn.host = "localhost";
    conn.port = 5432;
    conn.username = "postgres";
    conn.password = "password";
    conn.database = "mydb";
    agent.set_connection(conn);
    std::cout << "[PASS] test_pgsql_connection" << std::endl;
}

void test_pgsql_continuous() {
    obs::PostgresqlAgent agent;
    assert(!agent.test_connection());
    std::cout << "[PASS] test_pgsql_continuous" << std::endl;
}

void test_pgsql_factory() {
    auto agent = obs::AgentFactory::instance().create(obs::AgentType::POSTGRESQL);
    assert(agent != nullptr);
    assert(agent->type() == obs::AgentType::POSTGRESQL);
    assert(agent->can_handle("postgresql"));
    std::cout << "[PASS] test_pgsql_factory" << std::endl;
}

void test_all_db_agents() {
    auto mssql = obs::AgentFactory::instance().create_by_name("mssql");
    auto pgsql = obs::AgentFactory::instance().create_by_name("postgresql");
    auto oracle = obs::AgentFactory::instance().create_by_name("oracle");
    auto hana = obs::AgentFactory::instance().create_by_name("saphana");

    assert(mssql != nullptr);
    assert(pgsql != nullptr);
    assert(oracle != nullptr);
    assert(hana != nullptr);

    assert(mssql->can_handle("mssql"));
    assert(pgsql->can_handle("postgresql"));
    assert(oracle->can_handle("oracle"));
    assert(hana->can_handle("saphana"));

    std::cout << "[PASS] test_all_db_agents" << std::endl;
}

int main() {
    test_pgsql_creation();
    test_pgsql_can_handle();
    test_pgsql_connection();
    test_pgsql_continuous();
    test_pgsql_factory();
    test_all_db_agents();
    std::cout << "\nAll PostgreSQL agent tests passed!" << std::endl;
    return 0;
}
