#include "config_path.h"

#include <cstdlib>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <system_error>

namespace keyrecord {

namespace {

std::filesystem::path getHomeDir() {
#ifdef _WIN32
    const char* home = std::getenv("USERPROFILE");
#else
    const char* home = std::getenv("HOME");
#endif
    if (!home || home[0] == '\0') {
        throw std::runtime_error("Failed to get user home directory");
    }
    return std::filesystem::path(home);
}

} // namespace

std::string getConfigDir() {
    const auto configPath = getHomeDir() / ".config" / "keyrecord";
    std::error_code error;
    std::filesystem::create_directories(configPath, error);
    std::error_code statError;
    const bool isDirectory = std::filesystem::is_directory(configPath, statError);
    if (error || statError || !isDirectory) {
        throw std::runtime_error(
            "Failed to create config directory '" + configPath.string() +
            "': " + (error ? error.message() :
                      statError ? statError.message() : "path is not a directory"));
    }
    return configPath.string();
}

std::string defaultDatabaseFileName() {
    return "keyrecord.db";
}

std::string getDefaultDatabasePath() {
    return (std::filesystem::path(getConfigDir()) / defaultDatabaseFileName()).string();
}

bool ensureParentDirectoryExists(const std::string& filePath, std::string* errorMessage) {
    if (errorMessage) {
        errorMessage->clear();
    }

    const auto parent = std::filesystem::path(filePath).parent_path();
    if (parent.empty()) {
        // 仅有文件名（写入当前工作目录），无需创建目录。
        return true;
    }

    std::error_code createEc;
    std::filesystem::create_directories(parent, createEc);

    // create_directories 在目录已存在时返回 false 且不置错误码，
    // 故统一以“父路径最终是否为目录”作为成功判据，兼容已存在或并发创建。
    std::error_code statEc;
    if (std::filesystem::is_directory(parent, statEc)) {
        return true;
    }

    if (errorMessage) {
        *errorMessage = "failed to create database directory '" + parent.string() + "'";
        if (createEc) {
            *errorMessage += ": " + createEc.message();
        }
    }
    return false;
}

} // namespace keyrecord
