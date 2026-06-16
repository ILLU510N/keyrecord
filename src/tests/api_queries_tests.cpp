#include "../api_queries.h"

#include <filesystem>
#include <iostream>
#include <sqlite3.h>
#include <string>

namespace {

bool expectEqual(const std::string& actual, const std::string& expected, const char* message) {
    if (actual != expected) {
        std::cerr << message << "\n期望: " << expected << "\n实际: " << actual << "\n";
        return false;
    }
    return true;
}

bool execSql(sqlite3* database, const char* sql) {
    char* error = nullptr;
    const int rc = sqlite3_exec(database, sql, nullptr, nullptr, &error);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL 执行失败: " << (error ? error : sqlite3_errmsg(database)) << "\n";
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
                   "(1767312000,'2026-01-02',0,66,'B');");
}

} // namespace

int main() {
    const auto dbPath = std::filesystem::temp_directory_path() / "keyrecord_api_queries_test.db";
    std::filesystem::remove(dbPath);

    sqlite3* database = nullptr;
    if (sqlite3_open(dbPath.string().c_str(), &database) != SQLITE_OK) {
        std::cerr << "测试数据库打开失败\n";
        return 1;
    }

    bool ok = seedDatabase(database);

    const auto info = keyrecord::queryInfo(database);
    ok = expectEqual(info.body,
                     "{\"total_keys\":4,\"first_date\":\"2026-01-01\",\"last_date\":\"2026-01-02\",\"unique_keys\":3}",
                     "/api/info JSON 不匹配") &&
         ok;

    const auto dailyStats = keyrecord::queryDailyStats(database);
    ok = expectEqual(dailyStats.body,
                     "[{\"date\":\"2026-01-01\",\"count\":3},{\"date\":\"2026-01-02\",\"count\":1}]",
                     "/api/daily-stats JSON 不匹配") &&
         ok;

    const auto topKey = keyrecord::queryKeys(database, std::nullopt, std::nullopt, 1);
    ok = expectEqual(topKey.body,
                     "[{\"key_name\":\"A\",\"vk_code\":65,\"count\":2}]",
                     "/api/keys limit JSON 不匹配") &&
         ok;

    const auto rangedKeys = keyrecord::queryKeys(database, "2026-01-02", "2026-01-02", 20);
    ok = expectEqual(rangedKeys.body,
                     "[{\"key_name\":\"B\",\"vk_code\":66,\"count\":1}]",
                     "/api/keys 日期范围 JSON 不匹配") &&
         ok;

    const auto heatmap = keyrecord::queryHeatmap(database, "2026-01-01", std::nullopt, std::nullopt);
    ok = expectEqual(heatmap.body,
                     "[{\"vk_code\":65,\"key_name\":\"A\",\"count\":2,\"x\":175,\"y\":225},"
                     "{\"vk_code\":222,\"key_name\":\"'\",\"count\":1,\"x\":775,\"y\":225}]",
                     "/api/heatmap JSON 不匹配") &&
         ok;

    sqlite3_close(database);
    std::filesystem::remove(dbPath);

    return ok ? 0 : 1;
}
