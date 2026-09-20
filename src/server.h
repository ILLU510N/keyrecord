#pragma once

#include "server_config.h"
#include "visualization_service.h"

namespace keyrecord {

int runServer(const ServerConfig& config, VisualizationService& service);

} // namespace keyrecord
