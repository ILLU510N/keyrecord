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

constexpr int DEFAULT_KEYS_LIMIT = 20;
constexpr int MAX_KEYS_LIMIT = 100;
constexpr int DEFAULT_COMBOS_LIMIT = 20;
constexpr int MAX_COMBOS_LIMIT = 100;
constexpr int DEFAULT_TIMELINE_LIMIT = 2000;
constexpr int MAX_TIMELINE_LIMIT = 20000;
constexpr int DEFAULT_SPEED_BUCKET = 5;
constexpr int MAX_SPEED_BUCKET = 60;

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

int parseDatePart(std::string_view value, size_t offset, size_t length) {
    int result = 0;
    for (size_t i = 0; i < length; ++i) {
        const char ch = value[offset + i];
        if (ch < '0' || ch > '9') {
            return -1;
        }
        result = result * 10 + (ch - '0');
    }
    return result;
}

bool isLeapYear(int year) {
    return (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
}

int daysInMonth(int year, int month) {
    switch (month) {
        case 2:
            return isLeapYear(year) ? 29 : 28;
        case 4:
        case 6:
        case 9:
        case 11:
            return 30;
        default:
            return 31;
    }
}

bool isValidDateValue(std::string_view value) {
    if (value.size() != 10 || value[4] != '-' || value[7] != '-') {
        return false;
    }

    const int year = parseDatePart(value, 0, 4);
    const int month = parseDatePart(value, 5, 2);
    const int day = parseDatePart(value, 8, 2);

    if (year <= 0 || month < 1 || month > 12 || day < 1) {
        return false;
    }

    return day <= daysInMonth(year, month);
}

int parseBoundedInt(const ParsedTarget& target, const std::string& name, int defaultValue, int maxValue) {
    const auto value = findQuery(target, name);
    if (!value) {
        return defaultValue;
    }

    int parsed = defaultValue;
    const auto result = std::from_chars(value->data(), value->data() + value->size(), parsed);
    if (result.ec != std::errc() || result.ptr != value->data() + value->size() || parsed <= 0) {
        return defaultValue;
    }
    return parsed > maxValue ? maxValue : parsed;
}

int parseLimit(const ParsedTarget& target) {
    return parseBoundedInt(target, "limit", DEFAULT_KEYS_LIMIT, MAX_KEYS_LIMIT);
}

HttpResponse fromApiResponse(ApiResponse response) {
    return HttpResponse{
        response.status,
        std::move(response.contentType),
        std::move(response.body),
        {},
    };
}

HttpResponse jsonResponse(int status, std::string body) {
    return HttpResponse{
        status,
        "application/json; charset=utf-8",
        std::move(body),
        {},
    };
}

HttpResponse badRequest(const char* message) {
    std::string body = "{\"error\":\"";
    body += message;
    body += "\"}";
    return jsonResponse(400, std::move(body));
}

std::optional<HttpResponse> validateDateParameter(const ParsedTarget& target, const std::string& name) {
    const auto value = findQuery(target, name);
    if (!value) {
        return std::nullopt;
    }
    if (!isValidDateValue(*value)) {
        return badRequest("invalid date format");
    }
    return std::nullopt;
}

std::optional<HttpResponse> validateRequiredDateParameter(const ParsedTarget& target, const std::string& name) {
    const auto value = findQuery(target, name);
    if (!value) {
        return badRequest("date is required");
    }
    if (!isValidDateValue(*value)) {
        return badRequest("invalid date format");
    }
    return std::nullopt;
}

std::optional<HttpResponse> validateDateRange(const ParsedTarget& target) {
    const auto startDate = findQuery(target, "start");
    const auto endDate = findQuery(target, "end");

    if (startDate.has_value() != endDate.has_value()) {
        return badRequest("start and end must be provided together");
    }
    if (!startDate) {
        return std::nullopt;
    }
    if (!isValidDateValue(*startDate) || !isValidDateValue(*endDate)) {
        return badRequest("invalid date format");
    }
    if (*startDate > *endDate) {
        return badRequest("start date must not be later than end date");
    }

    return std::nullopt;
}

HttpResponse methodNotAllowed() {
    auto response = jsonResponse(405, "{\"error\":\"method not allowed\"}");
    response.headers.push_back({"Allow", "GET, OPTIONS"});
    return response;
}

HttpResponse optionsResponse() {
    return HttpResponse{
        204,
        "application/json; charset=utf-8",
        "",
        {{"Allow", "GET, OPTIONS"}},
    };
}

HttpResponse staticResponse(const EmbeddedResource& resource) {
    return HttpResponse{
        200,
        std::string(resource.contentType),
        std::string(reinterpret_cast<const char*>(resource.data.data()), resource.data.size()),
        {},
    };
}

} // namespace

