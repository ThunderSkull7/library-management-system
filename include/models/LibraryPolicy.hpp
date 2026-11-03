#pragma once

#include <optional>

namespace lms::models {

/**
 * @brief Configurable business rules governing circulation durations, loan limits, and overdue fines.
 */
class LibraryPolicy {
public:
    static constexpr int DEFAULT_LOAN_DURATION_DAYS = 14;
    static constexpr int DEFAULT_FINE_PER_DAY_MINOR_UNITS = 200; // ₹2.00 = 200 paise
    static constexpr int DEFAULT_MAX_ACTIVE_LOANS = 5;
    static constexpr int DEFAULT_GRACE_PERIOD_DAYS = 0;

    LibraryPolicy();

    LibraryPolicy(int loanDurationDays,
                  int finePerDayMinorUnits,
                  int maxActiveLoansPerMember,
                  int gracePeriodDays = DEFAULT_GRACE_PERIOD_DAYS,
                  std::optional<int> maxFineMinorUnits = std::nullopt);

    // Getters
    [[nodiscard]] int getLoanDurationDays() const noexcept { return loanDurationDays_; }
    [[nodiscard]] int getFinePerDayMinorUnits() const noexcept { return finePerDayMinorUnits_; }
    [[nodiscard]] int getMaxActiveLoansPerMember() const noexcept { return maxActiveLoansPerMember_; }
    [[nodiscard]] int getGracePeriodDays() const noexcept { return gracePeriodDays_; }
    [[nodiscard]] const std::optional<int>& getMaxFineMinorUnits() const noexcept { return maxFineMinorUnits_; }

    // Setters
    void setLoanDurationDays(int days);
    void setFinePerDayMinorUnits(int minorUnits);
    void setMaxActiveLoansPerMember(int maxLoans);
    void setGracePeriodDays(int days);
    void setMaxFineMinorUnits(std::optional<int> maxFine);

    bool operator==(const LibraryPolicy& other) const = default;

private:
    int loanDurationDays_{DEFAULT_LOAN_DURATION_DAYS};
    int finePerDayMinorUnits_{DEFAULT_FINE_PER_DAY_MINOR_UNITS};
    int maxActiveLoansPerMember_{DEFAULT_MAX_ACTIVE_LOANS};
    int gracePeriodDays_{DEFAULT_GRACE_PERIOD_DAYS};
    std::optional<int> maxFineMinorUnits_{std::nullopt};

    static void validate(int loanDurationDays,
                         int finePerDayMinorUnits,
                         int maxActiveLoansPerMember,
                         int gracePeriodDays,
                         const std::optional<int>& maxFineMinorUnits);
};

} // namespace lms::models
