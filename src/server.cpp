#include "server.h"

#include "server_response_adapter.h"

#include <boost/asio/ip/address.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/thread_pool.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

#include <chrono>
#include <exception>
#include <iostream>
#include <semaphore>
#include <string_view>
#include <utility>

#ifndef _WIN32
#include <sys/socket.h>
#include <sys/time.h>
#endif

namespace keyrecord {
namespace {

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

constexpr auto REQUEST_TIMEOUT = std::chrono::seconds(15);
constexpr std::size_t MAX_REQUEST_BODY_BYTES = 0;
constexpr std::size_t MAX_REQUEST_HEADER_BYTES = 16 * 1024;
constexpr std::ptrdiff_t MAX_CONCURRENT_SESSIONS = 16;

void logServerError(std::string_view stage, const beast::error_code& error) {
    std::cerr << "server " << stage << " failed: " << error.message() << "\n";
}

bool setSocketTimeouts(tcp::socket& socket) {
#ifdef _WIN32
    const DWORD timeoutMs = static_cast<DWORD>(
        std::chrono::duration_cast<std::chrono::milliseconds>(REQUEST_TIMEOUT).count());
    const char* timeout = reinterpret_cast<const char*>(&timeoutMs);
    const int timeoutSize = sizeof(timeoutMs);
#else
    const timeval timeoutValue{
        static_cast<time_t>(REQUEST_TIMEOUT.count()),
        0,
    };
    const void* timeout = &timeoutValue;
    const socklen_t timeoutSize = sizeof(timeoutValue);
#endif
    const auto handle = socket.native_handle();
    return setsockopt(handle, SOL_SOCKET, SO_RCVTIMEO, timeout, timeoutSize) == 0 &&
           setsockopt(handle, SOL_SOCKET, SO_SNDTIMEO, timeout, timeoutSize) == 0;
}

void doSession(tcp::socket socket, VisualizationService& service) {
    if (!setSocketTimeouts(socket)) {
        std::cerr << "server set-timeout failed\n";
        return;
    }
    beast::flat_buffer buffer;
    beast::error_code error;

    for (;;) {
        http::request_parser<http::string_body> parser;
        parser.body_limit(MAX_REQUEST_BODY_BYTES);
        parser.header_limit(MAX_REQUEST_HEADER_BYTES);
        http::read(socket, buffer, parser, error);
        if (error == http::error::end_of_stream) {
            break;
        }
        if (error) {
            logServerError("read", error);
            return;
        }
        auto request = parser.release();

        // 先复用现有的 method/target -> HttpResponse 适配层，再转换为 Beast 响应。
        const HttpResponse response = service.handleRequest(
            std::string_view(request.method_string().data(), request.method_string().size()),
            std::string_view(request.target().data(), request.target().size()));
        auto beastResponse = toBeastResponse(response, request.version(), request.keep_alive());

        http::write(socket, beastResponse, error);
        if (error) {
            logServerError("write", error);
            return;
        }
        if (!beastResponse.keep_alive()) {
            break;
        }
    }

    socket.shutdown(tcp::socket::shutdown_send, error);
}

} // namespace

int runServer(const ServerConfig& config, VisualizationService& service) {
    net::io_context ioContext{1};
    net::thread_pool sessionPool{MAX_CONCURRENT_SESSIONS};
    std::counting_semaphore<MAX_CONCURRENT_SESSIONS> sessionSlots{MAX_CONCURRENT_SESSIONS};
    beast::error_code error;

    const auto address = net::ip::make_address(config.address, error);
    if (error) {
        logServerError("parse-address", error);
        return 1;
    }

    tcp::acceptor acceptor{ioContext};
    const tcp::endpoint endpoint{address, config.port};

    acceptor.open(endpoint.protocol(), error);
    if (error) {
        logServerError("open", error);
        return 1;
    }

    acceptor.set_option(net::socket_base::reuse_address(true), error);
    if (error) {
        logServerError("set-option", error);
        return 1;
    }

    acceptor.bind(endpoint, error);
    if (error) {
        logServerError("bind", error);
        return 1;
    }

    acceptor.listen(net::socket_base::max_listen_connections, error);
    if (error) {
        logServerError("listen", error);
        return 1;
    }

    for (;;) {
        sessionSlots.acquire();
        tcp::socket socket{ioContext};
        acceptor.accept(socket, error);
        if (error) {
            sessionSlots.release();
            logServerError("accept", error);
            continue;
        }

        net::post(sessionPool, [socket = std::move(socket), &service, &sessionSlots]() mutable {
            try {
                doSession(std::move(socket), service);
            } catch (const std::exception& ex) {
                std::cerr << "server session failed: " << ex.what() << "\n";
            }
            sessionSlots.release();
        });
    }
}

} // namespace keyrecord
