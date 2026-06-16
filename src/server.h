#pragma once

#include "visualization_service.h"

#include <string>

namespace keyrecord {

struct ServerConfig {
    std::string address = "0.0.0.0";
    unsigned short port = 3000;
    std::string dbPath;
};

int runServer(const ServerConfig& config, VisualizationService& service);

} // namespace keyrecord
