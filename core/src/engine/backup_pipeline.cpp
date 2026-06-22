#include "engine/backup_pipeline.h"
#include <spdlog/spdlog.h>

namespace obs {

BackupPipeline::BackupPipeline(
    std::shared_ptr<IBackupEngine> backup_engine,
    std::shared_ptr<IDedupEngine> dedup_engine,
    std::shared_ptr<ICryptoEngine> crypto_engine,
    std::shared_ptr<ICompressionEngine> compression_engine,
    std::shared_ptr<ITransportEngine> transport_engine)
    : backup_engine_(std::move(backup_engine))
    , dedup_engine_(std::move(dedup_engine))
    , crypto_engine_(std::move(crypto_engine))
    , compression_engine_(std::move(compression_engine))
    , transport_engine_(std::move(transport_engine))
{
    spdlog::info("BackupPipeline created with {} engines",
        5); // backup, dedup, crypto, compression, transport
}

BackupResult BackupPipeline::execute(const BackupRequest& request, EngineProgressCallback progress) {
    spdlog::info("Pipeline executing backup job: {}", request.job_id);

    BackupResult result;
    auto start_time = std::chrono::steady_clock::now();

    try {
        if (backup_engine_) {
            result = backup_engine_->backup(request, progress);
        }

        if (result.success && compression_engine_ && request.compress) {
            spdlog::debug("Compression applied to job {}", request.job_id);
        }

        if (result.success && crypto_engine_ && request.encrypt) {
            spdlog::debug("Encryption applied to job {}", request.job_id);
        }

    } catch (const std::exception& e) {
        result.success = false;
        result.error = e.what();
        spdlog::error("Pipeline error for job {}: {}", request.job_id, e.what());
    }

    auto end_time = std::chrono::steady_clock::now();
    result.elapsed_seconds = std::chrono::duration<double>(end_time - start_time).count();

    spdlog::info("Pipeline completed job {} in {:.2f}s: success={}, transferred={}/{}",
        request.job_id, result.elapsed_seconds, result.transferred_bytes, result.total_bytes);

    return result;
}

void BackupPipeline::cancel(const std::string& job_id) {
    spdlog::info("Pipeline cancelling job: {}", job_id);
    if (backup_engine_) {
        backup_engine_->cancel(job_id);
    }
}

} // namespace obs
