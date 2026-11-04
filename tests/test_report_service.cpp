#include "database/DatabaseConnection.hpp"
#include "database/SchemaManager.hpp"
#include "models/Book.hpp"
#include "models/BookCopy.hpp"
#include "models/BookCopyStatus.hpp"
#include "models/LibraryPolicy.hpp"
#include "models/Member.hpp"
#include "models/MemberStatus.hpp"
#include "models/ReportModels.hpp"
#include "repositories/BookCopyRepository.hpp"
#include "repositories/BookRepository.hpp"
#include "repositories/LoanRepository.hpp"
#include "repositories/MemberRepository.hpp"
#include "services/BookCopyService.hpp"
#include "services/BookService.hpp"
#include "services/LibraryReportService.hpp"
#include "services/LoanService.hpp"
#include "services/MemberService.hpp"
#include "utils/ValidationException.hpp"

#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>

namespace {

using namespace lms;

class LibraryReportServiceTest : public ::testing::Test {
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

        reportService_ = std::make_unique<services::LibraryReportService>(
            *bookRepo_, *copyRepo_, *memberRepo_, *loanRepo_);
    }

    std::unique_ptr<database::DatabaseConnection> connection_;
    std::unique_ptr<database::SchemaManager> schemaManager_;
    std::unique_ptr<repositories::BookRepository> bookRepo_;
    std::unique_ptr<repositories::BookCopyRepository> copyRepo_;
    std::unique_ptr<repositories::MemberRepository> memberRepo_;
    std::unique_ptr<repositories::LoanRepository> loanRepo_;
    std::unique_ptr<services::LoanService> loanService_;
    std::unique_ptr<services::LibraryReportService> reportService_;
};

TEST_F(LibraryReportServiceTest, InventoryReportEmptyDatabase) {
    const auto report = reportService_->generateInventoryReport();
    EXPECT_EQ(report.totalTitles, 0);
    EXPECT_EQ(report.totalCopies, 0);
    EXPECT_EQ(report.availableCopies, 0);
    EXPECT_EQ(report.onLoanCopies, 0);
    EXPECT_EQ(report.lostCopies, 0);
    EXPECT_EQ(report.damagedCopies, 0);
    EXPECT_EQ(report.withdrawnCopies, 0);
}

TEST_F(LibraryReportServiceTest, InventoryReportWithCopiesInVariousStatuses) {
    models::Book b1("Book One", "Author A");
    const int b1Id = bookRepo_->addBook(b1);

    models::Book b2("Book Two", "Author B");
    const int b2Id = bookRepo_->addBook(b2);

    copyRepo_->addCopy(models::BookCopy(b1Id, "ACC001", models::BookCopyStatus::Available));
    copyRepo_->addCopy(models::BookCopy(b1Id, "ACC002", models::BookCopyStatus::OnLoan));
    copyRepo_->addCopy(models::BookCopy(b2Id, "ACC003", models::BookCopyStatus::Available));
    copyRepo_->addCopy(models::BookCopy(b2Id, "ACC004", models::BookCopyStatus::Damaged));
    copyRepo_->addCopy(models::BookCopy(b2Id, "ACC005", models::BookCopyStatus::Lost));
    copyRepo_->addCopy(models::BookCopy(b2Id, "ACC006", models::BookCopyStatus::Withdrawn));

    const auto report = reportService_->generateInventoryReport();
    EXPECT_EQ(report.totalTitles, 2);
    EXPECT_EQ(report.totalCopies, 6);
    EXPECT_EQ(report.availableCopies, 2);
    EXPECT_EQ(report.onLoanCopies, 1);
    EXPECT_EQ(report.damagedCopies, 1);
    EXPECT_EQ(report.lostCopies, 1);
    EXPECT_EQ(report.withdrawnCopies, 1);
}

TEST_F(LibraryReportServiceTest, CirculationReportEmptyDatabase) {
    const auto report = reportService_->generateCirculationReport("2026-01-01");
    EXPECT_EQ(report.totalLoans, 0);
    EXPECT_EQ(report.activeLoans, 0);
    EXPECT_EQ(report.returnedLoans, 0);
    EXPECT_EQ(report.overdueLoans, 0);
    EXPECT_FALSE(report.rangeStart.has_value());
    EXPECT_FALSE(report.rangeEnd.has_value());
}

