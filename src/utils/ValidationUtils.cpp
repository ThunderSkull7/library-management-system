#include "utils/ValidationUtils.hpp"

#include <cctype>
#include <string>

namespace lms::utils {

std::string trim(const std::string& str) {
    if (str.empty()) {
        return "";
    }

    size_t start = 0;
    while (start < str.size() && std::isspace(static_cast<unsigned char>(str[start]))) {
        ++start;
    }

    if (start == str.size()) {
        return "";
    }

    size_t end = str.size() - 1;
    while (end > start && std::isspace(static_cast<unsigned char>(str[end]))) {
        --end;
    }

    return str.substr(start, end - start + 1);
}

bool isWhitespaceOrEmpty(const std::string& str) {
    return trim(str).empty();
}

std::optional<std::string> normalizeOptionalString(const std::optional<std::string>& opt) {
    if (!opt.has_value()) {
        return std::nullopt;
    }
    std::string trimmed = trim(*opt);
    if (trimmed.empty()) {
        return std::nullopt;
    }
    return trimmed;
}

namespace {

bool isLeapYear(int year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

int daysInMonth(int year, int month) {
    switch (month) {
        case 1:  return 31;
        case 2:  return isLeapYear(year) ? 29 : 28;
        case 3:  return 31;
        case 4:  return 30;
        case 5:  return 31;
        case 6:  return 30;
        case 7:  return 31;
        case 8:  return 31;
        case 9:  return 30;
        case 10: return 31;
        case 11: return 30;
        case 12: return 31;
        default: return 0;
    }
}

} // namespace

bool isValidIsoDate(const std::string& dateStr) {
    // Format: YYYY-MM-DD (length 10)
    if (dateStr.length() != 10) {
        return false;
    }

    if (dateStr[4] != '-' || dateStr[7] != '-') {
        return false;
    }

    for (size_t i = 0; i < 10; ++i) {
        if (i == 4 || i == 7) {
            continue;
        }
        if (!std::isdigit(static_cast<unsigned char>(dateStr[i]))) {
            return false;
        }
    }

    int year = std::stoi(dateStr.substr(0, 4));
    int month = std::stoi(dateStr.substr(5, 2));
    int day = std::stoi(dateStr.substr(8, 2));

    if (year < 1000 || year > 9999) {
        return false;
    }

    if (month < 1 || month > 12) {
        return false;
    }

    int maxDays = daysInMonth(year, month);
    if (day < 1 || day > maxDays) {
        return false;
    }

    return true;
}

bool isValidPublicationYear(int year) {
    return year > 0 && year <= 2100;
}

} // namespace lms::utils
