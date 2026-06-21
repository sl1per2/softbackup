#pragma once
#include "common.h"
#include <functional>

namespace obs {

enum class FsType { EXT2, EXT3, EXT4, XFS, ZFS, BTRFS, FAT32, NTFS };
enum class OsType { LINUX, WINDOWS };

struct FilesystemInfo {
    FsType type;
    std::string device;
    std::string mount_point;
    uint64_t total_bytes;
    uint64_t used_bytes;
    std::string uuid;
    bool supports_snapshots;
    bool supports_reflink;
    bool supports_cow;
};

struct FileBackupConfig {
    std::string job_id;
    std::vector<std::string> source_paths;
    std::vector<std::string> exclude_patterns;
    std::string output_path;
    bool compress = true;
    bool encrypt = false;
    int compression_level = 1;
    bool follow_symlinks = false;
    bool preserve_permissions = true;
    bool preserve_acls = true;
    bool preserve_xattrs = true;
    bool preserve_extended_attrs = true;
    bool incremental = false;
    std::string previous_backup_path;
    int max_depth = -1; // -1 = unlimited
    uint64_t max_file_size = 0; // 0 = unlimited
    std::vector<std::string> include_extensions;
    std::vector<std::string> exclude_extensions;
};

struct FileBackupProgress {
    std::string job_id;
    std::string status;
    uint64_t total_files = 0;
    uint64_t backed_up_files = 0;
    uint64_t total_bytes = 0;
    uint64_t transferred_bytes = 0;
    std::string current_file;
    double speed_mbps = 0;
    double compression_ratio = 0;
    uint64_t dedup_saved = 0;
};

using FileBackupCallback = std::function<void(const FileBackupProgress&)>;

struct SnapshotInfo {
    std::string name;
    std::string created_at;
    uint64_t used_bytes;
    std::string mount_point;
};

// LVM snapshot support
class LvmSnapshot {
public:
    bool snapshot_exists(const std::string& vg_name, const std::string& lv_name, const std::string& snap_name);
    bool create_snapshot(const std::string& vg_name, const std::string& lv_name,
                        const std::string& snap_name, const std::string& size);
    bool remove_snapshot(const std::string& vg_name, const std::string& lv_name, const std::string& snap_name);
    bool activate_snapshot(const std::string& vg_name, const std::string& lv_name, const std::string& snap_name);
    bool deactivate_snapshot(const std::string& vg_name, const std::string& lv_name, const std::string& snap_name);
    std::vector<std::string> list_snapshots(const std::string& vg_name, const std::string& lv_name);
    bool merge_snapshot(const std::string& vg_name, const std::string& lv_name, const std::string& snap_name);
};

// ALD Pro / FreeIPA / Active Directory
class DirectoryService {
public:
    virtual ~DirectoryService() = default;
    virtual std::string name() const = 0;
    virtual bool connect(const std::string& server, const std::string& base_dn,
                        const std::string& bind_dn, const std::string& password) = 0;
    virtual bool backup(const std::string& output_path, FileBackupCallback callback) = 0;
    virtual bool restore(const std::string& backup_path, FileBackupCallback callback) = 0;
    virtual bool test_connection() = 0;
    virtual std::vector<std::string> list_users() = 0;
    virtual std::vector<std::string> list_groups() = 0;
    virtual bool backup_gpo(const std::string& output_path) { return false; }
    virtual bool backup_certificates(const std::string& output_path) { return false; }
    virtual bool backup_dns(const std::string& output_path) { return false; }
};

class AldProAdapter : public DirectoryService {
public:
    std::string name() const override { return "ALD Pro"; }
    bool connect(const std::string& server, const std::string& base_dn,
                const std::string& bind_dn, const std::string& password) override;
    bool backup(const std::string& output_path, FileBackupCallback callback) override;
    bool restore(const std::string& backup_path, FileBackupCallback callback) override;
    bool test_connection() override;
    std::vector<std::string> list_users() override;
    std::vector<std::string> list_groups() override;
    bool backup_gpo(const std::string& output_path) override;
    bool backup_dns(const std::string& output_path) override;
};

class FreeIpaAdapter : public DirectoryService {
public:
    std::string name() const override { return "FreeIPA"; }
    bool connect(const std::string& server, const std::string& base_dn,
                const std::string& bind_dn, const std::string& password) override;
    bool backup(const std::string& output_path, FileBackupCallback callback) override;
    bool restore(const std::string& backup_path, FileBackupCallback callback) override;
    bool test_connection() override;
    std::vector<std::string> list_users() override;
    std::vector<std::string> list_groups() override;
    bool backup_gpo(const std::string& output_path) override;
    bool backup_certificates(const std::string& output_path) override;
    bool backup_dns(const std::string& output_path) override;

