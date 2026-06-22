#include "framework/component.h"
#include "framework/service_registry.h"
#include "framework/event_bus.h"
#include "framework/command.h"
#include "agent/i_agent.h"
#include "agent/agent_factory.h"
#include "engine/i_backup_engine.h"
#include "engine/i_dedup_engine.h"
#include "engine/i_crypto_engine.h"
#include "engine/engine_factory.h"
#include "engine/backup_pipeline.h"
#include <cassert>
#include <iostream>

void test_service_registry() {
    obs::ServiceRegistry registry;
    auto event_bus = std::make_shared<obs::EventBus>();
    registry.register_instance(event_bus);

    auto resolved = registry.resolve<obs::EventBus>();
    assert(resolved != nullptr);
    assert(resolved.get() == event_bus.get());
    std::cout << "[PASS] test_service_registry" << std::endl;
}

void test_event_bus() {
    obs::EventBus bus;
    int count = 0;
    bus.subscribe<obs::JobStartedEvent>([&count](const obs::JobStartedEvent& e) {
        count++;
    });
    bus.publish(obs::JobStartedEvent{"job1", "agent1"});
    bus.publish(obs::JobStartedEvent{"job2", "agent1"});
    assert(count == 2);
    std::cout << "[PASS] test_event_bus" << std::endl;
}

void test_command_queue() {
    obs::CommandQueue queue;
    assert(queue.size() == 0);

    class SimpleCommand : public obs::ICommand {
    public:
        obs::CommandResult execute() override { return {true, "", {}}; }
        std::string command_name() const override { return "simple"; }
    };

    queue.enqueue(std::make_shared<SimpleCommand>());
    assert(queue.size() == 1);

    auto cmd = queue.dequeue();
    assert(cmd != nullptr);
    assert(cmd->command_name() == "simple");
    assert(queue.size() == 0);
    std::cout << "[PASS] test_command_queue" << std::endl;
}

void test_agent_factory() {
    auto& factory = obs::AgentFactory::instance();

    auto generic = factory.create(obs::AgentType::GENERIC);
    assert(generic != nullptr);
    assert(generic->type() == obs::AgentType::GENERIC);
    assert(generic->type_name() == "generic");

    auto esxi = factory.create_by_name("esxi");
    assert(esxi != nullptr);
    assert(esxi->type() == obs::AgentType::ESXi);

    auto hyperv = factory.create_by_name("hyperv");
    assert(hyperv != nullptr);
    assert(hyperv->type() == obs::AgentType::HYPERV);

    auto mssql = factory.create_by_name("mssql");
    assert(mssql != nullptr);
    assert(mssql->type() == obs::AgentType::MSSQL);

    auto pg = factory.create_by_name("postgresql");
    assert(pg != nullptr);
    assert(pg->type() == obs::AgentType::POSTGRESQL);

    auto oracle = factory.create_by_name("oracle");
    assert(oracle != nullptr);
    assert(oracle->type() == obs::AgentType::ORACLE);

    auto proxmox = factory.create_by_name("proxmox");
    assert(proxmox != nullptr);
    assert(proxmox->type() == obs::AgentType::PROXMOX);

    auto k8s = factory.create_by_name("kubernetes");
    assert(k8s != nullptr);
    assert(k8s->type() == obs::AgentType::KUBERNETES);

    auto linux_fs = factory.create_by_name("linux_fs");
    assert(linux_fs != nullptr);
    assert(linux_fs->type() == obs::AgentType::LINUX_FS);

    auto ndmp = factory.create_by_name("ndmp");
    assert(ndmp != nullptr);
    assert(ndmp->type() == obs::AgentType::NDMP);

    auto exchange = factory.create_by_name("exchange");
    assert(exchange != nullptr);
    assert(exchange->type() == obs::AgentType::EXCHANGE);

    auto types = factory.supported_types();
    assert(types.size() >= 12);

    std::cout << "[PASS] test_agent_factory" << std::endl;
}

void test_engine_factory() {
    auto& factory = obs::EngineFactory::instance();
    auto pipeline = factory.create_pipeline("/tmp/obs_test_data", "/tmp/obs_test_cache");

    assert(pipeline != nullptr);
    assert(pipeline->backup_engine() != nullptr);
    assert(pipeline->dedup_engine() != nullptr);
    assert(pipeline->crypto_engine() != nullptr);
    assert(pipeline->compression_engine() != nullptr);
    assert(pipeline->transport_engine() != nullptr);

    std::cout << "[PASS] test_engine_factory" << std::endl;
}

void test_backup_pipeline() {
    auto& factory = obs::EngineFactory::instance();
    auto pipeline = factory.create_pipeline("/tmp/obs_test_data", "/tmp/obs_test_cache");

    obs::BackupRequest request;
    request.job_id = "test_job_001";
    request.source_path = "/tmp/obs_test_data";
    request.target_path = "/tmp/obs_test_backup";
    request.compress = false;
    request.encrypt = false;

    obs::BackupResult result = pipeline->execute(request);
    assert(result.success);
    std::cout << "[PASS] test_backup_pipeline" << std::endl;
}

