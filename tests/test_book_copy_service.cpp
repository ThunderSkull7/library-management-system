#include "database/DatabaseConnection.hpp"
#include "database/SchemaManager.hpp"
#include "models/Book.hpp"
#include "models/BookCopy.hpp"
#include "models/BookCopyStatus.hpp"
#include "repositories/BookCopyRepository.hpp"
#include "repositories/BookRepository.hpp"
#include "services/BookCopyService.hpp"
#include "utils/ValidationException.hpp"

#include <gtest/gtest.h>
#include <memory>
#include <optional>
#include <string>

namespace {

class BookCopyServiceTest : public ::testing::Test {
protected:
    void SetUp() override {
        connection_ = std::make_unique<lms::database::DatabaseConnection>(":memory:");
        schemaManager_ = std::make_unique<lms::database::SchemaManager>(*connection_);
        schemaManager_->initializeSchema();

        bookRepo_ = std::make_unique<lms::repositories::BookRepository>(*connection_);
        copyRepo_ = std::make_unique<lms::repositories::BookCopyRepository>(*connection_);
        copyService_ = std::make_unique<lms::services::BookCopyService>(*copyRepo_, *bookRepo_);

        validBookId_ = bookRepo_->addBook(lms::models::Book("Valid Book", "Author"));
    }

    void TearDown() override {
        copyService_.reset();
        copyRepo_.reset();
        bookRepo_.reset();
        schemaManager_.reset();
        connection_.reset();
    }

    std::unique_ptr<lms::database::DatabaseConnection> connection_;
    std::unique_ptr<lms::database::SchemaManager> schemaManager_;
    std::unique_ptr<lms::repositories::BookRepository> bookRepo_;
    std::unique_ptr<lms::repositories::BookCopyRepository> copyRepo_;
    std::unique_ptr<lms::services::BookCopyService> copyService_;
    int validBookId_{0};
};

TEST_F(BookCopyServiceTest, AddValidCopy) {
    int id = copyService_->addCopy(validBookId_, "  ACC-001  ", "  2026-02-15  ");
    EXPECT_GT(id, 0);

    auto copy = copyService_->getCopy(id);
    EXPECT_EQ(copy.getBookId(), validBookId_);
    EXPECT_EQ(copy.getAccessionNumber(), "ACC-001");
    EXPECT_EQ(copy.getStatus(), lms::models::BookCopyStatus::Available);
    ASSERT_TRUE(copy.getAcquiredOn().has_value());
    EXPECT_EQ(*copy.getAcquiredOn(), "2026-02-15");
}

TEST_F(BookCopyServiceTest, RejectCopyWithNonexistentBook) {
    EXPECT_THROW(
        copyService_->addCopy(9999, "ACC-001"),
        lms::utils::NotFoundException);
}

TEST_F(BookCopyServiceTest, RejectEmptyOrWhitespaceAccessionNumber) {
    EXPECT_THROW(
        copyService_->addCopy(validBookId_, ""),
        lms::utils::ValidationException);

    EXPECT_THROW(
        copyService_->addCopy(validBookId_, "   \t\n  "),
        lms::utils::ValidationException);
}

TEST_F(BookCopyServiceTest, RejectDuplicateAccessionNumber) {
    copyService_->addCopy(validBookId_, "ACC-DUPLICATE");

    EXPECT_THROW(
        copyService_->addCopy(validBookId_, "  ACC-DUPLICATE  "),
        lms::utils::ValidationException);
}

TEST_F(BookCopyServiceTest, RejectManualOnLoanStatusOnAdd) {
    EXPECT_THROW(
        copyService_->addCopy(validBookId_, "ACC-ONLOAN", std::nullopt, lms::models::BookCopyStatus::OnLoan),
        lms::utils::ValidationException);
}

TEST_F(BookCopyServiceTest, AcquiredDateValidation) {
    // Valid calendar date
    EXPECT_NO_THROW(copyService_->addCopy(validBookId_, "ACC-DATE-1", "2024-02-29")); // leap year

    // Invalid leap year
    EXPECT_THROW(copyService_->addCopy(validBookId_, "ACC-DATE-2", "2023-02-29"), lms::utils::ValidationException);

    // Invalid format
    EXPECT_THROW(copyService_->addCopy(validBookId_, "ACC-DATE-3", "2026/01/01"), lms::utils::ValidationException);
    EXPECT_THROW(copyService_->addCopy(validBookId_, "ACC-DATE-4", "01-01-2026"), lms::utils::ValidationException);
    EXPECT_THROW(copyService_->addCopy(validBookId_, "ACC-DATE-5", "2026-13-01"), lms::utils::ValidationException);
    EXPECT_THROW(copyService_->addCopy(validBookId_, "ACC-DATE-6", "2026-00-10"), lms::utils::ValidationException);
    EXPECT_THROW(copyService_->addCopy(validBookId_, "ACC-DATE-7", "2026-04-31"), lms::utils::ValidationException); // April has 30 days
}

TEST_F(BookCopyServiceTest, GetCopiesByBook) {
    copyService_->addCopy(validBookId_, "ACC-1");
    copyService_->addCopy(validBookId_, "ACC-2");

    auto copies = copyService_->getCopiesByBook(validBookId_);
    EXPECT_EQ(copies.size(), 2);

    EXPECT_THROW((void)copyService_->getCopiesByBook(9999), lms::utils::NotFoundException);
}

TEST_F(BookCopyServiceTest, SearchByAccessionNumber) {
    copyService_->addCopy(validBookId_, "ACC-TARGET");

    auto found = copyService_->searchByAccessionNumber("  ACC-TARGET  ");
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->getAccessionNumber(), "ACC-TARGET");

