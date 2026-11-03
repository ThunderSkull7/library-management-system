#include "models/LibraryPolicy.hpp"
#include "utils/ValidationException.hpp"

namespace lms::models {

void LibraryPolicy::validate(int loanDurationDays,
                             int finePerDayMinorUnits,
                             int maxActiveLoansPerMember,
                             int gracePeriodDays,
                             const std::optional<int>& maxFineMinorUnits) {
    if (loanDurationDays <= 0) {
        throw utils::ValidationException("Loan duration must be at least 1 day. Received: " + std::to_string(loanDurationDays));
    }
    if (finePerDayMinorUnits < 0) {
        throw utils::ValidationException("Fine per day cannot be negative. Received: " + std::to_string(finePerDayMinorUnits));
    }
    if (maxActiveLoansPerMember <= 0) {
        throw utils::ValidationException("Maximum active loans per member must be at least 1. Received: " + std::to_string(maxActiveLoansPerMember));
    }
    if (gracePeriodDays < 0) {
        throw utils::ValidationException("Grace period cannot be negative. Received: " + std::to_string(gracePeriodDays));
    }
    if (maxFineMinorUnits.has_value() && *maxFineMinorUnits < 0) {
        throw utils::ValidationException("Maximum fine cannot be negative. Received: " + std::to_string(*maxFineMinorUnits));
    }
}

LibraryPolicy::LibraryPolicy()
    : loanDurationDays_(DEFAULT_LOAN_DURATION_DAYS),
      finePerDayMinorUnits_(DEFAULT_FINE_PER_DAY_MINOR_UNITS),
      maxActiveLoansPerMember_(DEFAULT_MAX_ACTIVE_LOANS),
      gracePeriodDays_(DEFAULT_GRACE_PERIOD_DAYS),
      maxFineMinorUnits_(std::nullopt) {}

LibraryPolicy::LibraryPolicy(int loanDurationDays,
                             int finePerDayMinorUnits,
                             int maxActiveLoansPerMember,
                             int gracePeriodDays,
                             std::optional<int> maxFineMinorUnits)
    : loanDurationDays_(loanDurationDays),
      finePerDayMinorUnits_(finePerDayMinorUnits),
      maxActiveLoansPerMember_(maxActiveLoansPerMember),
      gracePeriodDays_(gracePeriodDays),
      maxFineMinorUnits_(maxFineMinorUnits) {
    validate(loanDurationDays_, finePerDayMinorUnits_, maxActiveLoansPerMember_, gracePeriodDays_, maxFineMinorUnits_);
}

void LibraryPolicy::setLoanDurationDays(int days) {
    validate(days, finePerDayMinorUnits_, maxActiveLoansPerMember_, gracePeriodDays_, maxFineMinorUnits_);
    loanDurationDays_ = days;
}

void LibraryPolicy::setFinePerDayMinorUnits(int minorUnits) {
    validate(loanDurationDays_, minorUnits, maxActiveLoansPerMember_, gracePeriodDays_, maxFineMinorUnits_);
    finePerDayMinorUnits_ = minorUnits;
}

void LibraryPolicy::setMaxActiveLoansPerMember(int maxLoans) {
    validate(loanDurationDays_, finePerDayMinorUnits_, maxLoans, gracePeriodDays_, maxFineMinorUnits_);
    maxActiveLoansPerMember_ = maxLoans;
}

void LibraryPolicy::setGracePeriodDays(int days) {
    validate(loanDurationDays_, finePerDayMinorUnits_, maxActiveLoansPerMember_, days, maxFineMinorUnits_);
    gracePeriodDays_ = days;
}

void LibraryPolicy::setMaxFineMinorUnits(std::optional<int> maxFine) {
    validate(loanDurationDays_, finePerDayMinorUnits_, maxActiveLoansPerMember_, gracePeriodDays_, maxFine);
    maxFineMinorUnits_ = maxFine;
}

} // namespace lms::models