HttpResponse handleHttpRequest(std::string_view method, std::string_view target, sqlite3* database) {
    if (method == "OPTIONS") {
        return optionsResponse();
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
    if (parsed.path == "/api/hourly-stats") {
        if (auto error = validateRequiredDateParameter(parsed, "date")) {
            return std::move(*error);
        }
        return fromApiResponse(queryHourlyStats(database, *findQuery(parsed, "date")));
    }
    if (parsed.path == "/api/keys") {
        if (auto error = validateDateRange(parsed)) {
            return std::move(*error);
        }
        return fromApiResponse(queryKeys(database, findQuery(parsed, "start"), findQuery(parsed, "end"), parseLimit(parsed)));
    }
    if (parsed.path == "/api/heatmap") {
        if (auto error = validateDateParameter(parsed, "date")) {
            return std::move(*error);
        }
        if (auto error = validateDateRange(parsed)) {
            return std::move(*error);
        }
        return fromApiResponse(
            queryHeatmap(database, findQuery(parsed, "date"), findQuery(parsed, "start"), findQuery(parsed, "end")));
    }
    if (parsed.path == "/api/hourly-heatmap") {
        if (auto error = validateDateRange(parsed)) {
            return std::move(*error);
        }
        return fromApiResponse(queryHourlyHeatmap(database, findQuery(parsed, "start"), findQuery(parsed, "end")));
    }
    if (parsed.path == "/api/timeline") {
        if (auto error = validateRequiredDateParameter(parsed, "date")) {
            return std::move(*error);
        }
        return fromApiResponse(queryTimeline(
            database, *findQuery(parsed, "date"),
            parseBoundedInt(parsed, "limit", DEFAULT_TIMELINE_LIMIT, MAX_TIMELINE_LIMIT)));
    }
    if (parsed.path == "/api/combos") {
        if (auto error = validateDateParameter(parsed, "date")) {
            return std::move(*error);
        }
        if (auto error = validateDateRange(parsed)) {
            return std::move(*error);
        }
        return fromApiResponse(queryCombos(
            database, findQuery(parsed, "date"), findQuery(parsed, "start"), findQuery(parsed, "end"),
            parseBoundedInt(parsed, "limit", DEFAULT_COMBOS_LIMIT, MAX_COMBOS_LIMIT)));
    }
    if (parsed.path == "/api/region-stats") {
        if (auto error = validateDateRange(parsed)) {
            return std::move(*error);
        }
        return fromApiResponse(queryRegionStats(database, findQuery(parsed, "start"), findQuery(parsed, "end")));
    }
    if (parsed.path == "/api/hand-stats") {
        if (auto error = validateDateRange(parsed)) {
            return std::move(*error);
        }
        return fromApiResponse(queryHandStats(database, findQuery(parsed, "start"), findQuery(parsed, "end")));
    }
    if (parsed.path == "/api/speed") {
        if (auto error = validateRequiredDateParameter(parsed, "date")) {
            return std::move(*error);
        }
        return fromApiResponse(querySpeed(
            database, *findQuery(parsed, "date"),
            parseBoundedInt(parsed, "bucket", DEFAULT_SPEED_BUCKET, MAX_SPEED_BUCKET)));
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
