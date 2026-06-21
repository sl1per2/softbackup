#pragma once

#include <string>
#include <vector>
#include <map>
#include <optional>
#include <memory>
#include <functional>
#include <chrono>
#include <atomic>
#include <mutex>
#include <shared_mutex>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <algorithm>
#include <numeric>
#include <random>

namespace obs {

namespace fs = std::filesystem;
using Clock = std::chrono::system_clock;
using TimePoint = Clock::time_point;
using Duration = Clock::duration;

inline std::string current_time_string() {
    auto now = Clock::now();
    auto t = Clock::to_time_t(now);
    std::tm tm{};
#ifdef _WIN32
    struct tm tm_buf;
    gmtime_s(&tm_buf, &t);
    tm = tm_buf;
#else
    gmtime_r(&t, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}

inline std::string file_size_to_string(uint64_t bytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    double size = static_cast<double>(bytes);
    int unit = 0;
    while (size >= 1024.0 && unit < 4) {
        size /= 1024.0;
        unit++;
    }
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << size << " " << units[unit];
    return oss.str();
}

inline uint64_t parse_size(const std::string& str) {
    static const std::map<std::string, uint64_t> multipliers = {
        {"B", 1}, {"KB", 1024}, {"MB", 1024*1024},
        {"GB", 1024ULL*1024*1024}, {"TB", 1024ULL*1024*1024*1024}
    };
    for (auto& [suffix, mult] : multipliers) {
        if (str.size() >= suffix.size() &&
            str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0) {
            return static_cast<uint64_t>(std::stod(str.substr(0, str.size() - suffix.size())) * mult);
        }
    }
    return std::stoull(str);
}

} // namespace obs
