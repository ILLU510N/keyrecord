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
    ok = expect(!response.keep_alive(), "Single-threaded server responses must disable keep-alive to avoid blocking later static asset requests") && ok;
    ok = expect(response.need_eof(), "Response should require EOF after keep-alive is disabled") && ok;

    return ok ? 0 : 1;
}
