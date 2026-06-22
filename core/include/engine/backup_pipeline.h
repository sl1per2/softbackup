#pragma once
#include "common.h"
#include "engine/i_backup_engine.h"
#include "engine/i_dedup_engine.h"
#include "engine/i_crypto_engine.h"
#include "engine/i_compression.h"
#include "engine/i_transport.h"
#include <memory>

namespace obs {

class BackupPipeline {
public:
    BackupPipeline(
        std::shared_ptr<IBackupEngine> backup_engine,
        std::shared_ptr<IDedupEngine> dedup_engine,
        std::shared_ptr<ICryptoEngine> crypto_engine,
        std::shared_ptr<ICompressionEngine> compression_engine,
        std::shared_ptr<ITransportEngine> transport_engine
    );

    BackupResult execute(const BackupRequest& request, EngineProgressCallback progress = nullptr);
    void cancel(const std::string& job_id);

    std::shared_ptr<IBackupEngine> backup_engine() const { return backup_engine_; }
    std::shared_ptr<IDedupEngine> dedup_engine() const { return dedup_engine_; }
    std::shared_ptr<ICryptoEngine> crypto_engine() const { return crypto_engine_; }
    std::shared_ptr<ICompressionEngine> compression_engine() const { return compression_engine_; }
    std::shared_ptr<ITransportEngine> transport_engine() const { return transport_engine_; }

private:
    std::shared_ptr<IBackupEngine> backup_engine_;
    std::shared_ptr<IDedupEngine> dedup_engine_;
    std::shared_ptr<ICryptoEngine> crypto_engine_;
    std::shared_ptr<ICompressionEngine> compression_engine_;
    std::shared_ptr<ITransportEngine> transport_engine_;
};

} // namespace obs
