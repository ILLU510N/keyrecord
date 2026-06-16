#pragma once

#include "http_router.h"

#include <boost/beast/http.hpp>

namespace keyrecord {

boost::beast::http::response<boost::beast::http::string_body> toBeastResponse(
    const HttpResponse& response,
    unsigned version,
    bool keepAlive);

} // namespace keyrecord
