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
                   "(1767225602,'2026-01-01',13,222,'''');"
                   "INSERT INTO keys(timestamp,date,hour,vk_code,key_name) VALUES"
                   "(1767312000,'2026-01-02',5,66,'B');"
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

bool seedSummaryOnlyDatabase(sqlite3* database) {
    return execSql(database,
                   "CREATE TABLE keys("
                   "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                   "timestamp INTEGER,"
                   "date TEXT,"
                   "hour INTEGER,"
                   "vk_code INTEGER,"
                   "key_name TEXT);"
                   "CREATE TABLE daily_key_stats("
                   "date TEXT NOT NULL,"
                   "vk_code INTEGER NOT NULL,"
                   "key_name TEXT NOT NULL,"
                   "count INTEGER NOT NULL,"
                   "PRIMARY KEY(date, vk_code, key_name));"
                   "CREATE TABLE daily_hour_stats("
                   "date TEXT NOT NULL,"
                   "hour INTEGER NOT NULL,"
                   "count INTEGER NOT NULL,"
                   "PRIMARY KEY(date, hour));"
                   "INSERT INTO daily_key_stats(date,vk_code,key_name,count) VALUES"
                   "('2026-02-01',65,'A',2),"
                   "('2026-02-02',66,'B',1);"
                   "INSERT INTO daily_hour_stats(date,hour,count) VALUES"
                   "('2026-02-01',9,2),"
                   "('2026-02-02',10,1);");
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

    const auto hourlyStats = keyrecord::queryHourlyStats(database, "2026-01-01");
    ok = expectEqual(hourlyStats.body,
                     "[{\"hour\":0,\"count\":2},{\"hour\":13,\"count\":1}]",
                     "/api/hourly-stats JSON mismatch") &&
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

    const auto hourlyHeatmap = keyrecord::queryHourlyHeatmap(database, "2026-01-01", "2026-01-01");
    ok = expectEqual(hourlyHeatmap.body,
                     "[{\"weekday\":4,\"hour\":0,\"count\":2},{\"weekday\":4,\"hour\":13,\"count\":1}]",
                     "/api/hourly-heatmap weekday/hour JSON mismatch") &&
         ok;

    const auto timeline = keyrecord::queryTimeline(database, "2026-01-02", 100);
    ok = expectEqual(timeline.body,
                     "[{\"ts\":1767312000,\"vk_code\":66,\"key_name\":\"B\"}]",
                     "/api/timeline JSON mismatch") &&
         ok;

    const auto regionStats = keyrecord::queryRegionStats(database, "2026-01-03", "2026-01-03");
    ok = expectEqual(regionStats.body,
                     "[{\"region\":\"letters\",\"count\":0},{\"region\":\"digits\",\"count\":0},"
                     "{\"region\":\"numpad\",\"count\":1},{\"region\":\"function\",\"count\":4},"
                     "{\"region\":\"navigation\",\"count\":5},{\"region\":\"modifiers\",\"count\":0},"
                     "{\"region\":\"control\",\"count\":0},{\"region\":\"punctuation\",\"count\":0},"
                     "{\"region\":\"other\",\"count\":0}]",
                     "/api/region-stats JSON mismatch") &&
         ok;

    const auto handStats = keyrecord::queryHandStats(database, "2026-01-03", "2026-01-03");
    ok = expectEqual(handStats.body,
                     "[{\"hand\":\"left\",\"count\":4},{\"hand\":\"right\",\"count\":6},"
                     "{\"hand\":\"both\",\"count\":0},{\"hand\":\"unknown\",\"count\":0}]",
                     "/api/hand-stats JSON mismatch") &&
         ok;

    const auto speed = keyrecord::querySpeed(database, "2026-01-03", 5);
    ok = expectEqual(speed.body,
                     "[{\"minute\":0,\"count\":10}]",
                     "/api/speed JSON mismatch") &&
         ok;

    // 组合统计需要修饰键紧随普通键的有序事件，单独插入到 2026-01-04 验证。
    ok = execSql(database,
                 "INSERT INTO keys(timestamp,date,hour,vk_code,key_name) VALUES"
                 "(1767484800,'2026-01-04',9,162,'Ctrl'),"
                 "(1767484800,'2026-01-04',9,67,'C'),"
                 "(1767484801,'2026-01-04',9,162,'Ctrl'),"
                 "(1767484801,'2026-01-04',9,67,'C'),"
                 "(1767484802,'2026-01-04',9,162,'Ctrl'),"
                 "(1767484802,'2026-01-04',9,86,'V');") &&
         ok;

    const auto combos = keyrecord::queryCombos(database, "2026-01-04", std::nullopt, std::nullopt, 20);
    ok = expectEqual(combos.body,
                     "[{\"combo\":\"Ctrl+C\",\"count\":2},{\"combo\":\"Ctrl+V\",\"count\":1}]",
                     "/api/combos JSON mismatch") &&
         ok;

    sqlite3_close(database);
    std::filesystem::remove(dbPath);

    const auto summaryDbPath = std::filesystem::temp_directory_path() / "keyrecord_api_queries_summary_test.db";
    std::filesystem::remove(summaryDbPath);

    sqlite3* summaryDatabase = nullptr;
    if (sqlite3_open(summaryDbPath.string().c_str(), &summaryDatabase) != SQLITE_OK) {
        std::cerr << "summary test database open failed\n";
        return 1;
    }

    ok = seedSummaryOnlyDatabase(summaryDatabase) && ok;

    const auto summaryInfo = keyrecord::queryInfo(summaryDatabase);
    ok = expectEqual(summaryInfo.body,
                     "{\"total_keys\":3,\"first_date\":\"2026-02-01\",\"last_date\":\"2026-02-02\","
                     "\"unique_keys\":2}",
                     "summary-backed /api/info JSON mismatch") &&
         ok;

    const auto summaryDailyStats = keyrecord::queryDailyStats(summaryDatabase);
    ok = expectEqual(summaryDailyStats.body,
                     "[{\"date\":\"2026-02-01\",\"count\":2},{\"date\":\"2026-02-02\",\"count\":1}]",
                     "summary-backed /api/daily-stats JSON mismatch") &&
         ok;

    const auto summaryHourlyStats = keyrecord::queryHourlyStats(summaryDatabase, "2026-02-01");
    ok = expectEqual(summaryHourlyStats.body,
                     "[{\"hour\":9,\"count\":2}]",
                     "summary-backed /api/hourly-stats JSON mismatch") &&
         ok;

    const auto summaryTopKey = keyrecord::queryKeys(summaryDatabase, std::nullopt, std::nullopt, 1);
    ok = expectEqual(summaryTopKey.body,
                     "[{\"key_name\":\"A\",\"vk_code\":65,\"count\":2}]",
                     "summary-backed /api/keys JSON mismatch") &&
         ok;

    const auto summaryHeatmap = keyrecord::queryHeatmap(summaryDatabase, std::nullopt, "2026-02-01", "2026-02-02");
    ok = expectEqual(summaryHeatmap.body,
                     "[{\"vk_code\":65,\"key_name\":\"A\",\"count\":2,\"x\":175,\"y\":225},"
                     "{\"vk_code\":66,\"key_name\":\"B\",\"count\":1,\"x\":435,\"y\":285}]",
                     "summary-backed /api/heatmap JSON mismatch") &&
         ok;

    const auto summaryHourlyHeatmap =
        keyrecord::queryHourlyHeatmap(summaryDatabase, "2026-02-01", "2026-02-02");
    ok = expectEqual(summaryHourlyHeatmap.body,
                     "[{\"weekday\":0,\"hour\":9,\"count\":2},{\"weekday\":1,\"hour\":10,\"count\":1}]",
                     "summary-backed /api/hourly-heatmap JSON mismatch") &&
         ok;

    const auto summaryRegionStats = keyrecord::queryRegionStats(summaryDatabase, "2026-02-01", "2026-02-02");
    ok = expectEqual(summaryRegionStats.body,
                     "[{\"region\":\"letters\",\"count\":3},{\"region\":\"digits\",\"count\":0},"
                     "{\"region\":\"numpad\",\"count\":0},{\"region\":\"function\",\"count\":0},"
                     "{\"region\":\"navigation\",\"count\":0},{\"region\":\"modifiers\",\"count\":0},"
                     "{\"region\":\"control\",\"count\":0},{\"region\":\"punctuation\",\"count\":0},"
                     "{\"region\":\"other\",\"count\":0}]",
                     "summary-backed /api/region-stats JSON mismatch") &&
         ok;

    const auto summaryHandStats = keyrecord::queryHandStats(summaryDatabase, "2026-02-01", "2026-02-02");
    ok = expectEqual(summaryHandStats.body,
                     "[{\"hand\":\"left\",\"count\":3},{\"hand\":\"right\",\"count\":0},"
                     "{\"hand\":\"both\",\"count\":0},{\"hand\":\"unknown\",\"count\":0}]",
                     "summary-backed /api/hand-stats JSON mismatch") &&
         ok;

    sqlite3_close(summaryDatabase);
    std::filesystem::remove(summaryDbPath);

    return ok ? 0 : 1;
}
