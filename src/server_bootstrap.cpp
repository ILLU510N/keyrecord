#include "server_bootstrap.h"

#include "app_config.h"
#include "config_path.h"

namespace keyrecord {

std::string defaultServerDatabasePath() {
    return getDefaultDatabasePath();
}

void applyConfigFileValues(ServerConfig& config, const ConfigFileValues& values) {
    if (values.address) {
        config.address = *values.address;
    }
    if (values.port) {
        config.port = *values.port;
    }
    if (values.dbPath) {
        config.dbPath = *values.dbPath;
    }
}

std::optional<ServerConfig> buildServerConfig(int argc, char* argv[], std::string* errorMessage) {
    if (errorMessage) {
        errorMessage->clear();
    }

    // 优先级：内置默认值 < 配置文件 < 命令行参数。
    ServerConfig config;
    config.dbPath = defaultServerDatabasePath();
    applyConfigFileValues(config, parseConfigFile(getDefaultConfigFilePath()));

    if (argc > 2) {
        if (errorMessage) {
            *errorMessage = "usage: keyrecord_server [db_path]";
        }
        return std::nullopt;
    }

    if (argc == 2) {
        if (!argv[1] || argv[1][0] == '\0') {
            if (errorMessage) {
                *errorMessage = "db_path must not be empty";
            }
            return std::nullopt;
        }
        config.dbPath = argv[1];
    }

    return config;
}

} // namespace keyrecord
