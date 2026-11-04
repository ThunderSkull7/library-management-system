#include "database/DatabaseConnection.hpp"
#include "database/DatabaseException.hpp"
#include "database/SchemaManager.hpp"

#include <gtest/gtest.h>
#include <sqlite3.h>
#include <set>
#include <string>

namespace {

class SchemaManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        connection_ = std::make_unique<lms::database::DatabaseConnection>(":memory:");
        schemaManager_ = std::make_unique<lms::database::SchemaManager>(*connection_);
    }

    void TearDown() override {
        schemaManager_.reset();
        connection_.reset();
    }

    std::set<std::string> getTableColumns(const std::string& tableName) {
        std::set<std::string> columns;
        std::string sql = "PRAGMA table_info(" + tableName + ");";
        sqlite3_stmt* stmt = nullptr;
        int rc = sqlite3_prepare_v2(connection_->getRawHandle(), sql.c_str(), -1, &stmt, nullptr);
        if (rc == SQLITE_OK) {
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                const unsigned char* name = sqlite3_column_text(stmt, 1);
                if (name) {
                    columns.insert(reinterpret_cast<const char*>(name));
                }
            }
            sqlite3_finalize(stmt);
        }
        return columns;
    }

    bool tableExists(const std::string& tableName) {
        std::string sql = "SELECT count(*) FROM sqlite_master WHERE type='table' AND name='" + tableName + "';";
        sqlite3_stmt* stmt = nullptr;
        int rc = sqlite3_prepare_v2(connection_->getRawHandle(), sql.c_str(), -1, &stmt, nullptr);
        if (rc != SQLITE_OK) {
            return false;
        }
        int count = 0;
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            count = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
        return count > 0;
    }

    std::unique_ptr<lms::database::DatabaseConnection> connection_;
    std::unique_ptr<lms::database::SchemaManager> schemaManager_;
};

TEST_F(SchemaManagerTest, SuccessfulSchemaInitialization) {
    EXPECT_EQ(schemaManager_->getSchemaVersion(), 0);
    EXPECT_NO_THROW(schemaManager_->initializeSchema());
    EXPECT_EQ(schemaManager_->getSchemaVersion(), lms::database::SchemaManager::CURRENT_SCHEMA_VERSION);
}

TEST_F(SchemaManagerTest, AllFourTablesExist) {
    schemaManager_->initializeSchema();
    EXPECT_TRUE(tableExists("books"));
    EXPECT_TRUE(tableExists("book_copies"));
    EXPECT_TRUE(tableExists("members"));
    EXPECT_TRUE(tableExists("loans"));
}

TEST_F(SchemaManagerTest, BooksTableColumnsExist) {
    schemaManager_->initializeSchema();
    auto cols = getTableColumns("books");
    EXPECT_TRUE(cols.contains("id"));
    EXPECT_TRUE(cols.contains("isbn"));
    EXPECT_TRUE(cols.contains("title"));
    EXPECT_TRUE(cols.contains("author"));
    EXPECT_TRUE(cols.contains("publisher"));
    EXPECT_TRUE(cols.contains("publication_year"));
    EXPECT_TRUE(cols.contains("category"));
    EXPECT_TRUE(cols.contains("description"));
}

TEST_F(SchemaManagerTest, BookCopiesTableColumnsExist) {
    schemaManager_->initializeSchema();
    auto cols = getTableColumns("book_copies");
    EXPECT_TRUE(cols.contains("id"));
    EXPECT_TRUE(cols.contains("book_id"));
    EXPECT_TRUE(cols.contains("accession_number"));
    EXPECT_TRUE(cols.contains("status"));
    EXPECT_TRUE(cols.contains("acquired_on"));
}

