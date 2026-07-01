#pragma once

#include <string>

namespace keyrecord {

// 返回应用配置目录；目录不存在时会创建。
std::string getConfigDir();

// 返回默认数据库文件名（keyrecord.db）。
std::string defaultDatabaseFileName();

// 返回默认数据库路径：~/.config/keyrecord/keyrecord.db。
std::string getDefaultDatabasePath();

// 确保给定文件路径的父目录存在（必要时递归创建）。
// - 路径仅含文件名（无父目录）时视为成功，不创建任何目录。
// - 目录已存在或创建成功时返回 true。
// - 创建失败时返回 false，并在提供 errorMessage 时写入英文错误说明。
bool ensureParentDirectoryExists(const std::string& filePath, std::string* errorMessage = nullptr);

} // namespace keyrecord
