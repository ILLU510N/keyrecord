#include "http_router.h"

#include "api_queries.h"
#include "embedded_resources.h"

#include <charconv>
#include <map>
#include <optional>
#include <string>

namespace keyrecord {
namespace {

struct ParsedTarget {
    std::string path;
    std::map<std::string, std::string> query;
};

std::string percentDecode(std::string_view value) {
    std::string decoded;
    decoded.reserve(value.size());

    for (size_t i = 0; i < value.size(); ++i) {
        if (value[i] == '+') {
            decoded.push_back(' ');
            continue;
        }

        if (value[i] == '%' && i + 2 < value.size()) {
            int high = 0;
            int low = 0;
            const auto highResult = std::from_chars(value.data() + i + 1, value.data() + i + 2, high, 16);
            const auto lowResult = std::from_chars(value.data() + i + 2, value.data() + i + 3, low, 16);
            if (highResult.ec == std::errc() && lowResult.ec == std::errc()) {
                decoded.push_back(static_cast<char>(high * 16 + low));
                i += 2;
                continue;
            }
        }

        decoded.push_back(value[i]);
    }

    return decoded;
}

ParsedTarget parseTarget(std::string_view target) {
    ParsedTarget parsed;
    const auto queryStart = target.find('?');
    parsed.path = std::string(target.substr(0, queryStart));

    if (parsed.path.empty()) {
        parsed.path = "/";
    }

    if (queryStart == std::string_view::npos) {
        return parsed;
    }

    std::string_view query = target.substr(queryStart + 1);
    while (!query.empty()) {
        const auto amp = query.find('&');
        const std::string_view pair = query.substr(0, amp);
        const auto equals = pair.find('=');

        if (equals == std::string_view::npos) {
            parsed.query.emplace(percentDecode(pair), "");
        } else {
            parsed.query.emplace(percentDecode(pair.substr(0, equals)), percentDecode(pair.substr(equals + 1)));
        }

        if (amp == std::string_view::npos) {
            break;
        }
        query = query.substr(amp + 1);
    }

    return parsed;
}

std::optional<std::string_view> findQuery(const ParsedTarget& target, const std::string& name) {
    const auto it = target.query.find(name);
    if (it == target.query.end()) {
        return std::nullopt;
    }
    return std::string_view(it->second);
}

int parseLimit(const ParsedTarget& target) {
    const auto value = findQuery(target, "limit");
    if (!value) {
        return 20;
    }

    int limit = 20;
    const auto result = std::from_chars(value->data(), value->data() + value->size(), limit);
    if (result.ec != std::errc() || limit <= 0) {
        return 20;
    }
    return limit;
}

HttpResponse fromApiResponse(ApiResponse response) {
    return HttpResponse{
        response.status,
        std::move(response.contentType),
        std::move(response.body),
        {
            {"Access-Control-Allow-Origin", "*"},
            {"Access-Control-Allow-Headers", "Content-Type"},
        },
    };
}

HttpResponse jsonResponse(int status, std::string body) {
    return HttpResponse{
        status,
        "application/json; charset=utf-8",
        std::move(body),
        {
            {"Access-Control-Allow-Origin", "*"},
            {"Access-Control-Allow-Headers", "Content-Type"},
        },
    };
}

HttpResponse methodNotAllowed() {
    auto response = jsonResponse(405, "{\"error\":\"method not allowed\"}");
    response.headers.push_back({"Allow", "GET, OPTIONS"});
    return response;
}

HttpResponse corsPreflight() {
    return HttpResponse{
        204,
        "application/json; charset=utf-8",
        "",
        {
            {"Access-Control-Allow-Origin", "*"},
            {"Access-Control-Allow-Headers", "Content-Type"},
            {"Access-Control-Allow-Methods", "GET, OPTIONS"},
        },
    };
}

HttpResponse staticResponse(const EmbeddedResource& resource) {
    return HttpResponse{
        200,
        std::string(resource.contentType),
        std::string(reinterpret_cast<const char*>(resource.data.data()), resource.data.size()),
        {
            {"Access-Control-Allow-Origin", "*"},
            {"Access-Control-Allow-Headers", "Content-Type"},
        },
    };
}

} // namespace

HttpResponse handleHttpRequest(std::string_view method, std::string_view target, sqlite3* database) {
    if (method == "OPTIONS") {
        return corsPreflight();
    }

    if (method != "GET") {
        return methodNotAllowed();
    }

    const ParsedTarget parsed = parseTarget(target);

    if (parsed.path == "/api/info") {
        return fromApiResponse(queryInfo(database));
    }
    if (parsed.path == "/api/daily-stats") {
        return fromApiResponse(queryDailyStats(database));
    }
    if (parsed.path == "/api/keys") {
        return fromApiResponse(queryKeys(database, findQuery(parsed, "start"), findQuery(parsed, "end"), parseLimit(parsed)));
    }
    if (parsed.path == "/api/heatmap") {
        return fromApiResponse(
            queryHeatmap(database, findQuery(parsed, "date"), findQuery(parsed, "start"), findQuery(parsed, "end")));
    }

    if (parsed.path.rfind("/api/", 0) == 0) {
        return jsonResponse(404, "{\"error\":\"not found\"}");
    }

    const auto resource = findEmbeddedResource(target);
    if (resource) {
        return staticResponse(*resource);
    }

    return jsonResponse(404, "{\"error\":\"not found\"}");
}

} // namespace keyrecord
