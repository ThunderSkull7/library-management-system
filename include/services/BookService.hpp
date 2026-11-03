#pragma once

#include "models/Book.hpp"
#include "repositories/BookRepository.hpp"

#include <optional>
#include <string>
#include <vector>

namespace lms::services {

/**
 * @brief Service responsible for catalog book operations and business validation.
 */
class BookService {
public:
    explicit BookService(repositories::BookRepository& bookRepository);

    /**
     * @brief Adds a new book to the catalog after validating business rules.
     * @throws ValidationException if title, author, ISBN, or publication year are invalid or if duplicate ISBN.
     * @return Generated ID of the created book.
     */
    int addBook(const models::Book& book);

    /**
     * @brief Updates an existing book's details after validating business rules.
     * @throws NotFoundException if book ID does not exist.
     * @throws ValidationException on validation or duplicate ISBN conflict.
     */
    void updateBook(const models::Book& book);

    /**
     * @brief Retrieves a book by ID.
     * @throws NotFoundException if book does not exist.
     */
    [[nodiscard]] models::Book getBook(int id);

    /**
     * @brief Retrieves a book by ISBN.
     * @throws ValidationException if isbn parameter is empty or whitespace.
     */
    [[nodiscard]] std::optional<models::Book> getBookByIsbn(const std::string& isbn);

    /**
     * @brief Retrieves all catalog books.
     */
    [[nodiscard]] std::vector<models::Book> getAllBooks();

    /**
     * @brief Searches catalog books by title substring.
     */
    [[nodiscard]] std::vector<models::Book> searchBooksByTitle(const std::string& titleQuery);

    /**
     * @brief Searches catalog books by author substring.
     */
    [[nodiscard]] std::vector<models::Book> searchBooksByAuthor(const std::string& authorQuery);

    /**
     * @brief Searches catalog books by category substring.
     */
    [[nodiscard]] std::vector<models::Book> searchBooksByCategory(const std::string& categoryQuery);

    /**
     * @brief Safely deletes a book from the catalog.
     * @throws NotFoundException if book does not exist.
     * @throws ValidationException if physical copies reference this book.
     */
    void deleteBook(int id);

private:
    repositories::BookRepository& bookRepository_;

    static models::Book validateAndNormalize(const models::Book& book);
};

} // namespace lms::services
