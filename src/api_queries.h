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
ApiResponse queryHourlyStats(sqlite3* database, std::string_view date);
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

// 小时级热力图：返回 [{weekday, hour, count}]，weekday 0=周日。
ApiResponse queryHourlyHeatmap(
    sqlite3* database,
    std::optional<std::string_view> startDate,
    std::optional<std::string_view> endDate);

// 按键时序：返回某日按 id 顺序的事件 [{ts, vk_code, key_name}]，用于动画回放。
ApiResponse queryTimeline(sqlite3* database, std::string_view date, int limit);

// 多键组合统计：扫描有序事件，统计 Ctrl+C 等修饰键组合，返回 [{combo, count}]。
ApiResponse queryCombos(
    sqlite3* database,
    std::optional<std::string_view> date,
    std::optional<std::string_view> startDate,
    std::optional<std::string_view> endDate,
    int limit);

// 键盘分区统计：返回 [{region, count}]。
ApiResponse queryRegionStats(
    sqlite3* database,
    std::optional<std::string_view> startDate,
    std::optional<std::string_view> endDate);

// 左右手使用频率对比：返回 [{hand, count}]。
ApiResponse queryHandStats(
    sqlite3* database,
    std::optional<std::string_view> startDate,
    std::optional<std::string_view> endDate);

// 输入速度曲线：返回某日按分钟桶聚合的击键数 [{minute, count}]。
ApiResponse querySpeed(sqlite3* database, std::string_view date, int bucketMinutes);

} // namespace keyrecord
