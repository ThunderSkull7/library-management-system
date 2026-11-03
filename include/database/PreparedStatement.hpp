#pragma once

#include "database/DatabaseException.hpp"

#include <sqlite3.h>
#include <memory>
#include <string>

namespace lms::database {

struct SqliteStatementDeleter {
    void operator()(sqlite3_stmt* stmt) const noexcept {
        if (stmt != nullptr) {
            sqlite3_finalize(stmt);
        }
    }
};

using StatementHandle = std::unique_ptr<sqlite3_stmt, SqliteStatementDeleter>;

inline StatementHandle prepareStatement(sqlite3* db, const std::string& sql) {
    if (db == nullptr) {
        throw DatabaseException("Cannot prepare statement: database handle is null");
    }

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::string errMsg = sqlite3_errmsg(db);
        throw DatabaseException("Failed to prepare statement [" + sql + "]: " + errMsg, rc);
    }
    return StatementHandle(stmt);
}

} // namespace lms::database
