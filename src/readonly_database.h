#pragma once

#include <optional>
#include <string>

struct sqlite3;

namespace keyrecord {

class ReadOnlyDatabase {
public:
    static std::optional<ReadOnlyDatabase> open(const std::string& dbPath, std::string* errorMessage = nullptr);

    ReadOnlyDatabase(const ReadOnlyDatabase&) = delete;
    ReadOnlyDatabase& operator=(const ReadOnlyDatabase&) = delete;
    ReadOnlyDatabase(ReadOnlyDatabase&& other) noexcept;
    ReadOnlyDatabase& operator=(ReadOnlyDatabase&& other) noexcept;
    ~ReadOnlyDatabase();

    sqlite3* get() const;

private:
    explicit ReadOnlyDatabase(sqlite3* database);
    void close();

    sqlite3* database_ = nullptr;
};

} // namespace keyrecord
