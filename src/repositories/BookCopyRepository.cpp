#include "repositories/BookCopyRepository.hpp"
#include "database/DatabaseException.hpp"
#include "database/PreparedStatement.hpp"

#include <sqlite3.h>

namespace lms::repositories {

BookCopyRepository::BookCopyRepository(database::DatabaseConnection& connection)
    : connection_(connection) {}

int BookCopyRepository::addCopy(const models::BookCopy& copy) {
    sqlite3* db = connection_.getRawHandle();
    const std::string sql =
        "INSERT INTO book_copies (book_id, accession_number, status, acquired_on) "
        "VALUES (?, ?, ?, ?);";

    auto stmt = database::prepareStatement(db, sql);

    // 1: book_id
    sqlite3_bind_int(stmt.get(), 1, copy.getBookId());

    // 2: accession_number
    sqlite3_bind_text(stmt.get(), 2, copy.getAccessionNumber().c_str(), -1, SQLITE_TRANSIENT);

    // 3: status
    std::string statusStr = models::toString(copy.getStatus());
    sqlite3_bind_text(stmt.get(), 3, statusStr.c_str(), -1, SQLITE_TRANSIENT);

    // 4: acquired_on
    if (copy.getAcquiredOn().has_value()) {
        sqlite3_bind_text(stmt.get(), 4, copy.getAcquiredOn()->c_str(), -1, SQLITE_TRANSIENT);
    } else {
        sqlite3_bind_null(stmt.get(), 4);
    }

    int rc = sqlite3_step(stmt.get());
    if (rc != SQLITE_DONE) {
        std::string errMsg = sqlite3_errmsg(db);
        throw database::DatabaseException("Failed to add book copy: " + errMsg, rc);
    }

    return static_cast<int>(sqlite3_last_insert_rowid(db));
}

bool BookCopyRepository::updateCopy(const models::BookCopy& copy) {
    sqlite3* db = connection_.getRawHandle();
    const std::string sql =
        "UPDATE book_copies SET book_id = ?, accession_number = ?, status = ?, acquired_on = ? "
        "WHERE id = ?;";

    auto stmt = database::prepareStatement(db, sql);

    // 1: book_id
    sqlite3_bind_int(stmt.get(), 1, copy.getBookId());

    // 2: accession_number
    sqlite3_bind_text(stmt.get(), 2, copy.getAccessionNumber().c_str(), -1, SQLITE_TRANSIENT);

    // 3: status
    std::string statusStr = models::toString(copy.getStatus());
    sqlite3_bind_text(stmt.get(), 3, statusStr.c_str(), -1, SQLITE_TRANSIENT);

    // 4: acquired_on
    if (copy.getAcquiredOn().has_value()) {
        sqlite3_bind_text(stmt.get(), 4, copy.getAcquiredOn()->c_str(), -1, SQLITE_TRANSIENT);
    } else {
        sqlite3_bind_null(stmt.get(), 4);
    }

    // 5: id
    sqlite3_bind_int(stmt.get(), 5, copy.getId());

    int rc = sqlite3_step(stmt.get());
    if (rc != SQLITE_DONE) {
        std::string errMsg = sqlite3_errmsg(db);
        throw database::DatabaseException("Failed to update book copy: " + errMsg, rc);
    }

    return sqlite3_changes(db) > 0;
}

std::optional<models::BookCopy> BookCopyRepository::getCopyById(int id) {
    sqlite3* db = connection_.getRawHandle();
    const std::string sql =
        "SELECT id, book_id, accession_number, status, acquired_on "
        "FROM book_copies WHERE id = ?;";

    auto stmt = database::prepareStatement(db, sql);
    sqlite3_bind_int(stmt.get(), 1, id);

    int rc = sqlite3_step(stmt.get());
    if (rc == SQLITE_ROW) {
        return extractCopyFromStatement(stmt.get());
    }
    if (rc == SQLITE_DONE) {
        return std::nullopt;
    }

    std::string errMsg = sqlite3_errmsg(db);
    throw database::DatabaseException("Error querying book copy by ID: " + errMsg, rc);
}

std::optional<models::BookCopy> BookCopyRepository::getCopyByAccessionNumber(const std::string& accessionNumber) {
    sqlite3* db = connection_.getRawHandle();
    const std::string sql =
        "SELECT id, book_id, accession_number, status, acquired_on "
        "FROM book_copies WHERE accession_number = ?;";

    auto stmt = database::prepareStatement(db, sql);
    sqlite3_bind_text(stmt.get(), 1, accessionNumber.c_str(), -1, SQLITE_TRANSIENT);

    int rc = sqlite3_step(stmt.get());
    if (rc == SQLITE_ROW) {
        return extractCopyFromStatement(stmt.get());
    }
    if (rc == SQLITE_DONE) {
        return std::nullopt;
    }

    std::string errMsg = sqlite3_errmsg(db);
    throw database::DatabaseException("Error querying book copy by accession number: " + errMsg, rc);
}

std::vector<models::BookCopy> BookCopyRepository::getCopiesByBookId(int bookId) {
    sqlite3* db = connection_.getRawHandle();
    const std::string sql =
        "SELECT id, book_id, accession_number, status, acquired_on "
        "FROM book_copies WHERE book_id = ? ORDER BY id ASC;";

    auto stmt = database::prepareStatement(db, sql);
    sqlite3_bind_int(stmt.get(), 1, bookId);

    std::vector<models::BookCopy> copies;
    while (true) {
        int rc = sqlite3_step(stmt.get());
        if (rc == SQLITE_ROW) {
            copies.push_back(extractCopyFromStatement(stmt.get()));
        } else if (rc == SQLITE_DONE) {
            break;
        } else {
            std::string errMsg = sqlite3_errmsg(db);
            throw database::DatabaseException("Error querying copies by book ID: " + errMsg, rc);
        }
    }
    return copies;
}

int BookCopyRepository::getAvailableCopiesCountForBook(int bookId) {
    sqlite3* db = connection_.getRawHandle();
    const std::string sql =
        "SELECT COUNT(*) FROM book_copies WHERE book_id = ? AND status = 'Available';";

    auto stmt = database::prepareStatement(db, sql);
    sqlite3_bind_int(stmt.get(), 1, bookId);

    int rc = sqlite3_step(stmt.get());
    if (rc == SQLITE_ROW) {
        return sqlite3_column_int(stmt.get(), 0);
    }

    std::string errMsg = sqlite3_errmsg(db);
    throw database::DatabaseException("Error counting available copies for book: " + errMsg, rc);
}

std::vector<models::BookCopy> BookCopyRepository::getAllCopies() {
    sqlite3* db = connection_.getRawHandle();
    const std::string sql =
        "SELECT id, book_id, accession_number, status, acquired_on "
        "FROM book_copies ORDER BY id ASC;";

    auto stmt = database::prepareStatement(db, sql);
    std::vector<models::BookCopy> copies;

    while (true) {
        int rc = sqlite3_step(stmt.get());
        if (rc == SQLITE_ROW) {
            copies.push_back(extractCopyFromStatement(stmt.get()));
        } else if (rc == SQLITE_DONE) {
            break;
        } else {
            std::string errMsg = sqlite3_errmsg(db);
            throw database::DatabaseException("Error querying all book copies: " + errMsg, rc);
        }
    }
    return copies;
}

std::vector<models::BookCopy> BookCopyRepository::getCopiesByStatus(models::BookCopyStatus status) {
    sqlite3* db = connection_.getRawHandle();
    const std::string sql =
        "SELECT id, book_id, accession_number, status, acquired_on "
        "FROM book_copies WHERE status = ? ORDER BY id ASC;";

    auto stmt = database::prepareStatement(db, sql);
    std::string statusStr = models::toString(status);
    sqlite3_bind_text(stmt.get(), 1, statusStr.c_str(), -1, SQLITE_TRANSIENT);

    std::vector<models::BookCopy> copies;
    while (true) {
        int rc = sqlite3_step(stmt.get());
        if (rc == SQLITE_ROW) {
            copies.push_back(extractCopyFromStatement(stmt.get()));
        } else if (rc == SQLITE_DONE) {
            break;
        } else {
            std::string errMsg = sqlite3_errmsg(db);
            throw database::DatabaseException("Error querying copies by status: " + errMsg, rc);
        }
    }
    return copies;
}

bool BookCopyRepository::updateCopyStatus(int copyId, models::BookCopyStatus newStatus) {
    sqlite3* db = connection_.getRawHandle();
    const std::string sql = "UPDATE book_copies SET status = ? WHERE id = ?;";

    auto stmt = database::prepareStatement(db, sql);
    std::string statusStr = models::toString(newStatus);
    sqlite3_bind_text(stmt.get(), 1, statusStr.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt.get(), 2, copyId);

    int rc = sqlite3_step(stmt.get());
    if (rc != SQLITE_DONE) {
        std::string errMsg = sqlite3_errmsg(db);
        throw database::DatabaseException("Failed to update book copy status: " + errMsg, rc);
    }

    return sqlite3_changes(db) > 0;
}

bool BookCopyRepository::hasLoanHistory(int copyId) {
    sqlite3* db = connection_.getRawHandle();
    const std::string sql = "SELECT COUNT(*) FROM loans WHERE copy_id = ?;";

    auto stmt = database::prepareStatement(db, sql);
    sqlite3_bind_int(stmt.get(), 1, copyId);

    int rc = sqlite3_step(stmt.get());
    if (rc == SQLITE_ROW) {
        return sqlite3_column_int(stmt.get(), 0) > 0;
    }

    std::string errMsg = sqlite3_errmsg(db);
    throw database::DatabaseException("Error checking copy loan history: " + errMsg, rc);
}

bool BookCopyRepository::hasActiveLoan(int copyId) {
    sqlite3* db = connection_.getRawHandle();
    const std::string sql = "SELECT COUNT(*) FROM loans WHERE copy_id = ? AND returned_on IS NULL;";

    auto stmt = database::prepareStatement(db, sql);
    sqlite3_bind_int(stmt.get(), 1, copyId);

    int rc = sqlite3_step(stmt.get());
    if (rc == SQLITE_ROW) {
        return sqlite3_column_int(stmt.get(), 0) > 0;
    }

    std::string errMsg = sqlite3_errmsg(db);
    throw database::DatabaseException("Error checking copy active loan: " + errMsg, rc);
}

bool BookCopyRepository::deleteCopy(int id) {
    if (hasLoanHistory(id)) {
        throw database::DatabaseException("Cannot delete physical copy: loan history exists.");
    }

    sqlite3* db = connection_.getRawHandle();
    const std::string sql = "DELETE FROM book_copies WHERE id = ?;";

    auto stmt = database::prepareStatement(db, sql);
    sqlite3_bind_int(stmt.get(), 1, id);

    int rc = sqlite3_step(stmt.get());
    if (rc != SQLITE_DONE) {
        std::string errMsg = sqlite3_errmsg(db);
        throw database::DatabaseException("Failed to delete book copy: " + errMsg, rc);
    }

    return sqlite3_changes(db) > 0;
}

models::BookCopy BookCopyRepository::extractCopyFromStatement(sqlite3_stmt* stmt) {
    int id = sqlite3_column_int(stmt, 0);
    int bookId = sqlite3_column_int(stmt, 1);
    std::string accessionNumber = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));

    std::string statusStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
    models::BookCopyStatus status = models::bookCopyStatusFromString(statusStr);

    std::optional<std::string> acquiredOn = std::nullopt;
    if (sqlite3_column_type(stmt, 4) != SQLITE_NULL) {
        acquiredOn = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
    }

    return models::BookCopy(id, bookId, std::move(accessionNumber), status, std::move(acquiredOn));
}

} // namespace lms::repositories
