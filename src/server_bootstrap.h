#pragma once

#include "server.h"

#include <optional>
#include <string>

namespace keyrecord {

std::string defaultServerDatabasePath();

std::optional<ServerConfig> buildServerConfig(
    int argc,
    char* argv[],
    std::string* errorMessage = nullptr);

} // namespace keyrecord
