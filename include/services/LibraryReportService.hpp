#pragma once

#include "models/LibraryPolicy.hpp"
#include "models/ReportModels.hpp"
#include "repositories/BookCopyRepository.hpp"
#include "repositories/BookRepository.hpp"
#include "repositories/LoanRepository.hpp"
#include "repositories/MemberRepository.hpp"

#include <optional>
#include <string>
#include <vector>

namespace lms::services {

/**
 * @brief Service responsible for aggregating and generating administrative, circulation, and financial reports.
 */
class LibraryReportService {
public:
    LibraryReportService(repositories::BookRepository& bookRepo,
                         repositories::BookCopyRepository& copyRepo,
                         repositories::MemberRepository& memberRepo,
                         repositories::LoanRepository& loanRepo,
                         models::LibraryPolicy policy = models::LibraryPolicy());

    // Policy accessors
    [[nodiscard]] const models::LibraryPolicy& getPolicy() const noexcept { return policy_; }
    void setPolicy(const models::LibraryPolicy& policy) { policy_ = policy; }

    /**
     * @brief Generates an inventory summary of titles and physical copies grouped by status.
     */
    [[nodiscard]] models::InventoryReport generateInventoryReport();

    /**
     * @brief Generates a circulation report summarizing total, active, returned, and overdue loans,
     * with an optional date range filter on loan issue dates.
     * @throws ValidationException if rangeStart or rangeEnd is invalid or rangeStart > rangeEnd.
     */
    [[nodiscard]] models::CirculationReport generateCirculationReport(
        const std::string& asOfDate,
        const std::optional<std::string>& rangeStart = std::nullopt,
        const std::optional<std::string>& rangeEnd = std::nullopt);

    /**
     * @brief Generates a report on registered member statuses and active borrowing statistics.
     * @param topBorrowersLimit Maximum number of top active borrowers to include.
     */
    [[nodiscard]] models::MemberActivityReport generateMemberActivityReport(int topBorrowersLimit = 10);

    /**
     * @brief Generates a financial accounting report on assessed fines and received payments in integer paise.
     */
    [[nodiscard]] models::FineReport generateFineReport();

    /**
     * @brief Generates a detailed report of all loans currently overdue as of a specified calendar date.
     * @throws ValidationException if asOfDate is invalid.
     */
    [[nodiscard]] models::OverdueReport generateOverdueReport(const std::string& asOfDate);

private:
    repositories::BookRepository& bookRepo_;
    repositories::BookCopyRepository& copyRepo_;
    repositories::MemberRepository& memberRepo_;
    repositories::LoanRepository& loanRepo_;
    models::LibraryPolicy policy_;

    int calculateOverdueFine(const std::string& dueDate, const std::string& asOfDate) const;
};

} // namespace lms::services