    EXPECT_FALSE(copyService_->searchByAccessionNumber("ACC-MISSING").has_value());
    EXPECT_THROW((void)copyService_->searchByAccessionNumber("   "), lms::utils::ValidationException);
}

TEST_F(BookCopyServiceTest, FilterCopiesByStatus) {
    copyService_->addCopy(validBookId_, "ACC-1", std::nullopt, lms::models::BookCopyStatus::Available);
    copyService_->addCopy(validBookId_, "ACC-2", std::nullopt, lms::models::BookCopyStatus::Damaged);
    copyService_->addCopy(validBookId_, "ACC-3", std::nullopt, lms::models::BookCopyStatus::Available);

    auto available = copyService_->filterCopiesByStatus(lms::models::BookCopyStatus::Available);
    EXPECT_EQ(available.size(), 2);

    auto damaged = copyService_->filterCopiesByStatus(lms::models::BookCopyStatus::Damaged);
    EXPECT_EQ(damaged.size(), 1);
}

TEST_F(BookCopyServiceTest, UpdateCopySafety) {
    int id = copyService_->addCopy(validBookId_, "ACC-OLD", "2026-01-01");

    // Valid update
    lms::models::BookCopy updated(id, validBookId_, "ACC-NEW", lms::models::BookCopyStatus::Lost, "2026-01-15");
    EXPECT_NO_THROW(copyService_->updateCopy(updated));

    auto retrieved = copyService_->getCopy(id);
    EXPECT_EQ(retrieved.getAccessionNumber(), "ACC-NEW");
    EXPECT_EQ(retrieved.getStatus(), lms::models::BookCopyStatus::Lost);

    // Reject manual transition to OnLoan
    lms::models::BookCopy onLoanUpdate(id, validBookId_, "ACC-NEW", lms::models::BookCopyStatus::OnLoan);
    EXPECT_THROW(copyService_->updateCopy(onLoanUpdate), lms::utils::ValidationException);

    // Reject update on nonexistent copy
    lms::models::BookCopy nonexistent(9999, validBookId_, "ACC-NONE", lms::models::BookCopyStatus::Available);
    EXPECT_THROW(copyService_->updateCopy(nonexistent), lms::utils::NotFoundException);
}

TEST_F(BookCopyServiceTest, UpdateStatusSafety) {
    int id = copyService_->addCopy(validBookId_, "ACC-STATUS-TEST");

    // Allowed status transitions
    EXPECT_NO_THROW(copyService_->updateStatus(id, lms::models::BookCopyStatus::Damaged));
    EXPECT_EQ(copyService_->getCopy(id).getStatus(), lms::models::BookCopyStatus::Damaged);

    // Disallowed manual transition to OnLoan
    EXPECT_THROW(copyService_->updateStatus(id, lms::models::BookCopyStatus::OnLoan), lms::utils::ValidationException);

    // Disallowed status change when copy is actively on loan
    connection_->execute("INSERT INTO members (id, full_name, joined_on, status) VALUES (1, 'Patron', '2026-01-01', 'Active');");
    connection_->execute("INSERT INTO loans (copy_id, member_id, issued_on, due_on, returned_on) "
                         "VALUES (" + std::to_string(id) + ", 1, '2026-01-01', '2026-01-15', NULL);");

    EXPECT_THROW(copyService_->updateStatus(id, lms::models::BookCopyStatus::Available), lms::utils::ValidationException);
    EXPECT_THROW(copyService_->updateStatus(id, lms::models::BookCopyStatus::Lost), lms::utils::ValidationException);

    // Disallowed updateCopy when on active loan
    lms::models::BookCopy modCopy(id, validBookId_, "ACC-STATUS-TEST", lms::models::BookCopyStatus::Withdrawn);
    EXPECT_THROW(copyService_->updateCopy(modCopy), lms::utils::ValidationException);
}

TEST_F(BookCopyServiceTest, DeleteCopySafety) {
    int id = copyService_->addCopy(validBookId_, "ACC-DELETE-ME");

    connection_->execute("INSERT INTO members (id, full_name, joined_on, status) VALUES (1, 'Patron', '2026-01-01', 'Active');");

    // 1. Cannot delete when active loan exists
    connection_->execute("INSERT INTO loans (id, copy_id, member_id, issued_on, due_on, returned_on) "
                         "VALUES (10, " + std::to_string(id) + ", 1, '2026-01-01', '2026-01-15', NULL);");
    EXPECT_THROW(copyService_->deleteCopy(id), lms::utils::ValidationException);

    // 2. Cannot delete even after loan is returned (loan history preservation)
    connection_->execute("UPDATE loans SET returned_on = '2026-01-10' WHERE id = 10;");
    EXPECT_THROW(copyService_->deleteCopy(id), lms::utils::ValidationException);

    // 3. New copy without loan history can be deleted safely
    int freshId = copyService_->addCopy(validBookId_, "ACC-FRESH-DEL");
    EXPECT_NO_THROW(copyService_->deleteCopy(freshId));
    EXPECT_THROW((void)copyService_->getCopy(freshId), lms::utils::NotFoundException);

    // 4. Missing copy throws NotFoundException
    EXPECT_THROW(copyService_->deleteCopy(9999), lms::utils::NotFoundException);
}

} // namespace
