#pragma once

#include <optional>
#include <string>

namespace lms::utils {

/**
 * @brief Trims leading and trailing whitespace characters (' ', '\t', '\n', '\r') from str.
 */
std::string trim(const std::string& str);

/**
 * @brief Returns true if str is empty or consists solely of whitespace characters.
 */
bool isWhitespaceOrEmpty(const std::string& str);

/**
 * @brief Trims an optional string; returns std::nullopt if absent or if the trimmed result is empty.
 */
std::optional<std::string> normalizeOptionalString(const std::optional<std::string>& opt);

/**
 * @brief Validates that dateStr conforms to ISO-8601 YYYY-MM-DD and represents a valid calendar date.
 */
bool isValidIsoDate(const std::string& dateStr);

/**
 * @brief Validates that an email address has a sensible basic structure (user@domain.tld).
 */
bool isValidEmail(const std::string& email);

/**
 * @brief Validates that a publication year is a sensible positive year (e.g. 1 to 2100).
 */
bool isValidPublicationYear(int year);

} // namespace lms::utils