TEST_F(LibraryReportServiceTest, CirculationReportWithActiveAndReturnedLoans) {
    models::Book b1("Book One", "Author A");
    const int b1Id = bookRepo_->addBook(b1);
    const int c1Id = copyRepo_->addCopy(models::BookCopy(b1Id, "ACC001", models::BookCopyStatus::Available));
    const int c2Id = copyRepo_->addCopy(models::BookCopy(b1Id, "ACC002", models::BookCopyStatus::Available));

    models::Member m1("Alice Smith", "2026-01-01");
    const int m1Id = memberRepo_->addMember(m1);

    // Active loan (issued 2026-01-01, due 2026-01-15)
    loanService_->checkoutBook(m1Id, c1Id, "2026-01-01");

    // Returned loan (issued 2026-01-05, returned 2026-01-10)
    loanService_->checkoutBook(m1Id, c2Id, "2026-01-05");
    loanService_->returnBook(c2Id, "2026-01-10");

    // As of 2026-01-12: 2 total, 1 active, 1 returned, 0 overdue
    const auto report1 = reportService_->generateCirculationReport("2026-01-12");
    EXPECT_EQ(report1.totalLoans, 2);
    EXPECT_EQ(report1.activeLoans, 1);
    EXPECT_EQ(report1.returnedLoans, 1);
    EXPECT_EQ(report1.overdueLoans, 0);

    // As of 2026-01-20: loan 1 is overdue! (due was 2026-01-15)
    const auto report2 = reportService_->generateCirculationReport("2026-01-20");
    EXPECT_EQ(report2.totalLoans, 2);
    EXPECT_EQ(report2.activeLoans, 1);
    EXPECT_EQ(report2.returnedLoans, 1);
    EXPECT_EQ(report2.overdueLoans, 1);
}

TEST_F(LibraryReportServiceTest, CirculationReportDateFiltering) {
    models::Book b("Book One", "Author A");
    const int bId = bookRepo_->addBook(b);
    const int c1 = copyRepo_->addCopy(models::BookCopy(bId, "ACC001", models::BookCopyStatus::Available));
    const int c2 = copyRepo_->addCopy(models::BookCopy(bId, "ACC002", models::BookCopyStatus::Available));

    models::Member m("Bob Johnson", "2026-01-01");
    const int mId = memberRepo_->addMember(m);

    loanService_->checkoutBook(mId, c1, "2026-01-05");
    loanService_->checkoutBook(mId, c2, "2026-02-10");

    // Filter January
    const auto janReport = reportService_->generateCirculationReport("2026-03-01", "2026-01-01", "2026-01-31");
    EXPECT_EQ(janReport.totalLoans, 1);

    // Filter February
    const auto febReport = reportService_->generateCirculationReport("2026-03-01", "2026-02-01", "2026-02-28");
    EXPECT_EQ(febReport.totalLoans, 1);

    // Invalid date formats or inverted range
    EXPECT_THROW((void)reportService_->generateCirculationReport("2026-03-01", "invalid", "2026-01-31"), utils::ValidationException);
    EXPECT_THROW((void)reportService_->generateCirculationReport("2026-03-01", "2026-02-01", "2026-01-01"), utils::ValidationException);
}

TEST_F(LibraryReportServiceTest, MemberActivityReportAndTopBorrowers) {
    models::Book b("Book One", "Author A");
    const int bId = bookRepo_->addBook(b);
    const int c1 = copyRepo_->addCopy(models::BookCopy(bId, "ACC001", models::BookCopyStatus::Available));
    const int c2 = copyRepo_->addCopy(models::BookCopy(bId, "ACC002", models::BookCopyStatus::Available));
    const int c3 = copyRepo_->addCopy(models::BookCopy(bId, "ACC003", models::BookCopyStatus::Available));

    models::Member m1("Alice", "2026-01-01");
    const int m1Id = memberRepo_->addMember(m1);
    models::Member m2("Bob", "2026-01-01");
    const int m2Id = memberRepo_->addMember(m2);

    // Alice borrows 2 books
    loanService_->checkoutBook(m1Id, c1, "2026-01-01");
    loanService_->checkoutBook(m1Id, c2, "2026-01-02");

    // Bob borrows 1 book
    loanService_->checkoutBook(m2Id, c3, "2026-01-01");

    const auto report = reportService_->generateMemberActivityReport(10);
    EXPECT_EQ(report.totalMembers, 2);
    EXPECT_EQ(report.activeMembers, 2);
    EXPECT_EQ(report.membersWithActiveLoans, 2);
    ASSERT_EQ(report.topBorrowers.size(), 2u);

    // Alice has 2 active loans, Bob has 1 -> Alice is first
    EXPECT_EQ(report.topBorrowers[0].memberId, m1Id);
    EXPECT_EQ(report.topBorrowers[0].activeLoanCount, 2);
    EXPECT_EQ(report.topBorrowers[1].memberId, m2Id);
    EXPECT_EQ(report.topBorrowers[1].activeLoanCount, 1);

    // Verify limit of 1
    const auto top1Report = reportService_->generateMemberActivityReport(1);
    EXPECT_EQ(top1Report.topBorrowers.size(), 1u);
    EXPECT_EQ(top1Report.topBorrowers[0].memberId, m1Id);
}

