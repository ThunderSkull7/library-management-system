#include "services/BookCopyService.hpp"
#include "utils/ValidationException.hpp"
#include "utils/ValidationUtils.hpp"

namespace lms::services {

BookCopyService::BookCopyService(repositories::BookCopyRepository& copyRepository,
                                 repositories::BookRepository& bookRepository)
    : copyRepository_(copyRepository), bookRepository_(bookRepository) {}

std::string BookCopyService::validateAccessionNumber(const std::string& accessionNumber) {
    std::string trimmed = utils::trim(accessionNumber);
    if (trimmed.empty()) {
        throw utils::ValidationException("Accession number cannot be empty or whitespace-only.");
    }
    return trimmed;
}

std::optional<std::string> BookCopyService::validateAcquiredDate(const std::optional<std::string>& acquiredOn) {
    if (!acquiredOn.has_value()) {
        return std::nullopt;
    }
    std::string trimmed = utils::trim(*acquiredOn);
    if (trimmed.empty()) {
        return std::nullopt;
    }
    if (!utils::isValidIsoDate(trimmed)) {
        throw utils::ValidationException(
            "Acquired date must be a valid calendar date in YYYY-MM-DD format. Received: '" + trimmed + "'");
    }
    return trimmed;
}

int BookCopyService::addCopy(int bookId,
                            const std::string& accessionNumber,
                            std::optional<std::string> acquiredOn,
                            models::BookCopyStatus status) {
    // 1. Validate referenced book exists
    auto book = bookRepository_.getBookById(bookId);
    if (!book.has_value()) {
        throw utils::NotFoundException(
            "Referenced book with ID " + std::to_string(bookId) + " does not exist.");
    }

    // 2. Validate accession number
    std::string validAcc = validateAccessionNumber(accessionNumber);
    auto existingCopy = copyRepository_.getCopyByAccessionNumber(validAcc);
    if (existingCopy.has_value()) {
        throw utils::ValidationException(
            "A physical copy with accession number '" + validAcc + "' already exists.");
    }

    // 3. Status safety: OnLoan cannot be set manually via ordinary catalog operations
    if (status == models::BookCopyStatus::OnLoan) {
        throw utils::ValidationException(
            "Ordinary catalog operations cannot set copy status to 'OnLoan'. "
            "The loan subsystem must manage checkout transitions.");
    }

    // 4. Validate acquired date
    std::optional<std::string> validDate = validateAcquiredDate(acquiredOn);

    models::BookCopy newCopy(bookId, std::move(validAcc), status, std::move(validDate));
    return copyRepository_.addCopy(newCopy);
}

void BookCopyService::updateCopy(const models::BookCopy& copy) {
    // 1. Verify copy exists
    auto existing = copyRepository_.getCopyById(copy.getId());
    if (!existing.has_value()) {
        throw utils::NotFoundException(
            "Book copy with ID " + std::to_string(copy.getId()) + " does not exist.");
    }

    // 2. Verify referenced book exists
    auto book = bookRepository_.getBookById(copy.getBookId());
    if (!book.has_value()) {
        throw utils::NotFoundException(
            "Referenced book with ID " + std::to_string(copy.getBookId()) + " does not exist.");
    }

    // 3. Validate accession number
    std::string validAcc = validateAccessionNumber(copy.getAccessionNumber());
    auto duplicateCheck = copyRepository_.getCopyByAccessionNumber(validAcc);
    if (duplicateCheck.has_value() && duplicateCheck->getId() != copy.getId()) {
        throw utils::ValidationException(
            "A physical copy with accession number '" + validAcc + "' already exists.");
    }

    // 4. Status safety checks
    if (copy.getStatus() == models::BookCopyStatus::OnLoan && existing->getStatus() != models::BookCopyStatus::OnLoan) {
        throw utils::ValidationException(
            "Ordinary catalog operations cannot set copy status to 'OnLoan'.");
    }

    if (existing->getStatus() != copy.getStatus()) {
        if (copyRepository_.hasActiveLoan(copy.getId())) {
            throw utils::ValidationException(
                "Cannot change status of a copy that currently has an active loan.");
        }
    }

    // 5. Validate acquired date
    std::optional<std::string> validDate = validateAcquiredDate(copy.getAcquiredOn());

    models::BookCopy updated(copy.getId(), copy.getBookId(), std::move(validAcc), copy.getStatus(), std::move(validDate));
    copyRepository_.updateCopy(updated);
}

models::BookCopy BookCopyService::getCopy(int id) {
    auto copy = copyRepository_.getCopyById(id);
    if (!copy.has_value()) {
        throw utils::NotFoundException(
            "Book copy with ID " + std::to_string(id) + " does not exist.");
    }
    return *copy;
}

std::vector<models::BookCopy> BookCopyService::getCopiesByBook(int bookId) {
    auto book = bookRepository_.getBookById(bookId);
    if (!book.has_value()) {
        throw utils::NotFoundException(
            "Referenced book with ID " + std::to_string(bookId) + " does not exist.");
    }
    return copyRepository_.getCopiesByBookId(bookId);
}

int BookCopyService::getAvailableCopiesCountForBook(int bookId) {
    auto book = bookRepository_.getBookById(bookId);
    if (!book.has_value()) {
        throw utils::NotFoundException(
            "Referenced book with ID " + std::to_string(bookId) + " does not exist.");
    }
    return copyRepository_.getAvailableCopiesCountForBook(bookId);
}

std::optional<models::BookCopy> BookCopyService::searchByAccessionNumber(const std::string& accessionNumber) {
    std::string validAcc = validateAccessionNumber(accessionNumber);
    return copyRepository_.getCopyByAccessionNumber(validAcc);
}

std::vector<models::BookCopy> BookCopyService::filterCopiesByStatus(models::BookCopyStatus status) {
    return copyRepository_.getCopiesByStatus(status);
}

std::vector<models::BookCopy> BookCopyService::getAllCopies() {
    return copyRepository_.getAllCopies();
}

void BookCopyService::updateStatus(int copyId, models::BookCopyStatus newStatus) {
    auto copy = copyRepository_.getCopyById(copyId);
    if (!copy.has_value()) {
        throw utils::NotFoundException(
            "Book copy with ID " + std::to_string(copyId) + " does not exist.");
    }

    if (newStatus == models::BookCopyStatus::OnLoan) {
        throw utils::ValidationException(
            "Ordinary catalog operations cannot transition a copy to 'OnLoan'.");
    }

    if (copyRepository_.hasActiveLoan(copyId)) {
        throw utils::ValidationException(
            "Cannot change status of a copy that currently has an active loan.");
    }

    copyRepository_.updateCopyStatus(copyId, newStatus);
}

void BookCopyService::deleteCopy(int id) {
    auto copy = copyRepository_.getCopyById(id);
    if (!copy.has_value()) {
        throw utils::NotFoundException(
            "Book copy with ID " + std::to_string(id) + " does not exist.");
    }

    if (copyRepository_.hasActiveLoan(id)) {
        throw utils::ValidationException(
            "Cannot delete physical copy with ID " + std::to_string(id) +
            ": copy is currently checked out on an active loan.");
    }

    if (copyRepository_.hasLoanHistory(id)) {
        throw utils::ValidationException(
            "Cannot delete physical copy with ID " + std::to_string(id) +
            ": loan circulation history exists and must be preserved.");
    }

    copyRepository_.deleteCopy(id);
}

} // namespace lms::services
