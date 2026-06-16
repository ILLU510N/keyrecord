#include "api_queries.h"

#include "keyboard_layout.h"

#include <sqlite3.h>

#include <iomanip>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>

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

} // namespace keyrecord
