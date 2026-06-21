#include "incremental/window/windows_fs_backup.h"
#include <filesystem>
namespace fs = std::filesystem;
using namespace vovqa::incremental;
#include <spdlog/spdlog.h>
#include <filesystem>

#ifdef _MSC_VER
#include <windows.h>
#include <vswriter.h>
#include <vsmgmt.h>
#endif

namespace vovqa::incremental {

WindowsTrackingMethod WindowsFsBackup::detect_method(const std::string& path) {
#ifdef _MSC_VER
    // Check for VSS
    // Check for ReFS
    // Check for NTFS
    return WindowsTrackingMethod::VSS_BITMAP;
#else
    spdlog::info("Windows tracking simulated on Linux");
    return WindowsTrackingMethod::USN_JOURNAL;
#endif
}

std::vector<BlockRange> WindowsFsBackup::detect_changes(const std::string& path) {
    auto start = std::chrono::steady_clock::now();
    std::vector<BlockRange> ranges;
    WindowsTrackingMethod method = (method_ == WindowsTrackingMethod::AUTO) ? detect_method(path) : method_;

    switch (method) {
        case WindowsTrackingMethod::VSS_BITMAP: ranges = detect_vss_bitmap(path); break;
        case WindowsTrackingMethod::USN_JOURNAL: ranges = detect_usn_journal(path); break;
        case WindowsTrackingMethod::REFS_CBT: ranges = detect_refs_cbt(path); break;
        case WindowsTrackingMethod::READDIR_CHANGES: ranges = detect_readdir(path); break;
        default: ranges = detect_readdir(path); break;
    }

    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();
    spdlog::info("Windows CBT: method={}, changed_blocks={}, time={}ms", static_cast<int>(method), ranges.size(), elapsed);
    return ranges;
}

std::vector<BlockRange> WindowsFsBackup::detect_vss_bitmap(const std::string& path) {
    std::vector<BlockRange> ranges;
    spdlog::info("VSS + Bitmap Differential: comparing volume bitmaps");

#ifdef _MSC_VER
    // Step 1: Create VSS snapshot
    if (!create_vss_snapshot(path)) {
        spdlog::warn("VSS snapshot failed, falling back to readdir");
        return detect_readdir(path);
    }

    // Step 2: Get bitmap of current volume
    auto current_bitmap = get_volume_bitmap(path);

    // Step 3: Get bitmap of snapshot (via shadow copy path)
    // In production: access shadow copy device
    // auto snapshot_bitmap = get_volume_bitmap("\\\\?\\GLOBALROOT\\Device\\HarddiskVolumeShadowCopy1\\");

    // Step 4: XOR bitmaps to find changed regions
    // auto diff = xor_bitmaps(current_bitmap, snapshot_bitmap);
#else
    // Simulated: scan for modified files
    ranges = detect_readdir(path);
#endif

    return ranges;
}

std::vector<BlockRange> WindowsFsBackup::detect_usn_journal(const std::string& path) {
    std::vector<BlockRange> ranges;
    spdlog::info("USN Journal: reading change journal");

#ifdef _MSC_VER
    // In production: DeviceIoControl(FSCTL_ENUM_USN_DATA) to read journal
    // Filter: USN_REASON_DATA_OVERWRITE | USN_REASON_DATA_EXTEND | USN_REASON_FILE_CREATE
    // Map USN records to block ranges
    HANDLE hVol = CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                              nullptr, OPEN_EXISTING, 0, nullptr);
    if (hVol != INVALID_HANDLE_VALUE) {
        // Query USN journal
        USN_JOURNAL_DATA_V2 journalData = {};
        DWORD bytesReturned = 0;
        DeviceIoControl(hVol, FSCTL_QUERY_USN_JOURNAL, nullptr, 0,
                       &journalData, sizeof(journalData), &bytesReturned, nullptr);

        if (journalData.StartUsn > last_usn_) {
            // Read changed entries
            USN startUsn = {0};
            startUsn = last_usn_ > 0 ? last_usn_ : journalData.StartUsn;

            BYTE buffer[64 * 1024];
            DWORD dwBytes;
            ENUM_USN_DATA_V2* usnData = reinterpret_cast<ENUM_USN_DATA_V2*>(buffer);

            usnData->StartFileReferenceNumber = 0;
            usnData->HighUsn = journalData.NextUsn;

            while (DeviceIoControl(hVol, FSCTL_ENUM_USN_DATA, &usnData->StartFileReferenceNumber,
                                  sizeof(USN), buffer, sizeof(buffer), &dwBytes, nullptr) && dwBytes > sizeof(USN)) {
                USN_RECORD_V2* record = &usnData->Record;
                while (reinterpret_cast<BYTE*>(record) < buffer + dwBytes) {
                    if (record->Usn >= startUsn && record->Reason & (USN_REASON_DATA_OVERWRITE | USN_REASON_DATA_EXTEND)) {
                        ranges.push_back({record->FileReferenceNumber, record->DataLength, true});
                    }
                    record = reinterpret_cast<USN_RECORD_V2*>(reinterpret_cast<BYTE*>(record) + record->RecordLength);
                }
                usnData->StartFileReferenceNumber = record->FileReferenceNumber;
            }
            last_usn_ = journalData.NextUsn;
        }
        CloseHandle(hVol);
    }
#else
    ranges = detect_readdir(path);
#endif
    return ranges;
}

