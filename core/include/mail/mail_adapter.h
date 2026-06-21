#pragma once
#include "common.h"
#include <functional>

namespace obs {

enum class MailSystem { EXCHANGE, COMMUNIGATE, VK_WORKSPACE, RUPOST, MAILION };

struct MailConfig {
    std::string server;
    uint16_t port = 443;
    std::string username;
    std::string password;
    bool use_ssl = true;
    std::string domain;
    std::string organization_unit;
};

struct MailBackupProgress {
    std::string job_id;
    std::string status;
    uint64_t total_items = 0;
    uint64_t backed_up_items = 0;
    uint64_t total_bytes = 0;
    uint64_t transferred_bytes = 0;
    std::string current_mailbox;
    double speed_mbps = 0;
};

using MailBackupCallback = std::function<void(const MailBackupProgress&)>;

struct MailboxInfo {
    std::string email;
    std::string display_name;
    uint64_t total_size;
    int32_t message_count;
    std::string last_activity;
    bool is_active;
};

struct PublicFolder {
    std::string path;
    uint64_t size;
    int32_t item_count;
};

class MailAdapter {
public:
    virtual ~MailAdapter() = default;
    virtual MailSystem type() const = 0;
    virtual std::string name() const = 0;

    virtual bool connect(const MailConfig& config) = 0;
    virtual void disconnect() = 0;
    virtual bool test_connection() = 0;

    virtual bool backup_all(const std::string& output_path, MailBackupCallback callback) = 0;
    virtual bool backup_mailbox(const std::string& mailbox, const std::string& output_path,
                               MailBackupCallback callback) = 0;
    virtual bool backup_public_folders(const std::string& output_path, MailBackupCallback callback) = 0;

    virtual bool restore_mailbox(const std::string& mailbox, const std::string& backup_path,
                                MailBackupCallback callback) = 0;
    virtual bool restore_single_item(const std::string& backup_path, const std::string& item_id,
                                    const std::string& target_mailbox) = 0;

    virtual std::vector<MailboxInfo> list_mailboxes() = 0;
    virtual std::vector<PublicFolder> list_public_folders() { return {}; }
    virtual std::vector<std::string> list_distribution_groups() { return {}; }

    virtual bool supports_granular_restore() const { return false; }
    virtual bool supports_public_folders() const { return false; }
    virtual bool supports_distribution_groups() const { return false; }
    virtual bool supports_journaling() const { return false; }
    virtual bool supports_compliance() const { return false; }
    virtual bool supports_shared_mailboxes() const { return false; }
    virtual bool supports_resource_mailboxes() const { return false; }
    virtual bool supports_archive() const { return false; }

    virtual std::vector<std::string> supported_versions() const = 0;
};

class ExchangeAdapter : public MailAdapter {
public:
    MailSystem type() const override { return MailSystem::EXCHANGE; }
    std::string name() const override { return "Microsoft Exchange"; }
    bool connect(const MailConfig& config) override;
    void disconnect() override;
    bool test_connection() override;
    bool backup_all(const std::string& output_path, MailBackupCallback callback) override;
    bool backup_mailbox(const std::string& mailbox, const std::string& output_path, MailBackupCallback callback) override;
    bool backup_public_folders(const std::string& output_path, MailBackupCallback callback) override;
    bool restore_mailbox(const std::string& mailbox, const std::string& backup_path, MailBackupCallback callback) override;
    bool restore_single_item(const std::string& backup_path, const std::string& item_id, const std::string& target_mailbox) override;
    std::vector<MailboxInfo> list_mailboxes() override;
    std::vector<PublicFolder> list_public_folders() override;
    std::vector<std::string> list_distribution_groups() override;

    bool supports_granular_restore() const override { return true; }
    bool supports_public_folders() const override { return true; }
    bool supports_distribution_groups() const override { return true; }
    bool supports_journaling() const override { return true; }
    bool supports_compliance() const override { return true; }
    bool supports_archive() const override { return true; }
    std::vector<std::string> supported_versions() const override {
        return {"Exchange 2013", "Exchange 2016", "Exchange 2019", "Exchange Online (M365)"};
    }

    // Exchange-specific: EWS/Graph API backup
    bool ews_backup(const std::string& mailbox, const std::string& output_path, MailBackupCallback callback);
    bool graph_api_backup(const std::string& mailbox, const std::string& output_path, MailBackupCallback callback);

    // Exchange-specific: journal backup
    bool backup_journal(const std::string& journal_mailbox, const std::string& output_path);

