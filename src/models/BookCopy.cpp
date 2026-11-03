#include "models/BookCopy.hpp"

#include <utility>

namespace lms::models {

BookCopy::BookCopy(int bookId,
                   std::string accessionNumber,
                   BookCopyStatus status,
                   std::optional<std::string> acquiredOn)
    : id_(0),
      bookId_(bookId),
      accessionNumber_(std::move(accessionNumber)),
      status_(status),
      acquiredOn_(std::move(acquiredOn)) {}

BookCopy::BookCopy(int id,
                   int bookId,
                   std::string accessionNumber,
                   BookCopyStatus status,
                   std::optional<std::string> acquiredOn)
    : id_(id),
      bookId_(bookId),
      accessionNumber_(std::move(accessionNumber)),
      status_(status),
      acquiredOn_(std::move(acquiredOn)) {}

} // namespace lms::models
