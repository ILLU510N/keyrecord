#include "../api_queries.h"
#include "../readonly_database.h"

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
        std::cerr << message << "\n期望: " << expected << "\n实际: " << actual << "\n";
        return false;
    }
    return true;
}

bool execSql(sqlite3* database, const char* sql) {
    char* error = nullptr;
    const int rc = sqlite3_exec(database, sql, nullptr, nullptr, &error);
    if (rc != SQLITE_OK) {
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
                            "INSERT INTO keys(timestamp,date,hour,vk_code,key_name) "
                            "VALUES(1767225600,'2026-01-01',0,65,'A');");
    sqlite3_close(database);
    return ok;
}

} // namespace

int main() {
    const auto dbPath = std::filesystem::temp_directory_path() / "keyrecord_readonly_test.db";
    const auto missingPath = std::filesystem::temp_directory_path() / "keyrecord_missing_readonly_test.db";
    std::filesystem::remove(dbPath);
    std::filesystem::remove(missingPath);

    bool ok = expect(createWritableFixture(dbPath), "测试数据库创建失败");

    std::string errorMessage;
    {
        auto database = keyrecord::ReadOnlyDatabase::open(dbPath.string(), &errorMessage);
        ok = expect(database.has_value(), "只读打开已有数据库失败") && ok;
        ok = expect(errorMessage.empty(), "成功打开时错误信息应为空") && ok;

        if (database) {
            const auto info = keyrecord::queryInfo(database->get());
            ok = expectEqual(info.body,
                             "{\"total_keys\":1,\"first_date\":\"2026-01-01\",\"last_date\":\"2026-01-01\",\"unique_keys\":1}",
                             "只读连接查询结果错误") &&
                 ok;
            ok = expect(!execSql(database->get(),
                                 "INSERT INTO keys(timestamp,date,hour,vk_code,key_name) "
                                 "VALUES(1767225601,'2026-01-01',0,66,'B');"),
                        "只读连接不应允许写入") &&
                 ok;
        }
    }

    errorMessage.clear();
    auto missing = keyrecord::ReadOnlyDatabase::open(missingPath.string(), &errorMessage);
    ok = expect(!missing.has_value(), "只读打开不存在数据库不应成功") && ok;
    ok = expect(!errorMessage.empty(), "打开不存在数据库失败时应返回错误信息") && ok;
    ok = expect(!std::filesystem::exists(missingPath), "只读打开不存在数据库不应创建文件") && ok;

    std::filesystem::remove(dbPath);
    return ok ? 0 : 1;
}