    // Exchange-specific: database backup (VSS)
    bool backup_database(const std::string& db_name, const std::string& backup_path);
};

class CommuniGateAdapter : public MailAdapter {
public:
    MailSystem type() const override { return MailSystem::COMMUNIGATE; }
    std::string name() const override { return "CommuniGate Pro"; }
    bool connect(const MailConfig& config) override;
    void disconnect() override;
    bool test_connection() override;
    bool backup_all(const std::string& output_path, MailBackupCallback callback) override;
    bool backup_mailbox(const std::string& mailbox, const std::string& output_path, MailBackupCallback callback) override;
    bool backup_public_folders(const std::string& output_path, MailBackupCallback callback) override;
    bool restore_mailbox(const std::string& mailbox, const std::string& backup_path, MailBackupCallback callback) override;
    bool restore_single_item(const std::string& backup_path, const std::string& item_id, const std::string& target_mailbox) override;
    std::vector<MailboxInfo> list_mailboxes() override;
    bool supports_public_folders() const override { return true; }
    std::vector<std::string> supported_versions() const override { return {"CommuniGate Pro 6.x-8.x"}; }

    // CommuniGate-specific: AdminWebClient API
    bool admin_api_backup(const std::string& output_path, MailBackupCallback callback);
    // WebDAV backup
    bool webdav_backup(const std::string& mailbox, const std::string& output_path);
};

class VKWorkspaceAdapter : public MailAdapter {
public:
    MailSystem type() const override { return MailSystem::VK_WORKSPACE; }
    std::string name() const override { return "VK WorkSpace (Почта)"; }
    bool connect(const MailConfig& config) override;
    void disconnect() override;
    bool test_connection() override;
    bool backup_all(const std::string& output_path, MailBackupCallback callback) override;
    bool backup_mailbox(const std::string& mailbox, const std::string& output_path, MailBackupCallback callback) override;
    bool backup_public_folders(const std::string& output_path, MailBackupCallback callback) override;
    bool restore_mailbox(const std::string& mailbox, const std::string& backup_path, MailBackupCallback callback) override;
    bool restore_single_item(const std::string& backup_path, const std::string& item_id, const std::string& target_mailbox) override;
    std::vector<MailboxInfo> list_mailboxes() override;
    std::vector<std::string> supported_versions() const override { return {"VK WorkSpace Mail"}; }

    // VK-specific: CalDAV/CardDAV backup
    bool backup_caldav(const std::string& mailbox, const std::string& output_path);
    bool backup_carddav(const std::string& mailbox, const std::string& output_path);
    // Disk backup
    bool backup_disk(const std::string& mailbox, const std::string& output_path);
};

class RuPostAdapter : public MailAdapter {
public:
    MailSystem type() const override { return MailSystem::RUPOST; }
    std::string name() const override { return "RuPost"; }
    bool connect(const MailConfig& config) override;
    void disconnect() override;
    bool test_connection() override;
    bool backup_all(const std::string& output_path, MailBackupCallback callback) override;
    bool backup_mailbox(const std::string& mailbox, const std::string& output_path, MailBackupCallback callback) override;
    bool backup_public_folders(const std::string& output_path, MailBackupCallback callback) override;
    bool restore_mailbox(const std::string& mailbox, const std::string& backup_path, MailBackupCallback callback) override;
    bool restore_single_item(const std::string& backup_path, const std::string& item_id, const std::string& target_mailbox) override;
    std::vector<MailboxInfo> list_mailboxes() override;
    std::vector<std::string> supported_versions() const override { return {"RuPost 3.x"}; }
};

class MailionAdapter : public MailAdapter {
public:
    MailSystem type() const override { return MailSystem::MAILION; }
    std::string name() const override { return "Mailion"; }
    bool connect(const MailConfig& config) override;
    void disconnect() override;
    bool test_connection() override;
    bool backup_all(const std::string& output_path, MailBackupCallback callback) override;
    bool backup_mailbox(const std::string& mailbox, const std::string& output_path, MailBackupCallback callback) override;
    bool backup_public_folders(const std::string& output_path, MailBackupCallback callback) override;
    bool restore_mailbox(const std::string& mailbox, const std::string& backup_path, MailBackupCallback callback) override;
    bool restore_single_item(const std::string& backup_path, const std::string& item_id, const std::string& target_mailbox) override;
    std::vector<MailboxInfo> list_mailboxes() override;
    std::vector<std::string> supported_versions() const override { return {"Mailion"}; }
};

} // namespace obs
