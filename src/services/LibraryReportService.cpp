#include "services/LibraryReportService.hpp"
#include "utils/DateUtils.hpp"
#include "utils/ValidationException.hpp"

#include <algorithm>
#include <cstdint>

namespace lms::services {

LibraryReportService::LibraryReportService(repositories::BookRepository& bookRepo,
                                           repositories::BookCopyRepository& copyRepo,
                                           repositories::MemberRepository& memberRepo,
                                           repositories::LoanRepository& loanRepo,
                                           models::LibraryPolicy policy)
    : bookRepo_(bookRepo),
      copyRepo_(copyRepo),
      memberRepo_(memberRepo),
      loanRepo_(loanRepo),
      policy_(policy) {}

int LibraryReportService::calculateOverdueFine(const std::string& dueDate, const std::string& asOfDate) const {
    std::string effectiveDue = utils::DateUtils::addDays(dueDate, policy_.getGracePeriodDays());
    int diffDays = utils::DateUtils::daysBetween(effectiveDue, asOfDate);
    int overdueDays = std::max(0, diffDays);

    if (overdueDays == 0 || policy_.getFinePerDayMinorUnits() == 0) {
        return 0;
    }

    int64_t rawFine = static_cast<int64_t>(overdueDays) * policy_.getFinePerDayMinorUnits();
    if (policy_.getMaxFineMinorUnits().has_value() && rawFine > *policy_.getMaxFineMinorUnits()) {
        rawFine = *policy_.getMaxFineMinorUnits();
    }

    return static_cast<int>(rawFine);
}

models::InventoryReport LibraryReportService::generateInventoryReport() {
    models::InventoryReport report;

    auto books = bookRepo_.getAllBooks();
    report.totalTitles = static_cast<int>(books.size());

    auto copies = copyRepo_.getAllCopies();
    report.totalCopies = static_cast<int>(copies.size());

    for (const auto& copy : copies) {
        switch (copy.getStatus()) {
            case models::BookCopyStatus::Available:
                report.availableCopies++;
                break;
            case models::BookCopyStatus::OnLoan:
                report.onLoanCopies++;
                break;
            case models::BookCopyStatus::Lost:
                report.lostCopies++;
                break;
            case models::BookCopyStatus::Damaged:
                report.damagedCopies++;
                break;
            case models::BookCopyStatus::Withdrawn:
                report.withdrawnCopies++;
                break;
        }
    }

    return report;
}

models::CirculationReport LibraryReportService::generateCirculationReport(
    const std::string& asOfDate,
    const std::optional<std::string>& rangeStart,
    const std::optional<std::string>& rangeEnd) {
    if (!utils::DateUtils::isValidIsoDate(asOfDate)) {
        throw utils::ValidationException("As-of date must be a valid ISO date in YYYY-MM-DD format. Received: '" + asOfDate + "'");
    }

    models::CirculationReport report;
    std::vector<models::Loan> targetLoans;

    if (rangeStart.has_value() || rangeEnd.has_value()) {
        if (!rangeStart.has_value() || !rangeEnd.has_value()) {
            throw utils::ValidationException("Both range start and range end dates must be provided for range filtering.");
        }
        if (!utils::DateUtils::isValidIsoDate(*rangeStart)) {
            throw utils::ValidationException("Range start date must be a valid ISO date in YYYY-MM-DD format. Received: '" + *rangeStart + "'");
        }
        if (!utils::DateUtils::isValidIsoDate(*rangeEnd)) {
            throw utils::ValidationException("Range end date must be a valid ISO date in YYYY-MM-DD format. Received: '" + *rangeEnd + "'");
        }
        if (utils::DateUtils::isDateAfter(*rangeStart, *rangeEnd)) {
            throw utils::ValidationException("Range start date ('" + *rangeStart + "') cannot be after range end date ('" + *rangeEnd + "').");
        }

        report.rangeStart = rangeStart;
        report.rangeEnd = rangeEnd;
        targetLoans = loanRepo_.getLoansIssuedBetween(*rangeStart, *rangeEnd);
        report.rangeLoans = targetLoans;
    } else {
        targetLoans = loanRepo_.getAllLoans();
    }

    report.totalLoans = static_cast<int>(targetLoans.size());

    for (const auto& loan : targetLoans) {
        if (loan.isActive()) {
            report.activeLoans++;
            if (utils::DateUtils::isDateBefore(loan.getDueOn(), asOfDate)) {
                report.overdueLoans++;
            }
        } else {
            report.returnedLoans++;
        }
    }

    return report;
}

models::MemberActivityReport LibraryReportService::generateMemberActivityReport(int topBorrowersLimit) {
    models::MemberActivityReport report;

    auto members = memberRepo_.getAllMembers();
    report.totalMembers = static_cast<int>(members.size());

    for (const auto& member : members) {
        switch (member.getStatus()) {
            case models::MemberStatus::Active:
                report.activeMembers++;
                break;
            case models::MemberStatus::Suspended:
                report.suspendedMembers++;
                break;
            case models::MemberStatus::Closed:
                report.closedMembers++;
                break;
        }

        int activeCount = loanRepo_.getActiveLoanCountForMember(member.getId());
        if (activeCount > 0) {
            report.membersWithActiveLoans++;
        }

        report.topBorrowers.push_back({
            member.getId(),
            member.getFullName(),
            models::toString(member.getStatus()),
            activeCount
        });
    }

    // Sort top borrowers descending by active loan count, then by memberId
    std::sort(report.topBorrowers.begin(), report.topBorrowers.end(),
              [](const models::MemberLoanStats& a, const models::MemberLoanStats& b) {
                  if (a.activeLoanCount != b.activeLoanCount) {
                      return a.activeLoanCount > b.activeLoanCount;
                  }
                  return a.memberId < b.memberId;
              });

    if (topBorrowersLimit > 0 && static_cast<int>(report.topBorrowers.size()) > topBorrowersLimit) {
        report.topBorrowers.resize(static_cast<size_t>(topBorrowersLimit));
    }

    return report;
}

models::FineReport LibraryReportService::generateFineReport() {
    models::FineReport report;

    auto allLoans = loanRepo_.getAllLoans();
    for (const auto& loan : allLoans) {
        if (loan.getFineMinorUnits() > 0) {
            report.totalFinesAssessedMinorUnits += loan.getFineMinorUnits();
            if (loan.isFinePaid()) {
                report.fullyPaidFineCount++;
            } else {
                report.unpaidFineCount++;
            }
        }
    }

    report.totalPaymentsReceivedMinorUnits = loanRepo_.getTotalPaymentsReceivedAllLoans();
    int balance = report.totalFinesAssessedMinorUnits - report.totalPaymentsReceivedMinorUnits;
    report.outstandingBalanceMinorUnits = std::max(0, balance);

    return report;
}

models::OverdueReport LibraryReportService::generateOverdueReport(const std::string& asOfDate) {
    if (!utils::DateUtils::isValidIsoDate(asOfDate)) {
        throw utils::ValidationException("As-of date must be a valid ISO date in YYYY-MM-DD format. Received: '" + asOfDate + "'");
    }

    models::OverdueReport report;
    report.asOfDate = asOfDate;

    auto overdueLoans = loanRepo_.getOverdueLoans(asOfDate);
    report.totalOverdueLoans = static_cast<int>(overdueLoans.size());

    for (const auto& loan : overdueLoans) {
        models::OverdueLoanDetail detail;
        detail.loanId = loan.getId();
        detail.memberId = loan.getMemberId();
        detail.copyId = loan.getCopyId();
        detail.issuedOn = loan.getIssuedOn();
        detail.dueOn = loan.getDueOn();
        detail.isActive = loan.isActive();
        detail.overdueDays = std::max(0, utils::DateUtils::daysBetween(loan.getDueOn(), asOfDate));
        detail.assessedFineMinorUnits = calculateOverdueFine(loan.getDueOn(), asOfDate);

        auto member = memberRepo_.getMemberById(loan.getMemberId());
        detail.memberName = member ? member->getFullName() : ("Member #" + std::to_string(loan.getMemberId()));

        auto copy = copyRepo_.getCopyById(loan.getCopyId());
        if (copy) {
            detail.accessionNumber = copy->getAccessionNumber();
            auto book = bookRepo_.getBookById(copy->getBookId());
            detail.bookTitle = book ? book->getTitle() : ("Book #" + std::to_string(copy->getBookId()));
        } else {
            detail.accessionNumber = "Copy #" + std::to_string(loan.getCopyId());
            detail.bookTitle = "Unknown Title";
        }

        report.items.push_back(std::move(detail));
    }

    return report;
}

} // namespace lms::services
