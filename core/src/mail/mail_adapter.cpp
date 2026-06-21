#include "mail/mail_adapter.h"
#include <spdlog/spdlog.h>

namespace obs {

// Exchange
bool ExchangeAdapter::connect(const MailConfig& c) { spdlog::info("Exchange: connecting to {}", c.server); return true; }
void ExchangeAdapter::disconnect() {}
bool ExchangeAdapter::test_connection() { return true; }
bool ExchangeAdapter::backup_all(const std::string& out, MailBackupCallback cb) { spdlog::info("Exchange: backup all mailboxes"); return true; }
bool ExchangeAdapter::backup_mailbox(const std::string& mb, const std::string& out, MailBackupCallback cb) { spdlog::info("Exchange: backup mailbox {}", mb); return true; }
bool ExchangeAdapter::backup_public_folders(const std::string& out, MailBackupCallback cb) { return true; }
bool ExchangeAdapter::restore_mailbox(const std::string& mb, const std::string& bp, MailBackupCallback cb) { return true; }
bool ExchangeAdapter::restore_single_item(const std::string& bp, const std::string& id, const std::string& tgt) { return true; }
std::vector<MailboxInfo> ExchangeAdapter::list_mailboxes() { return {}; }
std::vector<PublicFolder> ExchangeAdapter::list_public_folders() { return {}; }
std::vector<std::string> ExchangeAdapter::list_distribution_groups() { return {}; }
bool ExchangeAdapter::ews_backup(const std::string& mb, const std::string& out, MailBackupCallback cb) { return true; }
bool ExchangeAdapter::graph_api_backup(const std::string& mb, const std::string& out, MailBackupCallback cb) { return true; }
bool ExchangeAdapter::backup_journal(const std::string& jm, const std::string& out) { return true; }
bool ExchangeAdapter::backup_database(const std::string& db, const std::string& bp) { return true; }

// CommuniGate Pro
bool CommuniGateAdapter::connect(const MailConfig& c) { spdlog::info("CommuniGate: connecting to {}", c.server); return true; }
void CommuniGateAdapter::disconnect() {}
bool CommuniGateAdapter::test_connection() { return true; }
bool CommuniGateAdapter::backup_all(const std::string& out, MailBackupCallback cb) { spdlog::info("CommuniGate: backup all"); return true; }
bool CommuniGateAdapter::backup_mailbox(const std::string& mb, const std::string& out, MailBackupCallback cb) { return true; }
bool CommuniGateAdapter::backup_public_folders(const std::string& out, MailBackupCallback cb) { return true; }
bool CommuniGateAdapter::restore_mailbox(const std::string& mb, const std::string& bp, MailBackupCallback cb) { return true; }
bool CommuniGateAdapter::restore_single_item(const std::string& bp, const std::string& id, const std::string& tgt) { return true; }
std::vector<MailboxInfo> CommuniGateAdapter::list_mailboxes() { return {}; }
bool CommuniGateAdapter::admin_api_backup(const std::string& out, MailBackupCallback cb) { return true; }
bool CommuniGateAdapter::webdav_backup(const std::string& mb, const std::string& out) { return true; }

// VK WorkSpace
bool VKWorkspaceAdapter::connect(const MailConfig& c) { spdlog::info("VK WorkSpace: connecting to {}", c.server); return true; }
void VKWorkspaceAdapter::disconnect() {}
bool VKWorkspaceAdapter::test_connection() { return true; }
bool VKWorkspaceAdapter::backup_all(const std::string& out, MailBackupCallback cb) { spdlog::info("VK WorkSpace: backup all"); return true; }
bool VKWorkspaceAdapter::backup_mailbox(const std::string& mb, const std::string& out, MailBackupCallback cb) { return true; }
bool VKWorkspaceAdapter::backup_public_folders(const std::string& out, MailBackupCallback cb) { return true; }
bool VKWorkspaceAdapter::restore_mailbox(const std::string& mb, const std::string& bp, MailBackupCallback cb) { return true; }
bool VKWorkspaceAdapter::restore_single_item(const std::string& bp, const std::string& id, const std::string& tgt) { return true; }
std::vector<MailboxInfo> VKWorkspaceAdapter::list_mailboxes() { return {}; }
bool VKWorkspaceAdapter::backup_caldav(const std::string& mb, const std::string& out) { return true; }
bool VKWorkspaceAdapter::backup_carddav(const std::string& mb, const std::string& out) { return true; }
bool VKWorkspaceAdapter::backup_disk(const std::string& mb, const std::string& out) { return true; }

// RuPost
bool RuPostAdapter::connect(const MailConfig& c) { spdlog::info("RuPost: connecting to {}", c.server); return true; }
void RuPostAdapter::disconnect() {}
bool RuPostAdapter::test_connection() { return true; }
bool RuPostAdapter::backup_all(const std::string& out, MailBackupCallback cb) { spdlog::info("RuPost: backup all"); return true; }
bool RuPostAdapter::backup_mailbox(const std::string& mb, const std::string& out, MailBackupCallback cb) { return true; }
bool RuPostAdapter::backup_public_folders(const std::string& out, MailBackupCallback cb) { return true; }
bool RuPostAdapter::restore_mailbox(const std::string& mb, const std::string& bp, MailBackupCallback cb) { return true; }
bool RuPostAdapter::restore_single_item(const std::string& bp, const std::string& id, const std::string& tgt) { return true; }
std::vector<MailboxInfo> RuPostAdapter::list_mailboxes() { return {}; }

// Mailion
bool MailionAdapter::connect(const MailConfig& c) { spdlog::info("Mailion: connecting to {}", c.server); return true; }
void MailionAdapter::disconnect() {}
bool MailionAdapter::test_connection() { return true; }
bool MailionAdapter::backup_all(const std::string& out, MailBackupCallback cb) { spdlog::info("Mailion: backup all"); return true; }
bool MailionAdapter::backup_mailbox(const std::string& mb, const std::string& out, MailBackupCallback cb) { return true; }
bool MailionAdapter::backup_public_folders(const std::string& out, MailBackupCallback cb) { return true; }
bool MailionAdapter::restore_mailbox(const std::string& mb, const std::string& bp, MailBackupCallback cb) { return true; }
bool MailionAdapter::restore_single_item(const std::string& bp, const std::string& id, const std::string& tgt) { return true; }
std::vector<MailboxInfo> MailionAdapter::list_mailboxes() { return {}; }

} // namespace obs
