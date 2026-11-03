#pragma once

#include "models/BookCopy.hpp"
#include "repositories/BookCopyRepository.hpp"
#include "repositories/BookRepository.hpp"

#include <optional>
#include <string>
#include <vector>

namespace lms::services {

/**
 * @brief Service responsible for physical book copy operations, lifecycle states, and business validation.
 */
class BookCopyService {
public:
    BookCopyService(repositories::BookCopyRepository& copyRepository,
                    repositories::BookRepository& bookRepository);

    /**
     * @brief Adds a physical copy for an existing catalog book.
     * @throws NotFoundException if the referenced book does not exist.
     * @throws ValidationException on empty/duplicate accession number, invalid acquired date, or manual OnLoan status.
     * @return Generated ID of the created copy.
     */
    int addCopy(int bookId,
                const std::string& accessionNumber,
                std::optional<std::string> acquiredOn = std::nullopt,
                models::BookCopyStatus status = models::BookCopyStatus::Available);

    /**
     * @brief Updates an existing copy's metadata and/or status.
     * @throws NotFoundException if copy or book does not exist.
     * @throws ValidationException on empty/duplicate accession number, invalid date, manual OnLoan, or changing active loan status.
     */
    void updateCopy(const models::BookCopy& copy);

    /**
     * @brief Retrieves a physical copy by ID.
     * @throws NotFoundException if copy does not exist.
     */
    [[nodiscard]] models::BookCopy getCopy(int id);

    /**
     * @brief Retrieves all physical copies of a specific book.
     * @throws NotFoundException if the referenced book does not exist.
     */
    [[nodiscard]] std::vector<models::BookCopy> getCopiesByBook(int bookId);

    /**
     * @brief Searches for a physical copy by its exact accession number.
     * @throws ValidationException if accessionNumber is empty or whitespace.
     */
    [[nodiscard]] std::optional<models::BookCopy> searchByAccessionNumber(const std::string& accessionNumber);

    /**
     * @brief Filters physical copies by their current status.
     */
    [[nodiscard]] std::vector<models::BookCopy> filterCopiesByStatus(models::BookCopyStatus status);

    /**
     * @brief Retrieves all physical copies in the library.
     */
    [[nodiscard]] std::vector<models::BookCopy> getAllCopies();

    /**
     * @brief Updates the status of a physical copy, enforcing loan safety constraints.
     * @throws NotFoundException if copy does not exist.
     * @throws ValidationException if setting to OnLoan manually or modifying a copy currently on active loan.
     */
    void updateStatus(int copyId, models::BookCopyStatus newStatus);

    /**
     * @brief Safely deletes a physical copy if no loan history or active loan exists.
     * @throws NotFoundException if copy does not exist.
     * @throws ValidationException if the copy has historical or active loan records.
     */
    void deleteCopy(int id);

private:
    repositories::BookCopyRepository& copyRepository_;
    repositories::BookRepository& bookRepository_;

    static std::string validateAccessionNumber(const std::string& accessionNumber);
    static std::optional<std::string> validateAcquiredDate(const std::optional<std::string>& acquiredOn);
};

} // namespace lms::services
