#include "api_queries.h"

#include "key_classification.h"
#include "keyboard_layout.h"

#include <sqlite3.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <iomanip>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace keyrecord {
namespace {

ApiResponse jsonOk(std::string body) {
    return ApiResponse{200, "application/json; charset=utf-8", std::move(body)};
}

ApiResponse jsonError(sqlite3* database) {
    std::string message = database ? sqlite3_errmsg(database) : "database is null";
    std::string body = "{\"error\":\"";
    body.reserve(message.size() + 12);
    for (const char ch : message) {
        if (ch == '"' || ch == '\\') {
            body.push_back('\\');
        }
        body.push_back(ch);
    }
    body += "\"}";
    return ApiResponse{500, "application/json; charset=utf-8", std::move(body)};
}

void appendJsonString(std::string& output, std::string_view value) {
    output.push_back('"');
    for (const unsigned char ch : value) {
        switch (ch) {
            case '"':
                output += "\\\"";
                break;
            case '\\':
                output += "\\\\";
                break;
            case '\b':
                output += "\\b";
                break;
            case '\f':
                output += "\\f";
                break;
            case '\n':
                output += "\\n";
                break;
            case '\r':
                output += "\\r";
                break;
            case '\t':
                output += "\\t";
                break;
            default:
                if (ch < 0x20) {
                    std::ostringstream escaped;
                    escaped << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(ch);
                    output += escaped.str();
                } else {
                    output.push_back(static_cast<char>(ch));
                }
                break;
        }
    }
    output.push_back('"');
}

void appendJsonTextOrNull(std::string& output, sqlite3_stmt* stmt, int column) {
    const unsigned char* value = sqlite3_column_text(stmt, column);
    if (!value) {
        output += "null";
        return;
    }
    appendJsonString(output, reinterpret_cast<const char*>(value));
}

std::string columnText(sqlite3_stmt* stmt, int column) {
    const unsigned char* value = sqlite3_column_text(stmt, column);
    return value ? reinterpret_cast<const char*>(value) : "";
}

void appendNumber(std::string& output, double value) {
    std::ostringstream stream;
    stream << value;
    output += stream.str();
}

bool prepare(sqlite3* database, const char* sql, sqlite3_stmt** stmt) {
    return database && sqlite3_prepare_v2(database, sql, -1, stmt, nullptr) == SQLITE_OK;
}

bool bindText(sqlite3_stmt* stmt, int index, std::string_view value) {
    return sqlite3_bind_text(stmt, index, value.data(), static_cast<int>(value.size()), SQLITE_TRANSIENT) == SQLITE_OK;
}

ApiResponse queryKeysWithoutRange(sqlite3* database, int limit) {
    sqlite3_stmt* stmt = nullptr;
    const char* sql =
        "SELECT key_name, vk_code, COUNT(*) as count FROM keys "
        "GROUP BY key_name, vk_code ORDER BY count DESC LIMIT ?";

    if (!prepare(database, sql, &stmt)) {
        return jsonError(database);
    }
    sqlite3_bind_int(stmt, 1, limit);

    std::string body = "[";
    bool first = true;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        if (!first) {
            body.push_back(',');
        }
        first = false;
        body += "{\"key_name\":";
        appendJsonString(body, columnText(stmt, 0));
        body += ",\"vk_code\":" + std::to_string(sqlite3_column_int(stmt, 1));
        body += ",\"count\":" + std::to_string(sqlite3_column_int64(stmt, 2));
        body.push_back('}');
    }
    body.push_back(']');

    sqlite3_finalize(stmt);
    return jsonOk(std::move(body));
}

ApiResponse queryKeysWithRange(sqlite3* database, std::string_view startDate, std::string_view endDate, int limit) {
    sqlite3_stmt* stmt = nullptr;
    const char* sql =
        "SELECT key_name, vk_code, COUNT(*) as count FROM keys "
        "WHERE date BETWEEN ? AND ? "
        "GROUP BY key_name, vk_code ORDER BY count DESC LIMIT ?";

    if (!prepare(database, sql, &stmt)) {
        return jsonError(database);
    }
    if (!bindText(stmt, 1, startDate) || !bindText(stmt, 2, endDate) ||
        sqlite3_bind_int(stmt, 3, limit) != SQLITE_OK) {
        sqlite3_finalize(stmt);
        return jsonError(database);
    }

    std::string body = "[";
    bool first = true;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        if (!first) {
            body.push_back(',');
        }
        first = false;
        body += "{\"key_name\":";
        appendJsonString(body, columnText(stmt, 0));
        body += ",\"vk_code\":" + std::to_string(sqlite3_column_int(stmt, 1));
        body += ",\"count\":" + std::to_string(sqlite3_column_int64(stmt, 2));
        body.push_back('}');
    }
    body.push_back(']');

    sqlite3_finalize(stmt);
    return jsonOk(std::move(body));
}

