#include "database/DatabaseConnection.hpp"
#include "database/SchemaManager.hpp"
#include "models/Book.hpp"
#include "models/BookCopy.hpp"
#include "models/BookCopyStatus.hpp"
#include "models/LibraryPolicy.hpp"
#include "models/Loan.hpp"
#include "models/Member.hpp"
#include "models/MemberStatus.hpp"
#include "repositories/BookCopyRepository.hpp"
#include "repositories/BookRepository.hpp"
#include "repositories/LoanRepository.hpp"
#include "repositories/MemberRepository.hpp"
#include "services/LoanService.hpp"
#include "utils/ValidationException.hpp"

#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>

namespace {

using namespace lms;

class LoanServiceTest : public ::testing::Test {
protected:
    void SetUp() override {
        connection_ = std::make_unique<database::DatabaseConnection>(":memory:");
        schemaManager_ = std::make_unique<database::SchemaManager>(*connection_);
        schemaManager_->initializeSchema();

        bookRepo_ = std::make_unique<repositories::BookRepository>(*connection_);
        copyRepo_ = std::make_unique<repositories::BookCopyRepository>(*connection_);
        memberRepo_ = std::make_unique<repositories::MemberRepository>(*connection_);
        loanRepo_ = std::make_unique<repositories::LoanRepository>(*connection_);

        loanService_ = std::make_unique<services::LoanService>(
            *loanRepo_, *copyRepo_, *memberRepo_, *connection_);

        // Setup base data
        models::Book book("Modern Operating Systems", "Andrew S. Tanenbaum");
        bookId_ = bookRepo_->addBook(book);

        models::BookCopy copy1(bookId_, "ACC-101", models::BookCopyStatus::Available);
        copy1Id_ = copyRepo_->addCopy(copy1);

        models::BookCopy copy2(bookId_, "ACC-102", models::BookCopyStatus::Available);
        copy2Id_ = copyRepo_->addCopy(copy2);

        models::BookCopy copy3(bookId_, "ACC-103", models::BookCopyStatus::Available);
        copy3Id_ = copyRepo_->addCopy(copy3);

        models::Member m1("Linus Torvalds", "2026-01-01", "linus@kernel.org", std::nullopt, models::MemberStatus::Active);
        memberActiveId_ = memberRepo_->addMember(m1);

        models::Member m2("Suspended User", "2026-01-01", "susp@domain.com", std::nullopt, models::MemberStatus::Suspended);
        memberSuspendedId_ = memberRepo_->addMember(m2);

        models::Member m3("Closed User", "2026-01-01", "closed@domain.com", std::nullopt, models::MemberStatus::Closed);
        memberClosedId_ = memberRepo_->addMember(m3);
    }

    void TearDown() override {
        loanService_.reset();
        loanRepo_.reset();
        memberRepo_.reset();
        copyRepo_.reset();
        bookRepo_.reset();
        schemaManager_.reset();
        connection_.reset();
    }

    std::unique_ptr<database::DatabaseConnection> connection_;
    std::unique_ptr<database::SchemaManager> schemaManager_;
    std::unique_ptr<repositories::BookRepository> bookRepo_;
    std::unique_ptr<repositories::BookCopyRepository> copyRepo_;
    std::unique_ptr<repositories::MemberRepository> memberRepo_;
    std::unique_ptr<repositories::LoanRepository> loanRepo_;
    std::unique_ptr<services::LoanService> loanService_;

