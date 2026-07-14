#pragma once

#include <functional>

namespace keyrecord {

using TrayExitCallback = std::function<void()>;

// 初始化当前平台的常驻入口。Linux 默认使用无托盘实现，直接返回成功。
bool initializeTray(TrayExitCallback exitCallback);

// 移除托盘资源；允许在未成功初始化时调用。
void shutdownTray();

} // namespace keyrecord
