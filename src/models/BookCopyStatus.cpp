#include "models/BookCopyStatus.hpp"

#include <stdexcept>

namespace lms::models {

std::string toString(BookCopyStatus status) {
    switch (status) {
        case BookCopyStatus::Available:
            return "Available";
        case BookCopyStatus::OnLoan:
            return "On Loan";
        case BookCopyStatus::Lost:
            return "Lost";
        case BookCopyStatus::Damaged:
            return "Damaged";
        case BookCopyStatus::Withdrawn:
            return "Withdrawn";
    }
    throw std::invalid_argument("Unknown BookCopyStatus enum value");
}

BookCopyStatus bookCopyStatusFromString(const std::string& str) {
    if (str == "Available") {
        return BookCopyStatus::Available;
    }
    if (str == "On Loan") {
        return BookCopyStatus::OnLoan;
    }
    if (str == "Lost") {
        return BookCopyStatus::Lost;
    }
    if (str == "Damaged") {
        return BookCopyStatus::Damaged;
    }
    if (str == "Withdrawn") {
        return BookCopyStatus::Withdrawn;
    }
    throw std::invalid_argument("Invalid BookCopyStatus string: '" + str + "'");
}

} // namespace lms::models
