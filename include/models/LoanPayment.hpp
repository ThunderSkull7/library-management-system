#pragma once

#include <string>

namespace lms::models {

/**
 * @brief Represents a payment transaction against an overdue loan fine.
 */
class LoanPayment {
public:
    LoanPayment() = default;

    LoanPayment(int loanId, int amountMinorUnits, std::string paidOn);
    LoanPayment(int id, int loanId, int amountMinorUnits, std::string paidOn);

    [[nodiscard]] int getId() const noexcept { return id_; }
    [[nodiscard]] int getLoanId() const noexcept { return loanId_; }
    [[nodiscard]] int getAmountMinorUnits() const noexcept { return amountMinorUnits_; }
    [[nodiscard]] const std::string& getPaidOn() const noexcept { return paidOn_; }

    void setId(int id) noexcept { id_ = id; }
    void setLoanId(int loanId) noexcept { loanId_ = loanId; }
    void setAmountMinorUnits(int amount) noexcept { amountMinorUnits_ = amount; }
    void setPaidOn(std::string paidOn) { paidOn_ = std::move(paidOn); }

    bool operator==(const LoanPayment& other) const = default;

private:
    int id_{0};
    int loanId_{0};
    int amountMinorUnits_{0};
    std::string paidOn_{};
};

} // namespace lms::models
