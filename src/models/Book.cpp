#include "models/Book.hpp"

#include <utility>

namespace lms::models {

Book::Book(std::string title,
           std::string author,
           std::optional<std::string> isbn,
           std::optional<std::string> publisher,
           std::optional<int> publicationYear,
           std::optional<std::string> category,
           std::optional<std::string> description)
    : id_(0),
      title_(std::move(title)),
      author_(std::move(author)),
      isbn_(std::move(isbn)),
      publisher_(std::move(publisher)),
      publicationYear_(publicationYear),
      category_(std::move(category)),
      description_(std::move(description)) {}

Book::Book(int id,
           std::string title,
           std::string author,
           std::optional<std::string> isbn,
           std::optional<std::string> publisher,
           std::optional<int> publicationYear,
           std::optional<std::string> category,
           std::optional<std::string> description)
    : id_(id),
      title_(std::move(title)),
      author_(std::move(author)),
      isbn_(std::move(isbn)),
      publisher_(std::move(publisher)),
      publicationYear_(publicationYear),
      category_(std::move(category)),
      description_(std::move(description)) {}

} // namespace lms::models