std::string heatmapRowsToJson(sqlite3_stmt* stmt) {
    std::string body = "[";
    bool first = true;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const int vkCode = sqlite3_column_int(stmt, 0);
        const auto position = getKeyPosition(vkCode);
        if (!position) {
            continue;
        }

        if (!first) {
            body.push_back(',');
        }
        first = false;
        body += "{\"vk_code\":" + std::to_string(vkCode);
        body += ",\"key_name\":";
        appendJsonString(body, columnText(stmt, 1));
        body += ",\"count\":" + std::to_string(sqlite3_column_int64(stmt, 2));
        body += ",\"x\":";
        appendNumber(body, position->x);
        body += ",\"y\":";
        appendNumber(body, position->y);
        body.push_back('}');
    }
    body.push_back(']');
    return body;
}

ApiResponse queryHeatmapRows(sqlite3* database, const char* sql) {
    sqlite3_stmt* stmt = nullptr;
    if (!prepare(database, sql, &stmt)) {
        return jsonError(database);
    }

    std::string body = heatmapRowsToJson(stmt);
    sqlite3_finalize(stmt);
    return jsonOk(std::move(body));
}

ApiResponse queryHeatmapWithDate(sqlite3* database, std::string_view date) {
    sqlite3_stmt* stmt = nullptr;
    const char* sql =
        "SELECT vk_code, key_name, COUNT(*) as count FROM keys "
        "WHERE date = ? GROUP BY vk_code, key_name ORDER BY count DESC";

    if (!prepare(database, sql, &stmt)) {
        return jsonError(database);
    }
    if (!bindText(stmt, 1, date)) {
        sqlite3_finalize(stmt);
        return jsonError(database);
    }

    std::string body = heatmapRowsToJson(stmt);
    sqlite3_finalize(stmt);
    return jsonOk(std::move(body));
}

ApiResponse queryHeatmapWithRange(sqlite3* database, std::string_view startDate, std::string_view endDate) {
    sqlite3_stmt* stmt = nullptr;
    const char* sql =
        "SELECT vk_code, key_name, COUNT(*) as count FROM keys "
        "WHERE date BETWEEN ? AND ? GROUP BY vk_code, key_name ORDER BY count DESC";

    if (!prepare(database, sql, &stmt)) {
        return jsonError(database);
    }
    if (!bindText(stmt, 1, startDate) || !bindText(stmt, 2, endDate)) {
        sqlite3_finalize(stmt);
        return jsonError(database);
    }

    std::string body = heatmapRowsToJson(stmt);
    sqlite3_finalize(stmt);
    return jsonOk(std::move(body));
}

// 可选的日期过滤：优先使用 start/end 范围，否则单日，否则全量。
struct RangeFilter {
    std::optional<std::string_view> date;
    std::optional<std::string_view> startDate;
    std::optional<std::string_view> endDate;
};

std::string rangeWhereClause(const RangeFilter& filter) {
    if (filter.startDate && filter.endDate) {
        return " WHERE date BETWEEN ? AND ?";
    }
    if (filter.date) {
        return " WHERE date = ?";
    }
    return "";
}

bool bindRange(sqlite3_stmt* stmt, const RangeFilter& filter, int startIndex) {
    if (filter.startDate && filter.endDate) {
        return bindText(stmt, startIndex, *filter.startDate) && bindText(stmt, startIndex + 1, *filter.endDate);
    }
    if (filter.date) {
        return bindText(stmt, startIndex, *filter.date);
    }
    return true;
}

// 拉取 (vk_code, count) 聚合，供分区与左右手统计在 C++ 侧归类。
std::optional<std::vector<std::pair<int, std::int64_t>>> fetchKeyCounts(sqlite3* database, const RangeFilter& filter) {
    const std::string sql = "SELECT vk_code, COUNT(*) FROM keys" + rangeWhereClause(filter) + " GROUP BY vk_code";
    sqlite3_stmt* stmt = nullptr;
    if (!prepare(database, sql.c_str(), &stmt) || !bindRange(stmt, filter, 1)) {
        sqlite3_finalize(stmt);
        return std::nullopt;
    }

    std::vector<std::pair<int, std::int64_t>> counts;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        counts.emplace_back(sqlite3_column_int(stmt, 0), sqlite3_column_int64(stmt, 1));
    }
    sqlite3_finalize(stmt);
    return counts;
}

