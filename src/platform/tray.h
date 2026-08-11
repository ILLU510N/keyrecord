#pragma once

#include <functional>

namespace keyrecord {

using TrayActionCallback = std::function<void()>;
using TrayExitCallback = TrayActionCallback;
using TrayOpenVisualizationCallback = TrayActionCallback;

// 初始化当前平台的常驻入口。openVisualizationCallback 目前仅由 Windows 菜单使用。
bool initializeTray(
    TrayExitCallback exitCallback,
    TrayOpenVisualizationCallback openVisualizationCallback);

// 移除托盘资源；允许在未成功初始化时调用。
void shutdownTray();

} // namespace keyrecord
