#pragma once
#include "common.h"
#include <thread>

namespace obs {

struct RescueImageConfig {
    std::string output_path;
    std::string kernel_path;
    std::string initrd_path;
    std::vector<std::string> extra_modules;
    std::string hostname;
    std::string ip_address;
    std::string netmask;
    std::string gateway;
    bool include_network_tools = true;
    bool include_fs_tools = true;
};

struct RescueImageProgress {
    std::string status; // "creating", "done", "failed"
    int percent = 0;
    std::string current_step;
    std::string error;
};

class RescueImage {
public:
    RescueImage();
    ~RescueImage();

    bool create(const RescueImageConfig& config);
    bool verify(const std::string& image_path);
    bool mount(const std::string& image_path, const std::string& mount_point);
    bool unmount(const std::string& mount_point);
    RescueImageProgress get_progress() const { return progress_; }

private:
    bool create_initramfs(const RescueImageConfig& config);
    bool create_grub_config(const RescueImageConfig& config);
    bool pack_iso(const RescueImageConfig& config);
    bool generate_network_config(const RescueImageConfig& config);
    bool generate_ssh_keys();

    RescueImageProgress progress_;
    mutable std::mutex progress_mutex_;
};

} // namespace obs