std::string bucketCountsToJson(const std::string& key, const std::vector<std::pair<std::string_view, std::int64_t>>& rows) {
    std::string body = "[";
    bool first = true;
    for (const auto& [name, count] : rows) {
        if (!first) {
            body.push_back(',');
        }
        first = false;
        body += "{\"" + key + "\":";
        appendJsonString(body, name);
        body += ",\"count\":" + std::to_string(count);
        body.push_back('}');
    }
    body.push_back(']');
    return body;
}

bool isModifierVk(int vkCode) {
    return vkCode == 16 || vkCode == 17 || vkCode == 18 || vkCode == 91 || vkCode == 92 ||
           (vkCode >= 160 && vkCode <= 165);
}

// 将修饰键虚拟码归一为组合名称前缀。
std::optional<std::string_view> modifierLabel(int vkCode) {
    switch (vkCode) {
        case 16:
        case 160:
        case 161:
            return "Shift";
        case 17:
        case 162:
        case 163:
            return "Ctrl";
        case 18:
        case 164:
        case 165:
            return "Alt";
        case 91:
        case 92:
            return "Win";
        default:
            return std::nullopt;
    }
}

} // namespace

ApiResponse queryInfo(sqlite3* database) {
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT COUNT(*), MIN(date), MAX(date), COUNT(DISTINCT vk_code) FROM keys";
    if (!prepare(database, sql, &stmt)) {
        return jsonError(database);
    }

    std::string body;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        body = "{\"total_keys\":" + std::to_string(sqlite3_column_int64(stmt, 0));
        body += ",\"first_date\":";
        appendJsonTextOrNull(body, stmt, 1);
        body += ",\"last_date\":";
        appendJsonTextOrNull(body, stmt, 2);
        body += ",\"unique_keys\":" + std::to_string(sqlite3_column_int64(stmt, 3));
        body.push_back('}');
    } else {
        sqlite3_finalize(stmt);
        return jsonError(database);
    }

    sqlite3_finalize(stmt);
    return jsonOk(std::move(body));
}

ApiResponse queryDailyStats(sqlite3* database) {
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT date, COUNT(*) as count FROM keys GROUP BY date ORDER BY date";
    if (!prepare(database, sql, &stmt)) {
        return jsonError(database);
    }

    std::string body = "[";
    bool first = true;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        if (!first) {
            body.push_back(',');
        }
        first = false;
        body += "{\"date\":";
        appendJsonString(body, columnText(stmt, 0));
        body += ",\"count\":" + std::to_string(sqlite3_column_int64(stmt, 1));
        body.push_back('}');
    }
    body.push_back(']');

    sqlite3_finalize(stmt);
    return jsonOk(std::move(body));
}

ApiResponse queryHourlyStats(sqlite3* database, std::string_view date) {
    sqlite3_stmt* stmt = nullptr;
    const char* sql =
        "SELECT hour, COUNT(*) as count FROM keys "
        "WHERE date = ? GROUP BY hour ORDER BY hour";

    if (!prepare(database, sql, &stmt)) {
        return jsonError(database);
    }
    if (!bindText(stmt, 1, date)) {
        sqlite3_finalize(stmt);
        return jsonError(database);
    }

    std::string body = "[";
    bool first = true;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        if (!first) {
            body.push_back(',');
        }
        first = false;
        body += "{\"hour\":" + std::to_string(sqlite3_column_int(stmt, 0));
        body += ",\"count\":" + std::to_string(sqlite3_column_int64(stmt, 1));
        body.push_back('}');
    }
    body.push_back(']');

    sqlite3_finalize(stmt);
    return jsonOk(std::move(body));
}

ApiResponse queryKeys(
    sqlite3* database,
    std::optional<std::string_view> startDate,
    std::optional<std::string_view> endDate,
    int limit) {
    if (startDate && endDate) {
        return queryKeysWithRange(database, *startDate, *endDate, limit);
    }
    return queryKeysWithoutRange(database, limit);
}

