#include "sqlite_statement.h"

#include <sqlite3.h>

namespace keyrecord {

SqliteStatement::SqliteStatement(SqliteStatement&& other) noexcept
    : statement_(other.statement_) {
    other.statement_ = nullptr;
}

SqliteStatement& SqliteStatement::operator=(SqliteStatement&& other) noexcept {
    if (this != &other) {
        reset();
        statement_ = other.statement_;
        other.statement_ = nullptr;
    }
    return *this;
}

SqliteStatement::~SqliteStatement() {
    reset();
}

sqlite3_stmt* SqliteStatement::get() const {
    return statement_;
}

sqlite3_stmt** SqliteStatement::out() {
    reset();
    return &statement_;
}

void SqliteStatement::reset() {
    if (statement_) {
        sqlite3_finalize(statement_);
        statement_ = nullptr;
    }
}

} // namespace keyrecord
