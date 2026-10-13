#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <string>

namespace keyrecord {

// 只管理当前用户的 KeyRecord Run 项；查询不会创建或修复注册表项。
bool getAutostartEnabled(bool& enabled, std::string* errorMessage);
bool setAutostartEnabled(bool enabled, std::string* errorMessage);

namespace autostart_detail {

// 接受调用方持有的注册表句柄，便于用隔离位置验证真实读写及访问权限。
bool queryRegistration(HKEY key, const std::wstring& executablePath,
                       bool& enabled, std::string* errorMessage);
bool writeRegistration(HKEY key, const std::wstring& executablePath,
                       bool enabled, std::string* errorMessage);

} // namespace autostart_detail
} // namespace keyrecord
