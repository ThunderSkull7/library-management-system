#pragma once

#include <stdexcept>
#include <string>

namespace lms::database {

/**
 * @brief Exception thrown for database connection, query, and schema errors.
 */
class DatabaseException : public std::runtime_error {
public:
    explicit DatabaseException(const std::string& message, int errorCode = 0)
        : std::runtime_error(message), errorCode_(errorCode) {}

    [[nodiscard]] int getErrorCode() const noexcept {
        return errorCode_;
    }

private:
    int errorCode_{0};
};

} // namespace lms::database