TEST_F(SchemaManagerTest, MembersTableColumnsExist) {
    schemaManager_->initializeSchema();
    auto cols = getTableColumns("members");
    EXPECT_TRUE(cols.contains("id"));
    EXPECT_TRUE(cols.contains("full_name"));
    EXPECT_TRUE(cols.contains("email"));
    EXPECT_TRUE(cols.contains("phone"));
    EXPECT_TRUE(cols.contains("joined_on"));
    EXPECT_TRUE(cols.contains("status"));
}

TEST_F(SchemaManagerTest, LoansTableColumnsExist) {
    schemaManager_->initializeSchema();
    auto cols = getTableColumns("loans");
    EXPECT_TRUE(cols.contains("id"));
    EXPECT_TRUE(cols.contains("copy_id"));
    EXPECT_TRUE(cols.contains("member_id"));
    EXPECT_TRUE(cols.contains("issued_on"));
    EXPECT_TRUE(cols.contains("due_on"));
    EXPECT_TRUE(cols.contains("returned_on"));
    EXPECT_TRUE(cols.contains("fine_minor_units"));
    EXPECT_TRUE(cols.contains("fine_paid"));
}

TEST_F(SchemaManagerTest, ForeignKeysExist) {
    schemaManager_->initializeSchema();

    // Check book_copies foreign keys
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(connection_->getRawHandle(), "PRAGMA foreign_key_list(book_copies);", -1, &stmt, nullptr);
    ASSERT_EQ(rc, SQLITE_OK);
    bool fkFound = false;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        std::string table = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        std::string from = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        std::string to = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        if (table == "books" && from == "book_id" && to == "id") {
            fkFound = true;
        }
    }
    sqlite3_finalize(stmt);
    EXPECT_TRUE(fkFound);

    // Check loans foreign keys
    rc = sqlite3_prepare_v2(connection_->getRawHandle(), "PRAGMA foreign_key_list(loans);", -1, &stmt, nullptr);
    ASSERT_EQ(rc, SQLITE_OK);
    bool fkCopyFound = false;
    bool fkMemberFound = false;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        std::string table = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        std::string from = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        std::string to = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        if (table == "book_copies" && from == "copy_id" && to == "id") {
            fkCopyFound = true;
        }
        if (table == "members" && from == "member_id" && to == "id") {
            fkMemberFound = true;
        }
    }
    sqlite3_finalize(stmt);
    EXPECT_TRUE(fkCopyFound);
    EXPECT_TRUE(fkMemberFound);
}

TEST_F(SchemaManagerTest, CopyStatusCheckConstraint) {
    schemaManager_->initializeSchema();
    connection_->execute("INSERT INTO books (id, title, author) VALUES (1, 'Test Book', 'Test Author');");

    // Valid statuses: 'Available', 'On Loan', 'Lost', 'Damaged', 'Withdrawn'
    EXPECT_NO_THROW(connection_->execute(
        "INSERT INTO book_copies (book_id, accession_number, status) VALUES (1, 'ACC-01', 'Available');"));
    EXPECT_NO_THROW(connection_->execute(
        "INSERT INTO book_copies (book_id, accession_number, status) VALUES (1, 'ACC-02', 'On Loan');"));
    EXPECT_NO_THROW(connection_->execute(
        "INSERT INTO book_copies (book_id, accession_number, status) VALUES (1, 'ACC-03', 'Lost');"));
    EXPECT_NO_THROW(connection_->execute(
        "INSERT INTO book_copies (book_id, accession_number, status) VALUES (1, 'ACC-04', 'Damaged');"));
    EXPECT_NO_THROW(connection_->execute(
        "INSERT INTO book_copies (book_id, accession_number, status) VALUES (1, 'ACC-05', 'Withdrawn');"));

    // Invalid status should be rejected
    EXPECT_THROW(connection_->execute(
        "INSERT INTO book_copies (book_id, accession_number, status) VALUES (1, 'ACC-06', 'Disposed');"),
        lms::database::DatabaseException);
}

