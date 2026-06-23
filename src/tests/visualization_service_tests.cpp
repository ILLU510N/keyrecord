#include "../visualization_service.h"

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
                            "key_name TEXT);"
                            "INSERT INTO keys(timestamp,date,hour,vk_code,key_name) VALUES"
                            "(1767225600,'2026-01-01',0,65,'A'),"
                            "(1767312000,'2026-01-02',0,66,'B');");
    sqlite3_close(database);
    return ok;
}

bool hasHeader(const keyrecord::HttpResponse& response, std::string_view name, std::string_view value) {
    for (const auto& header : response.headers) {
        if (header.name == name && header.value == value) {
            return true;
        }
    }
    return false;
}

} // namespace

int main() {
    const auto dbPath = std::filesystem::temp_directory_path() / "keyrecord_visualization_service_test.db";
    const auto missingPath =
        std::filesystem::temp_directory_path() / "keyrecord_visualization_service_missing_test.db";
    std::filesystem::remove(dbPath);
    std::filesystem::remove(missingPath);

    bool ok = expect(createWritableFixture(dbPath), "Failed to create test database");

    std::string errorMessage;
    auto missingService = keyrecord::VisualizationService::open(missingPath.string(), &errorMessage);
    ok = expect(!missingService.has_value(), "Missing database should not create a visualization service") && ok;
    ok = expect(!errorMessage.empty(), "Failed database open should return an error message") && ok;
    ok = expect(!std::filesystem::exists(missingPath), "Opening a missing database in read-only mode should not create a file") && ok;

    {
        errorMessage.clear();
        auto service = keyrecord::VisualizationService::open(dbPath.string(), &errorMessage);
        ok = expect(service.has_value(), "Failed to open visualization service for an existing database") && ok;
        ok = expect(errorMessage.empty(), "Error message should be empty after successfully opening the visualization service") && ok;

        if (service) {
            const auto index = service->handleRequest("GET", "/");
            ok = expect(index.status == 200, "Visualization service index should return status 200") && ok;
            ok = expect(index.contentType == "text/html; charset=utf-8", "Visualization service index Content-Type mismatch") && ok;
            ok = expect(index.body.find("KeyRecord") != std::string::npos, "Visualization service index should contain KeyRecord") && ok;
            ok = expect(hasHeader(index, "Access-Control-Allow-Origin", "*"), "Visualization service index should include the CORS header") && ok;

            const auto info = service->handleRequest("GET", "/api/info");
            ok = expectEqual(
                     info.body,
                     "{\"total_keys\":2,\"first_date\":\"2026-01-01\",\"last_date\":\"2026-01-02\",\"unique_keys\":2}",
                     "Visualization service /api/info JSON mismatch") &&
                 ok;
            ok = expect(hasHeader(info, "Access-Control-Allow-Origin", "*"), "Visualization service API should include the CORS header") && ok;
        }
    }

    std::filesystem::remove(dbPath);
    return ok ? 0 : 1;
}
