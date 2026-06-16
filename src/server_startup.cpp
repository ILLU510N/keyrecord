#include "server_startup.h"

#include <utility>

namespace keyrecord {

std::string buildServerStartupBanner(const ServerConfig& config) {
    std::string banner = "KeyRecord 可视化服务启动中\n";
    banner += "监听地址: http://" + config.address + ":" + std::to_string(config.port) + "\n";
    banner += "数据库路径: " + config.dbPath + "\n";
    return banner;
}

std::optional<ServerStartup> prepareServerStartup(const ServerConfig& config, std::string* errorMessage) {
    auto service = VisualizationService::open(config.dbPath, errorMessage);
    if (!service) {
        return std::nullopt;
    }

    return ServerStartup{
        std::move(*service),
        buildServerStartupBanner(config),
    };
}

} // namespace keyrecord