TEST_F(SchemaManagerTest, MemberStatusCheckConstraint) {
    schemaManager_->initializeSchema();

    // Valid statuses: 'Active', 'Suspended', 'Closed'
    EXPECT_NO_THROW(connection_->execute(
        "INSERT INTO members (full_name, email, joined_on, status) VALUES ('Alice', 'alice@test.com', '2026-01-01', 'Active');"));
    EXPECT_NO_THROW(connection_->execute(
        "INSERT INTO members (full_name, email, joined_on, status) VALUES ('Bob', 'bob@test.com', '2026-01-01', 'Suspended');"));
    EXPECT_NO_THROW(connection_->execute(
        "INSERT INTO members (full_name, email, joined_on, status) VALUES ('Charlie', 'charlie@test.com', '2026-01-01', 'Closed');"));

    // Invalid status should be rejected
    EXPECT_THROW(connection_->execute(
        "INSERT INTO members (full_name, email, joined_on, status) VALUES ('Dave', 'dave@test.com', '2026-01-01', 'Pending');"),
        lms::database::DatabaseException);
}

TEST_F(SchemaManagerTest, DuplicateIsbnIsRejected) {
    schemaManager_->initializeSchema();

    EXPECT_NO_THROW(connection_->execute(
        "INSERT INTO books (isbn, title, author) VALUES ('978-0134997834', 'Book A', 'Author A');"));

    // Duplicate non-null ISBN rejected
    EXPECT_THROW(connection_->execute(
        "INSERT INTO books (isbn, title, author) VALUES ('978-0134997834', 'Book B', 'Author B');"),
        lms::database::DatabaseException);

    // Nullable ISBN allows multiple NULLs
    EXPECT_NO_THROW(connection_->execute(
        "INSERT INTO books (isbn, title, author) VALUES (NULL, 'Book C', 'Author C');"));
    EXPECT_NO_THROW(connection_->execute(
        "INSERT INTO books (isbn, title, author) VALUES (NULL, 'Book D', 'Author D');"));
}

TEST_F(SchemaManagerTest, DuplicateAccessionNumberIsRejected) {
    schemaManager_->initializeSchema();
    connection_->execute("INSERT INTO books (id, title, author) VALUES (1, 'Book A', 'Author A');");

    EXPECT_NO_THROW(connection_->execute(
        "INSERT INTO book_copies (book_id, accession_number, status) VALUES (1, 'ACC-100', 'Available');"));

    // Duplicate accession number rejected
    EXPECT_THROW(connection_->execute(
        "INSERT INTO book_copies (book_id, accession_number, status) VALUES (1, 'ACC-100', 'Available');"),
        lms::database::DatabaseException);
}

TEST_F(SchemaManagerTest, DuplicateEmailIsRejectedWhenProvided) {
    schemaManager_->initializeSchema();

    EXPECT_NO_THROW(connection_->execute(
        "INSERT INTO members (full_name, email, joined_on, status) VALUES ('Member A', 'user@example.com', '2026-01-01', 'Active');"));

    // Duplicate non-null email rejected
    EXPECT_THROW(connection_->execute(
        "INSERT INTO members (full_name, email, joined_on, status) VALUES ('Member B', 'user@example.com', '2026-01-01', 'Active');"),
        lms::database::DatabaseException);

    // Nullable email allows multiple NULLs
    EXPECT_NO_THROW(connection_->execute(
        "INSERT INTO members (full_name, email, joined_on, status) VALUES ('Member C', NULL, '2026-01-01', 'Active');"));
    EXPECT_NO_THROW(connection_->execute(
        "INSERT INTO members (full_name, email, joined_on, status) VALUES ('Member D', NULL, '2026-01-01', 'Active');"));
}

