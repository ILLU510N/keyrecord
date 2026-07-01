#include "../server_bootstrap.h"
#include "../app_config.h"
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

bool expectEqual(const std::string& actual, const std::string& expected, const char* message) {
    if (actual != expected) {
        std::cerr << message << "\nExpected: " << expected << "\nActual: " << actual << "\n";
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
    return std::filesystem::temp_directory_path() / ("keyrecord_server_bootstrap_tests_" + std::to_string(now));
}

void writeConfigFile(const std::string& contents) {
    const auto path = std::filesystem::path(keyrecord::getDefaultConfigFilePath());
    std::filesystem::create_directories(path.parent_path());
    std::ofstream out(path, std::ios::binary);
    out << contents;
}

void removeConfigFile() {
    std::error_code ec;
    std::filesystem::remove(keyrecord::getDefaultConfigFilePath(), ec);
}

} // namespace

int main() {
    bool ok = true;

    // 使用临时 HOME，保证默认值断言不受开发机上真实 config.ini 影响。
    const auto tempHome = uniqueTempHome();
    std::filesystem::remove_all(tempHome);
    std::filesystem::create_directories(tempHome);
#ifdef _WIN32
    ScopedEnv userProfile("USERPROFILE", tempHome.string());
#else
    ScopedEnv home("HOME", tempHome.string());
#endif

    std::string errorMessage;
    char programName[] = "keyrecord_server";

    // 无配置文件：全部走内置默认值。
    {
        removeConfigFile();
        char* argv[] = {programName};
        auto config = keyrecord::buildServerConfig(1, argv, &errorMessage);
        ok = expect(config.has_value(), "Default arguments should build a server config") && ok;
        ok = expect(errorMessage.empty(), "Default arguments should not return an error message") && ok;
        if (config) {
            ok = expectEqual(
                     config->dbPath,
                     keyrecord::getDefaultDatabasePath(),
                     "Default database path should use the config directory") &&
                 ok;
            ok = expect(config->address == "0.0.0.0", "Default listen address should be 0.0.0.0") && ok;
            ok = expect(config->port == 3000, "Default listen port should be 3000") && ok;
        }
    }

    // 命令行 db_path（无配置文件）。
    {
        errorMessage.clear();
        removeConfigFile();
        char dbPath[] = "D:/tmp/custom.db";
        char* argv[] = {programName, dbPath};
        auto config = keyrecord::buildServerConfig(2, argv, &errorMessage);
        ok = expect(config.has_value(), "Explicit database path should build a server config") && ok;
        ok = expect(errorMessage.empty(), "Explicit database path should not return an error message") && ok;
        if (config) {
            ok = expectEqual(config->dbPath, dbPath, "Explicit database path was not stored in the server config") && ok;
        }
    }

    // 空 db_path 参数报错。
    {
        errorMessage.clear();
        char emptyPath[] = "";
        char* argv[] = {programName, emptyPath};
        auto config = keyrecord::buildServerConfig(2, argv, &errorMessage);
        ok = expect(!config.has_value(), "Empty database path should not build a server config") && ok;
        ok = expectEqual(errorMessage, "db_path must not be empty", "Unexpected error message for empty database path") && ok;
    }

    // 多余参数报错。
    {
        errorMessage.clear();
        char dbPath[] = "D:/tmp/custom.db";
        char extra[] = "unexpected";
        char* argv[] = {programName, dbPath, extra};
        auto config = keyrecord::buildServerConfig(3, argv, &errorMessage);
        ok = expect(!config.has_value(), "Extra arguments should not build a server config") && ok;
        ok = expectEqual(errorMessage, "usage: keyrecord_server [db_path]", "Unexpected error message for extra arguments") && ok;
    }

    // applyConfigFileValues：完整覆盖。
    {
        keyrecord::ServerConfig config;
        config.dbPath = "default.db";
        keyrecord::ConfigFileValues values;
        values.address = "10.0.0.1";
        values.port = static_cast<unsigned short>(9090);
        values.dbPath = "from_config.db";
        keyrecord::applyConfigFileValues(config, values);
        ok = expect(config.address == "10.0.0.1", "applyConfigFileValues should overlay address") && ok;
        ok = expect(config.port == 9090, "applyConfigFileValues should overlay port") && ok;
        ok = expectEqual(config.dbPath, "from_config.db", "applyConfigFileValues should overlay db path") && ok;
    }

    // applyConfigFileValues：部分覆盖不影响其他项。
    {
        keyrecord::ServerConfig config;  // 默认 address=0.0.0.0, port=3000
        config.dbPath = "keep.db";
        keyrecord::ConfigFileValues values;
        values.port = static_cast<unsigned short>(7000);
        keyrecord::applyConfigFileValues(config, values);
        ok = expect(config.port == 7000, "Partial overlay should set port") && ok;
        ok = expect(config.address == "0.0.0.0", "Partial overlay should leave address untouched") && ok;
        ok = expectEqual(config.dbPath, "keep.db", "Partial overlay should leave db path untouched") && ok;
    }

    // 集成：配置文件被 buildServerConfig 读取。
    {
        errorMessage.clear();
        writeConfigFile(
            "[server]\n"
            "address = 192.168.1.5\n"
            "port = 8888\n"
            "[storage]\n"
            "db_path = D:/data/from_config.db\n");
        char* argv[] = {programName};
        auto config = keyrecord::buildServerConfig(1, argv, &errorMessage);
        ok = expect(config.has_value(), "Config file case should build a server config") && ok;
        if (config) {
            ok = expect(config->address == "192.168.1.5", "Config file address should be applied") && ok;
            ok = expect(config->port == 8888, "Config file port should be applied") && ok;
            ok = expectEqual(config->dbPath, "D:/data/from_config.db", "Config file db_path should be applied") && ok;
        }
    }

    // 集成：命令行 db_path 覆盖配置文件的 db_path，但保留配置文件的 address/port。
    {
        errorMessage.clear();
        writeConfigFile(
            "[server]\n"
            "address = 192.168.1.5\n"
            "port = 8888\n"
            "[storage]\n"
            "db_path = D:/data/from_config.db\n");
        char cliPath[] = "D:/data/from_cli.db";
        char* argv[] = {programName, cliPath};
        auto config = keyrecord::buildServerConfig(2, argv, &errorMessage);
        ok = expect(config.has_value(), "CLI-over-config case should build a server config") && ok;
        if (config) {
            ok = expectEqual(config->dbPath, cliPath, "CLI db_path should override config file db_path") && ok;
            ok = expect(config->address == "192.168.1.5", "CLI override should keep config file address") && ok;
            ok = expect(config->port == 8888, "CLI override should keep config file port") && ok;
        }
    }

    removeConfigFile();
    std::filesystem::remove_all(tempHome);
    return ok ? 0 : 1;
}
