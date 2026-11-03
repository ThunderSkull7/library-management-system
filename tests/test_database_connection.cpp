#include "database/DatabaseConnection.hpp"
#include "database/DatabaseException.hpp"

#include <gtest/gtest.h>
#include <sqlite3.h>
#include <filesystem>
#include <string>

namespace {

class DatabaseConnectionTest : public ::testing::Test {
protected:
    void SetUp() override {
        testDbPath_ = std::filesystem::temp_directory_path() / "lms_conn_test.db";
        if (std::filesystem::exists(testDbPath_)) {
            std::filesystem::remove(testDbPath_);
        }
    }

    void TearDown() override {
        if (std::filesystem::exists(testDbPath_)) {
            std::filesystem::remove(testDbPath_);
        }
    }

    std::filesystem::path testDbPath_;
};

TEST_F(DatabaseConnectionTest, DefaultConstructorInitializesClosed) {
    lms::database::DatabaseConnection conn;
    EXPECT_FALSE(conn.isOpen());
    EXPECT_EQ(conn.getRawHandle(), nullptr);
    EXPECT_TRUE(conn.getPath().empty());
}

TEST_F(DatabaseConnectionTest, SuccessfulOpenAndCloseFileDatabase) {
    lms::database::DatabaseConnection conn;
    EXPECT_NO_THROW(conn.open(testDbPath_));
    EXPECT_TRUE(conn.isOpen());
    EXPECT_NE(conn.getRawHandle(), nullptr);
    EXPECT_EQ(conn.getPath(), testDbPath_);
    EXPECT_TRUE(std::filesystem::exists(testDbPath_));

    conn.close();
    EXPECT_FALSE(conn.isOpen());
    EXPECT_EQ(conn.getRawHandle(), nullptr);
    EXPECT_TRUE(conn.getPath().empty());

    // Calling close multiple times should be safe and no-op
    EXPECT_NO_THROW(conn.close());
    EXPECT_FALSE(conn.isOpen());
}

TEST_F(DatabaseConnectionTest, ConstructorOpensDatabase) {
    lms::database::DatabaseConnection conn(testDbPath_);
    EXPECT_TRUE(conn.isOpen());
    EXPECT_NE(conn.getRawHandle(), nullptr);
}

TEST_F(DatabaseConnectionTest, InMemoryDatabaseSupport) {
    lms::database::DatabaseConnection conn(":memory:");
    EXPECT_TRUE(conn.isOpen());
    EXPECT_NE(conn.getRawHandle(), nullptr);

    // Verify SQL execution in memory
    EXPECT_NO_THROW(conn.execute("CREATE TABLE test_table (id INTEGER PRIMARY KEY, val TEXT);"));
    EXPECT_NO_THROW(conn.execute("INSERT INTO test_table (val) VALUES ('memory_data');"));
}

TEST_F(DatabaseConnectionTest, ForeignKeyEnforcementIsEnabled) {
    lms::database::DatabaseConnection conn(":memory:");
    EXPECT_TRUE(conn.isOpen());

    // Verify foreign keys pragma returns 1
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(conn.getRawHandle(), "PRAGMA foreign_keys;", -1, &stmt, nullptr);
    ASSERT_EQ(rc, SQLITE_OK);
    ASSERT_EQ(sqlite3_step(stmt), SQLITE_ROW);
    EXPECT_EQ(sqlite3_column_int(stmt, 0), 1);
    sqlite3_finalize(stmt);

    // Verify enforcement rejects foreign key violations
    conn.execute("CREATE TABLE parent (id INTEGER PRIMARY KEY);");
    conn.execute("CREATE TABLE child (id INTEGER PRIMARY KEY, parent_id INTEGER, FOREIGN KEY(parent_id) REFERENCES parent(id));");

    EXPECT_THROW(conn.execute("INSERT INTO child (id, parent_id) VALUES (1, 999);"), lms::database::DatabaseException);
}

TEST_F(DatabaseConnectionTest, ExecuteOnClosedDatabaseThrows) {
    lms::database::DatabaseConnection conn;
    EXPECT_THROW(conn.execute("CREATE TABLE foo (id INT);"), lms::database::DatabaseException);
}

TEST_F(DatabaseConnectionTest, InvalidSqlThrowsDatabaseException) {
    lms::database::DatabaseConnection conn(":memory:");
    EXPECT_THROW(conn.execute("INVALID SQL SYNTAX HERE;"), lms::database::DatabaseException);
}

TEST_F(DatabaseConnectionTest, OpenInvalidPathThrows) {
    // Attempting to open a path with invalid / non-existent directory component on Windows
    std::filesystem::path invalidPath = "Z:/NonExistentPath_XYZ_123/database.db";
    lms::database::DatabaseConnection conn;
    EXPECT_THROW(conn.open(invalidPath), lms::database::DatabaseException);
    EXPECT_FALSE(conn.isOpen());
}

TEST_F(DatabaseConnectionTest, MultipleIndependentConnections) {
    lms::database::DatabaseConnection conn1(":memory:");
    lms::database::DatabaseConnection conn2(":memory:");

    conn1.execute("CREATE TABLE unique_tbl1 (val TEXT);");
    conn2.execute("CREATE TABLE unique_tbl2 (val TEXT);");

    // Table in conn1 should not exist in conn2
    EXPECT_THROW(conn1.execute("INSERT INTO unique_tbl2 VALUES ('test');"), lms::database::DatabaseException);
    EXPECT_THROW(conn2.execute("INSERT INTO unique_tbl1 VALUES ('test');"), lms::database::DatabaseException);
}

TEST_F(DatabaseConnectionTest, MoveConstructorTransfersOwnership) {
    lms::database::DatabaseConnection conn1(":memory:");
    EXPECT_TRUE(conn1.isOpen());
    sqlite3* rawHandle = conn1.getRawHandle();

    lms::database::DatabaseConnection conn2(std::move(conn1));
    EXPECT_FALSE(conn1.isOpen());
    EXPECT_EQ(conn1.getRawHandle(), nullptr);
    EXPECT_TRUE(conn2.isOpen());
    EXPECT_EQ(conn2.getRawHandle(), rawHandle);
}

TEST_F(DatabaseConnectionTest, MoveAssignmentTransfersOwnership) {
    lms::database::DatabaseConnection conn1(":memory:");
    lms::database::DatabaseConnection conn2;

    sqlite3* rawHandle = conn1.getRawHandle();
    conn2 = std::move(conn1);

    EXPECT_FALSE(conn1.isOpen());
    EXPECT_EQ(conn1.getRawHandle(), nullptr);
    EXPECT_TRUE(conn2.isOpen());
    EXPECT_EQ(conn2.getRawHandle(), rawHandle);
}

} // namespace

