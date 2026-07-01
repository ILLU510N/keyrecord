#include "app_config.h"

#include "config_path.h"

#include <cctype>
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>

namespace keyrecord {

namespace {

std::string trim(const std::string& text) {
    const auto begin = text.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) {
        return std::string();
    }
    const auto end = text.find_last_not_of(" \t\r\n");
    return text.substr(begin, end - begin + 1);
}

std::string toLower(std::string text) {
    for (char& c : text) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return text;
}

// 解析端口：必须是 1..65535 的纯十进制数字，否则返回 nullopt。
std::optional<unsigned short> parsePort(const std::string& value) {
    if (value.empty()) {
        return std::nullopt;
    }
    unsigned long parsed = 0;
    for (char c : value) {
        if (c < '0' || c > '9') {
            return std::nullopt;
        }
        parsed = parsed * 10 + static_cast<unsigned long>(c - '0');
        if (parsed > 65535) {
            return std::nullopt;
        }
    }
    if (parsed == 0) {
        return std::nullopt;
    }
    return static_cast<unsigned short>(parsed);
}

void appendWarning(std::string* warnings, int lineNumber, const std::string& reason) {
    if (warnings) {
        *warnings += "line " + std::to_string(lineNumber) + ": " + reason + "\n";
    }
}

} // namespace

ConfigFileValues parseConfigText(const std::string& text, std::string* warnings) {
    ConfigFileValues values;

    std::string content = text;
    // 去除可能存在的 UTF-8 BOM（Windows 记事本另存为 UTF-8 会写入）。
    if (content.size() >= 3 &&
        static_cast<unsigned char>(content[0]) == 0xEF &&
        static_cast<unsigned char>(content[1]) == 0xBB &&
        static_cast<unsigned char>(content[2]) == 0xBF) {
        content.erase(0, 3);
    }

    std::optional<std::string> dbPathRaw;   // [storage] db_path
    std::optional<std::string> dbDirRaw;    // [storage] db_dir

    std::istringstream stream(content);
    std::string line;
    std::string section;
    int lineNumber = 0;

    while (std::getline(stream, line)) {
        ++lineNumber;
        const std::string trimmed = trim(line);
        if (trimmed.empty() || trimmed.front() == '#' || trimmed.front() == ';') {
            continue;
        }

        if (trimmed.front() == '[') {
            if (trimmed.back() == ']') {
                section = toLower(trim(trimmed.substr(1, trimmed.size() - 2)));
            } else {
                appendWarning(warnings, lineNumber, "missing closing ']' in section header, ignored");
            }
            continue;
        }

        const auto equalsPos = trimmed.find('=');
        if (equalsPos == std::string::npos) {
            appendWarning(warnings, lineNumber, "missing '=', ignored");
            continue;
        }

        const std::string key = toLower(trim(trimmed.substr(0, equalsPos)));
        const std::string value = trim(trimmed.substr(equalsPos + 1));
        if (key.empty()) {
            appendWarning(warnings, lineNumber, "empty key, ignored");
            continue;
        }

        if (section == "server" && key == "address") {
            if (!value.empty()) {
                values.address = value;
            }
        } else if (section == "server" && key == "port") {
            if (auto port = parsePort(value)) {
                values.port = *port;
            } else {
                appendWarning(warnings, lineNumber, "invalid port (expected 1..65535), ignored");
            }
        } else if (section == "storage" && key == "db_path") {
            if (!value.empty()) {
                dbPathRaw = value;
            }
        } else if (section == "storage" && key == "db_dir") {
            if (!value.empty()) {
                dbDirRaw = value;
            }
        }
        // 其余未识别的节/键静默忽略，便于后续新增配置项时向前兼容。
    }

    // db_path（完整文件路径）优先于 db_dir（仅目录，文件名固定）。
    if (dbPathRaw) {
        values.dbPath = *dbPathRaw;
    } else if (dbDirRaw) {
        values.dbPath =
            (std::filesystem::path(*dbDirRaw) / defaultDatabaseFileName()).string();
    }

    return values;
}

ConfigFileValues parseConfigFile(const std::string& path, std::string* warnings) {
    std::error_code ec;
    if (path.empty() || !std::filesystem::exists(path, ec) || ec) {
        return ConfigFileValues();
    }

    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return ConfigFileValues();
    }

    std::ostringstream buffer;
    buffer << file.rdbuf();
    return parseConfigText(buffer.str(), warnings);
}

std::string getDefaultConfigFilePath() {
    return (std::filesystem::path(getConfigDir()) / "config.ini").string();
}

std::string resolveDatabasePath() {
    const auto values = parseConfigFile(getDefaultConfigFilePath());
    if (values.dbPath) {
        return *values.dbPath;
    }
    return getDefaultDatabasePath();
}

} // namespace keyrecord