TEST_F(SchemaManagerTest, PartialUniqueIndexPreventsMultipleActiveLoansForSameCopy) {
    schemaManager_->initializeSchema();
    connection_->execute("INSERT INTO books (id, title, author) VALUES (1, 'Book 1', 'Author 1');");
    connection_->execute("INSERT INTO book_copies (id, book_id, accession_number, status) VALUES (1, 1, 'ACC-001', 'On Loan');");
    connection_->execute("INSERT INTO members (id, full_name, joined_on, status) VALUES (1, 'Member 1', '2026-01-01', 'Active');");
    connection_->execute("INSERT INTO members (id, full_name, joined_on, status) VALUES (2, 'Member 2', '2026-01-01', 'Active');");

    // First active loan (returned_on is NULL)
    EXPECT_NO_THROW(connection_->execute(
        "INSERT INTO loans (copy_id, member_id, issued_on, due_on, returned_on) "
        "VALUES (1, 1, '2026-02-01', '2026-02-15', NULL);"));

    // Second active loan for the same copy while previous is active must fail
    EXPECT_THROW(connection_->execute(
        "INSERT INTO loans (copy_id, member_id, issued_on, due_on, returned_on) "
        "VALUES (1, 2, '2026-02-05', '2026-02-19', NULL);"),
        lms::database::DatabaseException);
}

TEST_F(SchemaManagerTest, ReturnedCopyCanSubsequentlyBeLoanedAgain) {
    schemaManager_->initializeSchema();
    connection_->execute("INSERT INTO books (id, title, author) VALUES (1, 'Book 1', 'Author 1');");
    connection_->execute("INSERT INTO book_copies (id, book_id, accession_number, status) VALUES (1, 1, 'ACC-001', 'Available');");
    connection_->execute("INSERT INTO members (id, full_name, joined_on, status) VALUES (1, 'Member 1', '2026-01-01', 'Active');");
    connection_->execute("INSERT INTO members (id, full_name, joined_on, status) VALUES (2, 'Member 2', '2026-01-01', 'Active');");

    // Initial loan
    connection_->execute(
        "INSERT INTO loans (id, copy_id, member_id, issued_on, due_on, returned_on) "
        "VALUES (1, 1, 1, '2026-02-01', '2026-02-15', NULL);");

    // Return the copy
    connection_->execute("UPDATE loans SET returned_on = '2026-02-10' WHERE id = 1;");

    // Subsequent active loan for the returned copy must now succeed
    EXPECT_NO_THROW(connection_->execute(
        "INSERT INTO loans (id, copy_id, member_id, issued_on, due_on, returned_on) "
        "VALUES (2, 1, 2, '2026-02-12', '2026-02-26', NULL);"));
}

TEST_F(SchemaManagerTest, LoanCheckConstraintsEnforced) {
    schemaManager_->initializeSchema();
    connection_->execute("INSERT INTO books (id, title, author) VALUES (1, 'Book 1', 'Author 1');");
    connection_->execute("INSERT INTO book_copies (id, book_id, accession_number, status) VALUES (1, 1, 'ACC-001', 'Available');");
    connection_->execute("INSERT INTO members (id, full_name, joined_on, status) VALUES (1, 'Member 1', '2026-01-01', 'Active');");

    // Negative fine_minor_units rejected
    EXPECT_THROW(connection_->execute(
        "INSERT INTO loans (copy_id, member_id, issued_on, due_on, fine_minor_units) "
        "VALUES (1, 1, '2026-02-01', '2026-02-15', -50);"),
        lms::database::DatabaseException);

    // Invalid fine_paid rejected (not in (0, 1))
    EXPECT_THROW(connection_->execute(
        "INSERT INTO loans (copy_id, member_id, issued_on, due_on, fine_paid) "
        "VALUES (1, 1, '2026-02-01', '2026-02-15', 5);"),
        lms::database::DatabaseException);

    // due_on preceding issued_on rejected
    EXPECT_THROW(connection_->execute(
        "INSERT INTO loans (copy_id, member_id, issued_on, due_on) "
        "VALUES (1, 1, '2026-02-15', '2026-02-01');"),
        lms::database::DatabaseException);

    // returned_on preceding issued_on rejected
    EXPECT_THROW(connection_->execute(
        "INSERT INTO loans (copy_id, member_id, issued_on, due_on, returned_on) "
        "VALUES (1, 1, '2026-02-15', '2026-02-25', '2026-02-10');"),
        lms::database::DatabaseException);
}

