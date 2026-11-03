#include "database/DatabaseConnection.hpp"
#include "database/DatabaseException.hpp"
#include "database/SchemaManager.hpp"
#include "models/Book.hpp"
#include "models/BookCopy.hpp"
#include "models/BookCopyStatus.hpp"
#include "repositories/BookCopyRepository.hpp"
#include "repositories/BookRepository.hpp"

#include <gtest/gtest.h>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace {

class BookCopyRepositoryTest : public ::testing::Test {
protected:
    void SetUp() override {
        connection_ = std::make_unique<lms::database::DatabaseConnection>(":memory:");
        schemaManager_ = std::make_unique<lms::database::SchemaManager>(*connection_);
        schemaManager_->initializeSchema();

        bookRepo_ = std::make_unique<lms::repositories::BookRepository>(*connection_);
        copyRepo_ = std::make_unique<lms::repositories::BookCopyRepository>(*connection_);

        // Insert default book for copy tests
        validBookId_ = bookRepo_->addBook(lms::models::Book("Test Title", "Test Author"));
    }

    void TearDown() override {
        copyRepo_.reset();
        bookRepo_.reset();
        schemaManager_.reset();
        connection_.reset();
    }

    std::unique_ptr<lms::database::DatabaseConnection> connection_;
    std::unique_ptr<lms::database::SchemaManager> schemaManager_;
    std::unique_ptr<lms::repositories::BookRepository> bookRepo_;
    std::unique_ptr<lms::repositories::BookCopyRepository> copyRepo_;
    int validBookId_{0};
};

TEST_F(BookCopyRepositoryTest, AddAndRetrieveCopyById) {
    lms::models::BookCopy copy(validBookId_, "ACC-1001", lms::models::BookCopyStatus::Available, "2026-01-10");
    int copyId = copyRepo_->addCopy(copy);
    EXPECT_GT(copyId, 0);

    auto retrieved = copyRepo_->getCopyById(copyId);
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->getId(), copyId);
    EXPECT_EQ(retrieved->getBookId(), validBookId_);
    EXPECT_EQ(retrieved->getAccessionNumber(), "ACC-1001");
    EXPECT_EQ(retrieved->getStatus(), lms::models::BookCopyStatus::Available);
    ASSERT_TRUE(retrieved->getAcquiredOn().has_value());
    EXPECT_EQ(*retrieved->getAcquiredOn(), "2026-01-10");
}

TEST_F(BookCopyRepositoryTest, RejectCopyWithNonexistentBookId) {
    lms::models::BookCopy copy(9999, "ACC-INVALID");
    EXPECT_THROW(copyRepo_->addCopy(copy), lms::database::DatabaseException);
}

TEST_F(BookCopyRepositoryTest, AddMultipleCopiesForSameBook) {
    copyRepo_->addCopy(lms::models::BookCopy(validBookId_, "ACC-A"));
    copyRepo_->addCopy(lms::models::BookCopy(validBookId_, "ACC-B"));
    copyRepo_->addCopy(lms::models::BookCopy(validBookId_, "ACC-C"));

    auto copies = copyRepo_->getCopiesByBookId(validBookId_);
    EXPECT_EQ(copies.size(), 3);
    EXPECT_EQ(copies[0].getAccessionNumber(), "ACC-A");
    EXPECT_EQ(copies[1].getAccessionNumber(), "ACC-B");
    EXPECT_EQ(copies[2].getAccessionNumber(), "ACC-C");
}

TEST_F(BookCopyRepositoryTest, RetrieveByAccessionNumber) {
    copyRepo_->addCopy(lms::models::BookCopy(validBookId_, "ACC-UNIQUE-42"));

    auto found = copyRepo_->getCopyByAccessionNumber("ACC-UNIQUE-42");
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->getAccessionNumber(), "ACC-UNIQUE-42");

    EXPECT_FALSE(copyRepo_->getCopyByAccessionNumber("ACC-NONEXISTENT").has_value());
}

TEST_F(BookCopyRepositoryTest, FilterCopiesByStatus) {
    copyRepo_->addCopy(lms::models::BookCopy(validBookId_, "ACC-1", lms::models::BookCopyStatus::Available));
    copyRepo_->addCopy(lms::models::BookCopy(validBookId_, "ACC-2", lms::models::BookCopyStatus::OnLoan));
    copyRepo_->addCopy(lms::models::BookCopy(validBookId_, "ACC-3", lms::models::BookCopyStatus::Lost));
    copyRepo_->addCopy(lms::models::BookCopy(validBookId_, "ACC-4", lms::models::BookCopyStatus::Available));

    auto available = copyRepo_->getCopiesByStatus(lms::models::BookCopyStatus::Available);
    EXPECT_EQ(available.size(), 2);

    auto onLoan = copyRepo_->getCopiesByStatus(lms::models::BookCopyStatus::OnLoan);
    EXPECT_EQ(onLoan.size(), 1);
    EXPECT_EQ(onLoan[0].getAccessionNumber(), "ACC-2");

    auto damaged = copyRepo_->getCopiesByStatus(lms::models::BookCopyStatus::Damaged);
    EXPECT_TRUE(damaged.empty());
}

