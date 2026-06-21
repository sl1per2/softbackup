#include "rescue/rescue_image.h"
#include <spdlog/spdlog.h>
#include <fstream>
#include <sstream>
#ifdef _WIN32
#include <io.h>
#define chmod(path, mode) 0
#else
#include <sys/stat.h>
#endif

namespace obs {

RescueImage::RescueImage() {}
RescueImage::~RescueImage() {}

bool RescueImage::create(const RescueImageConfig& config) {
    std::lock_guard<std::mutex> lock(progress_mutex_);
    progress_.status = "creating";
    progress_.percent = 0;

    spdlog::info("Creating rescue image at {}", config.output_path);

    try {
        progress_.current_step = "Preparing initramfs";
        progress_.percent = 10;
        if (!create_initramfs(config)) {
            progress_.status = "failed";
            progress_.error = "Failed to create initramfs";
            return false;
        }

        progress_.current_step = "Configuring GRUB";
        progress_.percent = 30;
        if (!create_grub_config(config)) {
            progress_.status = "failed";
            progress_.error = "Failed to create GRUB config";
            return false;
        }

        progress_.current_step = "Generating network configuration";
        progress_.percent = 50;
        generate_network_config(config);

        progress_.current_step = "Generating SSH keys for rescue";
        progress_.percent = 60;
        generate_ssh_keys();

        progress_.current_step = "Packing ISO image";
        progress_.percent = 70;
        if (!pack_iso(config)) {
            progress_.status = "failed";
            progress_.error = "Failed to pack ISO";
            return false;
        }

        progress_.current_step = "Verifying image";
        progress_.percent = 90;
        if (!verify(config.output_path)) {
            progress_.status = "failed";
            progress_.error = "Image verification failed";
            return false;
        }

        progress_.status = "done";
        progress_.percent = 100;
        progress_.current_step = "Complete";
        spdlog::info("Rescue image created: {}", config.output_path);
        return true;

    } catch (const std::exception& e) {
        progress_.status = "failed";
        progress_.error = e.what();
        spdlog::error("Rescue image creation failed: {}", e.what());
        return false;
    }
}

bool RescueImage::create_initramfs(const RescueImageConfig& config) {
    // In production: assemble initramfs with busybox, udev, fsck, network tools
    std::string initramfs_dir = "/tmp/obs_rescue_initramfs";
    fs::create_directories(initramfs_dir + "/bin");
    fs::create_directories(initramfs_dir + "/sbin");
    fs::create_directories(initramfs_dir + "/etc");
    fs::create_directories(initramfs_dir + "/proc");
    fs::create_directories(initramfs_dir + "/sys");
    fs::create_directories(initramfs_dir + "/dev");
    fs::create_directories(initramfs_dir + "/mnt");

    // Write minimal init script
    std::ofstream init(initramfs_dir + "/init");
    init << "#!/bin/sh\n"
         << "mount -t proc proc /proc\n"
         << "mount -t sysfs sysfs /sys\n"
         << "mount -t devtmpfs devtmpfs /dev\n"
         << "echo 'OBS Rescue Image'\n"
         << "echo 'Waiting for recovery source...'\n"
         << "exec /bin/sh\n";
    init.close();
    chmod((initramfs_dir + "/init").c_str(), 0755);

    return true;
}

bool RescueImage::create_grub_config(const RescueImageConfig& /*config*/) {
    // In production: write grub.cfg for ISO boot
    return true;
}

bool RescueImage::pack_iso(const RescueImageConfig& config) {
    // In production: use xorriso or mkisofs to create bootable ISO
    // For now, create placeholder
    std::ofstream ofs(config.output_path, std::ios::binary);
    ofs << "OBS_RESCUE_IMAGE_v1.0";
    ofs.close();
    return true;
}

bool RescueImage::generate_network_config(const RescueImageConfig& config) {
    std::string path = "/tmp/obs_rescue_initramfs/etc/network.conf";
    std::ofstream ofs(path);
    ofs << "IP=" << config.ip_address << "\n"
        << "NETMASK=" << config.netmask << "\n"
        << "GATEWAY=" << config.gateway << "\n"
        << "HOSTNAME=" << config.hostname << "\n";
    return true;
}

bool RescueImage::generate_ssh_keys() {
    // In production: generate ephemeral SSH keys for rescue session
    return true;
}

bool RescueImage::verify(const std::string& image_path) {
    return fs::exists(image_path) && fs::file_size(image_path) > 0;
}

bool RescueImage::mount(const std::string& image_path, const std::string& mount_point) {
    fs::create_directories(mount_point);
    // In production: use loop device mount for ISO
    spdlog::info("Mounting rescue image {} -> {}", image_path, mount_point);
    return true;
}

bool RescueImage::unmount(const std::string& mount_point) {
    spdlog::info("Unmounting {}", mount_point);
    return true;
}

} // namespace obs
