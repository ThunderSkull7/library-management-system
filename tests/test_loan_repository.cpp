#include "database/DatabaseConnection.hpp"
#include "database/DatabaseException.hpp"
#include "database/SchemaManager.hpp"
#include "models/Book.hpp"
#include "models/BookCopy.hpp"
#include "models/BookCopyStatus.hpp"
#include "models/Loan.hpp"
#include "models/LoanPayment.hpp"
#include "models/Member.hpp"
#include "models/MemberStatus.hpp"
#include "repositories/BookCopyRepository.hpp"
#include "repositories/BookRepository.hpp"
#include "repositories/LoanRepository.hpp"
#include "repositories/MemberRepository.hpp"

#include <gtest/gtest.h>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace {

using namespace lms;

class LoanRepositoryTest : public ::testing::Test {
protected:
    void SetUp() override {
        connection_ = std::make_unique<database::DatabaseConnection>(":memory:");
        schemaManager_ = std::make_unique<database::SchemaManager>(*connection_);
        schemaManager_->initializeSchema();

        bookRepo_ = std::make_unique<repositories::BookRepository>(*connection_);
        copyRepo_ = std::make_unique<repositories::BookCopyRepository>(*connection_);
        memberRepo_ = std::make_unique<repositories::MemberRepository>(*connection_);
        loanRepo_ = std::make_unique<repositories::LoanRepository>(*connection_);

        // Seed helper data
        models::Book book("Structure and Interpretation of Computer Programs", "Harold Abelson");
        bookId_ = bookRepo_->addBook(book);

        models::BookCopy copy1(bookId_, "ACC-001", models::BookCopyStatus::Available);
        copy1Id_ = copyRepo_->addCopy(copy1);

        models::BookCopy copy2(bookId_, "ACC-002", models::BookCopyStatus::Available);
        copy2Id_ = copyRepo_->addCopy(copy2);

        models::Member m1("Alan Turing", "2026-01-01", "alan@turing.org");
        member1Id_ = memberRepo_->addMember(m1);

        models::Member m2("Grace Hopper", "2026-01-02", "grace@navy.mil");
        member2Id_ = memberRepo_->addMember(m2);
    }

    void TearDown() override {
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

    int bookId_{0};
    int copy1Id_{0};
    int copy2Id_{0};
    int member1Id_{0};
    int member2Id_{0};
};

TEST_F(LoanRepositoryTest, AddAndRetrieveLoanById) {
    models::Loan loan(copy1Id_, member1Id_, "2026-03-01", "2026-03-15");
    int id = loanRepo_->addLoan(loan);
    EXPECT_GT(id, 0);

    auto retrieved = loanRepo_->getLoanById(id);
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->getId(), id);
    EXPECT_EQ(retrieved->getCopyId(), copy1Id_);
    EXPECT_EQ(retrieved->getMemberId(), member1Id_);
    EXPECT_EQ(retrieved->getIssuedOn(), "2026-03-01");
    EXPECT_EQ(retrieved->getDueOn(), "2026-03-15");
    EXPECT_FALSE(retrieved->getReturnedOn().has_value());
    EXPECT_EQ(retrieved->getFineMinorUnits(), 0);
    EXPECT_FALSE(retrieved->isFinePaid());
    EXPECT_TRUE(retrieved->isActive());
}

TEST_F(LoanRepositoryTest, UpdateLoan) {
    models::Loan loan(copy1Id_, member1Id_, "2026-03-01", "2026-03-15");
    int id = loanRepo_->addLoan(loan);

    auto retrieved = loanRepo_->getLoanById(id);
    ASSERT_TRUE(retrieved.has_value());

    retrieved->setReturnedOn("2026-03-18");
    retrieved->setFineMinorUnits(600);
    retrieved->setFinePaid(true);

    EXPECT_TRUE(loanRepo_->updateLoan(*retrieved));

    auto updated = loanRepo_->getLoanById(id);
    ASSERT_TRUE(updated.has_value());
    ASSERT_TRUE(updated->getReturnedOn().has_value());
    EXPECT_EQ(*updated->getReturnedOn(), "2026-03-18");
    EXPECT_EQ(updated->getFineMinorUnits(), 600);
    EXPECT_TRUE(updated->isFinePaid());
    EXPECT_FALSE(updated->isActive());
}

TEST_F(LoanRepositoryTest, PartialUniqueIndexPreventsDoubleActiveCheckout) {
    models::Loan l1(copy1Id_, member1Id_, "2026-03-01", "2026-03-15");
    EXPECT_GT(loanRepo_->addLoan(l1), 0);

    models::Loan l2(copy1Id_, member2Id_, "2026-03-02", "2026-03-16");
    EXPECT_THROW((void)loanRepo_->addLoan(l2), database::DatabaseException);
}

TEST_F(LoanRepositoryTest, MultipleReturnedLoansForSameCopyAllowed) {
    models::Loan l1(copy1Id_, member1Id_, "2026-03-01", "2026-03-15");
    int id1 = loanRepo_->addLoan(l1);

    auto retrieved1 = loanRepo_->getLoanById(id1);
    ASSERT_TRUE(retrieved1.has_value());
    retrieved1->setReturnedOn("2026-03-14");
    EXPECT_TRUE(loanRepo_->updateLoan(*retrieved1));

    // Now copy1 is no longer active in loans, second loan should succeed
    models::Loan l2(copy1Id_, member2Id_, "2026-03-15", "2026-03-29");
    EXPECT_NO_THROW({
        int id2 = loanRepo_->addLoan(l2);
        EXPECT_GT(id2, 0);
    });
}

