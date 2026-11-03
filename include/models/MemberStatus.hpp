#pragma once

#include <string>

namespace lms::models {

/**
 * @brief Represents the membership account status of a patron.
 */
enum class MemberStatus {
    Active,
    Suspended,
    Closed
};

/**
 * @brief Converts a MemberStatus enum to its exact database string representation.
 */
std::string toString(MemberStatus status);

/**
 * @brief Parses an exact database string into a MemberStatus enum.
 * @throws std::invalid_argument if the string does not match any recognized status.
 */
MemberStatus memberStatusFromString(const std::string& str);

} // namespace lms::models

