#include "backup_engine/db_backup.h"
#include <spdlog/spdlog.h>

namespace obs {

bool DatabaseBackup::backup(const DbBackupConfig& config, DbBackupResult& result) {
    spdlog::info("DatabaseBackup: starting backup for {}", config.database_name);
    result.status = "running";
    result.success = true;
    result.started_at = current_time_string();
    return true;
}

bool DatabaseBackup::backup_postgresql(const DbBackupConfig& config, DbBackupResult& result) {
    spdlog::info("DatabaseBackup: PostgreSQL backup of {}", config.database_name);
    return backup(config, result);
}

bool DatabaseBackup::backup_mysql(const DbBackupConfig& config, DbBackupResult& result) {
    spdlog::info("DatabaseBackup: MySQL backup of {}", config.database_name);
    return backup(config, result);
}

bool DatabaseBackup::backup_mongodb(const DbBackupConfig& config, DbBackupResult& result) {
    spdlog::info("DatabaseBackup: MongoDB backup of {}", config.database_name);
    return backup(config, result);
}

} // namespace obs
