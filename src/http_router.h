#pragma once

#include <string>
#include <string_view>
#include <vector>

struct sqlite3;

namespace keyrecord {

struct HttpHeader {
    std::string name;
    std::string value;
};

struct HttpResponse {
    int status;
    std::string contentType;
    std::string body;
    std::vector<HttpHeader> headers;
};

HttpResponse handleHttpRequest(std::string_view method, std::string_view target, sqlite3* database);

} // namespace keyrecord
