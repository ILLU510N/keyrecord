#pragma once

#include <string>

namespace keyrecord {

// 返回应用配置目录；目录不存在时会创建。
std::string getConfigDir();

// 返回默认数据库文件名（keyrecord.db）。
std::string defaultDatabaseFileName();

// 返回默认数据库路径：~/.config/keyrecord/keyrecord.db。
std::string getDefaultDatabasePath();

} // namespace keyrecord
