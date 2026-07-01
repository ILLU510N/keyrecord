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
    return std::filesystem::temp_directory_path() / ("keyrecord_app_config_tests_" + std::to_string(now));
}

// 用配置文件路径中的正斜杠比较，避免平台分隔符差异。
std::string normalizePath(const std::string& path) {
    return std::filesystem::path(path).lexically_normal().generic_string();
}

} // namespace

int main() {
    using keyrecord::parseConfigText;
    using keyrecord::parseConfigFile;

    bool ok = true;

    // 空文本 → 全部 nullopt。
    {
        const auto values = parseConfigText("");
        ok = expect(!values.address && !values.port && !values.dbPath,
                    "Empty text should yield no values") && ok;
    }

    // 完整配置 → address / port / dbPath 均被解析。
    {
        const std::string text =
            "[server]\n"
            "address = 127.0.0.1\n"
            "port = 8080\n"
            "[storage]\n"
            "db_path = D:/data/keyrecord.db\n";
        const auto values = parseConfigText(text);
        ok = expect(values.address.has_value() && *values.address == "127.0.0.1",
                    "address should be parsed") && ok;
        ok = expect(values.port.has_value() && *values.port == 8080,
                    "port should be parsed") && ok;
        ok = expect(values.dbPath.has_value() &&
                        normalizePath(*values.dbPath) == "D:/data/keyrecord.db",
                    "db_path should be parsed") && ok;
    }

    // 注释（# 与 ;）、空行、首尾空白都应被正确处理。
    {
        const std::string text =
            "# this is a comment\n"
            "; this is also a comment\n"
            "\n"
            "  [server]  \n"
            "   address   =   0.0.0.0   \n"
            "\tport\t=\t3000\t\n";
        const auto values = parseConfigText(text);
        ok = expect(values.address.has_value() && *values.address == "0.0.0.0",
                    "address with surrounding whitespace should be trimmed") && ok;
        ok = expect(values.port.has_value() && *values.port == 3000,
                    "port with surrounding whitespace should be parsed") && ok;
    }

    // 节名与键名大小写不敏感。
    {
        const std::string text =
            "[SERVER]\n"
            "ADDRESS = host.local\n"
            "Port = 1234\n";
        const auto values = parseConfigText(text);
        ok = expect(values.address.has_value() && *values.address == "host.local",
                    "section/key names should be case-insensitive (address)") && ok;
        ok = expect(values.port.has_value() && *values.port == 1234,
                    "section/key names should be case-insensitive (port)") && ok;
    }

    // 非法端口 → 保持 nullopt 并产生告警。
    {
        std::string warnings;
        const auto values = parseConfigText("[server]\nport = abc\n", &warnings);
        ok = expect(!values.port.has_value(), "Non-numeric port should be rejected") && ok;
        ok = expect(!warnings.empty(), "Invalid port should produce a warning") && ok;
    }

    // 端口越界与 0 → 拒绝。
    {
        ok = expect(!parseConfigText("[server]\nport = 70000\n").port.has_value(),
                    "Port above 65535 should be rejected") && ok;
        ok = expect(!parseConfigText("[server]\nport = 0\n").port.has_value(),
                    "Port 0 should be rejected") && ok;
        ok = expect(!parseConfigText("[server]\nport = -1\n").port.has_value(),
                    "Negative port should be rejected") && ok;
    }

    // 未识别的节/键应被忽略，不影响已知项。
    {
        const std::string text =
            "[unknown]\n"
            "foo = bar\n"
            "[server]\n"
            "unknown_key = x\n"
            "port = 4000\n";
        const auto values = parseConfigText(text);
        ok = expect(values.port.has_value() && *values.port == 4000,
                    "Unknown sections/keys should be ignored without dropping known keys") && ok;
        ok = expect(!values.address.has_value(),
                    "Unknown key should not set address") && ok;
    }

    // db_dir → 推导为 目录/keyrecord.db。
    {
        const auto values = parseConfigText("[storage]\ndb_dir = D:/data/kr\n");
        const std::string expected =
            normalizePath((std::filesystem::path("D:/data/kr") / keyrecord::defaultDatabaseFileName()).string());
        ok = expect(values.dbPath.has_value() && normalizePath(*values.dbPath) == expected,
                    "db_dir should resolve to <dir>/keyrecord.db") && ok;
    }

    // db_path 优先于 db_dir。
    {
        const std::string text =
            "[storage]\n"
            "db_dir = D:/data/kr\n"
            "db_path = E:/explicit/keyrecord.db\n";
        const auto values = parseConfigText(text);
        ok = expect(values.dbPath.has_value() &&
                        normalizePath(*values.dbPath) == "E:/explicit/keyrecord.db",
                    "db_path should take precedence over db_dir") && ok;
    }

    // 含 BOM 的文本应能正常解析首节。
    {
        const std::string text = std::string("\xEF\xBB\xBF") + "[server]\nport = 5000\n";
        const auto values = parseConfigText(text);
        ok = expect(values.port.has_value() && *values.port == 5000,
                    "UTF-8 BOM should be stripped before parsing") && ok;
    }

    // 值中包含 '#' 不应被当作行内注释截断（整行注释才生效）。
    {
        const auto values = parseConfigText("[storage]\ndb_path = C:/data#1/keyrecord.db\n");
        ok = expect(values.dbPath.has_value() &&
                        normalizePath(*values.dbPath) == "C:/data#1/keyrecord.db",
                    "'#' inside a value should be preserved") && ok;
    }

    // 缺少 '=' 的行应被忽略并告警。
    {
        std::string warnings;
        const auto values = parseConfigText("[server]\nthis is not a pair\n", &warnings);
        ok = expect(!values.address && !values.port,
                    "Line without '=' should be ignored") && ok;
        ok = expect(!warnings.empty(), "Line without '=' should produce a warning") && ok;
    }

    // 文件不存在 → 返回空值集合（非错误）。
    {
        const auto values = parseConfigFile("Z:/definitely/missing/config.ini");
        ok = expect(!values.address && !values.port && !values.dbPath,
                    "Missing config file should yield no values") && ok;
    }

    // 文件系统集成：临时 HOME + 真实 config.ini + 默认路径。
    {
        const auto tempHome = uniqueTempHome();
        std::filesystem::remove_all(tempHome);
        std::filesystem::create_directories(tempHome);
#ifdef _WIN32
        ScopedEnv userProfile("USERPROFILE", tempHome.string());
#else
        ScopedEnv home("HOME", tempHome.string());
#endif

        const auto configPath = std::filesystem::path(keyrecord::getDefaultConfigFilePath());
        ok = expectEqual(configPath.filename().string(), "config.ini",
                         "Default config file should be named config.ini") && ok;

        // 无配置文件时，resolveDatabasePath 回退默认库路径。
        ok = expect(
            normalizePath(keyrecord::resolveDatabasePath()) ==
                normalizePath(keyrecord::getDefaultDatabasePath()),
            "resolveDatabasePath should fall back to the default DB path when no config exists") && ok;

        // 写入一个指定 db_dir 的配置文件后，resolveDatabasePath 应改用它。
        std::filesystem::create_directories(configPath.parent_path());
        {
            std::ofstream out(configPath, std::ios::binary);
            out << "[storage]\ndb_dir = " << (tempHome / "custom").generic_string() << "\n";
        }
        const std::string expectedDb =
            normalizePath((tempHome / "custom" / keyrecord::defaultDatabaseFileName()).string());
        ok = expect(normalizePath(keyrecord::resolveDatabasePath()) == expectedDb,
                    "resolveDatabasePath should honor db_dir from the config file") && ok;

        std::filesystem::remove_all(tempHome);
    }

    return ok ? 0 : 1;
}
