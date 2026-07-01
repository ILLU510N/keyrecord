#pragma once

#include "app_config.h"
#include "server.h"

#include <optional>
#include <string>

namespace keyrecord {

std::string defaultServerDatabasePath();

// 将配置文件中出现的项覆盖到 config 上（未出现的项保持不变）。
void applyConfigFileValues(ServerConfig& config, const ConfigFileValues& values);

std::optional<ServerConfig> buildServerConfig(
    int argc,
    char* argv[],
    std::string* errorMessage = nullptr);

} // namespace keyrecord