TEST_F(LibraryReportServiceTest, FineReportAndPayments) {
    models::Book b("Book One", "Author A");
    const int bId = bookRepo_->addBook(b);
    const int c1 = copyRepo_->addCopy(models::BookCopy(bId, "ACC001", models::BookCopyStatus::Available));

    models::Member m("Charlie", "2026-01-01");
    const int mId = memberRepo_->addMember(m);

    // Borrow and return late: 5 days late = 1000 paise (₹10.00)
    loanService_->checkoutBook(mId, c1, "2026-01-01"); // Due 2026-01-15
    const auto returnedLoan = loanService_->returnBook(c1, "2026-01-20");
    EXPECT_EQ(returnedLoan.getFineMinorUnits(), 1000);

    auto fineReport1 = reportService_->generateFineReport();
    EXPECT_EQ(fineReport1.totalFinesAssessedMinorUnits, 1000);
    EXPECT_EQ(fineReport1.totalPaymentsReceivedMinorUnits, 0);
    EXPECT_EQ(fineReport1.outstandingBalanceMinorUnits, 1000);
    EXPECT_EQ(fineReport1.unpaidFineCount, 1);
    EXPECT_EQ(fineReport1.fullyPaidFineCount, 0);

    // Pay partial 400 paise
    loanService_->payFine(returnedLoan.getId(), 400, "2026-01-26");

    auto fineReport2 = reportService_->generateFineReport();
    EXPECT_EQ(fineReport2.totalFinesAssessedMinorUnits, 1000);
    EXPECT_EQ(fineReport2.totalPaymentsReceivedMinorUnits, 400);
    EXPECT_EQ(fineReport2.outstandingBalanceMinorUnits, 600);
    EXPECT_EQ(fineReport2.unpaidFineCount, 1);
    EXPECT_EQ(fineReport2.fullyPaidFineCount, 0);

    // Pay remaining 600 paise
    loanService_->payFine(returnedLoan.getId(), 600, "2026-01-27");

    auto fineReport3 = reportService_->generateFineReport();
    EXPECT_EQ(fineReport3.totalFinesAssessedMinorUnits, 1000);
    EXPECT_EQ(fineReport3.totalPaymentsReceivedMinorUnits, 1000);
    EXPECT_EQ(fineReport3.outstandingBalanceMinorUnits, 0);
    EXPECT_EQ(fineReport3.unpaidFineCount, 0);
    EXPECT_EQ(fineReport3.fullyPaidFineCount, 1);
}

TEST_F(LibraryReportServiceTest, OverdueReportCalculationsAndCap) {
    models::Book b("Book One", "Author A");
    const int bId = bookRepo_->addBook(b);
    const int c1 = copyRepo_->addCopy(models::BookCopy(bId, "ACC001", models::BookCopyStatus::Available));

    models::Member m("David", "2026-01-01");
    const int mId = memberRepo_->addMember(m);

    // Checkout issued 2026-01-01 -> Due 2026-01-15
    loanService_->checkoutBook(mId, c1, "2026-01-01");

    // Check as of 2026-01-14: not overdue yet
    const auto repNotOverdue = reportService_->generateOverdueReport("2026-01-14");
    EXPECT_EQ(repNotOverdue.items.size(), 0u);
    EXPECT_EQ(repNotOverdue.totalOverdueLoans, 0);

    // Check as of 2026-01-20: 5 days overdue @ ₹2.00/day = 1000 paise
    const auto repOverdue5Days = reportService_->generateOverdueReport("2026-01-20");
    EXPECT_EQ(repOverdue5Days.items.size(), 1u);
    EXPECT_EQ(repOverdue5Days.totalOverdueLoans, 1);
    EXPECT_EQ(repOverdue5Days.items[0].overdueDays, 5);
    EXPECT_EQ(repOverdue5Days.items[0].assessedFineMinorUnits, 1000);

    // Now test policy cap:
    models::LibraryPolicy policyWithCap;
    policyWithCap.setMaxFineMinorUnits(5000); // Max ₹50.00
    reportService_->setPolicy(policyWithCap);

    // Check as of 2026-04-01: over 70 days overdue. Max fine is capped at ₹50.00 (5000 paise).
    const auto repCapped = reportService_->generateOverdueReport("2026-04-01");
    EXPECT_EQ(repCapped.items.size(), 1u);
    EXPECT_GT(repCapped.items[0].overdueDays, 50);
    EXPECT_EQ(repCapped.items[0].assessedFineMinorUnits, 5000);
}