    // FreeIPA-specific: IPA backup/restore
    bool ipa_backup(const std::string& output_path);
    bool ipa_restore(const std::string& backup_path);
    bool ipa_topology_backup(const std::string& output_path);
};

class ActiveDirectoryAdapter : public DirectoryService {
public:
    std::string name() const override { return "Microsoft Active Directory"; }
    bool connect(const std::string& server, const std::string& base_dn,
                const std::string& bind_dn, const std::string& password) override;
    bool backup(const std::string& output_path, FileBackupCallback callback) override;
    bool restore(const std::string& backup_path, FileBackupCallback callback) override;
    bool test_connection() override;
    std::vector<std::string> list_users() override;
    std::vector<std::string> list_groups() override;
    bool backup_gpo(const std::string& output_path) override;
    bool backup_certificates(const std::string& output_path) override;
    bool backup_dns(const std::string& output_path) override;

    // AD-specific: system state backup
    bool backup_system_state(const std::string& output_path);
    bool backup_ntds(const std::string& output_path);
    bool backup_sysvol(const std::string& output_path);
    bool backup_adcs(const std::string& output_path); // Certificate Services
    bool backup_adfs(const std::string& output_path); // Federation Services
    bool backup_dhcp(const std::string& output_path);
    bool backup_dfs(const std::string& output_path);
};

// Storage type adapters
class StorageAdapter {
public:
    virtual ~StorageAdapter() = default;
    virtual std::string name() const = 0;
    virtual bool connect(const std::string& endpoint, const std::string& credentials) = 0;
    virtual bool test_connection() = 0;
    virtual uint64_t get_total_bytes() = 0;
    virtual uint64_t get_used_bytes() = 0;
    virtual bool write(const std::string& path, const uint8_t* data, size_t size) = 0;
    virtual bool read(const std::string& path, uint8_t* data, size_t size, size_t offset = 0) = 0;
    virtual bool delete_object(const std::string& path) = 0;
    virtual std::vector<std::string> list_objects(const std::string& prefix = "") = 0;
    virtual bool supports_fast_clone() const { return false; }
    virtual bool supports_dedup() const { return false; }
    virtual bool supports_encryption() const { return true; }
    virtual bool supports_compression() const { return true; }
    virtual bool supports_versioning() const { return false; }
};

class S3Adapter : public StorageAdapter {
public:
    std::string name() const override { return "S3/Object Storage"; }
    bool connect(const std::string& endpoint, const std::string& credentials) override;
    bool test_connection() override;
    uint64_t get_total_bytes() override;
    uint64_t get_used_bytes() override;
    bool write(const std::string& path, const uint8_t* data, size_t size) override;
    bool read(const std::string& path, uint8_t* data, size_t size, size_t offset = 0) override;
    bool delete_object(const std::string& path) override;
    std::vector<std::string> list_objects(const std::string& prefix = "") override;
    bool supports_versioning() const override { return true; }
    bool supports_encryption() const override { return true; }

    // S3-specific
    bool create_bucket(const std::string& bucket);
    bool set_bucket_policy(const std::string& bucket, const std::string& policy);
    bool setup_lifecycle(const std::string& bucket, const std::string& rule);
    bool enable_versioning(const std::string& bucket);
    bool enable_glacier(const std::string& bucket, int transition_days);
};

class CephAdapter : public StorageAdapter {
public:
    std::string name() const override { return "Ceph RADOS"; }
    bool connect(const std::string& endpoint, const std::string& credentials) override;
    bool test_connection() override;
    uint64_t get_total_bytes() override;
    uint64_t get_used_bytes() override;
    bool write(const std::string& path, const uint8_t* data, size_t size) override;
    bool read(const std::string& path, uint8_t* data, size_t size, size_t offset = 0) override;
    bool delete_object(const std::string& path) override;
    std::vector<std::string> list_objects(const std::string& prefix = "") override;
    bool supports_dedup() const override { return true; }

    // Ceph-specific
    bool create_pool(const std::string& pool, int pg_num);
    bool create_rbd_image(const std::string& pool, const std::string& image, uint64_t size);
    bool snapshot_rbd(const std::string& pool, const std::string& image, const std::string& snap_name);
};

class NfsAdapter : public StorageAdapter {
public:
    std::string name() const override { return "NFS"; }
    bool connect(const std::string& endpoint, const std::string& credentials) override;
    bool test_connection() override;
    uint64_t get_total_bytes() override;
    uint64_t get_used_bytes() override;
    bool write(const std::string& path, const uint8_t* data, size_t size) override;
    bool read(const std::string& path, uint8_t* data, size_t size, size_t offset = 0) override;
    bool delete_object(const std::string& path) override;
    std::vector<std::string> list_objects(const std::string& prefix = "") override;
    bool supports_fast_clone() const override { return true; }

    // NFS-specific
    bool mount_nfs(const std::string& server, const std::string& share, const std::string& mount_point);
    bool unmount_nfs(const std::string& mount_point);
    bool export_nfs(const std::string& path, const std::string& allowed_hosts);
};

} // namespace obs
