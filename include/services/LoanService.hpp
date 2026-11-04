#pragma once

#include "database/DatabaseConnection.hpp"
#include "models/LibraryPolicy.hpp"
#include "models/Loan.hpp"
#include "models/LoanPayment.hpp"
#include "repositories/BookCopyRepository.hpp"
#include "repositories/LoanRepository.hpp"
#include "repositories/MemberRepository.hpp"

#include <optional>
#include <string>
#include <vector>

namespace lms::services {

/**
 * @brief Service responsible for library circulation workflows (checkout, returns, fines, and payments).
 */
class LoanService {
public:
    LoanService(repositories::LoanRepository& loanRepository,
                repositories::BookCopyRepository& copyRepository,
                repositories::MemberRepository& memberRepository,
                database::DatabaseConnection& connection,
                models::LibraryPolicy policy = models::LibraryPolicy());

    // Policy accessors
    [[nodiscard]] const models::LibraryPolicy& getPolicy() const noexcept { return policy_; }
    void setPolicy(const models::LibraryPolicy& policy) { policy_ = policy; }

    /**
     * @brief Checks out a book copy to an active library member.
     * @throws NotFoundException if member or copy does not exist.
     * @throws ValidationException if member is inactive, copy is unavailable, member has reached loan limit, or issueDate is invalid.
     * @return Created Loan record.
     */
    models::Loan checkoutBook(int memberId, int copyId, const std::string& issueDate);

    /**
     * @brief Returns a checked-out book copy, assessing any overdue fines.
     * @throws NotFoundException if copy does not exist.
     * @throws ValidationException if no active loan exists for copy or if returnDate is invalid / precedes issue date.
     * @return Updated Loan record with return date and assessed fine.
     */
    models::Loan returnBook(int copyId, const std::string& returnDate);

    /**
     * @brief Records a fine payment against a returned loan.
     * @throws NotFoundException if loan does not exist.
     * @throws ValidationException on invalid amount, unreturned loan, overpayment, or already-paid fine.
     */
    void payFine(int loanId, int amountMinorUnits, const std::string& paymentDate);

    // Queries
    [[nodiscard]] models::Loan getLoan(int loanId);
    [[nodiscard]] std::vector<models::Loan> getActiveLoansForMember(int memberId);
    [[nodiscard]] std::vector<models::Loan> getLoanHistoryForMember(int memberId);
    [[nodiscard]] std::vector<models::Loan> getLoanHistoryForCopy(int copyId);
    [[nodiscard]] std::vector<models::Loan> getAllLoans();
    [[nodiscard]] std::vector<models::Loan> getActiveLoans();
    [[nodiscard]] std::vector<models::Loan> getLoansIssuedBetween(const std::string& startDate, const std::string& endDate);
    [[nodiscard]] std::vector<models::Loan> getOverdueLoans(const std::string& currentDate);
    [[nodiscard]] std::vector<models::Loan> getOutstandingFines();
    [[nodiscard]] std::vector<models::LoanPayment> getPaymentsForLoan(int loanId);
    [[nodiscard]] int getRemainingFine(int loanId);

private:
    repositories::LoanRepository& loanRepository_;
    repositories::BookCopyRepository& copyRepository_;
    repositories::MemberRepository& memberRepository_;
    database::DatabaseConnection& connection_;
    models::LibraryPolicy policy_;

    int calculateFine(const std::string& dueDate, const std::string& returnDate) const;
};

} // namespace lms::services
