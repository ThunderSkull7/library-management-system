#include "database/SchemaManager.hpp"
#include "database/DatabaseException.hpp"

#include <sqlite3.h>
#include <string>

namespace lms::database {

SchemaManager::SchemaManager(DatabaseConnection& connection) : connection_(connection) {}

int SchemaManager::getSchemaVersion() const {
    if (!connection_.isOpen()) {
        throw DatabaseException("Cannot get schema version: Database connection is not open");
    }

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(connection_.getRawHandle(), "PRAGMA user_version;", -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::string errMsg = sqlite3_errmsg(connection_.getRawHandle());
        throw DatabaseException("Failed to prepare PRAGMA user_version query: " + errMsg, rc);
    }

    int version = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        version = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return version;
}

void SchemaManager::setSchemaVersion(int version) {
    if (!connection_.isOpen()) {
        throw DatabaseException("Cannot set schema version: Database connection is not open");
    }
    connection_.execute("PRAGMA user_version = " + std::to_string(version) + ";");
}

void SchemaManager::initializeSchema() {
    int currentVersion = getSchemaVersion();
    if (currentVersion > CURRENT_SCHEMA_VERSION) {
        throw DatabaseException(
            "Unsupported future schema version: " + std::to_string(currentVersion) +
            ". Current supported version is " + std::to_string(CURRENT_SCHEMA_VERSION) + ".");
    }

    connection_.execute("BEGIN IMMEDIATE;");
    try {
        createTablesAndIndexes();
        if (currentVersion < CURRENT_SCHEMA_VERSION) {
            setSchemaVersion(CURRENT_SCHEMA_VERSION);
        }
        connection_.execute("COMMIT;");
    } catch (...) {
        try {
            connection_.execute("ROLLBACK;");
        } catch (...) {
            // Suppress rollback errors during exception propagation
        }
        throw;
    }
}

void SchemaManager::createTablesAndIndexes() {
    // 1. books table
    connection_.execute(R"sql(
        CREATE TABLE IF NOT EXISTS books (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            isbn TEXT UNIQUE,
            title TEXT NOT NULL,
            author TEXT NOT NULL,
            publisher TEXT,
            publication_year INTEGER,
            category TEXT,
            description TEXT
        );
    )sql");

    // 2. book_copies table
    connection_.execute(R"sql(
        CREATE TABLE IF NOT EXISTS book_copies (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            book_id INTEGER NOT NULL,
            accession_number TEXT NOT NULL UNIQUE,
            status TEXT NOT NULL CHECK(status IN ('Available', 'On Loan', 'Lost', 'Damaged', 'Withdrawn')),
            acquired_on TEXT,
            FOREIGN KEY (book_id) REFERENCES books(id) ON DELETE RESTRICT
        );
    )sql");

    // 3. members table
    connection_.execute(R"sql(
        CREATE TABLE IF NOT EXISTS members (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            full_name TEXT NOT NULL,
            email TEXT UNIQUE,
            phone TEXT,
            joined_on TEXT NOT NULL,
            status TEXT NOT NULL CHECK(status IN ('Active', 'Suspended', 'Closed'))
        );
    )sql");

    // 4. loans table
    connection_.execute(R"sql(
        CREATE TABLE IF NOT EXISTS loans (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            copy_id INTEGER NOT NULL,
            member_id INTEGER NOT NULL,
            issued_on TEXT NOT NULL,
            due_on TEXT NOT NULL,
            returned_on TEXT,
            fine_minor_units INTEGER NOT NULL DEFAULT 0 CHECK(fine_minor_units >= 0),
            fine_paid INTEGER NOT NULL DEFAULT 0 CHECK(fine_paid IN (0, 1)),
            FOREIGN KEY (copy_id) REFERENCES book_copies(id) ON DELETE RESTRICT,
            FOREIGN KEY (member_id) REFERENCES members(id) ON DELETE RESTRICT,
            CHECK(due_on >= issued_on),
            CHECK(returned_on IS NULL OR returned_on >= issued_on)
        );
    )sql");

    // 5. Indexes
    // Partial unique index: strictly one active loan per physical book copy
    connection_.execute(R"sql(
        CREATE UNIQUE INDEX IF NOT EXISTS one_active_loan_per_copy
        ON loans(copy_id)
        WHERE returned_on IS NULL;
    )sql");

    // Book search indexes (ISBN is already uniquely indexed via UNIQUE constraint)
    connection_.execute(R"sql(
        CREATE INDEX IF NOT EXISTS idx_books_title ON books(title);
        CREATE INDEX IF NOT EXISTS idx_books_author ON books(author);
    )sql");

    // Book copy lookup indexes (Accession number is uniquely indexed via UNIQUE constraint)
    connection_.execute(R"sql(
        CREATE INDEX IF NOT EXISTS idx_book_copies_book_id ON book_copies(book_id);
        CREATE INDEX IF NOT EXISTS idx_book_copies_status ON book_copies(status);
    )sql");

    // Member search indexes (Email is uniquely indexed via UNIQUE constraint)
    connection_.execute(R"sql(
        CREATE INDEX IF NOT EXISTS idx_members_full_name ON members(full_name);
    )sql");

    // Loan query indexes
    connection_.execute(R"sql(
        CREATE INDEX IF NOT EXISTS idx_loans_member_id ON loans(member_id);
        CREATE INDEX IF NOT EXISTS idx_loans_copy_id ON loans(copy_id);
        CREATE INDEX IF NOT EXISTS idx_loans_due_on ON loans(due_on);
        CREATE INDEX IF NOT EXISTS idx_loans_active_due ON loans(due_on) WHERE returned_on IS NULL;
    )sql");
}

} // namespace lms::database

