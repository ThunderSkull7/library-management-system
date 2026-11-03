#pragma once

#include <string>

namespace lms::models {

/**
 * @brief Represents the physical availability and condition state of a book copy.
 */
enum class BookCopyStatus {
    Available,
    OnLoan,
    Lost,
    Damaged,
    Withdrawn
};

/**
 * @brief Converts a BookCopyStatus enum to its exact SQLite string representation.
 */
std::string toString(BookCopyStatus status);

/**
 * @brief Parses an exact SQLite string representation into a BookCopyStatus enum.
 * @throws std::invalid_argument if the string does not match any recognized status.
 */
BookCopyStatus bookCopyStatusFromString(const std::string& str);

} // namespace lms::models