TEST_F(BookCopyRepositoryTest, GetAllCopies) {
    EXPECT_TRUE(copyRepo_->getAllCopies().empty());

    copyRepo_->addCopy(lms::models::BookCopy(validBookId_, "ACC-1"));
    copyRepo_->addCopy(lms::models::BookCopy(validBookId_, "ACC-2"));

    auto all = copyRepo_->getAllCopies();
    EXPECT_EQ(all.size(), 2);
}

TEST_F(BookCopyRepositoryTest, UpdateCopyInformation) {
    int id = copyRepo_->addCopy(lms::models::BookCopy(validBookId_, "ACC-ORIG", lms::models::BookCopyStatus::Available));

    lms::models::BookCopy updated(id, validBookId_, "ACC-MOD", lms::models::BookCopyStatus::Damaged, "2026-02-20");
    EXPECT_TRUE(copyRepo_->updateCopy(updated));

    auto retrieved = copyRepo_->getCopyById(id);
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->getAccessionNumber(), "ACC-MOD");
    EXPECT_EQ(retrieved->getStatus(), lms::models::BookCopyStatus::Damaged);
    EXPECT_EQ(retrieved->getAcquiredOn().value_or(""), "2026-02-20");
}

TEST_F(BookCopyRepositoryTest, UpdateCopyStatusDirectly) {
    int id = copyRepo_->addCopy(lms::models::BookCopy(validBookId_, "ACC-STAT", lms::models::BookCopyStatus::Available));

    EXPECT_TRUE(copyRepo_->updateCopyStatus(id, lms::models::BookCopyStatus::Withdrawn));

    auto retrieved = copyRepo_->getCopyById(id);
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->getStatus(), lms::models::BookCopyStatus::Withdrawn);

    EXPECT_FALSE(copyRepo_->updateCopyStatus(9999, lms::models::BookCopyStatus::Available));
}

TEST_F(BookCopyRepositoryTest, RejectDuplicateAccessionNumber) {
    copyRepo_->addCopy(lms::models::BookCopy(validBookId_, "ACC-DUP"));

    EXPECT_THROW(
        copyRepo_->addCopy(lms::models::BookCopy(validBookId_, "ACC-DUP")),
        lms::database::DatabaseException);
}

TEST_F(BookCopyRepositoryTest, RejectInvalidStatusDirectSql) {
    // Attempting raw SQL insertion of invalid status violates database CHECK constraint
    EXPECT_THROW(
        connection_->execute("INSERT INTO book_copies (book_id, accession_number, status) VALUES (1, 'BAD', 'InvalidStatus');"),
        lms::database::DatabaseException);
}

TEST_F(BookCopyRepositoryTest, PreventDeleteWhenLoanHistoryExists) {
    int copyId = copyRepo_->addCopy(lms::models::BookCopy(validBookId_, "ACC-LOANED"));

    // Insert a dummy member and loan record to simulate loan history
    connection_->execute("INSERT INTO members (id, full_name, joined_on, status) VALUES (1, 'Member', '2026-01-01', 'Active');");
    connection_->execute("INSERT INTO loans (copy_id, member_id, issued_on, due_on, returned_on) "
                         "VALUES (" + std::to_string(copyId) + ", 1, '2026-01-01', '2026-01-15', '2026-01-10');");

    EXPECT_TRUE(copyRepo_->hasLoanHistory(copyId));
    EXPECT_FALSE(copyRepo_->hasActiveLoan(copyId));

    // Deleting copy must throw DatabaseException
    EXPECT_THROW(copyRepo_->deleteCopy(copyId), lms::database::DatabaseException);
    EXPECT_TRUE(copyRepo_->getCopyById(copyId).has_value());
}

TEST_F(BookCopyRepositoryTest, ActiveLoanCheck) {
    int copyId = copyRepo_->addCopy(lms::models::BookCopy(validBookId_, "ACC-ACTIVE"));
    connection_->execute("INSERT INTO members (id, full_name, joined_on, status) VALUES (1, 'Member', '2026-01-01', 'Active');");

    EXPECT_FALSE(copyRepo_->hasActiveLoan(copyId));

    // Insert active loan (returned_on is NULL)
    connection_->execute("INSERT INTO loans (copy_id, member_id, issued_on, due_on, returned_on) "
                         "VALUES (" + std::to_string(copyId) + ", 1, '2026-01-01', '2026-01-15', NULL);");

    EXPECT_TRUE(copyRepo_->hasActiveLoan(copyId));
    EXPECT_TRUE(copyRepo_->hasLoanHistory(copyId));
}

TEST_F(BookCopyRepositoryTest, DeleteCopyWithoutLoanHistorySucceeds) {
    int copyId = copyRepo_->addCopy(lms::models::BookCopy(validBookId_, "ACC-FRESH"));
    EXPECT_TRUE(copyRepo_->deleteCopy(copyId));
    EXPECT_FALSE(copyRepo_->getCopyById(copyId).has_value());
}

TEST_F(BookCopyRepositoryTest, MissingCopyReturnsNullopt) {
    EXPECT_FALSE(copyRepo_->getCopyById(9999).has_value());
    EXPECT_FALSE(copyRepo_->getCopyByAccessionNumber("NONEXISTENT").has_value());
}

} // namespace
