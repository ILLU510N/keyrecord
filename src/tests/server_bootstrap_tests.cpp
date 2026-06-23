#include "../server_bootstrap.h"
#include "../config_path.h"

#include <iostream>
#include <string>

namespace {

bool expect(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << "\n";
        return false;
    }
    return true;
}

bool expectEqual(const std::string& actual, const std::string& expected, const char* message) {
    if (actual != expected) {
        std::cerr << message << "\nExpected: " << expected << "\nActual: " << actual << "\n";
        return false;
    }
    return true;
}

} // namespace

int main() {
    bool ok = true;

    std::string errorMessage;
    char programName[] = "keyrecord_server";

    {
        char* argv[] = {programName};
        auto config = keyrecord::buildServerConfig(1, argv, &errorMessage);
        ok = expect(config.has_value(), "Default arguments should build a server config") && ok;
        ok = expect(errorMessage.empty(), "Default arguments should not return an error message") && ok;
        if (config) {
            ok = expectEqual(
                     config->dbPath,
                     keyrecord::getDefaultDatabasePath(),
                     "Default database path should use the config directory") &&
                 ok;
            ok = expect(config->address == "0.0.0.0", "Default listen address should be 0.0.0.0") && ok;
            ok = expect(config->port == 3000, "Default listen port should be 3000") && ok;
        }
    }

    {
        errorMessage.clear();
        char dbPath[] = "D:/tmp/custom.db";
        char* argv[] = {programName, dbPath};
        auto config = keyrecord::buildServerConfig(2, argv, &errorMessage);
        ok = expect(config.has_value(), "Explicit database path should build a server config") && ok;
        ok = expect(errorMessage.empty(), "Explicit database path should not return an error message") && ok;
        if (config) {
            ok = expectEqual(config->dbPath, dbPath, "Explicit database path was not stored in the server config") && ok;
        }
    }

    {
        errorMessage.clear();
        char emptyPath[] = "";
        char* argv[] = {programName, emptyPath};
        auto config = keyrecord::buildServerConfig(2, argv, &errorMessage);
        ok = expect(!config.has_value(), "Empty database path should not build a server config") && ok;
        ok = expectEqual(errorMessage, "db_path must not be empty", "Unexpected error message for empty database path") && ok;
    }

    {
        errorMessage.clear();
        char dbPath[] = "D:/tmp/custom.db";
        char extra[] = "unexpected";
        char* argv[] = {programName, dbPath, extra};
        auto config = keyrecord::buildServerConfig(3, argv, &errorMessage);
        ok = expect(!config.has_value(), "Extra arguments should not build a server config") && ok;
        ok = expectEqual(errorMessage, "usage: keyrecord_server [db_path]", "Unexpected error message for extra arguments") && ok;
    }

    return ok ? 0 : 1;
}
