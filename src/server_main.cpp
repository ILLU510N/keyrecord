#include "server.h"
#include "server_bootstrap.h"
#include "server_startup.h"

#include <exception>
#include <iostream>

int main(int argc, char* argv[]) {
    try {
        std::string errorMessage;
        auto config = keyrecord::buildServerConfig(argc, argv, &errorMessage);
        if (!config) {
            std::cerr << "Server startup argument error: " << errorMessage << "\n";
            return 1;
        }

        errorMessage.clear();
        auto startup = keyrecord::prepareServerStartup(*config, &errorMessage);
        if (!startup) {
            std::cerr << "Failed to open visualization database: " << errorMessage << "\n";
            return 1;
        }

        std::cout << startup->banner;

        return keyrecord::runServer(*config, startup->service);
    } catch (const std::exception& ex) {
        std::cerr << "Server startup failed: " << ex.what() << "\n";
        return 1;
    }
}
