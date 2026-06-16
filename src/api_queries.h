#pragma once

#include <optional>
#include <string>
#include <string_view>

struct sqlite3;

namespace keyrecord {

struct ApiResponse {
    int status;
    std::string contentType;
    std::string body;
};

ApiResponse queryInfo(sqlite3* database);
ApiResponse queryDailyStats(sqlite3* database);
ApiResponse queryKeys(
    sqlite3* database,
    std::optional<std::string_view> startDate,
    std::optional<std::string_view> endDate,
    int limit);
ApiResponse queryHeatmap(
    sqlite3* database,
    std::optional<std::string_view> date,
    std::optional<std::string_view> startDate,
    std::optional<std::string_view> endDate);

} // namespace keyrecord
