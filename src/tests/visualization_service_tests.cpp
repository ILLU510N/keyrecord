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

    bool ok = expect(createWritableFixture(dbPath), "测试数据库创建失败");

    std::string errorMessage;
    auto missingService = keyrecord::VisualizationService::open(missingPath.string(), &errorMessage);
    ok = expect(!missingService.has_value(), "不存在的数据库不应创建可视化服务") && ok;
    ok = expect(!errorMessage.empty(), "数据库打开失败时应返回错误信息") && ok;
    ok = expect(!std::filesystem::exists(missingPath), "只读服务打开不存在数据库不应创建文件") && ok;

    {
        errorMessage.clear();
        auto service = keyrecord::VisualizationService::open(dbPath.string(), &errorMessage);
        ok = expect(service.has_value(), "可视化服务打开已有数据库失败") && ok;
        ok = expect(errorMessage.empty(), "成功打开可视化服务时错误信息应为空") && ok;

        if (service) {
            const auto index = service->handleRequest("GET", "/");
            ok = expect(index.status == 200, "可视化服务首页状态码应为 200") && ok;
            ok = expect(index.contentType == "text/html; charset=utf-8", "可视化服务首页 Content-Type 错误") && ok;
            ok = expect(index.body.find("KeyRecord") != std::string::npos, "可视化服务首页内容缺少 KeyRecord") && ok;
            ok = expect(hasHeader(index, "Access-Control-Allow-Origin", "*"), "可视化服务首页缺少 CORS 头") && ok;

            const auto info = service->handleRequest("GET", "/api/info");
            ok = expectEqual(
                     info.body,
                     "{\"total_keys\":2,\"first_date\":\"2026-01-01\",\"last_date\":\"2026-01-02\",\"unique_keys\":2}",
                     "可视化服务 /api/info JSON 不匹配") &&
                 ok;
            ok = expect(hasHeader(info, "Access-Control-Allow-Origin", "*"), "可视化服务 API 缺少 CORS 头") && ok;
        }
    }

    std::filesystem::remove(dbPath);
    return ok ? 0 : 1;
}
