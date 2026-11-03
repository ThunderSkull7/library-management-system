#pragma once

#include "database/DatabaseConnection.hpp"
#include "models/BookCopy.hpp"

#include <optional>
#include <string>
#include <vector>

struct sqlite3_stmt;

namespace lms::repositories {

/**
 * @brief Repository for persisting and querying BookCopy entities in SQLite.
 */
class BookCopyRepository {
public:
    explicit BookCopyRepository(database::DatabaseConnection& connection);

    /**
     * @brief Inserts a new BookCopy record.
     * @return Generated database ID.
     * @throws DatabaseException on constraint violation (e.g. duplicate accession number or invalid book_id).
     */
    int addCopy(const models::BookCopy& copy);

    /**
     * @brief Updates an existing BookCopy record.
     * @return True if updated, false if not found.
     * @throws DatabaseException if update fails.
     */
    bool updateCopy(const models::BookCopy& copy);

    /**
     * @brief Retrieves a physical copy by its primary key ID.
     */
    [[nodiscard]] std::optional<models::BookCopy> getCopyById(int id);

    /**
     * @brief Retrieves a physical copy by its accession number.
     */
    [[nodiscard]] std::optional<models::BookCopy> getCopyByAccessionNumber(const std::string& accessionNumber);

    /**
     * @brief Retrieves all physical copies associated with a book ID.
     */
    [[nodiscard]] std::vector<models::BookCopy> getCopiesByBookId(int bookId);

    /**
     * @brief Retrieves all physical copies in the library.
     */
    [[nodiscard]] std::vector<models::BookCopy> getAllCopies();

    /**
     * @brief Retrieves physical copies filtered by their current status.
     */
    [[nodiscard]] std::vector<models::BookCopy> getCopiesByStatus(models::BookCopyStatus status);

    /**
     * @brief Updates the status of a physical copy.
     * @return True if updated, false if copy not found.
     */
    bool updateCopyStatus(int copyId, models::BookCopyStatus newStatus);

    /**
     * @brief Returns true if any loan record exists for this copy (active or returned).
     */
    [[nodiscard]] bool hasLoanHistory(int copyId);

    /**
     * @brief Returns true if an active (unreturned) loan exists for this copy.
     */
    [[nodiscard]] bool hasActiveLoan(int copyId);

    /**
     * @brief Deletes a BookCopy by ID.
     * @throws DatabaseException if loan history exists or deletion fails.
     * @return True if deleted, false if copy not found.
     */
    bool deleteCopy(int id);

private:
    database::DatabaseConnection& connection_;

    [[nodiscard]] static models::BookCopy extractCopyFromStatement(sqlite3_stmt* stmt);
};

} // namespace lms::repositories
