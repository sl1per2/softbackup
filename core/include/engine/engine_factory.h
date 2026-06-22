#pragma once
#include "common.h"
#include "engine/i_backup_engine.h"
#include "engine/i_dedup_engine.h"
#include "engine/i_crypto_engine.h"
#include "engine/i_cdp_engine.h"
#include "engine/i_compression.h"
#include "engine/i_transport.h"
#include "engine/backup_pipeline.h"
#include "framework/service_registry.h"
#include "framework/event_bus.h"
#include <memory>

namespace obs {

class EngineFactory {
public:
    static EngineFactory& instance() {
        static EngineFactory factory;
        return factory;
    }

    std::shared_ptr<BackupPipeline> create_pipeline(
        const std::string& data_dir = "/var/lib/obs",
        const std::string& cache_dir = "/var/cache/obs")
    {
        auto backup_engine = create_backup_engine(data_dir);
        auto dedup_engine = create_dedup_engine(cache_dir);
        auto crypto_engine = create_crypto_engine();
        auto compression_engine = create_compression_engine();
        auto transport_engine = create_transport_engine();

        auto pipeline = std::make_shared<BackupPipeline>(
            backup_engine, dedup_engine, crypto_engine,
            compression_engine, transport_engine);

        if (registry_) {
            registry_->register_instance<IBackupEngine>(backup_engine);
            registry_->register_instance<IDedupEngine>(dedup_engine);
            registry_->register_instance<ICryptoEngine>(crypto_engine);
            registry_->register_instance<ICompressionEngine>(compression_engine);
            registry_->register_instance<ITransportEngine>(transport_engine);
        }

        return pipeline;
    }

    void set_registry(std::shared_ptr<ServiceRegistry> registry) { registry_ = registry; }

private:
    EngineFactory() = default;

    std::shared_ptr<IBackupEngine> create_backup_engine(const std::string& data_dir);
    std::shared_ptr<IDedupEngine> create_dedup_engine(const std::string& cache_dir);
    std::shared_ptr<ICryptoEngine> create_crypto_engine();
    std::shared_ptr<ICompressionEngine> create_compression_engine();
    std::shared_ptr<ITransportEngine> create_transport_engine();

    std::shared_ptr<ServiceRegistry> registry_;
};

} // namespace obs