// ============================================================================
// Comprehensive End-to-End Workflow Validation (Workflows A, B, C, D)
// ============================================================================

class EndToEndWorkflowTest : public ::testing::Test {
protected:
    void SetUp() override {
        connection_ = std::make_unique<database::DatabaseConnection>(":memory:");
        schemaManager_ = std::make_unique<database::SchemaManager>(*connection_);
        schemaManager_->initializeSchema();

        bookRepo_ = std::make_unique<repositories::BookRepository>(*connection_);
        copyRepo_ = std::make_unique<repositories::BookCopyRepository>(*connection_);
        memberRepo_ = std::make_unique<repositories::MemberRepository>(*connection_);
        loanRepo_ = std::make_unique<repositories::LoanRepository>(*connection_);

        bookService_ = std::make_unique<services::BookService>(*bookRepo_);
        copyService_ = std::make_unique<services::BookCopyService>(*copyRepo_, *bookRepo_);
        memberService_ = std::make_unique<services::MemberService>(*memberRepo_);
        loanService_ = std::make_unique<services::LoanService>(*loanRepo_, *copyRepo_, *memberRepo_, *connection_);
        reportService_ = std::make_unique<services::LibraryReportService>(*bookRepo_, *copyRepo_, *memberRepo_, *loanRepo_);
    }

    std::unique_ptr<database::DatabaseConnection> connection_;
    std::unique_ptr<database::SchemaManager> schemaManager_;
    std::unique_ptr<repositories::BookRepository> bookRepo_;
    std::unique_ptr<repositories::BookCopyRepository> copyRepo_;
    std::unique_ptr<repositories::MemberRepository> memberRepo_;
    std::unique_ptr<repositories::LoanRepository> loanRepo_;
    std::unique_ptr<services::BookService> bookService_;
    std::unique_ptr<services::BookCopyService> copyService_;
    std::unique_ptr<services::MemberService> memberService_;
    std::unique_ptr<services::LoanService> loanService_;
    std::unique_ptr<services::LibraryReportService> reportService_;
};

