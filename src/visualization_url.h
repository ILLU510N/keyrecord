#pragma once

#include "app_config.h"

#include <string>

namespace keyrecord {

// 根据服务端配置生成本机浏览器访问展示页时使用的 URL。
std::string buildVisualizationPageUrl(const ConfigFileValues& values);

} // namespace keyrecord
