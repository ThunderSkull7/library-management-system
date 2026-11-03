#include "models/LoanPayment.hpp"

#include <utility>

namespace lms::models {

LoanPayment::LoanPayment(int loanId, int amountMinorUnits, std::string paidOn)
    : id_(0), loanId_(loanId), amountMinorUnits_(amountMinorUnits), paidOn_(std::move(paidOn)) {}

LoanPayment::LoanPayment(int id, int loanId, int amountMinorUnits, std::string paidOn)
    : id_(id), loanId_(loanId), amountMinorUnits_(amountMinorUnits), paidOn_(std::move(paidOn)) {}

} // namespace lms::models
