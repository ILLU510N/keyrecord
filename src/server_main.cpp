#include "server.h"
#include "server_bootstrap.h"
#include "server_startup.h"

#include <iostream>

int main(int argc, char* argv[]) {
    std::string errorMessage;
    auto config = keyrecord::buildServerConfig(argc, argv, &errorMessage);
    if (!config) {
        std::cerr << "服务端启动参数错误: " << errorMessage << "\n";
        return 1;
    }

    errorMessage.clear();
    auto startup = keyrecord::prepareServerStartup(*config, &errorMessage);
    if (!startup) {
        std::cerr << "打开可视化数据库失败: " << errorMessage << "\n";
        return 1;
    }

    std::cout << startup->banner;

    return keyrecord::runServer(*config, startup->service);
}
