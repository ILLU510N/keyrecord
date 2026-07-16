#include "../key_event_writer.h"
#include "../key_names.h"
#include "../platform/virtual_keys.h"

#include <filesystem>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <sqlite3.h>

namespace {

bool hasIndex(sqlite3* database, const char* indexName) {
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT 1 FROM sqlite_master WHERE type = 'index' AND name = ? LIMIT 1";
    if (sqlite3_prepare_v2(database, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare index lookup: " << sqlite3_errmsg(database) << "\n";
        return false;
    }
    sqlite3_bind_text(stmt, 1, indexName, -1, SQLITE_TRANSIENT);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_ROW;
}

bool hasTable(sqlite3* database, const char* tableName) {
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT 1 FROM sqlite_master WHERE type = 'table' AND name = ? LIMIT 1";
    if (sqlite3_prepare_v2(database, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare table lookup: " << sqlite3_errmsg(database) << "\n";
        return false;
    }
    sqlite3_bind_text(stmt, 1, tableName, -1, SQLITE_TRANSIENT);
    const int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_ROW;
}

bool hasMetaValue(sqlite3* database, const char* name, const char* value) {
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(database,
                           "SELECT value FROM keyrecord_meta WHERE name = ?",
                           -1,
                           &stmt,
                           nullptr) != SQLITE_OK) {
        return false;
    }
    sqlite3_bind_text(stmt, 1, name, -1, SQLITE_TRANSIENT);
    const unsigned char* actual = nullptr;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        actual = sqlite3_column_text(stmt, 0);
    }
    const bool ok = actual && std::string(reinterpret_cast<const char*>(actual)) == value;
    sqlite3_finalize(stmt);
    return ok;
}

bool singleQuoteKeyWasInserted(sqlite3* database) {
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT COUNT(*) FROM keys WHERE key_name = ?";
    if (sqlite3_prepare_v2(database, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare key lookup: " << sqlite3_errmsg(database) << "\n";
        return false;
    }
    sqlite3_bind_text(stmt, 1, "'", -1, SQLITE_TRANSIENT);
    int count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return count == 1;
}

bool legacySummaryWasBackfilled(sqlite3* database) {
    sqlite3_stmt* stmt = nullptr;
    const char* sql =
        "SELECT count FROM daily_key_stats WHERE date = '2026-01-02' AND vk_code = 65 AND key_name = 'A'";
    if (sqlite3_prepare_v2(database, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare legacy summary lookup: " << sqlite3_errmsg(database) << "\n";
        return false;
    }
    const bool ok = sqlite3_step(stmt) == SQLITE_ROW && sqlite3_column_int(stmt, 0) == 1;
    sqlite3_finalize(stmt);
    return ok;
}

bool summaryMatchesInsertedSingleQuote(sqlite3* database) {
    sqlite3_stmt* stmt = nullptr;
    const char* keySql = "SELECT date, hour, vk_code, key_name FROM keys WHERE key_name = ? LIMIT 1";
    if (sqlite3_prepare_v2(database, keySql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare inserted key lookup: " << sqlite3_errmsg(database) << "\n";
        return false;
    }
    sqlite3_bind_text(stmt, 1, "'", -1, SQLITE_TRANSIENT);

    std::string date;
    int hour = -1;
    int vkCode = -1;
    std::string keyName;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const unsigned char* dateText = sqlite3_column_text(stmt, 0);
        const unsigned char* keyText = sqlite3_column_text(stmt, 3);
        date = dateText ? reinterpret_cast<const char*>(dateText) : "";
        hour = sqlite3_column_int(stmt, 1);
        vkCode = sqlite3_column_int(stmt, 2);
        keyName = keyText ? reinterpret_cast<const char*>(keyText) : "";
    }
    sqlite3_finalize(stmt);

    if (date.empty() || keyName.empty()) {
        return false;
    }

    const char* keySummarySql =
        "SELECT count FROM daily_key_stats WHERE date = ? AND vk_code = ? AND key_name = ?";
    if (sqlite3_prepare_v2(database, keySummarySql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare daily key summary lookup: " << sqlite3_errmsg(database) << "\n";
        return false;
    }
    sqlite3_bind_text(stmt, 1, date.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, vkCode);
    sqlite3_bind_text(stmt, 3, keyName.c_str(), -1, SQLITE_TRANSIENT);
    bool ok = sqlite3_step(stmt) == SQLITE_ROW && sqlite3_column_int(stmt, 0) == 1;
    sqlite3_finalize(stmt);

    const char* hourSummarySql = "SELECT count FROM daily_hour_stats WHERE date = ? AND hour = ?";
    if (sqlite3_prepare_v2(database, hourSummarySql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare daily hour summary lookup: " << sqlite3_errmsg(database) << "\n";
        return false;
    }
    sqlite3_bind_text(stmt, 1, date.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, hour);
    ok = (sqlite3_step(stmt) == SQLITE_ROW && sqlite3_column_int(stmt, 0) == 1) && ok;
    sqlite3_finalize(stmt);

    return ok;
}

bool journalModeIsWal(sqlite3* database) {
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(database, "PRAGMA journal_mode", -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare journal_mode query: " << sqlite3_errmsg(database) << "\n";
        return false;
    }
    bool isWal = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const unsigned char* value = sqlite3_column_text(stmt, 0);
        isWal = value && std::string(reinterpret_cast<const char*>(value)) == "wal";
    }
    sqlite3_finalize(stmt);
    return isWal;
}

bool createLegacyKeysOnlyDatabase(const std::filesystem::path& dbPath) {
    sqlite3* database = nullptr;
    if (sqlite3_open(dbPath.string().c_str(), &database) != SQLITE_OK) {
        return false;
    }

    const bool ok = sqlite3_exec(
                        database,
                        "CREATE TABLE keys("
                        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                        "timestamp INTEGER,"
                        "date TEXT,"
                        "hour INTEGER,"
                        "vk_code INTEGER,"
                        "key_name TEXT);"
                        "INSERT INTO keys(timestamp,date,hour,vk_code,key_name) VALUES"
                        "(1767312000,'2026-01-02',5,65,'A');",
                        nullptr,
                        nullptr,
                        nullptr) == SQLITE_OK;
    sqlite3_close(database);
    return ok;
}

bool writerFailurePreservesBatch(const std::filesystem::path& dbPath) {
    std::filesystem::remove(dbPath);
    std::mutex failureMutex;
    std::condition_variable failureCv;
    bool failureObserved = false;

    if (!keyrecord::startWriter(dbPath.string(), [&] {
            std::lock_guard<std::mutex> lock(failureMutex);
            failureObserved = true;
            failureCv.notify_one();
        })) {
        return false;
    }

    sqlite3* blocker = nullptr;
    if (sqlite3_open(dbPath.string().c_str(), &blocker) != SQLITE_OK ||
        sqlite3_exec(blocker, "BEGIN EXCLUSIVE;", nullptr, nullptr, nullptr) != SQLITE_OK) {
        if (blocker) {
            sqlite3_close(blocker);
        }
        keyrecord::stopWriter();
        return false;
    }

    const auto eventTime = std::chrono::system_clock::from_time_t(1767139200);
    const bool accepted = keyrecord::enqueueKeyEvent(keyrecord::vk::Oem7, eventTime);
    {
        std::unique_lock<std::mutex> lock(failureMutex);
        failureCv.wait_for(lock, std::chrono::seconds(12), [&] { return failureObserved; });
    }

    sqlite3_exec(blocker, "ROLLBACK;", nullptr, nullptr, nullptr);
    sqlite3_close(blocker);
    keyrecord::stopWriter();

    if (!accepted || !failureObserved || keyrecord::getWriterState() != keyrecord::WriterState::Failed) {
        return false;
    }

    if (!keyrecord::startWriter(dbPath.string())) {
        return false;
    }
    keyrecord::stopWriter();

    sqlite3* database = nullptr;
    if (sqlite3_open(dbPath.string().c_str(), &database) != SQLITE_OK) {
        return false;
    }
    const bool inserted = singleQuoteKeyWasInserted(database);
    sqlite3_close(database);
    std::filesystem::remove(dbPath);
    std::filesystem::remove(dbPath.string() + "-wal");
    std::filesystem::remove(dbPath.string() + "-shm");
    return inserted;
}

} // namespace

int main() {
    const auto dbPath = std::filesystem::temp_directory_path() / "keyrecord_writer_test.db";
    std::filesystem::remove(dbPath);
    std::filesystem::remove(dbPath.string() + "-wal");
    std::filesystem::remove(dbPath.string() + "-shm");

    if (!createLegacyKeysOnlyDatabase(dbPath)) {
        std::cerr << "Failed to create legacy test database\n";
        return 1;
    }

    if (!keyrecord::startWriter(dbPath.string())) {
        std::cerr << "Failed to start writer\n";
        return 1;
    }

    const auto eventTime = std::chrono::system_clock::from_time_t(1767139200);
    keyrecord::enqueueKeyEvent(keyrecord::vk::Oem7, eventTime);
    keyrecord::stopWriter();
    bool ok = keyrecord::getWriterState() == keyrecord::WriterState::Stopped;
    ok = !keyrecord::enqueueKeyEvent(keyrecord::vk::Oem7, eventTime) && ok;

    sqlite3* database = nullptr;
    if (sqlite3_open(dbPath.string().c_str(), &database) != SQLITE_OK) {
        std::cerr << "Failed to open test database\n";
        return 1;
    }

    ok = ok && journalModeIsWal(database);
    ok = ok && hasIndex(database, "idx_keys_date");
    ok = ok && hasIndex(database, "idx_keys_date_vk_name");
    ok = ok && hasIndex(database, "idx_keys_key_name_vk_code");
    ok = ok && hasTable(database, "daily_key_stats");
    ok = ok && hasTable(database, "daily_hour_stats");
    ok = ok && hasTable(database, "keyrecord_meta");
    ok = ok && hasMetaValue(database, "summary_backfill_v1", "complete");
    ok = ok && singleQuoteKeyWasInserted(database);
    ok = ok && legacySummaryWasBackfilled(database);
    ok = ok && summaryMatchesInsertedSingleQuote(database);

    sqlite3_close(database);
    std::filesystem::remove(dbPath);
    std::filesystem::remove(dbPath.string() + "-wal");
    std::filesystem::remove(dbPath.string() + "-shm");

    const auto failureDbPath = std::filesystem::temp_directory_path() / "keyrecord_writer_failure_test.db";
    ok = writerFailurePreservesBatch(failureDbPath) && ok;

    if (!ok) {
        return 1;
    }
    return 0;
}
