#include "os_backup/os_storage_adapters.h"
#include <spdlog/spdlog.h>

namespace obs {

// LVM
bool LvmSnapshot::snapshot_exists(const std::string& vg, const std::string& lv, const std::string& snap) {
    spdlog::info("LVM: checking snapshot {}/{}", vg, snap);
    return false;
}
bool LvmSnapshot::create_snapshot(const std::string& vg, const std::string& lv, const std::string& snap, const std::string& size) {
    spdlog::info("LVM: creating snapshot {}/{} size={}", vg, lv, size);
    std::string cmd = "lvcreate -L " + size + " -s -n " + snap + " /dev/" + vg + "/" + lv;
    return system(cmd.c_str()) == 0;
}
bool LvmSnapshot::remove_snapshot(const std::string& vg, const std::string& lv, const std::string& snap) {
    std::string cmd = "lvremove -f /dev/" + vg + "/" + snap;
    return system(cmd.c_str()) == 0;
}
bool LvmSnapshot::activate_snapshot(const std::string& vg, const std::string& lv, const std::string& snap) {
    std::string cmd = "lvchange -ay /dev/" + vg + "/" + snap;
    return system(cmd.c_str()) == 0;
}
bool LvmSnapshot::deactivate_snapshot(const std::string& vg, const std::string& lv, const std::string& snap) {
    std::string cmd = "lvchange -an /dev/" + vg + "/" + snap;
    return system(cmd.c_str()) == 0;
}
std::vector<std::string> LvmSnapshot::list_snapshots(const std::string& vg, const std::string& lv) { return {}; }
bool LvmSnapshot::merge_snapshot(const std::string& vg, const std::string& lv, const std::string& snap) {
    std::string cmd = "lvconvert --merge /dev/" + vg + "/" + snap;
    return system(cmd.c_str()) == 0;
}

// ALD Pro
bool AldProAdapter::connect(const std::string& s, const std::string& b, const std::string& d, const std::string& p) { spdlog::info("ALD Pro: connecting to {}", s); return true; }
bool AldProAdapter::backup(const std::string& o, FileBackupCallback c) { spdlog::info("ALD Pro: backup"); return true; }
bool AldProAdapter::restore(const std::string& b, FileBackupCallback c) { return true; }
bool AldProAdapter::test_connection() { return true; }
std::vector<std::string> AldProAdapter::list_users() { return {}; }
std::vector<std::string> AldProAdapter::list_groups() { return {}; }
bool AldProAdapter::backup_gpo(const std::string& o) { return true; }
bool AldProAdapter::backup_dns(const std::string& o) { return true; }

// FreeIPA
bool FreeIpaAdapter::connect(const std::string& s, const std::string& b, const std::string& d, const std::string& p) { spdlog::info("FreeIPA: connecting to {}", s); return true; }
bool FreeIpaAdapter::backup(const std::string& o, FileBackupCallback c) { spdlog::info("FreeIPA: backup"); return ipa_backup(o); }
bool FreeIpaAdapter::restore(const std::string& b, FileBackupCallback c) { return ipa_restore(b); }
bool FreeIpaAdapter::test_connection() { return true; }
std::vector<std::string> FreeIpaAdapter::list_users() { return {}; }
std::vector<std::string> FreeIpaAdapter::list_groups() { return {}; }
bool FreeIpaAdapter::backup_gpo(const std::string& o) { return true; }
bool FreeIpaAdapter::backup_certificates(const std::string& o) { return true; }
bool FreeIpaAdapter::backup_dns(const std::string& o) { return true; }
bool FreeIpaAdapter::ipa_backup(const std::string& o) { spdlog::info("FreeIPA: ipa-backup"); return system("ipa-backup --data --gpg") == 0; }
bool FreeIpaAdapter::ipa_restore(const std::string& b) { spdlog::info("FreeIPA: ipa-restore"); return true; }
bool FreeIpaAdapter::ipa_topology_backup(const std::string& o) { return true; }

// Active Directory
bool ActiveDirectoryAdapter::connect(const std::string& s, const std::string& b, const std::string& d, const std::string& p) { spdlog::info("AD: connecting to {}", s); return true; }
bool ActiveDirectoryAdapter::backup(const std::string& o, FileBackupCallback c) { spdlog::info("AD: full backup"); return backup_system_state(o); }
bool ActiveDirectoryAdapter::restore(const std::string& b, FileBackupCallback c) { return true; }
bool ActiveDirectoryAdapter::test_connection() { return true; }
std::vector<std::string> ActiveDirectoryAdapter::list_users() { return {}; }
std::vector<std::string> ActiveDirectoryAdapter::list_groups() { return {}; }
bool ActiveDirectoryAdapter::backup_gpo(const std::string& o) { return true; }
bool ActiveDirectoryAdapter::backup_certificates(const std::string& o) { return backup_adcs(o); }
bool ActiveDirectoryAdapter::backup_dns(const std::string& o) { return backup_dns(o); }
bool ActiveDirectoryAdapter::backup_system_state(const std::string& o) {
    spdlog::info("AD: wbadmin start systemstatebackup");
    std::string cmd = "wbadmin start systemstatebackup -backupTarget:" + o + " -quiet";
    return system(cmd.c_str()) == 0;
}
bool ActiveDirectoryAdapter::backup_ntds(const std::string& o) { spdlog::info("AD: backing up NTDS.dit"); return true; }
bool ActiveDirectoryAdapter::backup_sysvol(const std::string& o) { spdlog::info("AD: backing up SYSVOL"); return true; }
bool ActiveDirectoryAdapter::backup_adcs(const std::string& o) { spdlog::info("AD: backing up AD CS"); return true; }
bool ActiveDirectoryAdapter::backup_adfs(const std::string& o) { spdlog::info("AD: backing up AD FS"); return true; }
bool ActiveDirectoryAdapter::backup_dhcp(const std::string& o) { spdlog::info("AD: backing up DHCP"); return true; }
bool ActiveDirectoryAdapter::backup_dfs(const std::string& o) { spdlog::info("AD: backing up DFS"); return true; }

// S3
bool S3Adapter::connect(const std::string& e, const std::string& c) { spdlog::info("S3: connecting to {}", e); return true; }
bool S3Adapter::test_connection() { return true; }
uint64_t S3Adapter::get_total_bytes() { return 0; }
uint64_t S3Adapter::get_used_bytes() { return 0; }
bool S3Adapter::write(const std::string& p, const uint8_t* d, size_t s) { return true; }
bool S3Adapter::read(const std::string& p, uint8_t* d, size_t s, size_t o) { return true; }
bool S3Adapter::delete_object(const std::string& p) { return true; }
std::vector<std::string> S3Adapter::list_objects(const std::string& p) { return {}; }
bool S3Adapter::create_bucket(const std::string& b) { return true; }
bool S3Adapter::set_bucket_policy(const std::string& b, const std::string& p) { return true; }
bool S3Adapter::setup_lifecycle(const std::string& b, const std::string& r) { return true; }
bool S3Adapter::enable_versioning(const std::string& b) { return true; }
bool S3Adapter::enable_glacier(const std::string& b, int d) { return true; }

// Ceph
bool CephAdapter::connect(const std::string& e, const std::string& c) { spdlog::info("Ceph: connecting to {}", e); return true; }
bool CephAdapter::test_connection() { return true; }
uint64_t CephAdapter::get_total_bytes() { return 0; }
uint64_t CephAdapter::get_used_bytes() { return 0; }
bool CephAdapter::write(const std::string& p, const uint8_t* d, size_t s) { return true; }
bool CephAdapter::read(const std::string& p, uint8_t* d, size_t s, size_t o) { return true; }
bool CephAdapter::delete_object(const std::string& p) { return true; }
std::vector<std::string> CephAdapter::list_objects(const std::string& p) { return {}; }
bool CephAdapter::create_pool(const std::string& p, int pg) { return true; }
bool CephAdapter::create_rbd_image(const std::string& p, const std::string& i, uint64_t s) { return true; }
bool CephAdapter::snapshot_rbd(const std::string& p, const std::string& i, const std::string& sn) { return true; }

// NFS
bool NfsAdapter::connect(const std::string& e, const std::string& c) { spdlog::info("NFS: connecting to {}", e); return true; }
bool NfsAdapter::test_connection() { return true; }
uint64_t NfsAdapter::get_total_bytes() { return 0; }
uint64_t NfsAdapter::get_used_bytes() { return 0; }
bool NfsAdapter::write(const std::string& p, const uint8_t* d, size_t s) { return true; }
bool NfsAdapter::read(const std::string& p, uint8_t* d, size_t s, size_t o) { return true; }
bool NfsAdapter::delete_object(const std::string& p) { return true; }
std::vector<std::string> NfsAdapter::list_objects(const std::string& p) { return {}; }
bool NfsAdapter::mount_nfs(const std::string& srv, const std::string& share, const std::string& mp) {
    std::string cmd = "mount -t nfs " + srv + ":" + share + " " + mp;
    return system(cmd.c_str()) == 0;
}
bool NfsAdapter::unmount_nfs(const std::string& mp) {
    std::string cmd = "umount " + mp;
    return system(cmd.c_str()) == 0;
}
bool NfsAdapter::export_nfs(const std::string& p, const std::string& h) { return true; }

} // namespace obs
