#include "server.h"

#include "server_response_adapter.h"

#include <boost/asio/ip/address.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

#include <iostream>
#include <string_view>
#include <utility>

namespace keyrecord {
namespace {

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

void logServerError(std::string_view stage, const beast::error_code& error) {
    std::cerr << "server " << stage << " 失败: " << error.message() << "\n";
}

void doSession(tcp::socket socket, VisualizationService& service) {
    beast::flat_buffer buffer;
    beast::error_code error;

    for (;;) {
        http::request<http::string_body> request;
        http::read(socket, buffer, request, error);
        if (error == http::error::end_of_stream) {
            break;
        }
        if (error) {
            logServerError("read", error);
            return;
        }

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
        tcp::socket socket{ioContext};
        acceptor.accept(socket, error);
        if (error) {
            logServerError("accept", error);
            continue;
        }

        doSession(std::move(socket), service);
    }
}

} // namespace keyrecord
