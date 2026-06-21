#pragma once
#include "common.h"
#include <functional>
#include <vector>
#include <string>
#include <map>

namespace obs {

enum class S3Provider { AWS, YANDEX, VK_CLOUD, CEPH_RGW, MINIO, CUSTOM };
enum class StorageClass { STANDARD, STANDARD_IA, GLACIER, DEEP_GLACIER, INTELLIGENT_TIERING };

struct S3Config {
    std::string endpoint;
    std::string access_key;
    std::string secret_key;
    std::string bucket;
    std::string region;
    S3Provider provider = S3Provider::CUSTOM;
    bool use_ssl = true;
    bool path_style = true;
    bool versioning_enabled = false;
    bool inventory_enabled = false;
    bool events_enabled = false;
    std::string inventory_id;
    std::string sqs_queue_url;
    std::string sqs_arn;
};

struct S3ObjectInfo {
    std::string key;
    uint64_t size;
    std::string etag;
    std::string last_modified;
    std::string storage_class;
    std::string version_id;
    bool is_latest;
};

struct S3InventoryRow {
    std::string key;
    uint64_t size;
    std::string etag;
    std::string last_modified;
    std::string storage_class;
    std::string is_delete_marker;
};

struct S3BackupResult {
    bool success = false;
    int32_t objects_total = 0;
    int32_t objects_changed = 0;
    int32_t objects_new = 0;
    int32_t objects_deleted = 0;
    int32_t objects_skipped = 0;
    uint64_t bytes_transferred = 0;
    uint64_t bytes_saved = 0;
    double dedup_ratio = 0;
    int32_t elapsed_seconds = 0;
    std::string error;
};

class S3Backup {
public:
    S3Backup();
    ~S3Backup();

    bool test_connection(const S3Config& config);
    std::vector<S3ObjectInfo> list_objects(const S3Config& config, const std::string& prefix = "");

    S3BackupResult inventory_backup(const S3Config& config, const std::string& inventory_path);
    S3BackupResult versioning_backup(const S3Config& config);
    S3BackupResult events_backup(const S3Config& config, const std::string& since_time);
    S3BackupResult etag_dedup_backup(const S3Config& config, const std::string& local_cache_path);

    S3BackupResult full_backup(const S3Config& config, const std::string& output_path,
                               std::function<void(const S3BackupResult&)> callback = nullptr);
    S3BackupResult incremental_backup(const S3Config& config, const std::string& output_path,
                                      std::function<void(const S3BackupResult&)> callback = nullptr);

    bool restore_object(const S3Config& config, const std::string& key, const std::string& output_path);
    bool restore_version(const S3Config& config, const std::string& key, const std::string& version_id,
                        const std::string& output_path);

    bool set_storage_class(const S3Config& config, const std::string& key, StorageClass sc);
    bool restore_from_glacier(const S3Config& config, const std::string& key, int days = 7);

    std::vector<S3InventoryRow> parse_inventory_csv(const std::string& csv_path);
    std::vector<S3InventoryRow> diff_inventories(const std::vector<S3InventoryRow>& baseline,
                                                  const std::vector<S3InventoryRow>& current);

private:
    std::string aws_cli(const std::string& command, const S3Config& config);
    std::string curl_request(const std::string& url, const std::string& method,
                             const std::string& body, const S3Config& config);
    std::string sign_request(const std::string& method, const std::string& url,
                            const std::string& body, const S3Config& config);
    std::string compute_etag_md5(const std::string& path);
};

} // namespace obs
