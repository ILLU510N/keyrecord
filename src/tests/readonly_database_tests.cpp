#include "../api_queries.h"
#include "../readonly_database.h"

#include <filesystem>
#include <iostream>
#include <sqlite3.h>
#include <string>

namespace {

bool expect(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << "\n";
        return false;
    }
    return true;
}

bool expectEqual(const std::string& actual, const std::string& expected, const char* message) {
    if (actual != expected) {
        std::cerr << message << "\nExpected: " << expected << "\nActual: " << actual << "\n";
        return false;
    }
    return true;
}

bool execSql(sqlite3* database, const char* sql) {
    char* error = nullptr;
    const int rc = sqlite3_exec(database, sql, nullptr, nullptr, &error);
    if (rc != SQLITE_OK) {
        sqlite3_free(error);
        return false;
    }
    return true;
}

bool createWritableFixture(const std::filesystem::path& dbPath) {
    sqlite3* database = nullptr;
    if (sqlite3_open(dbPath.string().c_str(), &database) != SQLITE_OK) {
        return false;
    }

    const bool ok = execSql(database,
                            "CREATE TABLE keys("
                            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                            "timestamp INTEGER,"
                            "date TEXT,"
                            "hour INTEGER,"
                            "vk_code INTEGER,"
                            "key_name TEXT);"
                            "INSERT INTO keys(timestamp,date,hour,vk_code,key_name) "
                            "VALUES(1767225600,'2026-01-01',0,65,'A');");
    sqlite3_close(database);
    return ok;
}

} // namespace

int main() {
    const auto dbPath = std::filesystem::temp_directory_path() / "keyrecord_readonly_test.db";
    const auto missingPath = std::filesystem::temp_directory_path() / "keyrecord_missing_readonly_test.db";
    std::filesystem::remove(dbPath);
    std::filesystem::remove(missingPath);

    bool ok = expect(createWritableFixture(dbPath), "Failed to create test database");

    std::string errorMessage;
    {
        auto database = keyrecord::ReadOnlyDatabase::open(dbPath.string(), &errorMessage);
        ok = expect(database.has_value(), "Failed to open existing database in read-only mode") && ok;
        ok = expect(errorMessage.empty(), "Error message should be empty after a successful open") && ok;

        if (database) {
            const auto info = keyrecord::queryInfo(database->get());
            ok = expectEqual(info.body,
                             "{\"total_keys\":1,\"first_date\":\"2026-01-01\",\"last_date\":\"2026-01-01\",\"unique_keys\":1}",
                             "Unexpected read-only query result") &&
                 ok;
            ok = expect(!execSql(database->get(),
                                 "INSERT INTO keys(timestamp,date,hour,vk_code,key_name) "
                                 "VALUES(1767225601,'2026-01-01',0,66,'B');"),
                        "Read-only connection should not allow writes") &&
                 ok;
        }
    }

    errorMessage.clear();
    auto missing = keyrecord::ReadOnlyDatabase::open(missingPath.string(), &errorMessage);
    ok = expect(!missing.has_value(), "Opening a missing database in read-only mode should fail") && ok;
    ok = expect(!errorMessage.empty(), "Opening a missing database should return an error message") && ok;
    ok = expect(!std::filesystem::exists(missingPath), "Opening a missing database in read-only mode should not create a file") && ok;

    std::filesystem::remove(dbPath);
    return ok ? 0 : 1;
}
