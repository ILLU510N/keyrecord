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
        std::cerr << "测试数据库打开失败\n";
        return 1;
    }

    bool ok = seedDatabase(database);

    const auto index = keyrecord::handleHttpRequest("GET", "/", database);
    ok = expect(index.status == 200, "首页状态码应为 200") && ok;
    ok = expect(index.contentType == "text/html; charset=utf-8", "首页 Content-Type 错误") && ok;
    ok = expect(index.body.find("KeyRecord") != std::string::npos, "首页内容缺少 KeyRecord") && ok;
    ok = expect(hasHeader(index, "Access-Control-Allow-Origin", "*"), "首页响应缺少 CORS 头") && ok;

    const auto heatmap = keyrecord::handleHttpRequest("GET", "/api/heatmap?start=2026-01-02&end=2026-01-02", database);
    ok = expectEqual(heatmap.body,
                     "[{\"vk_code\":66,\"key_name\":\"B\",\"count\":1,\"x\":435,\"y\":285}]",
                     "heatmap 路由没有正确解析 start/end query") &&
         ok;
    ok = expect(hasHeader(heatmap, "Access-Control-Allow-Origin", "*"), "API 响应缺少 CORS 头") && ok;

    const auto keys = keyrecord::handleHttpRequest("GET", "/api/keys?limit=1", database);
    ok = expectEqual(keys.body,
                     "[{\"key_name\":\"A\",\"vk_code\":65,\"count\":2}]",
                     "keys 路由没有正确解析 limit query") &&
         ok;

    const auto missing = keyrecord::handleHttpRequest("GET", "/api/missing", database);
    ok = expect(missing.status == 404, "未知 API 状态码应为 404") && ok;
    ok = expectEqual(missing.body, "{\"error\":\"not found\"}", "未知 API 响应体错误") && ok;

    const auto post = keyrecord::handleHttpRequest("POST", "/api/info", database);
    ok = expect(post.status == 405, "非 GET 方法状态码应为 405") && ok;
    ok = expect(hasHeader(post, "Allow", "GET, OPTIONS"), "405 响应缺少完整 Allow 头") && ok;

    const auto options = keyrecord::handleHttpRequest("OPTIONS", "/api/info", database);
    ok = expect(options.status == 204, "OPTIONS 预检状态码应为 204") && ok;
    ok = expect(options.body.empty(), "OPTIONS 预检响应体应为空") && ok;
    ok = expect(hasHeader(options, "Access-Control-Allow-Origin", "*"), "OPTIONS 响应缺少 CORS Origin 头") && ok;
    ok = expect(hasHeader(options, "Access-Control-Allow-Methods", "GET, OPTIONS"), "OPTIONS 响应缺少允许方法头") && ok;

    sqlite3_close(database);
    std::filesystem::remove(dbPath);

    return ok ? 0 : 1;
}