TEST_F(LoanRepositoryTest, GetActiveLoanByCopyId) {
    EXPECT_FALSE(loanRepo_->getActiveLoanByCopyId(copy1Id_).has_value());

    models::Loan l1(copy1Id_, member1Id_, "2026-03-01", "2026-03-15");
    int id = loanRepo_->addLoan(l1);

    auto active = loanRepo_->getActiveLoanByCopyId(copy1Id_);
    ASSERT_TRUE(active.has_value());
    EXPECT_EQ(active->getId(), id);

    active->setReturnedOn("2026-03-14");
    loanRepo_->updateLoan(*active);

    EXPECT_FALSE(loanRepo_->getActiveLoanByCopyId(copy1Id_).has_value());
}

TEST_F(LoanRepositoryTest, GetActiveLoansAndCountForMember) {
    EXPECT_EQ(loanRepo_->getActiveLoanCountForMember(member1Id_), 0);

    models::Loan l1(copy1Id_, member1Id_, "2026-03-01", "2026-03-15");
    models::Loan l2(copy2Id_, member1Id_, "2026-03-05", "2026-03-19");
    loanRepo_->addLoan(l1);
    loanRepo_->addLoan(l2);

    EXPECT_EQ(loanRepo_->getActiveLoanCountForMember(member1Id_), 2);
    auto activeLoans = loanRepo_->getActiveLoansByMemberId(member1Id_);
    EXPECT_EQ(activeLoans.size(), 2);
    EXPECT_TRUE(loanRepo_->hasActiveLoanForCopy(copy1Id_));
    EXPECT_TRUE(loanRepo_->hasActiveLoanForCopy(copy2Id_));
}

TEST_F(LoanRepositoryTest, GetLoanHistory) {
    models::Loan l1(copy1Id_, member1Id_, "2026-03-01", "2026-03-15");
    int id1 = loanRepo_->addLoan(l1);

    auto active1 = loanRepo_->getLoanById(id1);
    active1->setReturnedOn("2026-03-10");
    loanRepo_->updateLoan(*active1);

    models::Loan l2(copy1Id_, member1Id_, "2026-03-15", "2026-03-29");
    loanRepo_->addLoan(l2);

    auto historyMember = loanRepo_->getLoanHistoryByMemberId(member1Id_);
    EXPECT_EQ(historyMember.size(), 2);

    auto historyCopy = loanRepo_->getLoanHistoryByCopyId(copy1Id_);
    EXPECT_EQ(historyCopy.size(), 2);

    auto allLoans = loanRepo_->getAllLoans();
    EXPECT_EQ(allLoans.size(), 2);
}

TEST_F(LoanRepositoryTest, GetOverdueLoans) {
    // Due 2026-03-15
    models::Loan l1(copy1Id_, member1Id_, "2026-03-01", "2026-03-15");
    // Due 2026-03-25
    models::Loan l2(copy2Id_, member2Id_, "2026-03-11", "2026-03-25");
    loanRepo_->addLoan(l1);
    loanRepo_->addLoan(l2);

    // Current date is 2026-03-20: l1 is overdue, l2 is not
    auto overdue = loanRepo_->getOverdueLoans("2026-03-20");
    ASSERT_EQ(overdue.size(), 1);
    EXPECT_EQ(overdue[0].getCopyId(), copy1Id_);

    // Return l1 late
    auto r1 = loanRepo_->getLoanById(overdue[0].getId());
    r1->setReturnedOn("2026-03-20");
    loanRepo_->updateLoan(*r1);

    // Now overdue should be empty for 2026-03-20 because returned loans are not active
    EXPECT_TRUE(loanRepo_->getOverdueLoans("2026-03-20").empty());
}

TEST_F(LoanRepositoryTest, GetOutstandingFines) {
    models::Loan l1(copy1Id_, member1Id_, "2026-03-01", "2026-03-15");
    int id1 = loanRepo_->addLoan(l1);

    auto r1 = loanRepo_->getLoanById(id1);
    r1->setReturnedOn("2026-03-18");
    r1->setFineMinorUnits(600);
    r1->setFinePaid(false);
    loanRepo_->updateLoan(*r1);

    auto outstanding = loanRepo_->getOutstandingFines();
    ASSERT_EQ(outstanding.size(), 1);
    EXPECT_EQ(outstanding[0].getId(), id1);
    EXPECT_EQ(outstanding[0].getFineMinorUnits(), 600);

    // Mark paid
    r1->setFinePaid(true);
    loanRepo_->updateLoan(*r1);
    EXPECT_TRUE(loanRepo_->getOutstandingFines().empty());
}

TEST_F(LoanRepositoryTest, PaymentTracking) {
    models::Loan l1(copy1Id_, member1Id_, "2026-03-01", "2026-03-15");
    int loanId = loanRepo_->addLoan(l1);

    EXPECT_EQ(loanRepo_->getTotalPaidForLoan(loanId), 0);
    EXPECT_TRUE(loanRepo_->getPaymentsForLoan(loanId).empty());

    int p1 = loanRepo_->addPayment(loanId, 200, "2026-03-18");
    EXPECT_GT(p1, 0);
    EXPECT_EQ(loanRepo_->getTotalPaidForLoan(loanId), 200);

    int p2 = loanRepo_->addPayment(loanId, 400, "2026-03-19");
    EXPECT_GT(p2, 0);
    EXPECT_EQ(loanRepo_->getTotalPaidForLoan(loanId), 600);

    auto payments = loanRepo_->getPaymentsForLoan(loanId);
    ASSERT_EQ(payments.size(), 2);
    EXPECT_EQ(payments[0].getAmountMinorUnits(), 200);
    EXPECT_EQ(payments[0].getPaidOn(), "2026-03-18");
    EXPECT_EQ(payments[1].getAmountMinorUnits(), 400);
    EXPECT_EQ(payments[1].getPaidOn(), "2026-03-19");
}

} // namespace
