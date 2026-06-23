#include "../server_startup.h"

#include <filesystem>
#include <iostream>
#include <sqlite3.h>
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

bool expectContains(const std::string& actual, const char* expected, const char* message) {
    if (actual.find(expected) == std::string::npos) {
        std::cerr << message << "\nMissing: " << expected << "\n";
        return false;
    }
    return true;
}

bool execSql(sqlite3* database, const char* sql) {
    char* error = nullptr;
    const int rc = sqlite3_exec(database, sql, nullptr, nullptr, &error);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL execution failed: " << (error ? error : sqlite3_errmsg(database)) << "\n";
        sqlite3_free(error);
        return false;
    }
    return true;
}

bool createWritableFixture(const std::filesystem::path& dbPath) {
    sqlite3* database = nullptr;
    if (sqlite3_open(dbPath.string().c_str(), &database) != SQLITE_OK) {
        return false;
    }

    const bool ok = execSql(database,
                            "CREATE TABLE keys("
                            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                            "timestamp INTEGER,"
                            "date TEXT,"
                            "hour INTEGER,"
                            "vk_code INTEGER,"
                            "key_name TEXT);");
    sqlite3_close(database);
    return ok;
}

} // namespace

int main() {
    bool ok = true;

    const auto dbPath = std::filesystem::temp_directory_path() / "keyrecord_server_startup_test.db";
    const auto missingPath = std::filesystem::temp_directory_path() / "keyrecord_server_startup_missing.db";
    std::filesystem::remove(dbPath);
    std::filesystem::remove(missingPath);

    ok = expect(createWritableFixture(dbPath), "Failed to create test database") && ok;

    keyrecord::ServerConfig config;
    config.address = "127.0.0.1";
    config.port = 3456;
    config.dbPath = dbPath.string();

    std::string errorMessage;
    {
        auto startup = keyrecord::prepareServerStartup(config, &errorMessage);
        ok = expect(startup.has_value(), "Existing database should prepare server startup") && ok;
        ok = expect(errorMessage.empty(), "Error message should be empty after successful server startup preparation") && ok;
        if (startup) {
            ok = expectContains(startup->banner, "http://127.0.0.1:3456", "Startup banner should include the listen address") && ok;
            ok = expectContains(startup->banner, config.dbPath.c_str(), "Startup banner should include the database path") && ok;
        }
    }

    config.dbPath = missingPath.string();
    errorMessage.clear();
    auto missingStartup = keyrecord::prepareServerStartup(config, &errorMessage);
    ok = expect(!missingStartup.has_value(), "Missing database should not prepare server startup") && ok;
    ok = expect(!errorMessage.empty(), "Failed database open should return an error message") && ok;
    ok = expectEqual(keyrecord::buildServerStartupBanner(config),
                     "KeyRecord visualization server starting\nListen address: http://127.0.0.1:3456\nDatabase path: " + missingPath.string() + "\n",
                     "Startup banner format mismatch") &&
         ok;

    std::filesystem::remove(dbPath);
    return ok ? 0 : 1;
}
