#pragma once

#include <string>

namespace keyrecord {

// 启动当前程序目录内的可视化服务，并在默认浏览器中打开展示页。
bool openVisualizationPage(std::string* errorMessage = nullptr);

// 仅关闭由当前托盘进程启动的可视化服务。
void shutdownVisualizationServer();

} // namespace keyrecord
