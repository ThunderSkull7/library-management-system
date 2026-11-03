#pragma once

#include <stdexcept>
#include <string>

namespace lms::utils {

/**
 * @brief Exception thrown when domain or business rule validation fails.
 */
class ValidationException : public std::runtime_error {
public:
    explicit ValidationException(const std::string& message)
        : std::runtime_error(message) {}
};

/**
 * @brief Exception thrown when an entity is not found in the repository or service.
 */
class NotFoundException : public std::runtime_error {
public:
    explicit NotFoundException(const std::string& message)
        : std::runtime_error(message) {}
};

/**
 * @brief Exception thrown when an operation conflicts with existing state (e.g. duplicate key or active loan).
 */
class ConflictException : public std::runtime_error {
public:
    explicit ConflictException(const std::string& message)
        : std::runtime_error(message) {}
};

} // namespace lms::utils
