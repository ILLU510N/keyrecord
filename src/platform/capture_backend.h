#pragma once

#include "platform/key_code.h"

#include <chrono>
#include <functional>

namespace keyrecord {

// 采集端平台抽象：各平台后端拥有自己的事件循环（Windows 消息泵 / Linux evdev
// 读取循环 / macOS CFRunLoop），命中按键按下事件时回调，回调里携带的键码必须
// 已归一化为 Windows 虚拟键码数值（数据库与前端以该数值为契约）。
using KeyEventCallback =
    std::function<void(KeyCode vk, std::chrono::system_clock::time_point when)>;

// 启动全局按键捕获并阻塞运行，直到 requestStopCapture() 被调用；返回进程退出码。
int runCaptureLoop(const KeyEventCallback& callback);

// 从信号处理器 / 托盘退出项 / 其它线程请求停止捕获循环。
void requestStopCapture();

} // namespace keyrecord