void test_dedup_engine() {
    std::cout << "  Creating pipeline..." << std::endl;
    auto& factory = obs::EngineFactory::instance();
    auto pipeline = factory.create_pipeline("/tmp/obs_test_data", "/tmp/obs_test_cache");
    auto dedup = pipeline->dedup_engine();
    std::cout << "  Engine created, hashing..." << std::endl;

    const char* data1 = "Hello, OBS Backup deduplication test!";
    std::string hash1 = dedup->hash_data(
        reinterpret_cast<const uint8_t*>(data1), strlen(data1));
    std::cout << "  Hash1: " << hash1.substr(0, 16) << "... size=" << hash1.size() << std::endl;
    assert(hash1.size() == 128); // BLAKE2b-512

    // Deterministic
    std::string hash2 = dedup->hash_data(
        reinterpret_cast<const uint8_t*>(data1), strlen(data1));
    assert(hash1 == hash2);

    // Different data -> different hash
    const char* data2 = "Different data";
    std::string hash3 = dedup->hash_data(
        reinterpret_cast<const uint8_t*>(data2), strlen(data2));
    assert(hash1 != hash3);

    // Store and retrieve
    dedup->store_chunk(hash1, reinterpret_cast<const uint8_t*>(data1), strlen(data1));
    assert(dedup->chunk_exists(hash1));
    assert(!dedup->chunk_exists(hash3));

    auto chunk = dedup->retrieve_chunk(hash1);
    assert(chunk.has_value());
    assert(chunk->hash == hash1);

    std::cout << "[PASS] test_dedup_engine" << std::endl;
}

void test_crypto_engine() {
    auto& factory = obs::EngineFactory::instance();
    auto pipeline = factory.create_pipeline("/tmp/obs_test_data", "/tmp/obs_test_cache");
    auto crypto = pipeline->crypto_engine();

    // Generate key
    std::string key = crypto->generate_key();
    assert(key.size() == 64); // 256-bit = 64 hex

    // Encrypt/decrypt roundtrip
    const char* plaintext = "Secret message for OBS Backup!";
    std::vector<uint8_t> data(plaintext, plaintext + strlen(plaintext));

    auto encrypted = crypto->encrypt(data.data(), data.size(), key);
    assert(encrypted.size() > data.size());

    auto decrypted = crypto->decrypt(encrypted.data(), encrypted.size(), key);
    assert(decrypted == data);

    // Wrong key fails
    std::string key2 = crypto->generate_key();
    try {
        crypto->decrypt(encrypted.data(), encrypted.size(), key2);
        assert(false && "Should have thrown");
    } catch (const std::exception&) {}

    // Hash
    std::string hash = crypto->hash(data.data(), data.size());
    assert(hash.size() == 128);

    std::cout << "[PASS] test_crypto_engine" << std::endl;
}

void test_compression_engine() {
    auto& factory = obs::EngineFactory::instance();
    auto pipeline = factory.create_pipeline("/tmp/obs_test_data", "/tmp/obs_test_cache");
    auto comp = pipeline->compression_engine();

    const char* data = "AAAAABBBBBCCCCCDDDDD";
    std::vector<uint8_t> input(data, data + strlen(data));

    auto compressed = comp->compress(input.data(), input.size());
    assert(compressed.size() <= input.size());

    auto decompressed = comp->decompress(compressed.data(), compressed.size());
    assert(decompressed == input);

    std::cout << "[PASS] test_compression_engine" << std::endl;
}

void test_agent_can_handle() {
    auto& factory = obs::AgentFactory::instance();

    auto esxi = factory.create_by_name("esxi");
    assert(esxi->can_handle("vm_backup"));
    assert(esxi->can_handle("vmware"));
    assert(esxi->can_handle("esxi"));
    assert(!esxi->can_handle("database_backup"));

    auto mssql = factory.create_by_name("mssql");
    assert(mssql->can_handle("database_backup"));
    assert(mssql->can_handle("mssql"));
    assert(!mssql->can_handle("vm_backup"));

    auto k8s = factory.create_by_name("kubernetes");
    assert(k8s->can_handle("kubernetes"));
    assert(k8s->can_handle("k8s"));

    std::cout << "[PASS] test_agent_can_handle" << std::endl;
}

int main() {
    test_service_registry();
    test_event_bus();
    test_command_queue();
    test_agent_factory();
    test_engine_factory();
    test_backup_pipeline();
    test_dedup_engine();
    test_crypto_engine();
    test_compression_engine();
    test_agent_can_handle();
    std::cout << "\nAll OOP tests passed!" << std::endl;
    return 0;
}
