#pragma once

#include "database/DatabaseConnection.hpp"
#include "models/Loan.hpp"
#include "models/LoanPayment.hpp"

#include <optional>
#include <string>
#include <vector>

struct sqlite3_stmt;

namespace lms::repositories {

/**
 * @brief Repository for persisting and querying Loan records and Fine Payments in SQLite.
 */
class LoanRepository {
public:
    explicit LoanRepository(database::DatabaseConnection& connection);

    /**
     * @brief Inserts a new circulation loan record.
     * @return Generated primary key ID.
     * @throws DatabaseException on SQL failure or partial unique index violation.
     */
    int addLoan(const models::Loan& loan);

    /**
     * @brief Updates an existing loan record (e.g. return date, fine amounts, payment status).
     * @return True if record was updated, false if not found.
     */
    bool updateLoan(const models::Loan& loan);

    /**
     * @brief Retrieves a loan by primary key ID.
     */
    [[nodiscard]] std::optional<models::Loan> getLoanById(int id);

    /**
     * @brief Retrieves the current active (unreturned) loan for a physical copy, if one exists.
     */
    [[nodiscard]] std::optional<models::Loan> getActiveLoanByCopyId(int copyId);

    /**
     * @brief Retrieves all active (unreturned) loans checked out to a specific member.
     */
    [[nodiscard]] std::vector<models::Loan> getActiveLoansByMemberId(int memberId);

    /**
     * @brief Retrieves all loans (active and returned) for a member.
     */
    [[nodiscard]] std::vector<models::Loan> getLoanHistoryByMemberId(int memberId);

    /**
     * @brief Retrieves all loans (active and returned) for a physical copy.
     */
    [[nodiscard]] std::vector<models::Loan> getLoanHistoryByCopyId(int copyId);

    /**
     * @brief Retrieves all loans across the library ordered by ID ascending.
     */
    [[nodiscard]] std::vector<models::Loan> getAllLoans();

    /**
     * @brief Retrieves active loans whose due date is strictly before currentDate.
     */
    [[nodiscard]] std::vector<models::Loan> getOverdueLoans(const std::string& currentDate);

    /**
     * @brief Retrieves loans with an assessed fine that has not yet been marked fully paid.
     */
    [[nodiscard]] std::vector<models::Loan> getOutstandingFines();

    /**
     * @brief Counts the number of active (unreturned) loans checked out to a member.
     */
    [[nodiscard]] int getActiveLoanCountForMember(int memberId);

    /**
     * @brief Checks if a physical copy currently has an active unreturned loan.
     */
    [[nodiscard]] bool hasActiveLoanForCopy(int copyId);

    // Fine Payment Tracking
    /**
     * @brief Records a fine payment against a loan.
     * @return Generated payment ID.
     */
    int addPayment(int loanId, int amountMinorUnits, const std::string& paymentDate);

    /**
     * @brief Returns the total amount of paise paid so far for a loan.
     */
    [[nodiscard]] int getTotalPaidForLoan(int loanId);

    /**
     * @brief Retrieves all individual payment transactions recorded for a loan.
     */
    [[nodiscard]] std::vector<models::LoanPayment> getPaymentsForLoan(int loanId);

    /**
     * @brief Sets the fine_paid flag on a loan record.
     */
    bool setFinePaidStatus(int loanId, bool paid);

private:
    database::DatabaseConnection& connection_;

    [[nodiscard]] static models::Loan extractLoanFromStatement(sqlite3_stmt* stmt);
    [[nodiscard]] static models::LoanPayment extractPaymentFromStatement(sqlite3_stmt* stmt);
};

} // namespace lms::repositories
