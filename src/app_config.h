#pragma once

#include <optional>
#include <string>

namespace keyrecord {

// 从 INI 配置文件读取到的原始配置项；文件中未出现（或非法）的项为 nullopt。
// 采用 optional 是为了让调用方按“内置默认 < 配置文件 < 命令行”的顺序分层覆盖。
struct ConfigFileValues {
    std::optional<std::string> address;     // [server] address
    std::optional<unsigned short> port;     // [server] port
    std::optional<std::string> dbPath;      // [storage] db_path，或由 db_dir 推导得到
};

// 解析 INI 文本（纯函数，不做任何文件 I/O）。
// - 支持 [section] 节、key = value 键值、以 # 或 ; 开头的整行注释。
// - 节名与键名大小写不敏感；值按原样保留（仅去除首尾空白）。
// - 无法识别的节/键会被静默忽略，便于向前兼容。
// - 可选的 warnings 收集被忽略行的说明（每条一行）。
ConfigFileValues parseConfigText(const std::string& text, std::string* warnings = nullptr);

// 读取并解析配置文件。文件不存在或无法读取时返回空值集合（不视为错误）。
ConfigFileValues parseConfigFile(const std::string& path, std::string* warnings = nullptr);

// 默认配置文件路径：~/.config/keyrecord/config.ini。
std::string getDefaultConfigFilePath();

// 采集端与服务端使用的有效数据库路径：
// 若默认配置文件指定了数据库位置则用配置值，否则回退到默认数据库路径。
std::string resolveDatabasePath();

} // namespace keyrecord
