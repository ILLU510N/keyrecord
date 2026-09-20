#include "visualization_url.h"

#include "server_config.h"

namespace keyrecord {

std::string buildVisualizationPageUrl(const ConfigFileValues& values) {
    std::string address = values.address.value_or(DEFAULT_SERVER_ADDRESS);
    if (address == "0.0.0.0") {
        address = "127.0.0.1";
    } else if (address == "::") {
        address = "::1";
    }

    const unsigned short port = values.port.value_or(DEFAULT_SERVER_PORT);
    if (address.find(':') != std::string::npos) {
        return "http://[" + address + "]:" + std::to_string(port) + "/";
    }
    return "http://" + address + ":" + std::to_string(port) + "/";
}

} // namespace keyrecord
