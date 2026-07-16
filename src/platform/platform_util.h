#pragma once

#include <ctime>
#include <string>

namespace keyrecord {

// 把 time_t 转换为本地时间，屏蔽 localtime_s（MSVC）与 localtime_r（POSIX）
// 的签名差异；两者均为线程安全实现。转换成功返回 true。
bool localTime(std::tm& out, std::time_t value);

// 调试日志输出：Windows 走 OutputDebugString，其它平台写入 stderr。
void debugLog(const std::string& message);

// 向用户显示不可恢复的启动错误：Windows 使用消息框，其它平台写入 stderr。
void showError(const std::string& message);

} // namespace keyrecord
