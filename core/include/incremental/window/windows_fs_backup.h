#pragma once
#include "incremental/common/incremental_engine.h"

namespace vovqa::incremental {

enum class WindowsTrackingMethod { VSS_BITMAP, USN_JOURNAL, REFS_CBT, READDIR_CHANGES, AUTO };

class WindowsFsBackup : public IncrementalBackupEngine {
public:
    std::string name() const override { return "Windows FS Incremental"; }
    std::vector<BlockRange> detect_changes(const std::string& path) override;
    bool read_changed_blocks(const std::string& path, const std::vector<BlockRange>& ranges,
                             std::function<void(const uint8_t*, size_t, uint64_t)> callback) override;
    std::vector<BlockRange> merge_bitmaps(const std::vector<BlockRange>& a, const std::vector<BlockRange>& b) override;

    WindowsTrackingMethod detect_method(const std::string& path);
    void set_tracking_method(WindowsTrackingMethod m) { method_ = m; }

private:
    std::vector<BlockRange> detect_vss_bitmap(const std::string& path);
    std::vector<BlockRange> detect_usn_journal(const std::string& path);
    std::vector<BlockRange> detect_refs_cbt(const std::string& path);
    std::vector<BlockRange> detect_readdir(const std::string& path);

    bool create_vss_snapshot(const std::string& volume);
    bool read_usn_journal(const std::string& volume, uint64_t start_usn);
    std::vector<uint8_t> get_volume_bitmap(const std::string& volume);
    std::vector<uint8_t> xor_bitmaps(const std::vector<uint8_t>& a, const std::vector<uint8_t>& b);

    WindowsTrackingMethod method_ = WindowsTrackingMethod::AUTO;
    uint64_t last_usn_ = 0;
};

} // namespace vovqa::incremental
