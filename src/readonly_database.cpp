#include "readonly_database.h"

#include <sqlite3.h>

namespace keyrecord {

ReadOnlyDatabase::ReadOnlyDatabase(sqlite3* database)
    : database_(database) {
}

std::optional<ReadOnlyDatabase> ReadOnlyDatabase::open(const std::string& dbPath, std::string* errorMessage) {
    if (errorMessage) {
        errorMessage->clear();
    }

    sqlite3* database = nullptr;
    const int rc = sqlite3_open_v2(dbPath.c_str(), &database, SQLITE_OPEN_READONLY, nullptr);
    if (rc != SQLITE_OK) {
        if (errorMessage) {
            *errorMessage = database ? sqlite3_errmsg(database) : "unknown sqlite open error";
        }
        if (database) {
            sqlite3_close(database);
        }
        return std::nullopt;
    }

    return ReadOnlyDatabase(database);
}

ReadOnlyDatabase::ReadOnlyDatabase(ReadOnlyDatabase&& other) noexcept
    : database_(other.database_) {
    other.database_ = nullptr;
}

ReadOnlyDatabase& ReadOnlyDatabase::operator=(ReadOnlyDatabase&& other) noexcept {
    if (this != &other) {
        close();
        database_ = other.database_;
        other.database_ = nullptr;
    }
    return *this;
}

ReadOnlyDatabase::~ReadOnlyDatabase() {
    close();
}

sqlite3* ReadOnlyDatabase::get() const {
    return database_;
}

void ReadOnlyDatabase::close() {
    if (database_) {
        sqlite3_close(database_);
        database_ = nullptr;
    }
}

} // namespace keyrecord
