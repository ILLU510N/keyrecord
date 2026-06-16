#include "config_path.h"

#include <cstdlib>
#include <filesystem>
#include <stdexcept>
#include <string>

namespace keyrecord {

namespace {

std::filesystem::path getHomeDir() {
#ifdef _WIN32
    const char* home = std::getenv("USERPROFILE");
#else
    const char* home = std::getenv("HOME");
#endif
    if (!home || home[0] == '\0') {
        throw std::runtime_error("无法获取用户主目录");
    }
    return std::filesystem::path(home);
}

} // namespace

std::string getConfigDir() {
    const auto configPath = getHomeDir() / ".config" / "keyrecord";
    std::filesystem::create_directories(configPath);
    return configPath.string();
}

std::string getDefaultDatabasePath() {
    return (std::filesystem::path(getConfigDir()) / "keyrecord.db").string();
}

} // namespace keyrecord
