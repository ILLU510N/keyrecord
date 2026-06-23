#include "server_startup.h"

#include <utility>

namespace keyrecord {

std::string buildServerStartupBanner(const ServerConfig& config) {
    std::string banner = "KeyRecord visualization server starting\n";
    banner += "Listen address: http://" + config.address + ":" + std::to_string(config.port) + "\n";
    banner += "Database path: " + config.dbPath + "\n";
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
