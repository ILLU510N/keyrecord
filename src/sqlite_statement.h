#pragma once

struct sqlite3_stmt;

namespace keyrecord {

// sqlite3_stmt 的独占 RAII 包装，确保查询的所有返回路径都会释放 statement。
class SqliteStatement {
public:
    SqliteStatement() = default;
    SqliteStatement(const SqliteStatement&) = delete;
    SqliteStatement& operator=(const SqliteStatement&) = delete;
    SqliteStatement(SqliteStatement&& other) noexcept;
    SqliteStatement& operator=(SqliteStatement&& other) noexcept;
    ~SqliteStatement();

    sqlite3_stmt* get() const;
    sqlite3_stmt** out();
    void reset();

private:
    sqlite3_stmt* statement_ = nullptr;
};

} // namespace keyrecord