TEST_F(EndToEndWorkflowTest, ExecuteWorkflows_A_B_C_D) {
    // -------------------------------------------------------------------------
    // Workflow A: Book Creation, Copies, Member Registration, and Checkout
    // -------------------------------------------------------------------------
    models::Book cleanArch("Clean Architecture", "Robert C. Martin", "978-0134494166", "Prentice Hall", 2017, "Software");
    int bookId = bookService_->addBook(cleanArch);
    EXPECT_GT(bookId, 0);

    int copy1Id = copyService_->addCopy(bookId, "ACC-CA-01");
    int copy2Id = copyService_->addCopy(bookId, "ACC-CA-02");
    EXPECT_GT(copy1Id, 0);
    EXPECT_GT(copy2Id, 0);

    EXPECT_EQ(copyService_->getAvailableCopiesCountForBook(bookId), 2);

    models::Member member("Grace Hopper", "2026-01-01", "grace@navy.mil", "555-0199");
    int memberId = memberService_->registerMember(member);
    EXPECT_GT(memberId, 0);

    models::Loan loan1 = loanService_->checkoutBook(memberId, copy1Id, "2026-01-01");
    EXPECT_EQ(loan1.getMemberId(), memberId);
    EXPECT_EQ(loan1.getCopyId(), copy1Id);

    // Verify copy1 is now OnLoan and available count dropped to 1
    auto fetchedCopy1 = copyService_->getCopy(copy1Id);
    EXPECT_EQ(fetchedCopy1.getStatus(), models::BookCopyStatus::OnLoan);
    EXPECT_EQ(copyService_->getAvailableCopiesCountForBook(bookId), 1);

    // Verify active loan appears in member history and queries
    auto activeLoans = loanService_->getActiveLoansForMember(memberId);
    ASSERT_EQ(activeLoans.size(), 1u);
    EXPECT_EQ(activeLoans[0].getId(), loan1.getId());

    // -------------------------------------------------------------------------
    // Workflow B: Negative Validation Constraints and Concurrency Safety
    // -------------------------------------------------------------------------
    // Attempt checkout for already-loaned copy
    EXPECT_THROW((void)loanService_->checkoutBook(memberId, copy1Id, "2026-01-02"), utils::ValidationException);

    // Suspend member
    memberService_->updateStatus(memberId, models::MemberStatus::Suspended);
    EXPECT_EQ(memberService_->getMember(memberId).getStatus(), models::MemberStatus::Suspended);

    // Attempt checkout for suspended member
    EXPECT_THROW((void)loanService_->checkoutBook(memberId, copy2Id, "2026-01-02"), utils::ValidationException);

    // Verify copy2 remains untouched and Available
    auto fetchedCopy2 = copyService_->getCopy(copy2Id);
    EXPECT_EQ(fetchedCopy2.getStatus(), models::BookCopyStatus::Available);

    // -------------------------------------------------------------------------
    // Workflow C: Overdue Return, Fine Assessment, and Fine Payment
    // -------------------------------------------------------------------------
    // Reactivate member
    memberService_->updateStatus(memberId, models::MemberStatus::Active);

    // Check out copy2 on 2026-01-01 -> Due on 2026-01-15
    models::Loan loan2 = loanService_->checkoutBook(memberId, copy2Id, "2026-01-01");

    // Return copy2 on 2026-01-25 (10 days late @ 200 paise/day = 2000 paise = ₹20.00)
    models::Loan returnedLoan2 = loanService_->returnBook(copy2Id, "2026-01-25");
    EXPECT_EQ(returnedLoan2.getFineMinorUnits(), 2000);
    EXPECT_FALSE(returnedLoan2.isFinePaid());

    // Verify copy2 returned to Available
    EXPECT_EQ(copyService_->getCopy(copy2Id).getStatus(), models::BookCopyStatus::Available);

    // Partial fine payment: pay 800 paise (₹8.00)
    loanService_->payFine(returnedLoan2.getId(), 800, "2026-01-26");
    EXPECT_FALSE(loanService_->getLoan(returnedLoan2.getId()).isFinePaid());

    // Final fine payment: pay remaining 1200 paise (₹12.00)
    loanService_->payFine(returnedLoan2.getId(), 1200, "2026-01-27");
    EXPECT_TRUE(loanService_->getLoan(returnedLoan2.getId()).isFinePaid());

    // -------------------------------------------------------------------------
    // Workflow D: Reporting Subsystem Verification
    // -------------------------------------------------------------------------
    // Inventory Report
    auto invReport = reportService_->generateInventoryReport();
    EXPECT_EQ(invReport.totalTitles, 1);
    EXPECT_EQ(invReport.totalCopies, 2);
    EXPECT_EQ(invReport.availableCopies, 1); // copy2
    EXPECT_EQ(invReport.onLoanCopies, 1);    // copy1

    // Circulation Report as of 2026-01-30: 2 total loans (loan1 active overdue, loan2 returned)
    auto circReport = reportService_->generateCirculationReport("2026-01-30");
    EXPECT_EQ(circReport.totalLoans, 2);
    EXPECT_EQ(circReport.activeLoans, 1);
    EXPECT_EQ(circReport.returnedLoans, 1);
    EXPECT_EQ(circReport.overdueLoans, 1); // loan1 was due 2026-01-15, as of Jan 30 it's overdue

    // Overdue Report as of 2026-01-30
    auto overdueReport = reportService_->generateOverdueReport("2026-01-30");
    EXPECT_EQ(overdueReport.totalOverdueLoans, 1);
    ASSERT_EQ(overdueReport.items.size(), 1u);
    EXPECT_EQ(overdueReport.items[0].loanId, loan1.getId());
    EXPECT_EQ(overdueReport.items[0].overdueDays, 15);
    EXPECT_EQ(overdueReport.items[0].assessedFineMinorUnits, 15 * 200); // 3000 paise

    // Fine Report: 2000 assessed from returned loan2, 2000 paid, 0 outstanding
    auto fineReport = reportService_->generateFineReport();
    EXPECT_EQ(fineReport.totalFinesAssessedMinorUnits, 2000);
    EXPECT_EQ(fineReport.totalPaymentsReceivedMinorUnits, 2000);
    EXPECT_EQ(fineReport.outstandingBalanceMinorUnits, 0);
    EXPECT_EQ(fineReport.fullyPaidFineCount, 1);
    EXPECT_EQ(fineReport.unpaidFineCount, 0);
}

} // namespace
