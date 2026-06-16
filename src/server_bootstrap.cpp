#include "server_bootstrap.h"

#include "config_path.h"

namespace keyrecord {

std::string defaultServerDatabasePath() {
    return getDefaultDatabasePath();
}

std::optional<ServerConfig> buildServerConfig(int argc, char* argv[], std::string* errorMessage) {
    if (errorMessage) {
        errorMessage->clear();
    }

    ServerConfig config;
    config.dbPath = defaultServerDatabasePath();

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
