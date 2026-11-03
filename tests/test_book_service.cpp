#include "database/DatabaseConnection.hpp"
#include "database/SchemaManager.hpp"
#include "models/Book.hpp"
#include "repositories/BookCopyRepository.hpp"
#include "repositories/BookRepository.hpp"
#include "services/BookService.hpp"
#include "utils/ValidationException.hpp"

#include <gtest/gtest.h>
#include <memory>
#include <optional>
#include <string>

namespace {

class BookServiceTest : public ::testing::Test {
protected:
    void SetUp() override {
        connection_ = std::make_unique<lms::database::DatabaseConnection>(":memory:");
        schemaManager_ = std::make_unique<lms::database::SchemaManager>(*connection_);
        schemaManager_->initializeSchema();

        bookRepo_ = std::make_unique<lms::repositories::BookRepository>(*connection_);
        copyRepo_ = std::make_unique<lms::repositories::BookCopyRepository>(*connection_);
        bookService_ = std::make_unique<lms::services::BookService>(*bookRepo_);
    }

    void TearDown() override {
        bookService_.reset();
        copyRepo_.reset();
        bookRepo_.reset();
        schemaManager_.reset();
        connection_.reset();
    }

    std::unique_ptr<lms::database::DatabaseConnection> connection_;
    std::unique_ptr<lms::database::SchemaManager> schemaManager_;
    std::unique_ptr<lms::repositories::BookRepository> bookRepo_;
    std::unique_ptr<lms::repositories::BookCopyRepository> copyRepo_;
    std::unique_ptr<lms::services::BookService> bookService_;
};

TEST_F(BookServiceTest, AddValidBook) {
    lms::models::Book book("  Clean Architecture  ", "  Robert C. Martin  ", "  978-0134494166  ",
                           "  Prentice Hall  ", 2017, "  Software Design  ", "  Solid architecture  ");

    int id = bookService_->addBook(book);
    EXPECT_GT(id, 0);

    auto retrieved = bookService_->getBook(id);
    EXPECT_EQ(retrieved.getTitle(), "Clean Architecture");
    EXPECT_EQ(retrieved.getAuthor(), "Robert C. Martin");
    ASSERT_TRUE(retrieved.getIsbn().has_value());
    EXPECT_EQ(*retrieved.getIsbn(), "978-0134494166");
    ASSERT_TRUE(retrieved.getPublisher().has_value());
    EXPECT_EQ(*retrieved.getPublisher(), "Prentice Hall");
    ASSERT_TRUE(retrieved.getCategory().has_value());
    EXPECT_EQ(*retrieved.getCategory(), "Software Design");
    ASSERT_TRUE(retrieved.getDescription().has_value());
    EXPECT_EQ(*retrieved.getDescription(), "Solid architecture");
}

TEST_F(BookServiceTest, RejectEmptyOrWhitespaceTitle) {
    EXPECT_THROW(
        bookService_->addBook(lms::models::Book("", "Valid Author")),
        lms::utils::ValidationException);

    EXPECT_THROW(
        bookService_->addBook(lms::models::Book("   \t  \n  ", "Valid Author")),
        lms::utils::ValidationException);
}

TEST_F(BookServiceTest, RejectEmptyOrWhitespaceAuthor) {
    EXPECT_THROW(
        bookService_->addBook(lms::models::Book("Valid Title", "")),
        lms::utils::ValidationException);

    EXPECT_THROW(
        bookService_->addBook(lms::models::Book("Valid Title", "   \t  ")),
        lms::utils::ValidationException);
}

TEST_F(BookServiceTest, RejectEmptyOrWhitespaceIsbnWhenProvided) {
    EXPECT_THROW(
        bookService_->addBook(lms::models::Book("Valid Title", "Valid Author", "")),
        lms::utils::ValidationException);

    EXPECT_THROW(
        bookService_->addBook(lms::models::Book("Valid Title", "Valid Author", "    ")),
        lms::utils::ValidationException);
}

TEST_F(BookServiceTest, RejectDuplicateIsbn) {
    bookService_->addBook(lms::models::Book("Book 1", "Author 1", "978-0000000001"));

    EXPECT_THROW(
        bookService_->addBook(lms::models::Book("Book 2", "Author 2", "978-0000000001")),
        lms::utils::ValidationException);
}

TEST_F(BookServiceTest, RejectInvalidPublicationYear) {
    EXPECT_THROW(
        bookService_->addBook(lms::models::Book("Title", "Author", std::nullopt, std::nullopt, 0)),
        lms::utils::ValidationException);

    EXPECT_THROW(
        bookService_->addBook(lms::models::Book("Title", "Author", std::nullopt, std::nullopt, -100)),
        lms::utils::ValidationException);

    EXPECT_THROW(
        bookService_->addBook(lms::models::Book("Title", "Author", std::nullopt, std::nullopt, 2500)),
        lms::utils::ValidationException);

    EXPECT_NO_THROW(
        bookService_->addBook(lms::models::Book("Title", "Author", std::nullopt, std::nullopt, 2024)));
}

TEST_F(BookServiceTest, NormalizeOptionalFields) {
    int id = bookService_->addBook(
        lms::models::Book("Title", "Author", std::nullopt, "   ", 2020, "\t\t", ""));

    auto retrieved = bookService_->getBook(id);
    EXPECT_FALSE(retrieved.getPublisher().has_value());
    EXPECT_FALSE(retrieved.getCategory().has_value());
    EXPECT_FALSE(retrieved.getDescription().has_value());
}

TEST_F(BookServiceTest, UpdateBookDetails) {
    int id = bookService_->addBook(lms::models::Book("Old Title", "Old Author", "978-1000000001"));

    lms::models::Book updated(id, "Updated Title", "Updated Author", "978-1000000002", "New Pub", 2021);
    EXPECT_NO_THROW(bookService_->updateBook(updated));

    auto retrieved = bookService_->getBook(id);
    EXPECT_EQ(retrieved.getTitle(), "Updated Title");
    EXPECT_EQ(*retrieved.getIsbn(), "978-1000000002");
}

TEST_F(BookServiceTest, UpdateNonexistentBookThrowsNotFound) {
    lms::models::Book nonexistent(9999, "Title", "Author");
    EXPECT_THROW(bookService_->updateBook(nonexistent), lms::utils::NotFoundException);
}

TEST_F(BookServiceTest, UpdateWithDuplicateIsbnThrowsValidation) {
    int id1 = bookService_->addBook(lms::models::Book("Book 1", "Author 1", "978-1111111111"));
    int id2 = bookService_->addBook(lms::models::Book("Book 2", "Author 2", "978-2222222222"));

    // Attempting to change Book 2's ISBN to Book 1's ISBN should throw
    lms::models::Book conflictUpdate(id2, "Book 2 Updated", "Author 2", "978-1111111111");
    EXPECT_THROW(bookService_->updateBook(conflictUpdate), lms::utils::ValidationException);

    // Updating Book 1 with its own ISBN should succeed
    lms::models::Book sameIsbnUpdate(id1, "Book 1 Renamed", "Author 1", "978-1111111111");
    EXPECT_NO_THROW(bookService_->updateBook(sameIsbnUpdate));
}

TEST_F(BookServiceTest, GetMissingBookThrowsNotFound) {
    EXPECT_THROW((void)bookService_->getBook(9999), lms::utils::NotFoundException);
}

TEST_F(BookServiceTest, GetByEmptyIsbnThrowsValidation) {
    EXPECT_THROW((void)bookService_->getBookByIsbn("   "), lms::utils::ValidationException);
}

TEST_F(BookServiceTest, SearchMethodsDelegateProperly) {
    bookService_->addBook(lms::models::Book("The Hobbit", "J.R.R. Tolkien", std::nullopt, std::nullopt, 1937, "Fantasy"));
    bookService_->addBook(lms::models::Book("The Fellowship of the Ring", "J.R.R. Tolkien", std::nullopt, std::nullopt, 1954, "Fantasy"));

    EXPECT_EQ(bookService_->searchBooksByTitle("Hobbit").size(), 1);
    EXPECT_EQ(bookService_->searchBooksByAuthor("Tolkien").size(), 2);
    EXPECT_EQ(bookService_->searchBooksByCategory("Fantasy").size(), 2);
}

TEST_F(BookServiceTest, DeleteBookSafety) {
    int bookId = bookService_->addBook(lms::models::Book("Book to Delete", "Author"));

    // Add physical copy
    copyRepo_->addCopy(lms::models::BookCopy(bookId, "ACC-DEL-01"));

    // Deleting book with copies must throw ValidationException
    EXPECT_THROW(bookService_->deleteBook(bookId), lms::utils::ValidationException);

    // Remove the copy
    copyRepo_->deleteCopy(1);

    // Now delete book succeeds
    EXPECT_NO_THROW(bookService_->deleteBook(bookId));
    EXPECT_THROW((void)bookService_->getBook(bookId), lms::utils::NotFoundException);

    // Deleting nonexistent book throws NotFoundException
    EXPECT_THROW(bookService_->deleteBook(9999), lms::utils::NotFoundException);
}

} // namespace
