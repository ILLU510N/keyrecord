#pragma once

#include <chrono>
#include <string>

namespace keyrecord {

// 启动当前程序目录内的可视化服务，并在默认浏览器中打开展示页。
bool openVisualizationPage(std::string* errorMessage = nullptr);

// 仅关闭由当前托盘进程启动的可视化服务。
void shutdownVisualizationServer();

#ifdef _WIN32
// 检查指定进程是否正在监听指定的 TCP 端口。
bool isProcessListeningOnPort(unsigned long processId, unsigned short port);

// 观察指定进程并等待其在目标端口进入监听状态。
bool waitForProcessListening(
    void* processHandle,
    unsigned short port,
    std::chrono::milliseconds timeout = std::chrono::milliseconds(5000),
    std::string* errorMessage = nullptr);
#endif

} // namespace keyrecord
