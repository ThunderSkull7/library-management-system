#include "database/DatabaseConnection.hpp"
#include "database/DatabaseException.hpp"
#include "database/Transaction.hpp"

#include <gtest/gtest.h>
#include <sqlite3.h>
#include <filesystem>
#include <string>

namespace {

using lms::database::DatabaseConnection;
using lms::database::DatabaseException;
using lms::database::Transaction;

class TransactionTest : public ::testing::Test {
protected:
    void SetUp() override {
        testDbPath_ = std::filesystem::temp_directory_path() / "lms_tx_test.db";
        if (std::filesystem::exists(testDbPath_)) {
            std::filesystem::remove(testDbPath_);
        }
        conn_.open(testDbPath_);
        conn_.execute("CREATE TABLE test_items (id INTEGER PRIMARY KEY, name TEXT);");
    }

    void TearDown() override {
        conn_.close();
        if (std::filesystem::exists(testDbPath_)) {
            std::filesystem::remove(testDbPath_);
        }
    }

    int countItems() {
        sqlite3_stmt* stmt = nullptr;
        int rc = sqlite3_prepare_v2(conn_.getRawHandle(), "SELECT COUNT(*) FROM test_items;", -1, &stmt, nullptr);
        if (rc != SQLITE_OK) {
            return -1;
        }
        int count = 0;
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            count = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
        return count;
    }

    std::filesystem::path testDbPath_;
    DatabaseConnection conn_;
};

TEST_F(TransactionTest, CommitPersistsChanges) {
    {
        Transaction tx(conn_);
        EXPECT_TRUE(tx.isActive());
        EXPECT_FALSE(tx.isCommitted());

        conn_.execute("INSERT INTO test_items (name) VALUES ('Alpha');");
        conn_.execute("INSERT INTO test_items (name) VALUES ('Beta');");

        tx.commit();
        EXPECT_FALSE(tx.isActive());
        EXPECT_TRUE(tx.isCommitted());
    }

    EXPECT_EQ(countItems(), 2);
}

TEST_F(TransactionTest, DestructorRollsBackIfNotCommitted) {
    {
        Transaction tx(conn_);
        conn_.execute("INSERT INTO test_items (name) VALUES ('RollbackMe');");
        // tx destructs here without commit()
    }

    EXPECT_EQ(countItems(), 0);
}

TEST_F(TransactionTest, ExplicitRollback) {
    {
        Transaction tx(conn_);
        conn_.execute("INSERT INTO test_items (name) VALUES ('ExplicitRollback');");
        tx.rollback();
        EXPECT_FALSE(tx.isActive());
        EXPECT_FALSE(tx.isCommitted());
    }

    EXPECT_EQ(countItems(), 0);
}

TEST_F(TransactionTest, RollbackAfterExceptionPreservesPriorState) {
    conn_.execute("INSERT INTO test_items (name) VALUES ('Pre-existing');");
    ASSERT_EQ(countItems(), 1);

    EXPECT_THROW({
        Transaction tx(conn_);
        conn_.execute("INSERT INTO test_items (name) VALUES ('New Item');");
        // Intentional invalid SQL to throw
        conn_.execute("INSERT INTO non_existent_table VALUES (1);");
        tx.commit();
    }, DatabaseException);

    EXPECT_EQ(countItems(), 1);
}

TEST_F(TransactionTest, CannotCommitTwiceOrCommitAfterRollback) {
    Transaction tx(conn_);
    tx.commit();
    EXPECT_THROW(tx.commit(), DatabaseException);
    // Rollback is idempotent and safe to call even after commit
    EXPECT_NO_THROW(tx.rollback());

    Transaction tx2(conn_);
    tx2.rollback();
    // Rollback is idempotent
    EXPECT_NO_THROW(tx2.rollback());
    // Commit after rollback should throw
    EXPECT_THROW(tx2.commit(), DatabaseException);
}

TEST_F(TransactionTest, MoveConstructorTransfersOwnership) {
    {
        Transaction tx1(conn_);
        conn_.execute("INSERT INTO test_items (name) VALUES ('Moved');");

        Transaction tx2(std::move(tx1));
        EXPECT_FALSE(tx1.isActive());
        EXPECT_TRUE(tx2.isActive());

        tx2.commit();
    }

    EXPECT_EQ(countItems(), 1);
}

} // namespace
