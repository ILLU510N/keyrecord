#include "../http_router.h"

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
        std::cerr << message << "\nexpected: " << expected << "\nactual: " << actual << "\n";
        return false;
    }
    return true;
}

bool execSql(sqlite3* database, const char* sql) {
    char* error = nullptr;
    const int rc = sqlite3_exec(database, sql, nullptr, nullptr, &error);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL exec failed: " << (error ? error : sqlite3_errmsg(database)) << "\n";
        sqlite3_free(error);
        return false;
    }
    return true;
}

bool seedDatabase(sqlite3* database) {
    return execSql(database,
                   "CREATE TABLE keys("
                   "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                   "timestamp INTEGER,"
                   "date TEXT,"
                   "hour INTEGER,"
                   "vk_code INTEGER,"
                   "key_name TEXT);"
                   "INSERT INTO keys(timestamp,date,hour,vk_code,key_name) VALUES"
                   "(1767225600,'2026-01-01',0,65,'A'),"
                   "(1767225601,'2026-01-01',0,65,'A'),"
                   "(1767312000,'2026-01-02',0,66,'B');");
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
    const auto dbPath = std::filesystem::temp_directory_path() / "keyrecord_http_router_test.db";
    std::filesystem::remove(dbPath);

    sqlite3* database = nullptr;
    if (sqlite3_open(dbPath.string().c_str(), &database) != SQLITE_OK) {
        std::cerr << "test database open failed\n";
        return 1;
    }

    bool ok = seedDatabase(database);

    const auto index = keyrecord::handleHttpRequest("GET", "/", database);
    ok = expect(index.status == 200, "index status code should be 200") && ok;
    ok = expect(index.contentType == "text/html; charset=utf-8", "index Content-Type incorrect") && ok;
    ok = expect(index.body.find("KeyRecord") != std::string::npos, "index body missing KeyRecord") && ok;
    ok = expect(hasHeader(index, "Access-Control-Allow-Origin", "*"), "index response missing CORS header") && ok;

    const auto heatmap = keyrecord::handleHttpRequest("GET", "/api/heatmap?start=2026-01-02&end=2026-01-02", database);
    ok = expectEqual(heatmap.body,
                     "[{\"vk_code\":66,\"key_name\":\"B\",\"count\":1,\"x\":435,\"y\":285}]",
                     "heatmap route did not parse start/end query correctly") &&
         ok;
    ok = expect(hasHeader(heatmap, "Access-Control-Allow-Origin", "*"), "API response missing CORS header") && ok;

    const auto keys = keyrecord::handleHttpRequest("GET", "/api/keys?limit=1", database);
    ok = expectEqual(keys.body,
                     "[{\"key_name\":\"A\",\"vk_code\":65,\"count\":2}]",
                     "keys route did not parse limit query correctly") &&
         ok;

    const auto missing = keyrecord::handleHttpRequest("GET", "/api/missing", database);
    ok = expect(missing.status == 404, "unknown API status code should be 404") && ok;
    ok = expectEqual(missing.body, "{\"error\":\"not found\"}", "unknown API response body incorrect") && ok;

    const auto post = keyrecord::handleHttpRequest("POST", "/api/info", database);
    ok = expect(post.status == 405, "non-GET method status code should be 405") && ok;
    ok = expect(hasHeader(post, "Allow", "GET, OPTIONS"), "405 response missing complete Allow header") && ok;

    const auto options = keyrecord::handleHttpRequest("OPTIONS", "/api/info", database);
    ok = expect(options.status == 204, "OPTIONS preflight status code should be 204") && ok;
    ok = expect(options.body.empty(), "OPTIONS preflight response body should be empty") && ok;
    ok = expect(hasHeader(options, "Access-Control-Allow-Origin", "*"), "OPTIONS response missing CORS Origin header") && ok;
    ok = expect(hasHeader(options, "Access-Control-Allow-Methods", "GET, OPTIONS"), "OPTIONS response missing allowed methods header") && ok;

    sqlite3_close(database);
    std::filesystem::remove(dbPath);

    return ok ? 0 : 1;
}
