#include "utils/ValidationUtils.hpp"
#include "utils/DateUtils.hpp"

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

bool isValidIsoDate(const std::string& dateStr) {
    return DateUtils::isValidIsoDate(dateStr);
}

bool isValidEmail(const std::string& email) {
    if (email.empty() || email.front() == '.' || email.back() == '.') {
        return false;
    }

    // Must not contain any whitespace
    for (char c : email) {
        if (std::isspace(static_cast<unsigned char>(c))) {
            return false;
        }
    }

    size_t atPos = email.find('@');
    if (atPos == std::string::npos || atPos == 0 || atPos >= email.length() - 1) {
        return false;
    }

    // Must have only one '@' symbol
    if (email.find('@', atPos + 1) != std::string::npos) {
        return false;
    }

    // Character immediately preceding or following '@' must not be '.'
    if (email[atPos - 1] == '.' || email[atPos + 1] == '.') {
        return false;
    }

    // Must have at least one '.' after '@'
    size_t dotPos = email.find('.', atPos + 2);
    if (dotPos == std::string::npos || dotPos >= email.length() - 1) {
        return false;
    }

    return true;
}

bool isValidPublicationYear(int year) {
    return year > 0 && year <= 2100;
}

} // namespace lms::utils

