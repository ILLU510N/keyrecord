#pragma once

#include <string>

namespace keyrecord {

inline constexpr char DEFAULT_SERVER_ADDRESS[] = "127.0.0.1";
inline constexpr unsigned short DEFAULT_SERVER_PORT = 3000;

struct ServerConfig {
    std::string address = DEFAULT_SERVER_ADDRESS;
    unsigned short port = DEFAULT_SERVER_PORT;
    std::string dbPath;
};

} // namespace keyrecord