    int bookId_{0};
    int copy1Id_{0};
    int copy2Id_{0};
    int copy3Id_{0};
    int memberActiveId_{0};
    int memberSuspendedId_{0};
    int memberClosedId_{0};
};

TEST_F(LoanServiceTest, SuccessfulCheckout) {
    auto loan = loanService_->checkoutBook(memberActiveId_, copy1Id_, "2026-03-01");

    EXPECT_GT(loan.getId(), 0);
    EXPECT_EQ(loan.getCopyId(), copy1Id_);
    EXPECT_EQ(loan.getMemberId(), memberActiveId_);
    EXPECT_EQ(loan.getIssuedOn(), "2026-03-01");
    EXPECT_EQ(loan.getDueOn(), "2026-03-15"); // 14 days default
    EXPECT_FALSE(loan.getReturnedOn().has_value());
    EXPECT_EQ(loan.getFineMinorUnits(), 0);
    EXPECT_FALSE(loan.isFinePaid());
    EXPECT_TRUE(loan.isActive());

    // Physical copy status should transition to OnLoan
    auto copy = copyRepo_->getCopyById(copy1Id_);
    ASSERT_TRUE(copy.has_value());
    EXPECT_EQ(copy->getStatus(), models::BookCopyStatus::OnLoan);
}

TEST_F(LoanServiceTest, CheckoutRejectsInvalidDate) {
    EXPECT_THROW((void)loanService_->checkoutBook(memberActiveId_, copy1Id_, "invalid-date"),
                 utils::ValidationException);
}

TEST_F(LoanServiceTest, CheckoutRejectsMissingMemberOrCopy) {
    EXPECT_THROW((void)loanService_->checkoutBook(99999, copy1Id_, "2026-03-01"),
                 utils::NotFoundException);
    EXPECT_THROW((void)loanService_->checkoutBook(memberActiveId_, 99999, "2026-03-01"),
                 utils::NotFoundException);
}

TEST_F(LoanServiceTest, CheckoutRejectsInactiveMember) {
    EXPECT_THROW((void)loanService_->checkoutBook(memberSuspendedId_, copy1Id_, "2026-03-01"),
                 utils::ValidationException);
    EXPECT_THROW((void)loanService_->checkoutBook(memberClosedId_, copy1Id_, "2026-03-01"),
                 utils::ValidationException);
}

TEST_F(LoanServiceTest, CheckoutRejectsUnavailableCopy) {
    copyRepo_->updateCopyStatus(copy1Id_, models::BookCopyStatus::Lost);
    EXPECT_THROW((void)loanService_->checkoutBook(memberActiveId_, copy1Id_, "2026-03-01"),
                 utils::ValidationException);

    copyRepo_->updateCopyStatus(copy1Id_, models::BookCopyStatus::Damaged);
    EXPECT_THROW((void)loanService_->checkoutBook(memberActiveId_, copy1Id_, "2026-03-01"),
                 utils::ValidationException);

    copyRepo_->updateCopyStatus(copy1Id_, models::BookCopyStatus::Withdrawn);
    EXPECT_THROW((void)loanService_->checkoutBook(memberActiveId_, copy1Id_, "2026-03-01"),
                 utils::ValidationException);
}

TEST_F(LoanServiceTest, CheckoutRejectsAlreadyLoanedCopy) {
    (void)loanService_->checkoutBook(memberActiveId_, copy1Id_, "2026-03-01");

    // Second checkout of copy1 should fail
    EXPECT_THROW((void)loanService_->checkoutBook(memberActiveId_, copy1Id_, "2026-03-02"),
                 utils::ValidationException);
}

TEST_F(LoanServiceTest, CheckoutRejectsWhenMaxActiveLoansReached) {
    // Set max active loans to 2
    models::LibraryPolicy strictPolicy(14, 200, 2);
    loanService_->setPolicy(strictPolicy);

    (void)loanService_->checkoutBook(memberActiveId_, copy1Id_, "2026-03-01");
    (void)loanService_->checkoutBook(memberActiveId_, copy2Id_, "2026-03-01");

    // 3rd checkout should fail
    EXPECT_THROW((void)loanService_->checkoutBook(memberActiveId_, copy3Id_, "2026-03-01"),
                 utils::ValidationException);
}

TEST_F(LoanServiceTest, OnTimeReturnAssessesZeroFine) {
    (void)loanService_->checkoutBook(memberActiveId_, copy1Id_, "2026-03-01");

    // Returned on or before due date (2026-03-15)
    auto returnedLoan = loanService_->returnBook(copy1Id_, "2026-03-15");
    ASSERT_TRUE(returnedLoan.getReturnedOn().has_value());
    EXPECT_EQ(*returnedLoan.getReturnedOn(), "2026-03-15");
    EXPECT_EQ(returnedLoan.getFineMinorUnits(), 0);
    EXPECT_TRUE(returnedLoan.isFinePaid()); // Zero fine is automatically marked paid

    // Physical copy status should be Available again
    auto copy = copyRepo_->getCopyById(copy1Id_);
    ASSERT_TRUE(copy.has_value());
    EXPECT_EQ(copy->getStatus(), models::BookCopyStatus::Available);
}

TEST_F(LoanServiceTest, LateReturnCalculatesFineCorrectly) {
    (void)loanService_->checkoutBook(memberActiveId_, copy1Id_, "2026-03-01"); // Due 2026-03-15

    // Returned 3 days late (2026-03-18): 3 * 200 paise = 600 paise (₹6.00)
    auto returnedLoan = loanService_->returnBook(copy1Id_, "2026-03-18");
    ASSERT_TRUE(returnedLoan.getReturnedOn().has_value());
    EXPECT_EQ(*returnedLoan.getReturnedOn(), "2026-03-18");
    EXPECT_EQ(returnedLoan.getFineMinorUnits(), 600);
    EXPECT_FALSE(returnedLoan.isFinePaid());
    EXPECT_EQ(loanService_->getRemainingFine(returnedLoan.getId()), 600);
}

TEST_F(LoanServiceTest, GracePeriodCalculatesFineCorrectly) {
    // 2 days grace period
    models::LibraryPolicy gracePolicy(14, 200, 5, 2);
    loanService_->setPolicy(gracePolicy);

    (void)loanService_->checkoutBook(memberActiveId_, copy1Id_, "2026-03-01"); // Due 2026-03-15

    // Returned 2 days late (2026-03-17) within grace period: fine is 0
    auto returnedGrace = loanService_->returnBook(copy1Id_, "2026-03-17");
    EXPECT_EQ(returnedGrace.getFineMinorUnits(), 0);
    EXPECT_TRUE(returnedGrace.isFinePaid());

    // Check out copy2 and return 4 days late (2026-03-19): effective due was 2026-03-17, 2 days overdue = 400 paise
    (void)loanService_->checkoutBook(memberActiveId_, copy2Id_, "2026-03-01");
    auto returnedLate = loanService_->returnBook(copy2Id_, "2026-03-19");
    EXPECT_EQ(returnedLate.getFineMinorUnits(), 400);
}

TEST_F(LoanServiceTest, MaxFineCappedCorrectly) {
    // Max fine capped at ₹10.00 = 1000 paise
    models::LibraryPolicy capPolicy(14, 200, 5, 0, 1000);
    loanService_->setPolicy(capPolicy);

    (void)loanService_->checkoutBook(memberActiveId_, copy1Id_, "2026-03-01"); // Due 2026-03-15

    // Returned 20 days late (2026-04-04): 20 * 200 = 4000 paise, capped at 1000 paise
    auto returnedLoan = loanService_->returnBook(copy1Id_, "2026-04-04");
    EXPECT_EQ(returnedLoan.getFineMinorUnits(), 1000);
}

TEST_F(LoanServiceTest, ReturnRejectsInvalidDateOrNonexistentCopy) {
    EXPECT_THROW((void)loanService_->returnBook(99999, "2026-03-15"), utils::NotFoundException);

    (void)loanService_->checkoutBook(memberActiveId_, copy1Id_, "2026-03-01");
    EXPECT_THROW((void)loanService_->returnBook(copy1Id_, "bad-date"), utils::ValidationException);
}

TEST_F(LoanServiceTest, ReturnRejectsDateBeforeIssueDate) {
    (void)loanService_->checkoutBook(memberActiveId_, copy1Id_, "2026-03-10");
    EXPECT_THROW((void)loanService_->returnBook(copy1Id_, "2026-03-05"), utils::ValidationException);
}

TEST_F(LoanServiceTest, ReturnRejectsCopyWithoutActiveLoan) {
    EXPECT_THROW((void)loanService_->returnBook(copy1Id_, "2026-03-15"), utils::ValidationException);
}

TEST_F(LoanServiceTest, FinePaymentWorkflows) {
    (void)loanService_->checkoutBook(memberActiveId_, copy1Id_, "2026-03-01");
    auto returnedLoan = loanService_->returnBook(copy1Id_, "2026-03-20"); // 5 days late = 1000 paise
    int loanId = returnedLoan.getId();

    EXPECT_EQ(loanService_->getRemainingFine(loanId), 1000);

    // Negative / 0 payment rejected
    EXPECT_THROW(loanService_->payFine(loanId, 0, "2026-03-21"), utils::ValidationException);
    EXPECT_THROW(loanService_->payFine(loanId, -50, "2026-03-21"), utils::ValidationException);

    // Invalid date rejected
    EXPECT_THROW(loanService_->payFine(loanId, 500, "invalid"), utils::ValidationException);

    // Overpayment rejected (1200 > 1000)
    EXPECT_THROW(loanService_->payFine(loanId, 1200, "2026-03-21"), utils::ValidationException);

    // Partial payment 1: 400 paise
    loanService_->payFine(loanId, 400, "2026-03-21");
    EXPECT_EQ(loanService_->getRemainingFine(loanId), 600);
    auto loanAfterP1 = loanService_->getLoan(loanId);
    EXPECT_FALSE(loanAfterP1.isFinePaid());

    // Partial payment 2: 600 paise (completes payment)
    loanService_->payFine(loanId, 600, "2026-03-22");
    EXPECT_EQ(loanService_->getRemainingFine(loanId), 0);
    auto loanAfterP2 = loanService_->getLoan(loanId);
    EXPECT_TRUE(loanAfterP2.isFinePaid());

    // Subsequent payment rejected because fine is fully paid
    EXPECT_THROW(loanService_->payFine(loanId, 100, "2026-03-23"), utils::ValidationException);

    // Verify payment history
    auto payments = loanService_->getPaymentsForLoan(loanId);
    ASSERT_EQ(payments.size(), 2);
    EXPECT_EQ(payments[0].getAmountMinorUnits(), 400);
    EXPECT_EQ(payments[0].getPaidOn(), "2026-03-21");
    EXPECT_EQ(payments[1].getAmountMinorUnits(), 600);
    EXPECT_EQ(payments[1].getPaidOn(), "2026-03-22");
}

TEST_F(LoanServiceTest, PayFineRejectsActiveLoan) {
    auto activeLoan = loanService_->checkoutBook(memberActiveId_, copy1Id_, "2026-03-01");
    EXPECT_THROW(loanService_->payFine(activeLoan.getId(), 200, "2026-03-05"), utils::ValidationException);
}

TEST_F(LoanServiceTest, PayFineRejectsZeroAssessedFine) {
    (void)loanService_->checkoutBook(memberActiveId_, copy1Id_, "2026-03-01");
    auto returnedLoan = loanService_->returnBook(copy1Id_, "2026-03-10"); // On-time, fine = 0

    EXPECT_THROW(loanService_->payFine(returnedLoan.getId(), 100, "2026-03-11"), utils::ValidationException);
}

TEST_F(LoanServiceTest, QueryMethods) {
    auto l1 = loanService_->checkoutBook(memberActiveId_, copy1Id_, "2026-03-01");
    auto l2 = loanService_->checkoutBook(memberActiveId_, copy2Id_, "2026-03-01");

    EXPECT_EQ(loanService_->getAllLoans().size(), 2);
    EXPECT_EQ(loanService_->getActiveLoansForMember(memberActiveId_).size(), 2);

    loanService_->returnBook(copy1Id_, "2026-03-18"); // 3 days late -> 600 fine

    EXPECT_EQ(loanService_->getActiveLoansForMember(memberActiveId_).size(), 1);
    EXPECT_EQ(loanService_->getLoanHistoryForMember(memberActiveId_).size(), 2);
    EXPECT_EQ(loanService_->getLoanHistoryForCopy(copy1Id_).size(), 1);

    auto overdue = loanService_->getOverdueLoans("2026-03-20");
    ASSERT_EQ(overdue.size(), 1);
    EXPECT_EQ(overdue[0].getId(), l2.getId());

    auto outstandingFines = loanService_->getOutstandingFines();
    ASSERT_EQ(outstandingFines.size(), 1);
    EXPECT_EQ(outstandingFines[0].getId(), l1.getId());
}

} // namespace
