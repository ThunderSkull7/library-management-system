#pragma once

#include "models/BookCopyStatus.hpp"

#include <optional>
#include <string>

namespace lms::models {

/**
 * @brief Represents an individual physical copy of a book in the library.
 */
class BookCopy {
public:
    BookCopy() = default;

    /**
     * @brief Constructs a new BookCopy entity (typically before database insertion, id = 0).
     */
    BookCopy(int bookId,
             std::string accessionNumber,
             BookCopyStatus status = BookCopyStatus::Available,
             std::optional<std::string> acquiredOn = std::nullopt);

    /**
     * @brief Constructs a BookCopy entity with a persisted database identifier.
     */
    BookCopy(int id,
             int bookId,
             std::string accessionNumber,
             BookCopyStatus status,
             std::optional<std::string> acquiredOn = std::nullopt);

    // Getters
    [[nodiscard]] int getId() const noexcept { return id_; }
    [[nodiscard]] int getBookId() const noexcept { return bookId_; }
    [[nodiscard]] const std::string& getAccessionNumber() const noexcept { return accessionNumber_; }
    [[nodiscard]] BookCopyStatus getStatus() const noexcept { return status_; }
    [[nodiscard]] const std::optional<std::string>& getAcquiredOn() const noexcept { return acquiredOn_; }

    // Setters
    void setId(int id) noexcept { id_ = id; }
    void setBookId(int bookId) noexcept { bookId_ = bookId; }
    void setAccessionNumber(std::string accessionNumber) { accessionNumber_ = std::move(accessionNumber); }
    void setStatus(BookCopyStatus status) noexcept { status_ = status; }
    void setAcquiredOn(std::optional<std::string> acquiredOn) { acquiredOn_ = std::move(acquiredOn); }

    // Equality comparison
    bool operator==(const BookCopy& other) const = default;

private:
    int id_{0};
    int bookId_{0};
    std::string accessionNumber_{};
    BookCopyStatus status_{BookCopyStatus::Available};
    std::optional<std::string> acquiredOn_{std::nullopt};
};

} // namespace lms::models
