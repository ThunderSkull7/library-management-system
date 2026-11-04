#pragma once

#include <string>

namespace lms::utils {

/**
 * @brief Utilities for ISO-8601 YYYY-MM-DD calendar date parsing, arithmetic, and comparisons.
 */
class DateUtils {
public:
    /**
     * @brief Validates whether dateStr is a valid calendar date in YYYY-MM-DD format.
     */
    static bool isValidIsoDate(const std::string& dateStr);

    /**
     * @brief Adds a specified number of calendar days to an ISO date string.
     * @throws ValidationException if dateStr is invalid.
     * @return Resulting ISO date string in YYYY-MM-DD format.
     */
    static std::string addDays(const std::string& dateStr, int days);

    /**
     * @brief Calculates the signed difference in calendar days between fromDate and toDate (toDate - fromDate).
     * @throws ValidationException if either date is invalid.
     * @return Positive if toDate > fromDate, negative if toDate < fromDate, 0 if equal.
     */
    static int daysBetween(const std::string& fromDateStr, const std::string& toDateStr);

    /**
     * @brief Returns the current system date as an ISO date string (YYYY-MM-DD).
     */
    static std::string getTodayIsoDate();

    /**
     * @brief Returns true if d1 is strictly chronologically before d2.
     */
    static bool isDateBefore(const std::string& d1, const std::string& d2);

    /**
     * @brief Returns true if d1 is strictly chronologically after d2.
     */
    static bool isDateAfter(const std::string& d1, const std::string& d2);

    /**
     * @brief Returns true if d1 and d2 represent the same calendar date.
     */
    static bool isDateEqual(const std::string& d1, const std::string& d2);
};

} // namespace lms::utils
