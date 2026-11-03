#include "database/DatabaseConnection.hpp"
#include "database/DatabaseException.hpp"

#include <sqlite3.h>
#include <utility>

namespace lms::database {

DatabaseConnection::DatabaseConnection(const std::filesystem::path& dbPath) {
    open(dbPath);
}

DatabaseConnection::~DatabaseConnection() {
    close();
}

DatabaseConnection::DatabaseConnection(DatabaseConnection&& other) noexcept
    : db_(std::exchange(other.db_, nullptr)), path_(std::move(other.path_)) {}

DatabaseConnection& DatabaseConnection::operator=(DatabaseConnection&& other) noexcept {
    if (this != &other) {
        close();
        db_ = std::exchange(other.db_, nullptr);
        path_ = std::move(other.path_);
    }
    return *this;
}

void DatabaseConnection::open(const std::filesystem::path& dbPath) {
    if (isOpen()) {
        close();
    }

    path_ = dbPath;
    std::string pathStr = dbPath.string();

    int flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;
    if (pathStr == ":memory:") {
        flags |= SQLITE_OPEN_MEMORY;
    }

    int rc = sqlite3_open_v2(pathStr.c_str(), &db_, flags, nullptr);
    if (rc != SQLITE_OK) {
        std::string errMsg = db_ ? sqlite3_errmsg(db_) : "Failed to allocate SQLite handle";
        close();
        throw DatabaseException("Failed to open database '" + pathStr + "': " + errMsg, rc);
    }

    // Configure busy timeout (5000 ms)
    rc = sqlite3_busy_timeout(db_, 5000);
    if (rc != SQLITE_OK) {
        std::string errMsg = sqlite3_errmsg(db_);
        close();
        throw DatabaseException("Failed to set busy timeout: " + errMsg, rc);
    }

    // Configure and verify foreign key enforcement
    configurePragmas();
    verifyForeignKeysEnabled();
}

void DatabaseConnection::close() noexcept {
    if (db_ != nullptr) {
        sqlite3_close_v2(db_);
        db_ = nullptr;
    }
    path_.clear();
}

bool DatabaseConnection::isOpen() const noexcept {
    return db_ != nullptr;
}

sqlite3* DatabaseConnection::getRawHandle() const noexcept {
    return db_;
}

const std::filesystem::path& DatabaseConnection::getPath() const noexcept {
    return path_;
}

void DatabaseConnection::execute(const std::string& sql) {
    if (!isOpen()) {
        throw DatabaseException("Cannot execute query: Database connection is not open");
    }

    char* errMsg = nullptr;
    int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::string err = errMsg ? errMsg : sqlite3_errmsg(db_);
        sqlite3_free(errMsg);
        throw DatabaseException("SQL execution error: " + err, rc);
    }
}

void DatabaseConnection::configurePragmas() {
    execute("PRAGMA foreign_keys = ON;");
}

void DatabaseConnection::verifyForeignKeysEnabled() {
    if (!isOpen()) {
        throw DatabaseException("Foreign key verification failed: Connection is not open");
    }

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, "PRAGMA foreign_keys;", -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::string errMsg = sqlite3_errmsg(db_);
        close();
        throw DatabaseException("Failed to prepare foreign keys verification query: " + errMsg, rc);
    }

    int stepRc = sqlite3_step(stmt);
    int fkEnabled = 0;
    if (stepRc == SQLITE_ROW) {
        fkEnabled = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);

    if (fkEnabled != 1) {
        close();
        throw DatabaseException("Foreign key enforcement verification failed (PRAGMA foreign_keys != 1)");
    }
}

} // namespace lms::database

