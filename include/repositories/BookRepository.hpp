#pragma once

#include "database/DatabaseConnection.hpp"
#include "models/Book.hpp"

#include <optional>
#include <string>
#include <vector>

struct sqlite3_stmt;

namespace lms::repositories {

/**
 * @brief Repository for persisting and querying Book entities in SQLite.
 */
class BookRepository {
public:
    explicit BookRepository(database::DatabaseConnection& connection);

    /**
     * @brief Inserts a new Book record.
     * @return Generated database ID.
     * @throws DatabaseException if insertion fails (e.g. duplicate ISBN).
     */
    int addBook(const models::Book& book);

    /**
     * @brief Updates an existing Book record.
     * @return True if a record was updated, false if not found.
     * @throws DatabaseException if update fails.
     */
    bool updateBook(const models::Book& book);

    /**
     * @brief Retrieves a Book by its primary key ID.
     */
    [[nodiscard]] std::optional<models::Book> getBookById(int id);

    /**
     * @brief Retrieves a Book by its ISBN.
     */
    [[nodiscard]] std::optional<models::Book> getBookByIsbn(const std::string& isbn);

    /**
     * @brief Retrieves all books in the catalog ordered by ID.
     */
    [[nodiscard]] std::vector<models::Book> getAllBooks();

    /**
     * @brief Searches for books whose title contains titleQuery (case-insensitive ASCII).
     */
    [[nodiscard]] std::vector<models::Book> searchByTitle(const std::string& titleQuery);

    /**
     * @brief Searches for books whose author contains authorQuery (case-insensitive ASCII).
     */
    [[nodiscard]] std::vector<models::Book> searchByAuthor(const std::string& authorQuery);

    /**
     * @brief Searches for books whose category contains categoryQuery (case-insensitive ASCII).
     */
    [[nodiscard]] std::vector<models::Book> searchByCategory(const std::string& categoryQuery);

    /**
     * @brief Returns the number of physical copies referencing this book.
     */
    [[nodiscard]] int getCopiesCountForBook(int bookId);

    /**
     * @brief Deletes a Book by its ID.
     * @throws DatabaseException if physical copies exist or deletion fails.
     * @return True if deleted, false if book not found.
     */
    bool deleteBook(int id);

private:
    database::DatabaseConnection& connection_;

    [[nodiscard]] static models::Book extractBookFromStatement(sqlite3_stmt* stmt);
};

} // namespace lms::repositories
