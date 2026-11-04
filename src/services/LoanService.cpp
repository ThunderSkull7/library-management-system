#include "services/LoanService.hpp"
#include "database/Transaction.hpp"
#include "utils/DateUtils.hpp"
#include "utils/ValidationException.hpp"

#include <algorithm>
#include <cstdint>

namespace lms::services {

LoanService::LoanService(repositories::LoanRepository& loanRepository,
                         repositories::BookCopyRepository& copyRepository,
                         repositories::MemberRepository& memberRepository,
                         database::DatabaseConnection& connection,
                         models::LibraryPolicy policy)
    : loanRepository_(loanRepository),
      copyRepository_(copyRepository),
      memberRepository_(memberRepository),
      connection_(connection),
      policy_(policy) {}

int LoanService::calculateFine(const std::string& dueDate, const std::string& returnDate) const {
    std::string effectiveDue = utils::DateUtils::addDays(dueDate, policy_.getGracePeriodDays());
    int diffDays = utils::DateUtils::daysBetween(effectiveDue, returnDate);
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

models::Loan LoanService::checkoutBook(int memberId, int copyId, const std::string& issueDate) {
    // Validate purely local input before starting the transaction
    if (!utils::DateUtils::isValidIsoDate(issueDate)) {
        throw utils::ValidationException("Issue date must be a valid ISO date in YYYY-MM-DD format. Received: '" + issueDate + "'");
    }

    // Start BEGIN IMMEDIATE before reading database state used to authorize checkout
    database::Transaction txn(connection_);

    // 1. Retrieve and validate the member
    auto member = memberRepository_.getMemberById(memberId);
    if (!member.has_value()) {
        throw utils::NotFoundException("Member with ID " + std::to_string(memberId) + " does not exist.");
    }

    // 2. Confirm the member is Active
    if (member->getStatus() != models::MemberStatus::Active) {
        std::string statusStr = models::toString(member->getStatus());
        throw utils::ValidationException("Cannot checkout book: member account is " + statusStr + ". Only Active members may borrow books.");
    }

    // 3. Retrieve and validate the copy
    auto copy = copyRepository_.getCopyById(copyId);
    if (!copy.has_value()) {
        throw utils::NotFoundException("Book copy with ID " + std::to_string(copyId) + " does not exist.");
    }

    // 4. Confirm the copy is Available
    if (copy->getStatus() != models::BookCopyStatus::Available) {
        std::string statusStr = models::toString(copy->getStatus());
        throw utils::ValidationException("Cannot checkout book copy: current status is '" + statusStr + "'. Only 'Available' copies may be borrowed.");
    }

    // 5. Check that no active loan exists for the copy
    if (loanRepository_.hasActiveLoanForCopy(copyId)) {
        throw utils::ValidationException("Cannot checkout book copy: an active loan already exists for copy ID " + std::to_string(copyId) + ".");
    }

    // 6. Check the member's active loan count against the policy limit
    int currentLoans = loanRepository_.getActiveLoanCountForMember(memberId);
    if (currentLoans >= policy_.getMaxActiveLoansPerMember()) {
        throw utils::ValidationException("Cannot checkout book: member has reached the maximum active loan limit of " +
                                        std::to_string(policy_.getMaxActiveLoansPerMember()) + " books.");
    }

    // 7. Calculate due date
    std::string dueDate = utils::DateUtils::addDays(issueDate, policy_.getLoanDurationDays());

    // 8. Insert the loan
    models::Loan newLoan(copyId, memberId, issueDate, dueDate, std::nullopt, 0, false);
    int loanId = loanRepository_.addLoan(newLoan);

    // 9. Update the copy to OnLoan
    copyRepository_.updateCopyStatus(copyId, models::BookCopyStatus::OnLoan);

    // 10. Commit
    txn.commit();

    newLoan.setId(loanId);
    return newLoan;
}

models::Loan LoanService::returnBook(int copyId, const std::string& returnDate) {
    // Validate local date syntax before the transaction
    if (!utils::DateUtils::isValidIsoDate(returnDate)) {
        throw utils::ValidationException("Return date must be a valid ISO date in YYYY-MM-DD format. Received: '" + returnDate + "'");
    }

    // Start BEGIN IMMEDIATE before retrieving the active loan
    database::Transaction txn(connection_);

    // 1. Retrieve the copy and active loan
    auto copy = copyRepository_.getCopyById(copyId);
    if (!copy.has_value()) {
        throw utils::NotFoundException("Book copy with ID " + std::to_string(copyId) + " does not exist.");
    }

    auto activeLoan = loanRepository_.getActiveLoanByCopyId(copyId);
    if (!activeLoan.has_value()) {
        throw utils::ValidationException("Cannot return copy: no active loan found for copy ID " + std::to_string(copyId) + ".");
    }

    // 2. Validate the return date against the issue date
    if (utils::DateUtils::isDateBefore(returnDate, activeLoan->getIssuedOn())) {
        throw utils::ValidationException("Return date ('" + returnDate + "') cannot precede loan issue date ('" + activeLoan->getIssuedOn() + "').");
    }

    // 3. Calculate the fine
    int fine = calculateFine(activeLoan->getDueOn(), returnDate);

    // 4. Update the loan's return date and assessed fine
    activeLoan->setReturnedOn(returnDate);
    activeLoan->setFineMinorUnits(fine);
    activeLoan->setFinePaid(fine == 0);
    loanRepository_.updateLoan(*activeLoan);

    // 5. Update the copy to Available
    copyRepository_.updateCopyStatus(copyId, models::BookCopyStatus::Available);

    // 6. Commit
    txn.commit();

    return *activeLoan;
}

void LoanService::payFine(int loanId, int amountMinorUnits, const std::string& paymentDate) {
    // Validate purely local input first
    if (amountMinorUnits <= 0) {
        throw utils::ValidationException("Payment amount must be greater than zero. Received: " + std::to_string(amountMinorUnits));
    }

    if (!utils::DateUtils::isValidIsoDate(paymentDate)) {
        throw utils::ValidationException("Payment date must be a valid ISO date in YYYY-MM-DD format. Received: '" + paymentDate + "'");
    }

    // Start BEGIN IMMEDIATE before retrieving the loan and calculating its remaining balance
    database::Transaction txn(connection_);

    // 1. Retrieve the loan
    auto loan = loanRepository_.getLoanById(loanId);
    if (!loan.has_value()) {
        throw utils::NotFoundException("Loan with ID " + std::to_string(loanId) + " does not exist.");
    }

    // 2. Verify that it has been returned
    if (loan->isActive()) {
        throw utils::ValidationException("Cannot accept fine payment for an active loan. The copy must be returned before paying assessed fines.");
    }

    // 3. Verify that a positive fine was assessed
    if (loan->getFineMinorUnits() == 0) {
        throw utils::ValidationException("No fine was assessed for loan ID " + std::to_string(loanId) + ".");
    }

    // 4. Retrieve total payments already recorded
    int alreadyPaid = loanRepository_.getTotalPaidForLoan(loanId);

    // 5. Calculate the remaining balance
    int remaining = loan->getFineMinorUnits() - alreadyPaid;

    if (remaining <= 0 || loan->isFinePaid()) {
        throw utils::ValidationException("Fine for loan ID " + std::to_string(loanId) + " is already fully paid.");
    }

    // 6. Reject payments exceeding that balance
    if (amountMinorUnits > remaining) {
        throw utils::ValidationException("Payment amount (" + std::to_string(amountMinorUnits) +
                                        " paise) exceeds remaining fine balance (" + std::to_string(remaining) + " paise).");
    }

    // 7. Insert the payment
    loanRepository_.addPayment(loanId, amountMinorUnits, paymentDate);

    // 8. Mark fine_paid = true only when the full assessed fine has been paid
    if (alreadyPaid + amountMinorUnits == loan->getFineMinorUnits()) {
        loanRepository_.setFinePaidStatus(loanId, true);
    }

    // 9. Commit
    txn.commit();
}

models::Loan LoanService::getLoan(int loanId) {
    auto loan = loanRepository_.getLoanById(loanId);
    if (!loan.has_value()) {
        throw utils::NotFoundException("Loan with ID " + std::to_string(loanId) + " does not exist.");
    }
    return *loan;
}

std::vector<models::Loan> LoanService::getActiveLoansForMember(int memberId) {
    return loanRepository_.getActiveLoansByMemberId(memberId);
}

std::vector<models::Loan> LoanService::getLoanHistoryForMember(int memberId) {
    return loanRepository_.getLoanHistoryByMemberId(memberId);
}

std::vector<models::Loan> LoanService::getLoanHistoryForCopy(int copyId) {
    return loanRepository_.getLoanHistoryByCopyId(copyId);
}

std::vector<models::Loan> LoanService::getAllLoans() {
    return loanRepository_.getAllLoans();
}

std::vector<models::Loan> LoanService::getActiveLoans() {
    return loanRepository_.getActiveLoans();
}

std::vector<models::Loan> LoanService::getLoansIssuedBetween(const std::string& startDate, const std::string& endDate) {
    if (!utils::DateUtils::isValidIsoDate(startDate)) {
        throw utils::ValidationException("Start date must be a valid ISO date in YYYY-MM-DD format. Received: '" + startDate + "'");
    }
    if (!utils::DateUtils::isValidIsoDate(endDate)) {
        throw utils::ValidationException("End date must be a valid ISO date in YYYY-MM-DD format. Received: '" + endDate + "'");
    }
    if (utils::DateUtils::isDateAfter(startDate, endDate)) {
        throw utils::ValidationException("Start date ('" + startDate + "') cannot be after end date ('" + endDate + "').");
    }
    return loanRepository_.getLoansIssuedBetween(startDate, endDate);
}

std::vector<models::Loan> LoanService::getOverdueLoans(const std::string& currentDate) {
    if (!utils::DateUtils::isValidIsoDate(currentDate)) {
        throw utils::ValidationException("Current date must be a valid ISO date in YYYY-MM-DD format. Received: '" + currentDate + "'");
    }
    return loanRepository_.getOverdueLoans(currentDate);
}

std::vector<models::Loan> LoanService::getOutstandingFines() {
    return loanRepository_.getOutstandingFines();
}

std::vector<models::LoanPayment> LoanService::getPaymentsForLoan(int loanId) {
    return loanRepository_.getPaymentsForLoan(loanId);
}

int LoanService::getRemainingFine(int loanId) {
    auto loan = loanRepository_.getLoanById(loanId);
    if (!loan.has_value()) {
        throw utils::NotFoundException("Loan with ID " + std::to_string(loanId) + " does not exist.");
    }
    int paid = loanRepository_.getTotalPaidForLoan(loanId);
    return std::max(0, loan->getFineMinorUnits() - paid);
}

} // namespace lms::services
