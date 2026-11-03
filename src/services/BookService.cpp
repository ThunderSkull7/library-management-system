#include "services/BookService.hpp"
#include "utils/ValidationException.hpp"
#include "utils/ValidationUtils.hpp"

namespace lms::services {

BookService::BookService(repositories::BookRepository& bookRepository)
    : bookRepository_(bookRepository) {}

models::Book BookService::validateAndNormalize(const models::Book& book) {
    std::string title = utils::trim(book.getTitle());
    if (title.empty()) {
        throw utils::ValidationException("Book title cannot be empty or whitespace-only.");
    }

    std::string author = utils::trim(book.getAuthor());
    if (author.empty()) {
        throw utils::ValidationException("Book author cannot be empty or whitespace-only.");
    }

    std::optional<std::string> isbn = std::nullopt;
    if (book.getIsbn().has_value()) {
        std::string trimmedIsbn = utils::trim(*book.getIsbn());
        if (trimmedIsbn.empty()) {
            throw utils::ValidationException("ISBN, if supplied, cannot be empty or whitespace-only.");
        }
        isbn = trimmedIsbn;
    }

    std::optional<std::string> publisher = utils::normalizeOptionalString(book.getPublisher());

    std::optional<int> pubYear = book.getPublicationYear();
    if (pubYear.has_value()) {
        if (!utils::isValidPublicationYear(*pubYear)) {
            throw utils::ValidationException(
                "Publication year must be a sensible positive year (1-2100). Received: " +
                std::to_string(*pubYear));
        }
    }

    std::optional<std::string> category = utils::normalizeOptionalString(book.getCategory());
    std::optional<std::string> description = utils::normalizeOptionalString(book.getDescription());

    return models::Book(book.getId(), std::move(title), std::move(author), std::move(isbn),
                        std::move(publisher), pubYear, std::move(category), std::move(description));
}

int BookService::addBook(const models::Book& book) {
    models::Book normalized = validateAndNormalize(book);

    if (normalized.getIsbn().has_value()) {
        auto existing = bookRepository_.getBookByIsbn(*normalized.getIsbn());
        if (existing.has_value()) {
            throw utils::ValidationException(
                "A book with ISBN '" + *normalized.getIsbn() + "' already exists.");
        }
    }

    return bookRepository_.addBook(normalized);
}

void BookService::updateBook(const models::Book& book) {
    auto existing = bookRepository_.getBookById(book.getId());
    if (!existing.has_value()) {
        throw utils::NotFoundException("Book with ID " + std::to_string(book.getId()) + " does not exist.");
    }

    models::Book normalized = validateAndNormalize(book);

    if (normalized.getIsbn().has_value()) {
        auto duplicateCheck = bookRepository_.getBookByIsbn(*normalized.getIsbn());
        if (duplicateCheck.has_value() && duplicateCheck->getId() != book.getId()) {
            throw utils::ValidationException(
                "A book with ISBN '" + *normalized.getIsbn() + "' already exists.");
        }
    }

    bookRepository_.updateBook(normalized);
}

models::Book BookService::getBook(int id) {
    auto book = bookRepository_.getBookById(id);
    if (!book.has_value()) {
        throw utils::NotFoundException("Book with ID " + std::to_string(id) + " does not exist.");
    }
    return *book;
}

std::optional<models::Book> BookService::getBookByIsbn(const std::string& isbn) {
    std::string trimmed = utils::trim(isbn);
    if (trimmed.empty()) {
        throw utils::ValidationException("ISBN search query cannot be empty or whitespace-only.");
    }
    return bookRepository_.getBookByIsbn(trimmed);
}

std::vector<models::Book> BookService::getAllBooks() {
    return bookRepository_.getAllBooks();
}

std::vector<models::Book> BookService::searchBooksByTitle(const std::string& titleQuery) {
    return bookRepository_.searchByTitle(utils::trim(titleQuery));
}

std::vector<models::Book> BookService::searchBooksByAuthor(const std::string& authorQuery) {
    return bookRepository_.searchByAuthor(utils::trim(authorQuery));
}

std::vector<models::Book> BookService::searchBooksByCategory(const std::string& categoryQuery) {
    return bookRepository_.searchByCategory(utils::trim(categoryQuery));
}

void BookService::deleteBook(int id) {
    auto book = bookRepository_.getBookById(id);
    if (!book.has_value()) {
        throw utils::NotFoundException("Book with ID " + std::to_string(id) + " does not exist.");
    }

    int copyCount = bookRepository_.getCopiesCountForBook(id);
    if (copyCount > 0) {
        throw utils::ValidationException(
            "Cannot delete book with ID " + std::to_string(id) +
            ": " + std::to_string(copyCount) + " physical copy/copies still reference it.");
    }

    bookRepository_.deleteBook(id);
}

} // namespace lms::services