TEST_F(SchemaManagerTest, RepeatedInitializationIsIdempotent) {
    schemaManager_->initializeSchema();
    EXPECT_EQ(schemaManager_->getSchemaVersion(), lms::database::SchemaManager::CURRENT_SCHEMA_VERSION);

    // Call initializeSchema multiple times
    EXPECT_NO_THROW(schemaManager_->initializeSchema());
    EXPECT_NO_THROW(schemaManager_->initializeSchema());
    EXPECT_EQ(schemaManager_->getSchemaVersion(), lms::database::SchemaManager::CURRENT_SCHEMA_VERSION);
}

TEST_F(SchemaManagerTest, ExistingRecordsSurviveRepeatedInitialization) {
    schemaManager_->initializeSchema();
    connection_->execute("INSERT INTO books (id, title, author) VALUES (42, 'The C++ Programming Language', 'Bjarne Stroustrup');");

    schemaManager_->initializeSchema();

    // Verify record still exists
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(connection_->getRawHandle(), "SELECT title, author FROM books WHERE id = 42;", -1, &stmt, nullptr);
    ASSERT_EQ(rc, SQLITE_OK);
    ASSERT_EQ(sqlite3_step(stmt), SQLITE_ROW);
    EXPECT_STREQ(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)), "The C++ Programming Language");
    EXPECT_STREQ(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)), "Bjarne Stroustrup");
    sqlite3_finalize(stmt);
}

TEST_F(SchemaManagerTest, UnsupportedFutureSchemaVersionRejected) {
    // Simulate a future version
    connection_->execute("PRAGMA user_version = 99;");
    EXPECT_THROW(schemaManager_->initializeSchema(), lms::database::DatabaseException);
}

TEST_F(SchemaManagerTest, InvalidForeignKeyReferencesRejected) {
    schemaManager_->initializeSchema();

    // Invalid book_id in book_copies
    EXPECT_THROW(connection_->execute(
        "INSERT INTO book_copies (book_id, accession_number, status) VALUES (9999, 'ACC-ERR', 'Available');"),
        lms::database::DatabaseException);

    // Insert valid book and copy for loan FK tests
    connection_->execute("INSERT INTO books (id, title, author) VALUES (1, 'Book 1', 'Author 1');");
    connection_->execute("INSERT INTO book_copies (id, book_id, accession_number, status) VALUES (1, 1, 'ACC-001', 'Available');");
    connection_->execute("INSERT INTO members (id, full_name, joined_on, status) VALUES (1, 'Member 1', '2026-01-01', 'Active');");

    // Invalid copy_id in loans
    EXPECT_THROW(connection_->execute(
        "INSERT INTO loans (copy_id, member_id, issued_on, due_on) VALUES (9999, 1, '2026-02-01', '2026-02-15');"),
        lms::database::DatabaseException);

    // Invalid member_id in loans
    EXPECT_THROW(connection_->execute(
        "INSERT INTO loans (copy_id, member_id, issued_on, due_on) VALUES (1, 9999, '2026-02-01', '2026-02-15');"),
        lms::database::DatabaseException);
}

TEST_F(SchemaManagerTest, LoanPaymentsTableExists) {
    schemaManager_->initializeSchema();
    EXPECT_TRUE(tableExists("loan_payments"));
    auto cols = getTableColumns("loan_payments");
    EXPECT_TRUE(cols.contains("id"));
    EXPECT_TRUE(cols.contains("loan_id"));
    EXPECT_TRUE(cols.contains("amount_minor_units"));
    EXPECT_TRUE(cols.contains("paid_on"));
}

