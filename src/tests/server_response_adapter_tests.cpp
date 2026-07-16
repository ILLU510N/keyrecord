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
    ok = expect(response.keep_alive(), "Concurrent server responses should preserve requested keep-alive") && ok;
    ok = expect(!response.need_eof(), "Keep-alive response should not require EOF") && ok;

    const auto closeResponse = keyrecord::toBeastResponse(source, 11, false);
    ok = expect(!closeResponse.keep_alive(), "Connection-close request should disable keep-alive") && ok;
    ok = expect(closeResponse.need_eof(), "Connection-close response should require EOF") && ok;

    return ok ? 0 : 1;
}
