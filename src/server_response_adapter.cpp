#include "server_response_adapter.h"

#include <boost/beast/version.hpp>

namespace keyrecord {
namespace {

void applyHeaders(
    const HttpResponse& source,
    boost::beast::http::response<boost::beast::http::string_body>& target) {
    for (const auto& header : source.headers) {
        target.set(header.name, header.value);
    }
}

} // namespace

boost::beast::http::response<boost::beast::http::string_body> toBeastResponse(
    const HttpResponse& response,
    unsigned version,
    bool keepAlive) {
    namespace http = boost::beast::http;

    http::response<http::string_body> beastResponse{
        static_cast<http::status>(response.status),
        version,
    };
    beastResponse.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    beastResponse.set(http::field::content_type, response.contentType);
    beastResponse.keep_alive(keepAlive);
    applyHeaders(response, beastResponse);
    beastResponse.body() = response.body;
    beastResponse.prepare_payload();
    return beastResponse;
}

} // namespace keyrecord
