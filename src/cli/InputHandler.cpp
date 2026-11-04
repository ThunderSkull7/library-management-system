#include "cli/InputHandler.hpp"
#include "utils/DateUtils.hpp"
#include "utils/ValidationUtils.hpp"

#include <cctype>

namespace lms::cli {

std::string InputHandler::readString(std::istream& in,
                                     std::ostream& out,
                                     const std::string& prompt,
                                     bool allowEmpty) {
    while (true) {
        out << prompt;
        out.flush();

        std::string line;
        if (!std::getline(in, line)) {
            if (in.eof()) {
                throw EofException();
            }
            in.clear();
            out << "\n[!] Input error. Please try again.\n";
            continue;
        }

        std::string trimmed = utils::trim(line);
        if (trimmed.empty() && !allowEmpty) {
            out << "[!] Input cannot be empty. Please enter a value.\n";
            continue;
        }

        return trimmed;
    }
}

std::optional<int> InputHandler::readInt(std::istream& in,
                                         std::ostream& out,
                                         const std::string& prompt,
                                         std::optional<int> minVal,
                                         std::optional<int> maxVal,
                                         bool allowBlank) {
    while (true) {
        out << prompt;
        out.flush();

        std::string line;
        if (!std::getline(in, line)) {
            if (in.eof()) {
                throw EofException();
            }
            in.clear();
            out << "\n[!] Input error. Please try again.\n";
            continue;
        }

        std::string trimmed = utils::trim(line);
        if (trimmed.empty()) {
            if (allowBlank) {
                return std::nullopt;
            }
            out << "[!] Input cannot be empty. Please enter a valid number.\n";
            continue;
        }

        try {
            size_t idx = 0;
            long long val = std::stoll(trimmed, &idx);
            if (idx != trimmed.length()) {
                out << "[!] Invalid number. Please enter digits only.\n";
                continue;
            }
            if (val < std::numeric_limits<int>::min() || val > std::numeric_limits<int>::max()) {
                out << "[!] Number is out of integer range.\n";
                continue;
            }
            int intVal = static_cast<int>(val);
            if (minVal.has_value() && intVal < *minVal) {
                out << "[!] Value must be at least " << *minVal << ".\n";
                continue;
            }
            if (maxVal.has_value() && intVal > *maxVal) {
                out << "[!] Value must be at most " << *maxVal << ".\n";
                continue;
            }
            return intVal;
        } catch (...) {
            out << "[!] Invalid number format. Please enter a valid numeric value.\n";
        }
    }
}

std::string InputHandler::readDate(std::istream& in,
                                   std::ostream& out,
                                   const std::string& prompt,
                                   const std::string& defaultDate) {
    while (true) {
        if (!defaultDate.empty()) {
            out << prompt << " [" << defaultDate << "]: ";
        } else {
            out << prompt << " (YYYY-MM-DD): ";
        }
        out.flush();

        std::string line;
        if (!std::getline(in, line)) {
            if (in.eof()) {
                throw EofException();
            }
            in.clear();
            out << "\n[!] Input error. Please try again.\n";
            continue;
        }

        std::string trimmed = utils::trim(line);
        if (trimmed.empty() && !defaultDate.empty()) {
            return defaultDate;
        }

        if (utils::DateUtils::isValidIsoDate(trimmed)) {
            return trimmed;
        }

        out << "[!] Invalid calendar date. Format must be YYYY-MM-DD (e.g. 2026-03-15).\n";
    }
}

bool InputHandler::readConfirmation(std::istream& in,
                                    std::ostream& out,
                                    const std::string& prompt,
                                    bool defaultYes) {
    while (true) {
        out << prompt << (defaultYes ? " [Y/n]: " : " [y/N]: ");
        out.flush();

        std::string line;
        if (!std::getline(in, line)) {
            if (in.eof()) {
                throw EofException();
            }
            in.clear();
            out << "\n[!] Input error. Please try again.\n";
            continue;
        }

        std::string trimmed = utils::trim(line);
        if (trimmed.empty()) {
            return defaultYes;
        }

        char c = static_cast<char>(std::tolower(static_cast<unsigned char>(trimmed[0])));
        if (c == 'y') {
            return true;
        }
        if (c == 'n') {
            return false;
        }

        out << "[!] Please answer 'y' for yes or 'n' for no.\n";
    }
}

void InputHandler::waitForEnter(std::istream& in,
                                std::ostream& out,
                                const std::string& prompt) {
    out << prompt;
    out.flush();
    std::string dummy;
    if (!std::getline(in, dummy)) {
        if (in.eof()) {
            throw EofException();
        }
        in.clear();
    }
}

} // namespace lms::cli

