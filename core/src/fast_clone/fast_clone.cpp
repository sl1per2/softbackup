#include "fast_clone/fast_clone.h"
#include <spdlog/spdlog.h>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#define open _open
#define read _read
#define write _write
#define close _close
#define O_RDONLY _O_RDONLY
#define O_WRONLY _O_WRONLY
#define O_CREAT _O_CREAT
#define O_TRUNC _O_TRUNC
#else
#include <sys/ioctl.h>
#include <sys/statvfs.h>
#include <unistd.h>
#include <fcntl.h>
#ifndef FICLONE
#define FICLONE _IOW(0x94, 13, int)
#endif
#endif

namespace obs {

FilesystemInfo FastClone::detect_filesystem(const std::string& path) {
#ifdef _WIN32
    return {"ntfs", false};
#else
    struct statvfs st;
    if (statvfs(path.c_str(), &st) != 0) {
        return {"unknown", false};
    }

    std::ifstream mounts("/proc/mounts");
    std::string line;
    while (std::getline(mounts, line)) {
        if (line.find(path) != std::string::npos) {
            std::istringstream iss(line);
            std::string device, mountpoint, fstype;
            iss >> device >> mountpoint >> fstype;
            if (fstype == "btrfs" || fstype == "xfs") {
                return {fstype, true};
            }
            return {fstype, false};
        }
    }
    return {"unknown", false};
#endif
}

bool FastClone::clone_file(const std::string& source, const std::string& dest) {
#ifdef _WIN32
    int src_fd = open(source.c_str(), _O_RDONLY | _O_BINARY);
    if (src_fd < 0) {
        spdlog::error("Failed to open source: {}", source);
        return false;
    }

    int dst_fd = open(dest.c_str(), _O_WRONLY | _O_CREAT | _O_TRUNC | _O_BINARY, 0644);
    if (dst_fd < 0) {
        _close(src_fd);
        spdlog::error("Failed to open dest: {}", dest);
        return false;
    }

    constexpr size_t BUF_SIZE = 64 * 1024;
    std::vector<char> buf(BUF_SIZE);
    int n;
    while ((n = _read(src_fd, buf.data(), BUF_SIZE)) > 0) {
        int written = 0;
        while (written < n) {
            int w = _write(dst_fd, buf.data() + written, n - written);
            if (w <= 0) {
                _close(src_fd);
                _close(dst_fd);
                return false;
            }
            written += w;
        }
    }

    _close(src_fd);
    _close(dst_fd);
    spdlog::info("Copy: {} -> {}", source, dest);
    return true;
#else
    int src_fd = open(source.c_str(), O_RDONLY);
    if (src_fd < 0) {
        spdlog::error("Failed to open source: {}", source);
        return false;
    }

    int dst_fd = open(dest.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (dst_fd < 0) {
        close(src_fd);
        spdlog::error("Failed to open dest: {}", dest);
        return false;
    }

    int ret = ioctl(dst_fd, FICLONE, src_fd);
    if (ret == 0) {
        close(src_fd);
        close(dst_fd);
        spdlog::info("Reflink clone: {} -> {}", source, dest);
        return true;
    }

    spdlog::info("Reflink not supported, falling back to copy: {} -> {}", source, dest);
    constexpr size_t BUF_SIZE = 64 * 1024;
    std::vector<char> buf(BUF_SIZE);
    ssize_t n;
    while ((n = read(src_fd, buf.data(), BUF_SIZE)) > 0) {
        ssize_t written = 0;
        while (written < n) {
            ssize_t w = write(dst_fd, buf.data() + written, n - written);
            if (w <= 0) {
                close(src_fd);
                close(dst_fd);
                return false;
            }
            written += w;
        }
    }

    close(src_fd);
    close(dst_fd);
    return true;
#endif
}

bool SyntheticFull::create(const std::string& full_path,
                           const std::vector<std::string>& increment_paths,
                           const std::string& output_path) {
    auto fs_info = FastClone::detect_filesystem(output_path);

    if (fs_info.supports_reflink) {
        if (!FastClone::clone_file(full_path, output_path)) {
            spdlog::error("Failed to clone full backup for synthetic");
            return false;
        }
        spdlog::info("Synthetic full: created via reflink from {}", full_path);
    } else {
        if (!fs::exists(output_path)) {
            fs::create_symlink(full_path, output_path);
        }
        spdlog::info("Synthetic full: created symlink {}", output_path);
    }

    spdlog::info("Synthetic full created: {} with {} increments",
                 output_path, increment_paths.size());
    return true;
}

} // namespace obs