ApiResponse queryHeatmap(
    sqlite3* database,
    std::optional<std::string_view> date,
    std::optional<std::string_view> startDate,
    std::optional<std::string_view> endDate) {
    if (startDate && endDate) {
        return queryHeatmapWithRange(database, *startDate, *endDate);
    }
    if (date) {
        return queryHeatmapWithDate(database, *date);
    }
    return queryHeatmapRows(
        database,
        "SELECT vk_code, key_name, COUNT(*) as count FROM keys GROUP BY vk_code, key_name ORDER BY count DESC");
}

ApiResponse queryHourlyHeatmap(
    sqlite3* database,
    std::optional<std::string_view> startDate,
    std::optional<std::string_view> endDate) {
    const RangeFilter filter{std::nullopt, startDate, endDate};
    const std::string sql =
        "SELECT CAST(strftime('%w', date) AS INTEGER) AS weekday, hour, COUNT(*) AS count "
        "FROM keys" +
        rangeWhereClause(filter) + " GROUP BY weekday, hour ORDER BY weekday, hour";

    sqlite3_stmt* stmt = nullptr;
    if (!prepare(database, sql.c_str(), &stmt) || !bindRange(stmt, filter, 1)) {
        sqlite3_finalize(stmt);
        return jsonError(database);
    }

    std::string body = "[";
    bool first = true;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        if (!first) {
            body.push_back(',');
        }
        first = false;
        body += "{\"weekday\":" + std::to_string(sqlite3_column_int(stmt, 0));
        body += ",\"hour\":" + std::to_string(sqlite3_column_int(stmt, 1));
        body += ",\"count\":" + std::to_string(sqlite3_column_int64(stmt, 2));
        body.push_back('}');
    }
    body.push_back(']');

    sqlite3_finalize(stmt);
    return jsonOk(std::move(body));
}

ApiResponse queryTimeline(sqlite3* database, std::string_view date, int limit) {
    sqlite3_stmt* stmt = nullptr;
    const char* sql =
        "SELECT timestamp, vk_code, key_name FROM keys WHERE date = ? ORDER BY id LIMIT ?";

    if (!prepare(database, sql, &stmt)) {
        return jsonError(database);
    }
    if (!bindText(stmt, 1, date) || sqlite3_bind_int(stmt, 2, limit) != SQLITE_OK) {
        sqlite3_finalize(stmt);
        return jsonError(database);
    }

    std::string body = "[";
    bool first = true;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        if (!first) {
            body.push_back(',');
        }
        first = false;
        body += "{\"ts\":" + std::to_string(sqlite3_column_int64(stmt, 0));
        body += ",\"vk_code\":" + std::to_string(sqlite3_column_int(stmt, 1));
        body += ",\"key_name\":";
        appendJsonString(body, columnText(stmt, 2));
        body.push_back('}');
    }
    body.push_back(']');

    sqlite3_finalize(stmt);
    return jsonOk(std::move(body));
}

ApiResponse queryCombos(
    sqlite3* database,
    std::optional<std::string_view> date,
    std::optional<std::string_view> startDate,
    std::optional<std::string_view> endDate,
    int limit) {
    const RangeFilter filter{date, startDate, endDate};
    const std::string sql =
        "SELECT timestamp, vk_code, key_name FROM keys" + rangeWhereClause(filter) + " ORDER BY id";

    sqlite3_stmt* stmt = nullptr;
    if (!prepare(database, sql.c_str(), &stmt) || !bindRange(stmt, filter, 1)) {
        sqlite3_finalize(stmt);
        return jsonError(database);
    }

    // 维护"最近一次修饰键"，当 1 秒内紧随非修饰键时记为一次组合。
    std::unordered_map<std::string, std::int64_t> comboCounts;
    bool haveModifier = false;
    std::string_view modifierName;
    std::int64_t modifierTs = 0;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const std::int64_t ts = sqlite3_column_int64(stmt, 0);
        const int vk = sqlite3_column_int(stmt, 1);

        if (isModifierVk(vk)) {
            if (const auto label = modifierLabel(vk)) {
                haveModifier = true;
                modifierName = *label;
                modifierTs = ts;
            }
            continue;
        }

        if (haveModifier && ts - modifierTs <= 1) {
            std::string combo(modifierName);
            combo.push_back('+');
            combo += columnText(stmt, 2);
            ++comboCounts[combo];
        }
    }
    sqlite3_finalize(stmt);

    std::vector<std::pair<std::string, std::int64_t>> ordered(comboCounts.begin(), comboCounts.end());
    std::sort(ordered.begin(), ordered.end(), [](const auto& a, const auto& b) {
        if (a.second != b.second) {
            return a.second > b.second;
        }
        return a.first < b.first;
    });
    if (limit > 0 && static_cast<int>(ordered.size()) > limit) {
        ordered.resize(limit);
    }

    std::string body = "[";
    bool first = true;
    for (const auto& [combo, count] : ordered) {
        if (!first) {
            body.push_back(',');
        }
        first = false;
        body += "{\"combo\":";
        appendJsonString(body, combo);
        body += ",\"count\":" + std::to_string(count);
        body.push_back('}');
    }
    body.push_back(']');
    return jsonOk(std::move(body));
}

