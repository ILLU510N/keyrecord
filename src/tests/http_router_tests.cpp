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
    if (!execSql(database,
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
                 "(1767312000,'2026-01-02',5,66,'B');")) {
        return false;
    }

    for (int i = 0; i < 105; ++i) {
        const std::string sql =
            "INSERT INTO keys(timestamp,date,hour,vk_code,key_name) VALUES(1767398400,'2026-01-03',0," +
            std::to_string(1000 + i) + ",'Bulk" + std::to_string(i) + "');";
        if (!execSql(database, sql.c_str())) {
            return false;
        }
    }

    return true;
}

bool hasHeader(const keyrecord::HttpResponse& response, std::string_view name, std::string_view value) {
    for (const auto& header : response.headers) {
        if (header.name == name && header.value == value) {
            return true;
        }
    }
    return false;
}

int countOccurrences(std::string_view text, std::string_view needle) {
    int count = 0;
    size_t offset = 0;
    while (true) {
        const size_t found = text.find(needle, offset);
        if (found == std::string_view::npos) {
            return count;
        }
        ++count;
        offset = found + needle.size();
    }
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
    ok = expect(!hasHeader(index, "Access-Control-Allow-Origin", "*"), "index response must not allow arbitrary cross-origin reads") && ok;

    const auto heatmap = keyrecord::handleHttpRequest("GET", "/api/heatmap?start=2026-01-02&end=2026-01-02", database);
    ok = expectEqual(heatmap.body,
                     "[{\"vk_code\":66,\"key_name\":\"B\",\"count\":1,\"x\":435,\"y\":285}]",
                     "heatmap route did not parse start/end query correctly") &&
         ok;
    ok = expect(!hasHeader(heatmap, "Access-Control-Allow-Origin", "*"), "API response must not allow arbitrary cross-origin reads") && ok;

    const auto malformedLimit = keyrecord::handleHttpRequest("GET", "/api/keys?limit=1junk", database);
    ok = expect(countOccurrences(malformedLimit.body, "\"key_name\"") == 20, "Malformed limit should fall back to the default") && ok;

    const auto keys = keyrecord::handleHttpRequest("GET", "/api/keys?limit=1", database);
    ok = expectEqual(keys.body,
                     "[{\"key_name\":\"A\",\"vk_code\":65,\"count\":2}]",
                     "keys route did not parse limit query correctly") &&
         ok;

    const auto cappedKeys = keyrecord::handleHttpRequest("GET", "/api/keys?limit=100000", database);
    ok = expect(cappedKeys.status == 200, "keys route should accept a large positive limit by capping it") && ok;
    ok = expect(countOccurrences(cappedKeys.body, "\"key_name\"") == 100, "keys route should cap limit to 100 rows") &&
         ok;

    const auto hourlyStats = keyrecord::handleHttpRequest("GET", "/api/hourly-stats?date=2026-01-02", database);
    ok = expectEqual(hourlyStats.body,
                     "[{\"hour\":5,\"count\":1}]",
                     "hourly stats route did not parse date query correctly") &&
         ok;

    const auto missingHourlyDate = keyrecord::handleHttpRequest("GET", "/api/hourly-stats", database);
    ok = expect(missingHourlyDate.status == 400, "hourly stats should require a date query") && ok;
    ok = expectEqual(missingHourlyDate.body, "{\"error\":\"date is required\"}", "missing date error body incorrect") &&
         ok;

    const auto hourlyHeatmap =
        keyrecord::handleHttpRequest("GET", "/api/hourly-heatmap?start=2026-01-01&end=2026-01-01", database);
    ok = expectEqual(hourlyHeatmap.body,
                     "[{\"weekday\":4,\"hour\":0,\"count\":2}]",
                     "hourly heatmap route did not aggregate weekday/hour correctly") &&
         ok;

    const auto regionStats =
        keyrecord::handleHttpRequest("GET", "/api/region-stats?start=2026-01-01&end=2026-01-01", database);
    ok = expectEqual(regionStats.body,
                     "[{\"region\":\"letters\",\"count\":2},{\"region\":\"digits\",\"count\":0},"
                     "{\"region\":\"numpad\",\"count\":0},{\"region\":\"function\",\"count\":0},"
                     "{\"region\":\"navigation\",\"count\":0},{\"region\":\"modifiers\",\"count\":0},"
                     "{\"region\":\"control\",\"count\":0},{\"region\":\"punctuation\",\"count\":0},"
                     "{\"region\":\"other\",\"count\":0}]",
                     "region stats route mismatch") &&
         ok;

    const auto handStats =
        keyrecord::handleHttpRequest("GET", "/api/hand-stats?start=2026-01-01&end=2026-01-01", database);
    ok = expectEqual(handStats.body,
                     "[{\"hand\":\"left\",\"count\":2},{\"hand\":\"right\",\"count\":0},"
                     "{\"hand\":\"both\",\"count\":0},{\"hand\":\"unknown\",\"count\":0}]",
                     "hand stats route mismatch") &&
         ok;

    const auto timeline = keyrecord::handleHttpRequest("GET", "/api/timeline?date=2026-01-02", database);
    ok = expectEqual(timeline.body,
                     "[{\"ts\":1767312000,\"vk_code\":66,\"key_name\":\"B\"}]",
                     "timeline route mismatch") &&
         ok;

    const auto speed = keyrecord::handleHttpRequest("GET", "/api/speed?date=2026-01-01&bucket=5", database);
    ok = expectEqual(speed.body, "[{\"minute\":0,\"count\":2}]", "speed route mismatch") && ok;

    const auto combos = keyrecord::handleHttpRequest("GET", "/api/combos?start=2026-01-01&end=2026-01-01", database);
    ok = expectEqual(combos.body, "[]", "combos route should be empty without modifier sequences") && ok;

    const auto missingTimelineDate = keyrecord::handleHttpRequest("GET", "/api/timeline", database);
    ok = expect(missingTimelineDate.status == 400, "timeline should require a date query") && ok;

    const auto invalidDate = keyrecord::handleHttpRequest("GET", "/api/heatmap?date=2026-02-30", database);
    ok = expect(invalidDate.status == 400, "invalid date should be rejected") && ok;
    ok = expectEqual(invalidDate.body, "{\"error\":\"invalid date format\"}", "invalid date error body incorrect") && ok;

    const auto incompleteRange = keyrecord::handleHttpRequest("GET", "/api/keys?start=2026-01-01", database);
    ok = expect(incompleteRange.status == 400, "incomplete date range should be rejected") && ok;
    ok = expectEqual(
             incompleteRange.body,
             "{\"error\":\"start and end must be provided together\"}",
             "incomplete range error body incorrect") &&
         ok;

    const auto reversedRange =
        keyrecord::handleHttpRequest("GET", "/api/heatmap?start=2026-01-02&end=2026-01-01", database);
    ok = expect(reversedRange.status == 400, "reversed date range should be rejected") && ok;
    ok = expectEqual(
             reversedRange.body,
             "{\"error\":\"start date must not be later than end date\"}",
             "reversed range error body incorrect") &&
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
    ok = expect(!hasHeader(options, "Access-Control-Allow-Origin", "*"), "OPTIONS response must not allow arbitrary origins") && ok;
    ok = expect(hasHeader(options, "Allow", "GET, OPTIONS"), "OPTIONS response missing Allow header") && ok;

    sqlite3_close(database);
    std::filesystem::remove(dbPath);

    return ok ? 0 : 1;
}