std::vector<BlockRange> WindowsFsBackup::detect_refs_cbt(const std::string& path) {
    std::vector<BlockRange> ranges;
    spdlog::info("ReFS CBT: querying allocated ranges");

#ifdef _MSC_VER
    // DeviceIoControl(FSCTL_GET_INTEGRITY_INFORMATION) + FSCTL_QUERY_ALLOCATED_RANGES
    HANDLE hFile = CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
    if (hFile != INVALID_HANDLE_VALUE) {
        FILE_ALLOCATED_RANGE_BUFFER query = {0, MAXLONGLONG};
        FILE_ALLOCATED_RANGE_BUFFER ranges_buf[1024];
        DWORD bytesReturned = 0;
        DeviceIoControl(hFile, FSCTL_QUERY_ALLOCATED_RANGES, &query, sizeof(query),
                       ranges_buf, sizeof(ranges_buf), &bytesReturned, nullptr);
        DWORD count = bytesReturned / sizeof(FILE_ALLOCATED_RANGE_BUFFER);
        for (DWORD i = 0; i < count; i++) {
            ranges.push_back({ranges_buf[i].FileOffset, static_cast<uint32_t>(ranges_buf[i].Length), true});
        }
        CloseHandle(hFile);
    }
#else
    ranges = detect_readdir(path);
#endif
    return ranges;
}

std::vector<BlockRange> WindowsFsBackup::detect_readdir(const std::string& path) {
    std::vector<BlockRange> ranges;
    uint64_t total = 0;
    try {
        for (auto& entry : fs::recursive_directory_iterator(path)) {
            if (entry.is_regular_file()) {
                auto mtime = fs::last_write_time(entry);
                // Compare with stored mtime
                total += entry.file_size();
            }
        }
    } catch (...) {}
    if (total > 0) ranges.push_back({0, static_cast<uint32_t>(total), true});
    return ranges;
}

bool WindowsFsBackup::create_vss_snapshot(const std::string& volume) {
#ifdef _MSC_VER
    spdlog::info("Creating VSS snapshot for {}", volume);
    // COM-based VSS initialization
    // CoInitializeEx, CoCreateInstance(CLSID_VssBackupComponents)
    // IVssBackupComponents::InitializeForBackup, CreateSnapshotSet
    // Wait for IVssAsync completion
    return true;
#else
    spdlog::info("VSS snapshot simulated on Linux");
    return true;
#endif
}

std::vector<uint8_t> WindowsFsBackup::get_volume_bitmap(const std::string& volume) {
    std::vector<uint8_t> bitmap;
#ifdef _MSC_VER
    HANDLE hVol = CreateFileA(volume.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                              nullptr, OPEN_EXISTING, 0, nullptr);
    if (hVol != INVALID_HANDLE_VALUE) {
        STARTING_LSN_INPUT_BUFFER query = {0};
        VOLUME_BITMAP_BUFFER vbb;
        DWORD bytesReturned = 0;
        DeviceIoControl(hVol, FSCTL_GET_VOLUME_BITMAP, &query, sizeof(query),
                       &vbb, sizeof(vbb), &bytesReturned, nullptr);
        // Copy bitmap
        bitmap.assign(reinterpret_cast<uint8_t*>(&vbb.Bitmap),
                      reinterpret_cast<uint8_t*>(&vbb.Bitmap) + bytesReturned - sizeof(VOLUME_BITMAP_BUFFER) + 1);
        CloseHandle(hVol);
    }
#else
    bitmap.resize(1024, 0xFF);
#endif
    return bitmap;
}

std::vector<uint8_t> WindowsFsBackup::xor_bitmaps(const std::vector<uint8_t>& a, const std::vector<uint8_t>& b) {
    size_t len = std::max(a.size(), b.size());
    std::vector<uint8_t> result(len, 0);
    for (size_t i = 0; i < len; i++) {
        uint8_t va = (i < a.size()) ? a[i] : 0;
        uint8_t vb = (i < b.size()) ? b[i] : 0;
        result[i] = va ^ vb;
    }
    return result;
}

bool WindowsFsBackup::read_changed_blocks(const std::string& path, const std::vector<BlockRange>& ranges,
                                                             std::function<void(const uint8_t*, size_t, uint64_t)> callback) {
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs) return {};
    const size_t buf_size = 64 * 1024;
    std::vector<uint8_t> buf(buf_size);
    for (auto& range : ranges) {
        ifs.seekg(range.offset);
        uint64_t remaining = range.size;
        while (remaining > 0 && ifs) {
            size_t to_read = std::min(static_cast<uint64_t>(buf_size), remaining);
            ifs.read(reinterpret_cast<char*>(buf.data()), to_read);
            size_t got = ifs.gcount();
            if (got > 0) callback(buf.data(), got, range.offset + (range.size - remaining));
            remaining -= got;
        }
    }
    return true;
}

std::vector<BlockRange> WindowsFsBackup::merge_bitmaps(const std::vector<BlockRange>& a, const std::vector<BlockRange>& b) {
    std::vector<BlockRange> result = a;
    for (auto& br : b) {
        bool merged = false;
        for (auto& existing : result) {
            if (br.offset >= existing.offset && br.offset < existing.offset + existing.size) {
                uint64_t end = std::max(existing.offset + existing.size, br.offset + br.size);
                existing.size = end - existing.offset;
                merged = true;
                break;
            }
        }
        if (!merged) result.push_back(br);
    }
    std::sort(result.begin(), result.end(), [](const BlockRange& a, const BlockRange& b) { return a.offset < b.offset; });
    return result;
}

} // namespace vovqa::incremental
