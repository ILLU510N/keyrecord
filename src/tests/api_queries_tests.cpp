#include "../api_queries.h"

#include <filesystem>
#include <iostream>
#include <sqlite3.h>
#include <string>

namespace {

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
                   "(1767225602,'2026-01-01',0,222,'''');"
                   "INSERT INTO keys(timestamp,date,hour,vk_code,key_name) VALUES"
                   "(1767312000,'2026-01-02',0,66,'B');"
                   "INSERT INTO keys(timestamp,date,hour,vk_code,key_name) VALUES"
                   "(1767398400,'2026-01-03',0,116,'F5'),"
                   "(1767398401,'2026-01-03',0,116,'F5'),"
                   "(1767398402,'2026-01-03',0,116,'F5'),"
                   "(1767398403,'2026-01-03',0,116,'F5'),"
                   "(1767398404,'2026-01-03',0,36,'Home'),"
                   "(1767398405,'2026-01-03',0,36,'Home'),"
                   "(1767398406,'2026-01-03',0,36,'Home'),"
                   "(1767398407,'2026-01-03',0,39,'Right'),"
                   "(1767398408,'2026-01-03',0,39,'Right'),"
                   "(1767398409,'2026-01-03',0,98,'Numpad2');");
}

} // namespace

int main() {
    const auto dbPath = std::filesystem::temp_directory_path() / "keyrecord_api_queries_test.db";
    std::filesystem::remove(dbPath);

    sqlite3* database = nullptr;
    if (sqlite3_open(dbPath.string().c_str(), &database) != SQLITE_OK) {
        std::cerr << "test database open failed\n";
        return 1;
    }

    bool ok = seedDatabase(database);

    const auto info = keyrecord::queryInfo(database);
    ok = expectEqual(info.body,
                     "{\"total_keys\":14,\"first_date\":\"2026-01-01\",\"last_date\":\"2026-01-03\",\"unique_keys\":7}",
                     "/api/info JSON mismatch") &&
         ok;

    const auto dailyStats = keyrecord::queryDailyStats(database);
    ok = expectEqual(dailyStats.body,
                     "[{\"date\":\"2026-01-01\",\"count\":3},{\"date\":\"2026-01-02\",\"count\":1},"
                     "{\"date\":\"2026-01-03\",\"count\":10}]",
                     "/api/daily-stats JSON mismatch") &&
         ok;

    const auto topKey = keyrecord::queryKeys(database, std::nullopt, std::nullopt, 1);
    ok = expectEqual(topKey.body,
                     "[{\"key_name\":\"F5\",\"vk_code\":116,\"count\":4}]",
                     "/api/keys limit JSON mismatch") &&
         ok;

    const auto rangedKeys = keyrecord::queryKeys(database, "2026-01-02", "2026-01-02", 20);
    ok = expectEqual(rangedKeys.body,
                     "[{\"key_name\":\"B\",\"vk_code\":66,\"count\":1}]",
                     "/api/keys date range JSON mismatch") &&
         ok;

    const auto heatmap = keyrecord::queryHeatmap(database, "2026-01-01", std::nullopt, std::nullopt);
    ok = expectEqual(heatmap.body,
                     "[{\"vk_code\":65,\"key_name\":\"A\",\"count\":2,\"x\":175,\"y\":225},"
                     "{\"vk_code\":222,\"key_name\":\"'\",\"count\":1,\"x\":775,\"y\":225}]",
                     "/api/heatmap JSON mismatch") &&
         ok;

    const auto extendedHeatmap = keyrecord::queryHeatmap(database, "2026-01-03", std::nullopt, std::nullopt);
    ok = expectEqual(extendedHeatmap.body,
                     "[{\"vk_code\":116,\"key_name\":\"F5\",\"count\":4,\"x\":425,\"y\":45},"
                     "{\"vk_code\":36,\"key_name\":\"Home\",\"count\":3,\"x\":1055,\"y\":105},"
                     "{\"vk_code\":39,\"key_name\":\"Right\",\"count\":2,\"x\":1115,\"y\":345},"
                     "{\"vk_code\":98,\"key_name\":\"Numpad2\",\"count\":1,\"x\":1225,\"y\":285}]",
                     "/api/heatmap should keep function keys, navigation keys, arrow keys and numpad data") &&
         ok;

    sqlite3_close(database);
    std::filesystem::remove(dbPath);

    return ok ? 0 : 1;
}