ApiResponse queryRegionStats(
    sqlite3* database,
    std::optional<std::string_view> startDate,
    std::optional<std::string_view> endDate) {
    const auto counts = fetchKeyCounts(database, RangeFilter{std::nullopt, startDate, endDate});
    if (!counts) {
        return jsonError(database);
    }

    // 固定输出顺序，方便前端稳定渲染。
    constexpr std::array<KeyRegion, 9> kOrder{
        KeyRegion::Letters, KeyRegion::Digits, KeyRegion::Numpad, KeyRegion::Function,
        KeyRegion::Navigation, KeyRegion::Modifiers, KeyRegion::Control, KeyRegion::Punctuation,
        KeyRegion::Other};

    std::unordered_map<int, std::int64_t> totals;
    for (const auto& [vk, count] : *counts) {
        totals[static_cast<int>(getKeyRegion(vk))] += count;
    }

    std::vector<std::pair<std::string_view, std::int64_t>> rows;
    rows.reserve(kOrder.size());
    for (const KeyRegion region : kOrder) {
        rows.emplace_back(regionName(region), totals[static_cast<int>(region)]);
    }
    return jsonOk(bucketCountsToJson("region", rows));
}

ApiResponse queryHandStats(
    sqlite3* database,
    std::optional<std::string_view> startDate,
    std::optional<std::string_view> endDate) {
    const auto counts = fetchKeyCounts(database, RangeFilter{std::nullopt, startDate, endDate});
    if (!counts) {
        return jsonError(database);
    }

    constexpr std::array<KeyHand, 4> kOrder{KeyHand::Left, KeyHand::Right, KeyHand::Both, KeyHand::Unknown};

    std::unordered_map<int, std::int64_t> totals;
    for (const auto& [vk, count] : *counts) {
        totals[static_cast<int>(getKeyHand(vk))] += count;
    }

    std::vector<std::pair<std::string_view, std::int64_t>> rows;
    rows.reserve(kOrder.size());
    for (const KeyHand hand : kOrder) {
        rows.emplace_back(handName(hand), totals[static_cast<int>(hand)]);
    }
    return jsonOk(bucketCountsToJson("hand", rows));
}

ApiResponse querySpeed(sqlite3* database, std::string_view date, int bucketMinutes) {
    const int bucket = bucketMinutes > 0 ? bucketMinutes : 1;
    sqlite3_stmt* stmt = nullptr;
    // minute-of-day = hour*60 + 分钟分量；整点偏移时区下分钟分量与本地一致，避免依赖运行时区。
    const char* sql =
        "SELECT ((hour * 60 + CAST(strftime('%M', timestamp, 'unixepoch') AS INTEGER)) / ?) * ? AS bucket, "
        "COUNT(*) AS count FROM keys WHERE date = ? GROUP BY bucket ORDER BY bucket";

    if (!prepare(database, sql, &stmt)) {
        return jsonError(database);
    }
    if (sqlite3_bind_int(stmt, 1, bucket) != SQLITE_OK || sqlite3_bind_int(stmt, 2, bucket) != SQLITE_OK ||
        !bindText(stmt, 3, date)) {
        sqlite3_finalize(stmt);
        return jsonError(database);
    }

    std::string body = "[";
    bool first = true;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        if (!first) {
            body.push_back(',');
        }
        first = false;
        body += "{\"minute\":" + std::to_string(sqlite3_column_int(stmt, 0));
        body += ",\"count\":" + std::to_string(sqlite3_column_int64(stmt, 1));
        body.push_back('}');
    }
    body.push_back(']');

    sqlite3_finalize(stmt);
    return jsonOk(std::move(body));
}

} // namespace keyrecord
