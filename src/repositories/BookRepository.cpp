#include "repositories/BookRepository.hpp"
#include "database/DatabaseException.hpp"
#include "database/PreparedStatement.hpp"

#include <sqlite3.h>

namespace lms::repositories {

BookRepository::BookRepository(database::DatabaseConnection& connection)
    : connection_(connection) {}

int BookRepository::addBook(const models::Book& book) {
    sqlite3* db = connection_.getRawHandle();
    const std::string sql =
        "INSERT INTO books (isbn, title, author, publisher, publication_year, category, description) "
        "VALUES (?, ?, ?, ?, ?, ?, ?);";

    auto stmt = database::prepareStatement(db, sql);

    // 1: isbn
    if (book.getIsbn().has_value()) {
        sqlite3_bind_text(stmt.get(), 1, book.getIsbn()->c_str(), -1, SQLITE_TRANSIENT);
    } else {
        sqlite3_bind_null(stmt.get(), 1);
    }

    // 2: title
    sqlite3_bind_text(stmt.get(), 2, book.getTitle().c_str(), -1, SQLITE_TRANSIENT);

    // 3: author
    sqlite3_bind_text(stmt.get(), 3, book.getAuthor().c_str(), -1, SQLITE_TRANSIENT);

    // 4: publisher
    if (book.getPublisher().has_value()) {
        sqlite3_bind_text(stmt.get(), 4, book.getPublisher()->c_str(), -1, SQLITE_TRANSIENT);
    } else {
        sqlite3_bind_null(stmt.get(), 4);
    }

    // 5: publication_year
    if (book.getPublicationYear().has_value()) {
        sqlite3_bind_int(stmt.get(), 5, *book.getPublicationYear());
    } else {
        sqlite3_bind_null(stmt.get(), 5);
    }

    // 6: category
    if (book.getCategory().has_value()) {
        sqlite3_bind_text(stmt.get(), 6, book.getCategory()->c_str(), -1, SQLITE_TRANSIENT);
    } else {
        sqlite3_bind_null(stmt.get(), 6);
    }

    // 7: description
    if (book.getDescription().has_value()) {
        sqlite3_bind_text(stmt.get(), 7, book.getDescription()->c_str(), -1, SQLITE_TRANSIENT);
    } else {
        sqlite3_bind_null(stmt.get(), 7);
    }

    int rc = sqlite3_step(stmt.get());
    if (rc != SQLITE_DONE) {
        std::string errMsg = sqlite3_errmsg(db);
        throw database::DatabaseException("Failed to add book: " + errMsg, rc);
    }

    return static_cast<int>(sqlite3_last_insert_rowid(db));
}

bool BookRepository::updateBook(const models::Book& book) {
    sqlite3* db = connection_.getRawHandle();
    const std::string sql =
        "UPDATE books SET isbn = ?, title = ?, author = ?, publisher = ?, "
        "publication_year = ?, category = ?, description = ? WHERE id = ?;";

    auto stmt = database::prepareStatement(db, sql);

    // 1: isbn
    if (book.getIsbn().has_value()) {
        sqlite3_bind_text(stmt.get(), 1, book.getIsbn()->c_str(), -1, SQLITE_TRANSIENT);
    } else {
        sqlite3_bind_null(stmt.get(), 1);
    }

    // 2: title
    sqlite3_bind_text(stmt.get(), 2, book.getTitle().c_str(), -1, SQLITE_TRANSIENT);

    // 3: author
    sqlite3_bind_text(stmt.get(), 3, book.getAuthor().c_str(), -1, SQLITE_TRANSIENT);

    // 4: publisher
    if (book.getPublisher().has_value()) {
        sqlite3_bind_text(stmt.get(), 4, book.getPublisher()->c_str(), -1, SQLITE_TRANSIENT);
    } else {
        sqlite3_bind_null(stmt.get(), 4);
    }

    // 5: publication_year
    if (book.getPublicationYear().has_value()) {
        sqlite3_bind_int(stmt.get(), 5, *book.getPublicationYear());
    } else {
        sqlite3_bind_null(stmt.get(), 5);
    }

    // 6: category
    if (book.getCategory().has_value()) {
        sqlite3_bind_text(stmt.get(), 6, book.getCategory()->c_str(), -1, SQLITE_TRANSIENT);
    } else {
        sqlite3_bind_null(stmt.get(), 6);
    }

    // 7: description
    if (book.getDescription().has_value()) {
        sqlite3_bind_text(stmt.get(), 7, book.getDescription()->c_str(), -1, SQLITE_TRANSIENT);
    } else {
        sqlite3_bind_null(stmt.get(), 7);
    }

    // 8: id
    sqlite3_bind_int(stmt.get(), 8, book.getId());

    int rc = sqlite3_step(stmt.get());
    if (rc != SQLITE_DONE) {
        std::string errMsg = sqlite3_errmsg(db);
        throw database::DatabaseException("Failed to update book: " + errMsg, rc);
    }

    return sqlite3_changes(db) > 0;
}

std::optional<models::Book> BookRepository::getBookById(int id) {
    sqlite3* db = connection_.getRawHandle();
    const std::string sql =
        "SELECT id, isbn, title, author, publisher, publication_year, category, description "
        "FROM books WHERE id = ?;";

    auto stmt = database::prepareStatement(db, sql);
    sqlite3_bind_int(stmt.get(), 1, id);

    int rc = sqlite3_step(stmt.get());
    if (rc == SQLITE_ROW) {
        return extractBookFromStatement(stmt.get());
    }
    if (rc == SQLITE_DONE) {
        return std::nullopt;
    }

    std::string errMsg = sqlite3_errmsg(db);
    throw database::DatabaseException("Error querying book by ID: " + errMsg, rc);
}

std::optional<models::Book> BookRepository::getBookByIsbn(const std::string& isbn) {
    sqlite3* db = connection_.getRawHandle();
    const std::string sql =
        "SELECT id, isbn, title, author, publisher, publication_year, category, description "
        "FROM books WHERE isbn = ?;";

    auto stmt = database::prepareStatement(db, sql);
    sqlite3_bind_text(stmt.get(), 1, isbn.c_str(), -1, SQLITE_TRANSIENT);

    int rc = sqlite3_step(stmt.get());
    if (rc == SQLITE_ROW) {
        return extractBookFromStatement(stmt.get());
    }
    if (rc == SQLITE_DONE) {
        return std::nullopt;
    }

    std::string errMsg = sqlite3_errmsg(db);
    throw database::DatabaseException("Error querying book by ISBN: " + errMsg, rc);
}

std::vector<models::Book> BookRepository::getAllBooks() {
    sqlite3* db = connection_.getRawHandle();
    const std::string sql =
        "SELECT id, isbn, title, author, publisher, publication_year, category, description "
        "FROM books ORDER BY id ASC;";

    auto stmt = database::prepareStatement(db, sql);
    std::vector<models::Book> books;

    while (true) {
        int rc = sqlite3_step(stmt.get());
        if (rc == SQLITE_ROW) {
            books.push_back(extractBookFromStatement(stmt.get()));
        } else if (rc == SQLITE_DONE) {
            break;
        } else {
            std::string errMsg = sqlite3_errmsg(db);
            throw database::DatabaseException("Error querying all books: " + errMsg, rc);
        }
    }

    return books;
}

std::vector<models::Book> BookRepository::searchByTitle(const std::string& titleQuery) {
    sqlite3* db = connection_.getRawHandle();
    const std::string sql =
        "SELECT id, isbn, title, author, publisher, publication_year, category, description "
        "FROM books WHERE title LIKE ? ORDER BY title ASC;";

    auto stmt = database::prepareStatement(db, sql);
    std::string pattern = "%" + titleQuery + "%";
    sqlite3_bind_text(stmt.get(), 1, pattern.c_str(), -1, SQLITE_TRANSIENT);

    std::vector<models::Book> books;
    while (true) {
        int rc = sqlite3_step(stmt.get());
        if (rc == SQLITE_ROW) {
            books.push_back(extractBookFromStatement(stmt.get()));
        } else if (rc == SQLITE_DONE) {
            break;
        } else {
            std::string errMsg = sqlite3_errmsg(db);
            throw database::DatabaseException("Error searching books by title: " + errMsg, rc);
        }
    }
    return books;
}

std::vector<models::Book> BookRepository::searchByAuthor(const std::string& authorQuery) {
    sqlite3* db = connection_.getRawHandle();
    const std::string sql =
        "SELECT id, isbn, title, author, publisher, publication_year, category, description "
        "FROM books WHERE author LIKE ? ORDER BY author ASC;";

    auto stmt = database::prepareStatement(db, sql);
    std::string pattern = "%" + authorQuery + "%";
    sqlite3_bind_text(stmt.get(), 1, pattern.c_str(), -1, SQLITE_TRANSIENT);

    std::vector<models::Book> books;
    while (true) {
        int rc = sqlite3_step(stmt.get());
        if (rc == SQLITE_ROW) {
            books.push_back(extractBookFromStatement(stmt.get()));
        } else if (rc == SQLITE_DONE) {
            break;
        } else {
            std::string errMsg = sqlite3_errmsg(db);
            throw database::DatabaseException("Error searching books by author: " + errMsg, rc);
        }
    }
    return books;
}

std::vector<models::Book> BookRepository::searchByCategory(const std::string& categoryQuery) {
    sqlite3* db = connection_.getRawHandle();
    const std::string sql =
        "SELECT id, isbn, title, author, publisher, publication_year, category, description "
        "FROM books WHERE category LIKE ? ORDER BY title ASC;";

    auto stmt = database::prepareStatement(db, sql);
    std::string pattern = "%" + categoryQuery + "%";
    sqlite3_bind_text(stmt.get(), 1, pattern.c_str(), -1, SQLITE_TRANSIENT);

    std::vector<models::Book> books;
    while (true) {
        int rc = sqlite3_step(stmt.get());
        if (rc == SQLITE_ROW) {
            books.push_back(extractBookFromStatement(stmt.get()));
        } else if (rc == SQLITE_DONE) {
            break;
        } else {
            std::string errMsg = sqlite3_errmsg(db);
            throw database::DatabaseException("Error searching books by category: " + errMsg, rc);
        }
    }
    return books;
}

int BookRepository::getCopiesCountForBook(int bookId) {
    sqlite3* db = connection_.getRawHandle();
    const std::string sql = "SELECT COUNT(*) FROM book_copies WHERE book_id = ?;";

    auto stmt = database::prepareStatement(db, sql);
    sqlite3_bind_int(stmt.get(), 1, bookId);

    int rc = sqlite3_step(stmt.get());
    if (rc == SQLITE_ROW) {
        return sqlite3_column_int(stmt.get(), 0);
    }

    std::string errMsg = sqlite3_errmsg(db);
    throw database::DatabaseException("Error counting copies for book: " + errMsg, rc);
}

bool BookRepository::deleteBook(int id) {
    if (getCopiesCountForBook(id) > 0) {
        throw database::DatabaseException("Cannot delete book: physical copies exist referencing this book.");
    }

    sqlite3* db = connection_.getRawHandle();
    const std::string sql = "DELETE FROM books WHERE id = ?;";

    auto stmt = database::prepareStatement(db, sql);
    sqlite3_bind_int(stmt.get(), 1, id);

    int rc = sqlite3_step(stmt.get());
    if (rc != SQLITE_DONE) {
        std::string errMsg = sqlite3_errmsg(db);
        throw database::DatabaseException("Failed to delete book: " + errMsg, rc);
    }

    return sqlite3_changes(db) > 0;
}

models::Book BookRepository::extractBookFromStatement(sqlite3_stmt* stmt) {
    int id = sqlite3_column_int(stmt, 0);

    std::optional<std::string> isbn = std::nullopt;
    if (sqlite3_column_type(stmt, 1) != SQLITE_NULL) {
        isbn = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
    }

    std::string title = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
    std::string author = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));

    std::optional<std::string> publisher = std::nullopt;
    if (sqlite3_column_type(stmt, 4) != SQLITE_NULL) {
        publisher = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
    }

    std::optional<int> publicationYear = std::nullopt;
    if (sqlite3_column_type(stmt, 5) != SQLITE_NULL) {
        publicationYear = sqlite3_column_int(stmt, 5);
    }

    std::optional<std::string> category = std::nullopt;
    if (sqlite3_column_type(stmt, 6) != SQLITE_NULL) {
        category = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
    }

    std::optional<std::string> description = std::nullopt;
    if (sqlite3_column_type(stmt, 7) != SQLITE_NULL) {
        description = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
    }

    return models::Book(id, std::move(title), std::move(author), std::move(isbn),
                        std::move(publisher), publicationYear, std::move(category),
                        std::move(description));
}

} // namespace lms::repositories
