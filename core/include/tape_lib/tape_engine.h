#pragma once
#include "common.h"
#include <functional>
#include <vector>
#include <string>

namespace obs {

enum class TapePool { FREE, ACTIVE, ARCHIVE, RETIRED };
enum class TapeJobType { WRITE, READ, CLEAN, FORMAT };
enum class TapeJobStatus { PENDING, RUNNING, COMPLETED, FAILED };

struct TapeLibraryInfo {
    std::string vendor;
    std::string model;
    std::string serial;
    int slots_count = 0;
    int drives_count = 0;
    std::string device_path;
    bool online = false;
};

struct TapeInfo {
    std::string barcode;
    TapePool pool = TapePool::FREE;
    uint64_t capacity_bytes = 0;
    uint64_t used_bytes = 0;
    int write_count = 0;
    std::string last_write_at;
    std::string location;
    std::string slot;
    bool write_protected = false;
};

struct TapeJobRecord {
    std::string job_id;
    TapeJobType type = TapeJobType::WRITE;
    std::vector<std::string> tape_barcodes;
    std::vector<std::string> backup_job_ids;
    TapeJobStatus status = TapeJobStatus::PENDING;
    uint64_t bytes_written = 0;
    std::string started_at;
    std::string completed_at;
    std::string error;
};

using TapeJobCallback = std::function<void(const TapeJobRecord&)>;

class TapeEngine {
public:
    TapeEngine();
    ~TapeEngine();

    bool connect(const std::string& device_path);
    void disconnect();
    TapeLibraryInfo get_library_info();

    std::vector<TapeInfo> inventory();
    bool write_to_tape(const std::vector<std::string>& backup_paths, const std::vector<std::string>& barcodes,
                       TapeJobCallback callback = nullptr);
    bool read_from_tape(const std::string& barcode, const std::string& output_dir,
                        TapeJobCallback callback = nullptr);
    bool format_tape(const std::string& barcode);
    bool clean_drive(int drive_index);
    bool mount_tape(const std::string& barcode, int drive_index);
    bool unmount_tape(int drive_index);

    bool move_tape(const std::string& barcode, TapePool target_pool);
    std::vector<TapeInfo> get_tapes_by_pool(TapePool pool);
    TapeInfo get_tape(const std::string& barcode);

    bool write_multi_stream(const std::string& barcode,
                           const std::vector<std::vector<uint8_t>>& streams);
    bool write_spanning(const std::vector<std::string>& data_chunks,
                       std::vector<std::string>& output_barcodes);

private:
    bool execute_mtx(const std::string& command);
    bool write_to_ltfs(const std::string& path, const std::vector<uint8_t>& data);
    bool read_from_ltfs(const std::string& path, std::vector<uint8_t>& data);
    void simulate_inventory();

    TapeLibraryInfo library_;
    std::vector<TapeInfo> tapes_;
    std::string device_path_;
    bool connected_ = false;
    mutable std::mutex tapes_mutex_;
};

} // namespace obs
