#include "../config_path.h"

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>

namespace {

bool expect(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << "\n";
        return false;
    }
    return true;
}

bool expectPathEqual(
    const std::filesystem::path& actual,
    const std::filesystem::path& expected,
    const char* message) {
    if (actual.lexically_normal() != expected.lexically_normal()) {
        std::cerr << message << "\nExpected: " << expected.string() << "\nActual: " << actual.string() << "\n";
        return false;
    }
    return true;
}

std::optional<std::string> readEnv(const char* name) {
    const char* value = std::getenv(name);
    if (!value) {
        return std::nullopt;
    }
    return std::string(value);
}

void writeEnv(const char* name, const std::optional<std::string>& value) {
#ifdef _WIN32
    _putenv_s(name, value ? value->c_str() : "");
#else
    if (value) {
        setenv(name, value->c_str(), 1);
    } else {
        unsetenv(name);
    }
#endif
}

class ScopedEnv {
public:
    ScopedEnv(const char* name, const std::string& value)
        : name_(name), oldValue_(readEnv(name)) {
        writeEnv(name_.c_str(), value);
    }

    ~ScopedEnv() {
        writeEnv(name_.c_str(), oldValue_);
    }

private:
    std::string name_;
    std::optional<std::string> oldValue_;
};

std::filesystem::path uniqueTempHome() {
    const auto now = std::chrono::steady_clock::now().time_since_epoch().count();
    return std::filesystem::temp_directory_path() / ("keyrecord_config_path_tests_" + std::to_string(now));
}

} // namespace

int main() {
    bool ok = true;

    const auto tempHome = uniqueTempHome();
    std::filesystem::remove_all(tempHome);
    std::filesystem::create_directories(tempHome);

#ifdef _WIN32
    ScopedEnv userProfile("USERPROFILE", tempHome.string());
#else
    ScopedEnv home("HOME", tempHome.string());
#endif

    const auto expectedConfigDir = tempHome / ".config" / "keyrecord";
    const auto expectedDbPath = expectedConfigDir / "keyrecord.db";

    const auto actualConfigDir = std::filesystem::path(keyrecord::getConfigDir());
    ok = expectPathEqual(actualConfigDir, expectedConfigDir, "Config directory should be ~/.config/keyrecord") && ok;
    ok = expect(std::filesystem::is_directory(expectedConfigDir), "Config directory should be created automatically") && ok;

    const auto actualDbPath = std::filesystem::path(keyrecord::getDefaultDatabasePath());
    ok = expectPathEqual(actualDbPath, expectedDbPath, "Default database file should be inside the config directory") && ok;

    ok = expect(keyrecord::defaultDatabaseFileName() == "keyrecord.db", "Default database file name should be keyrecord.db") && ok;

    // ensureParentDirectoryExists: 为尚不存在的嵌套数据库路径递归创建父目录。
    {
        const auto freshDbPath = tempHome / "fresh" / "nested" / "keyrecord.db";
        std::string error;
        ok = expect(keyrecord::ensureParentDirectoryExists(freshDbPath.string(), &error),
                    "ensureParentDirectoryExists should create a missing parent directory") && ok;
        ok = expect(error.empty(), "ensureParentDirectoryExists should not report an error on success") && ok;
        ok = expect(std::filesystem::is_directory(freshDbPath.parent_path()),
                    "ensureParentDirectoryExists should create the database parent directory") && ok;

        // 目录已存在时应保持成功（幂等）。
        ok = expect(keyrecord::ensureParentDirectoryExists(freshDbPath.string()),
                    "ensureParentDirectoryExists should succeed when the directory already exists") && ok;
    }

    // ensureParentDirectoryExists: 仅有文件名（无父目录）时视为成功且不创建目录。
    {
        std::string error;
        ok = expect(keyrecord::ensureParentDirectoryExists("keyrecord.db", &error),
                    "ensureParentDirectoryExists should succeed for a bare file name") && ok;
        ok = expect(error.empty(), "ensureParentDirectoryExists should not report an error for a bare file name") && ok;
    }

    // ensureParentDirectoryExists: 父路径被普通文件占据时应失败并给出错误信息。
    {
        const auto blocker = tempHome / "blocker";
        {
            std::ofstream out(blocker, std::ios::binary);
            out << "not a directory";
        }
        const auto blockedDbPath = blocker / "sub" / "keyrecord.db";
        std::string error;
        ok = expect(!keyrecord::ensureParentDirectoryExists(blockedDbPath.string(), &error),
                    "ensureParentDirectoryExists should fail when a file blocks the directory path") && ok;
        ok = expect(!error.empty(),
                    "ensureParentDirectoryExists should report an error when directory creation fails") && ok;
    }

    std::filesystem::remove_all(tempHome);
    return ok ? 0 : 1;
}
