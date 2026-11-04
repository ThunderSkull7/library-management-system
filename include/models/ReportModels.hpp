#pragma once

#include "models/Loan.hpp"

#include <optional>
#include <string>
#include <vector>

namespace lms::models {

/**
 * @brief Statistics on library catalog inventory and physical copy statuses.
 */
struct InventoryReport {
    int totalTitles{0};
    int totalCopies{0};
    int availableCopies{0};
    int onLoanCopies{0};
    int lostCopies{0};
    int damagedCopies{0};
    int withdrawnCopies{0};
};

/**
 * @brief Statistics on loan circulation activity and optional date range breakdown.
 */
struct CirculationReport {
    int totalLoans{0};
    int activeLoans{0};
    int returnedLoans{0};
    int overdueLoans{0};
    std::optional<std::string> rangeStart;
    std::optional<std::string> rangeEnd;
    std::vector<Loan> rangeLoans;
};

/**
 * @brief Individual member borrowing summary.
 */
struct MemberLoanStats {
    int memberId{0};
    std::string memberName;
    std::string status;
    int activeLoanCount{0};
};

/**
 * @brief Member registration and active borrowing activity report.
 */
struct MemberActivityReport {
    int totalMembers{0};
    int activeMembers{0};
    int suspendedMembers{0};
    int closedMembers{0};
    int membersWithActiveLoans{0};
    std::vector<MemberLoanStats> topBorrowers;
};

/**
 * @brief Financial accounting summary of assessed fines, received payments, and balances in integer paise.
 */
struct FineReport {
    int totalFinesAssessedMinorUnits{0};
    int totalPaymentsReceivedMinorUnits{0};
    int outstandingBalanceMinorUnits{0};
    int unpaidFineCount{0};
    int fullyPaidFineCount{0};
};

/**
 * @brief Detailed line item representing an overdue circulation loan.
 */
struct OverdueLoanDetail {
    int loanId{0};
    int memberId{0};
    std::string memberName;
    int copyId{0};
    std::string accessionNumber;
    std::string bookTitle;
    std::string issuedOn;
    std::string dueOn;
    int overdueDays{0};
    int assessedFineMinorUnits{0};
    bool isActive{true};
};

/**
 * @brief Report detailing all active overdue loans as of a specific calendar date.
 */
struct OverdueReport {
    std::string asOfDate;
    int totalOverdueLoans{0};
    std::vector<OverdueLoanDetail> items;
};

} // namespace lms::models

