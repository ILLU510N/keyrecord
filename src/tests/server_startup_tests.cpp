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
        std::cerr << message << "\n期望: " << expected << "\n实际: " << actual << "\n";
        return false;
    }
    return true;
}

bool expectContains(const std::string& actual, const char* expected, const char* message) {
    if (actual.find(expected) == std::string::npos) {
        std::cerr << message << "\n缺少: " << expected << "\n";
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

    ok = expect(createWritableFixture(dbPath), "测试数据库创建失败") && ok;

    keyrecord::ServerConfig config;
    config.address = "127.0.0.1";
    config.port = 3456;
    config.dbPath = dbPath.string();

    std::string errorMessage;
    {
        auto startup = keyrecord::prepareServerStartup(config, &errorMessage);
        ok = expect(startup.has_value(), "已有数据库应能准备 server startup") && ok;
        ok = expect(errorMessage.empty(), "成功准备 server startup 时错误信息应为空") && ok;
        if (startup) {
            ok = expectContains(startup->banner, "http://127.0.0.1:3456", "启动信息缺少监听地址") && ok;
            ok = expectContains(startup->banner, config.dbPath.c_str(), "启动信息缺少数据库路径") && ok;
        }
    }

    config.dbPath = missingPath.string();
    errorMessage.clear();
    auto missingStartup = keyrecord::prepareServerStartup(config, &errorMessage);
    ok = expect(!missingStartup.has_value(), "不存在数据库不应准备 server startup") && ok;
    ok = expect(!errorMessage.empty(), "数据库打开失败时应返回错误信息") && ok;
    ok = expectEqual(keyrecord::buildServerStartupBanner(config),
                     "KeyRecord 可视化服务启动中\n监听地址: http://127.0.0.1:3456\n数据库路径: " + missingPath.string() + "\n",
                     "启动信息格式不匹配") &&
         ok;

    std::filesystem::remove(dbPath);
    return ok ? 0 : 1;
}
