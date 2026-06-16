#pragma once

#include "server.h"
#include "visualization_service.h"

#include <optional>
#include <string>

namespace keyrecord {

struct ServerStartup {
    VisualizationService service;
    std::string banner;
};

std::string buildServerStartupBanner(const ServerConfig& config);

std::optional<ServerStartup> prepareServerStartup(
    const ServerConfig& config,
    std::string* errorMessage = nullptr);

} // namespace keyrecord
