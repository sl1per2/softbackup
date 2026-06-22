#pragma once
#include "backup_engine/dirty_buffer_logger.h"
#include <cstdlib>
#include <fstream>
#include <thread>

namespace obs {

class PlatformBufferFlusher : public IBufferFlusher {
public:
    explicit PlatformBufferFlusher(const std::string& plugin_name) : plugin_name_(plugin_name) {}

    std::string plugin_name() const override { return plugin_name_; }

    BufferState query_dirty_state() override {
        BufferState state;
#ifdef __linux__
        state = query_linux_dirty();
#elif _WIN32
        state = query_windows_dirty();
#endif
        return state;
    }

    bool flush_dirty_buffers() override {
#ifdef __linux__
        int ret = system("sync");
        if (ret != 0) return false;
        ret = system("echo 3 > /proc/sys/vm/drop_caches 2>/dev/null");
        return true;
#elif _WIN32
        return true;
#else
        return true;
#endif
    }

    std::vector<ComponentDetail> get_component_details() override {
        std::vector<ComponentDetail> details;
#ifdef __linux__
        details = get_linux_component_details();
#endif
        return details;
    }

    uint64_t get_total_ram() override {
#ifdef __linux__
        std::ifstream meminfo("/proc/meminfo");
        std::string line;
        while (std::getline(meminfo, line)) {
            if (line.find("MemTotal:") == 0) {
                uint64_t kb = 0;
                sscanf(line.c_str(), "MemTotal: %lu kB", &kb);
                return kb * 1024;
            }
        }
#endif
        return 0;
    }

    uint64_t get_buffer_pool_size() override {
#ifdef __linux__
        uint64_t cached = 0, buffers = 0;
        std::ifstream meminfo("/proc/meminfo");
        std::string line;
        while (std::getline(meminfo, line)) {
            if (line.find("Cached:") == 0) sscanf(line.c_str(), "Cached: %lu kB", &cached);
            if (line.find("Buffers:") == 0) sscanf(line.c_str(), "Buffers: %lu kB", &buffers);
        }
        return (cached + buffers) * 1024;
#endif
        return 0;
    }

private:
#ifdef __linux__
    BufferState query_linux_dirty() {
        BufferState state;
        std::ifstream meminfo("/proc/meminfo");
        std::string line;
        uint64_t mem_total_kb = 0, dirty_kb = 0, writeback_kb = 0;

        while (std::getline(meminfo, line)) {
            if (line.find("MemTotal:") == 0) sscanf(line.c_str(), "MemTotal: %lu kB", &mem_total_kb);
            if (line.find("Dirty:") == 0) sscanf(line.c_str(), "Dirty: %lu kB", &dirty_kb);
            if (line.find("Writeback:") == 0) sscanf(line.c_str(), "Writeback: %lu kB", &writeback_kb);
        }

        dirty_kb += writeback_kb;
        state.page_count = dirty_kb * 1024 / 4096;
        state.size_bytes = dirty_kb * 1024;
        state.percent_of_ram = mem_total_kb > 0 ? (dirty_kb * 100.0 / mem_total_kb) : 0.0;
        return state;
    }

    std::vector<ComponentDetail> get_linux_component_details() {
        std::vector<ComponentDetail> details;
        uint64_t dirty_kb = 0, writeback_kb = 0, slab_kb = 0;
        std::ifstream meminfo("/proc/meminfo");
        std::string line;
        while (std::getline(meminfo, line)) {
            if (line.find("Dirty:") == 0) sscanf(line.c_str(), "Dirty: %lu kB", &dirty_kb);
            if (line.find("Writeback:") == 0) sscanf(line.c_str(), "Writeback: %lu kB", &writeback_kb);
            if (line.find("SReclaimable:") == 0) sscanf(line.c_str(), "SReclaimable: %lu kB", &slab_kb);
        }
        details.push_back({"page_cache_dirty", (int64_t)(dirty_kb * 1024 / 4096), dirty_kb * 1024});
        details.push_back({"writeback", (int64_t)(writeback_kb * 1024 / 4096), writeback_kb * 1024});
        details.push_back({"slab_reclaimable", (int64_t)(slab_kb * 1024 / 4096), slab_kb * 1024});
        return details;
    }
#endif

    std::string plugin_name_;
};

} // namespace obs
