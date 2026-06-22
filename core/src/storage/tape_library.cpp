#include "storage/tape_library.h"
#include <spdlog/spdlog.h>

namespace obs {

std::vector<TapeInfo> TapeLibrary::scan_devices() {
    spdlog::info("TapeLibrary: scanning devices");
    return {};
}

bool TapeLibrary::mount(const std::string& tape_id) {
    spdlog::info("TapeLibrary: mounting tape {}", tape_id);
    return true;
}

bool TapeLibrary::unmount(const std::string& tape_id) {
    spdlog::info("TapeLibrary: unmounting tape {}", tape_id);
    return true;
}

bool TapeLibrary::write(const std::string& tape_id, const std::string& data) {
    spdlog::info("TapeLibrary: writing to tape {}", tape_id);
    return true;
}

std::string TapeLibrary::read(const std::string& tape_id) {
    spdlog::info("TapeLibrary: reading from tape {}", tape_id);
    return "";
}

bool TapeLibrary::rewind(const std::string& tape_id) {
    spdlog::info("TapeLibrary: rewinding tape {}", tape_id);
    return true;
}

bool TapeLibrary::erase(const std::string& tape_id) {
    spdlog::info("TapeLibrary: erasing tape {}", tape_id);
    return true;
}

} // namespace obs
