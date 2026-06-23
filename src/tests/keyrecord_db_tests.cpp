#include "../key_event_writer.h"
#include "../key_names.h"

#include <filesystem>
#include <iostream>
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

} // namespace

int main() {
    const auto dbPath = std::filesystem::temp_directory_path() / "keyrecord_writer_test.db";
    std::filesystem::remove(dbPath);
    std::filesystem::remove(dbPath.string() + "-wal");
    std::filesystem::remove(dbPath.string() + "-shm");

    initKeyMap();
    if (!startWriter(dbPath.string())) {
        std::cerr << "Failed to start writer\n";
        return 1;
    }

    const auto eventTime = std::chrono::system_clock::from_time_t(1767139200);
    enqueueKeyEvent(VK_OEM_7, eventTime);
    stopWriter();

    sqlite3* database = nullptr;
    if (sqlite3_open(dbPath.string().c_str(), &database) != SQLITE_OK) {
        std::cerr << "Failed to open test database\n";
        return 1;
    }

    bool ok = true;
    ok = ok && journalModeIsWal(database);
    ok = ok && hasIndex(database, "idx_keys_date");
    ok = ok && hasIndex(database, "idx_keys_date_vk_name");
    ok = ok && hasIndex(database, "idx_keys_key_name_vk_code");
    ok = ok && singleQuoteKeyWasInserted(database);

    sqlite3_close(database);
    std::filesystem::remove(dbPath);
    std::filesystem::remove(dbPath.string() + "-wal");
    std::filesystem::remove(dbPath.string() + "-shm");

    if (!ok) {
        return 1;
    }
    return 0;
}