TEST_F(SchemaManagerTest, MigrationFromV1ToV2PreservesData) {
    // 1. Manually set up a V1 schema database
    connection_->execute("PRAGMA user_version = 1;");
    connection_->execute("CREATE TABLE books (id INTEGER PRIMARY KEY, title TEXT, author TEXT);");
    connection_->execute("INSERT INTO books (id, title, author) VALUES (10, 'Legacy Book', 'Legacy Author');");

    EXPECT_EQ(schemaManager_->getSchemaVersion(), 1);

    // 2. Run initializeSchema to trigger migration to V2
    schemaManager_->initializeSchema();
    EXPECT_EQ(schemaManager_->getSchemaVersion(), 2);
    EXPECT_TRUE(tableExists("loan_payments"));

    // 3. Verify existing data is preserved
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(connection_->getRawHandle(), "SELECT title FROM books WHERE id = 10;", -1, &stmt, nullptr);
    ASSERT_EQ(rc, SQLITE_OK);
    ASSERT_EQ(sqlite3_step(stmt), SQLITE_ROW);
    EXPECT_STREQ(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)), "Legacy Book");
    sqlite3_finalize(stmt);
}

TEST_F(SchemaManagerTest, FullV1DatabaseMigratesToV2WithCompleteHistoryAndFKEnforcement) {
    // 1. Create complete V1 schema with books, copies, members, and loans
    connection_->execute("PRAGMA user_version = 1;");
    connection_->execute(R"sql(
        CREATE TABLE books (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            isbn TEXT UNIQUE,
            title TEXT NOT NULL,
            author TEXT NOT NULL,
            publisher TEXT,
            publication_year INTEGER,
            category TEXT,
            description TEXT
        );
        CREATE TABLE book_copies (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            book_id INTEGER NOT NULL,
            accession_number TEXT NOT NULL UNIQUE,
            status TEXT NOT NULL CHECK(status IN ('Available', 'On Loan', 'Lost', 'Damaged', 'Withdrawn')),
            acquired_on TEXT,
            FOREIGN KEY (book_id) REFERENCES books(id) ON DELETE RESTRICT
        );
        CREATE TABLE members (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            full_name TEXT NOT NULL,
            email TEXT UNIQUE,
            phone TEXT,
            joined_on TEXT NOT NULL,
            status TEXT NOT NULL CHECK(status IN ('Active', 'Suspended', 'Closed'))
        );
        CREATE TABLE loans (
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

    // Seed V1 data
    connection_->execute("INSERT INTO books (id, title, author) VALUES (1, 'Design Patterns', 'GoF');");
    connection_->execute("INSERT INTO book_copies (id, book_id, accession_number, status) VALUES (1, 1, 'ACC-DP-01', 'Available');");
    connection_->execute("INSERT INTO members (id, full_name, joined_on, status) VALUES (1, 'Ada Lovelace', '2026-01-01', 'Active');");
    connection_->execute("INSERT INTO loans (id, copy_id, member_id, issued_on, due_on, returned_on, fine_minor_units, fine_paid) "
                         "VALUES (1, 1, 1, '2026-01-01', '2026-01-15', '2026-01-20', 1000, 0);");

    EXPECT_EQ(schemaManager_->getSchemaVersion(), 1);

    // 2. Perform Migration to V2
    schemaManager_->initializeSchema();
    EXPECT_EQ(schemaManager_->getSchemaVersion(), 2);
    EXPECT_TRUE(tableExists("loan_payments"));

    // 3. Insert payment against existing loan 1
    EXPECT_NO_THROW(connection_->execute(
        "INSERT INTO loan_payments (loan_id, amount_minor_units, paid_on) VALUES (1, 500, '2026-01-22');"));

    // 4. FK enforcement: invalid loan_id rejected
    EXPECT_THROW(connection_->execute(
        "INSERT INTO loan_payments (loan_id, amount_minor_units, paid_on) VALUES (999, 500, '2026-01-22');"),
        lms::database::DatabaseException);
}

} // namespace

