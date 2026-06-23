#include "../server_response_adapter.h"

#include <iostream>

namespace {

bool expect(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << "\n";
        return false;
    }
    return true;
}

} // namespace

int main() {
    const keyrecord::HttpResponse source{
        200,
        "text/plain; charset=utf-8",
        "ok",
        {},
    };

    const auto response = keyrecord::toBeastResponse(source, 11, true);

    bool ok = true;
    ok = expect(!response.keep_alive(), "单线程服务端响应必须关闭 keep-alive，避免阻塞后续静态资源请求") && ok;
    ok = expect(response.need_eof(), "关闭 keep-alive 后响应应要求连接结束") && ok;

    return ok ? 0 : 1;
}
