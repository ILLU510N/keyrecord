#include "../config_path.h"

#include <chrono>
#include <cstdlib>
#include <filesystem>
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

    std::filesystem::remove_all(tempHome);
    return ok ? 0 : 1;
}
