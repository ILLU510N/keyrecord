#include "../server_bootstrap.h"
#include "../config_path.h"

#include <iostream>
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

} // namespace

int main() {
    bool ok = true;

    std::string errorMessage;
    char programName[] = "keyrecord_server";

    {
        char* argv[] = {programName};
        auto config = keyrecord::buildServerConfig(1, argv, &errorMessage);
        ok = expect(config.has_value(), "默认启动参数应能构建 server config") && ok;
        ok = expect(errorMessage.empty(), "默认启动参数不应返回错误信息") && ok;
        if (config) {
            ok = expectEqual(
                     config->dbPath,
                     keyrecord::getDefaultDatabasePath(),
                     "默认数据库路径应使用配置目录") &&
                 ok;
            ok = expect(config->address == "0.0.0.0", "默认监听地址应为 0.0.0.0") && ok;
            ok = expect(config->port == 3000, "默认监听端口应为 3000") && ok;
        }
    }

    {
        errorMessage.clear();
        char dbPath[] = "D:/tmp/custom.db";
        char* argv[] = {programName, dbPath};
        auto config = keyrecord::buildServerConfig(2, argv, &errorMessage);
        ok = expect(config.has_value(), "显式数据库路径应能构建 server config") && ok;
        ok = expect(errorMessage.empty(), "显式数据库路径不应返回错误信息") && ok;
        if (config) {
            ok = expectEqual(config->dbPath, dbPath, "显式数据库路径没有写入 server config") && ok;
        }
    }

    {
        errorMessage.clear();
        char emptyPath[] = "";
        char* argv[] = {programName, emptyPath};
        auto config = keyrecord::buildServerConfig(2, argv, &errorMessage);
        ok = expect(!config.has_value(), "空数据库路径不应构建 server config") && ok;
        ok = expectEqual(errorMessage, "db_path must not be empty", "空数据库路径错误信息不匹配") && ok;
    }

    {
        errorMessage.clear();
        char dbPath[] = "D:/tmp/custom.db";
        char extra[] = "unexpected";
        char* argv[] = {programName, dbPath, extra};
        auto config = keyrecord::buildServerConfig(3, argv, &errorMessage);
        ok = expect(!config.has_value(), "多余参数不应构建 server config") && ok;
        ok = expectEqual(errorMessage, "usage: keyrecord_server [db_path]", "多余参数错误信息不匹配") && ok;
    }

    return ok ? 0 : 1;
}
