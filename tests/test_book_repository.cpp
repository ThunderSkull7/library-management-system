#include "database/DatabaseConnection.hpp"
#include "database/DatabaseException.hpp"
#include "database/SchemaManager.hpp"
#include "models/Book.hpp"
#include "repositories/BookCopyRepository.hpp"
#include "repositories/BookRepository.hpp"

#include <gtest/gtest.h>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace {

class BookRepositoryTest : public ::testing::Test {
protected:
    void SetUp() override {
        connection_ = std::make_unique<lms::database::DatabaseConnection>(":memory:");
        schemaManager_ = std::make_unique<lms::database::SchemaManager>(*connection_);
        schemaManager_->initializeSchema();

        bookRepo_ = std::make_unique<lms::repositories::BookRepository>(*connection_);
        copyRepo_ = std::make_unique<lms::repositories::BookCopyRepository>(*connection_);
    }

    void TearDown() override {
        copyRepo_.reset();
        bookRepo_.reset();
        schemaManager_.reset();
        connection_.reset();
    }

    std::unique_ptr<lms::database::DatabaseConnection> connection_;
    std::unique_ptr<lms::database::SchemaManager> schemaManager_;
    std::unique_ptr<lms::repositories::BookRepository> bookRepo_;
    std::unique_ptr<lms::repositories::BookCopyRepository> copyRepo_;
};

TEST_F(BookRepositoryTest, AddAndRetrieveBookById) {
    lms::models::Book book("The Pragmatic Programmer", "Andy Hunt, Dave Thomas",
                           "978-0201616224", "Addison-Wesley", 1999,
                           "Software Engineering", "From journeyman to master");

    int id = bookRepo_->addBook(book);
    EXPECT_GT(id, 0);

    auto retrieved = bookRepo_->getBookById(id);
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->getId(), id);
    EXPECT_EQ(retrieved->getTitle(), "The Pragmatic Programmer");
    EXPECT_EQ(retrieved->getAuthor(), "Andy Hunt, Dave Thomas");
    ASSERT_TRUE(retrieved->getIsbn().has_value());
    EXPECT_EQ(*retrieved->getIsbn(), "978-0201616224");
    ASSERT_TRUE(retrieved->getPublisher().has_value());
    EXPECT_EQ(*retrieved->getPublisher(), "Addison-Wesley");
    ASSERT_TRUE(retrieved->getPublicationYear().has_value());
    EXPECT_EQ(*retrieved->getPublicationYear(), 1999);
    ASSERT_TRUE(retrieved->getCategory().has_value());
    EXPECT_EQ(*retrieved->getCategory(), "Software Engineering");
    ASSERT_TRUE(retrieved->getDescription().has_value());
    EXPECT_EQ(*retrieved->getDescription(), "From journeyman to master");
}

TEST_F(BookRepositoryTest, RetrieveBookByIsbn) {
    lms::models::Book book("Clean Code", "Robert C. Martin", "978-0132350884");
    int id = bookRepo_->addBook(book);

    auto retrieved = bookRepo_->getBookByIsbn("978-0132350884");
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->getId(), id);
    EXPECT_EQ(retrieved->getTitle(), "Clean Code");

    EXPECT_FALSE(bookRepo_->getBookByIsbn("000-0000000000").has_value());
}

TEST_F(BookRepositoryTest, GetAllBooks) {
    EXPECT_TRUE(bookRepo_->getAllBooks().empty());

    bookRepo_->addBook(lms::models::Book("Book 1", "Author 1"));
    bookRepo_->addBook(lms::models::Book("Book 2", "Author 2"));
    bookRepo_->addBook(lms::models::Book("Book 3", "Author 3"));

    auto all = bookRepo_->getAllBooks();
    EXPECT_EQ(all.size(), 3);
    EXPECT_EQ(all[0].getTitle(), "Book 1");
    EXPECT_EQ(all[1].getTitle(), "Book 2");
    EXPECT_EQ(all[2].getTitle(), "Book 3");
}

TEST_F(BookRepositoryTest, SearchByTitle) {
    bookRepo_->addBook(lms::models::Book("Effective Modern C++", "Scott Meyers"));
    bookRepo_->addBook(lms::models::Book("More Effective C++", "Scott Meyers"));
    bookRepo_->addBook(lms::models::Book("Design Patterns", "Gang of Four"));

    auto results = bookRepo_->searchByTitle("Effective");
    EXPECT_EQ(results.size(), 2);

    results = bookRepo_->searchByTitle("patterns");
    EXPECT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].getTitle(), "Design Patterns");

    results = bookRepo_->searchByTitle("Nonexistent");
    EXPECT_TRUE(results.empty());
}

TEST_F(BookRepositoryTest, SearchByAuthor) {
    bookRepo_->addBook(lms::models::Book("Book A", "Martin Fowler"));
    bookRepo_->addBook(lms::models::Book("Book B", "Robert Martin"));
    bookRepo_->addBook(lms::models::Book("Book C", "Bjarne Stroustrup"));

    auto results = bookRepo_->searchByAuthor("Martin");
    EXPECT_EQ(results.size(), 2);

    results = bookRepo_->searchByAuthor("stroustrup");
    EXPECT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].getTitle(), "Book C");
}

TEST_F(BookRepositoryTest, SearchByCategory) {
    bookRepo_->addBook(lms::models::Book("Intro to Algorithms", "CLRS", std::nullopt, std::nullopt, std::nullopt, "Computer Science"));
    bookRepo_->addBook(lms::models::Book("Database Systems", "Silberschatz", std::nullopt, std::nullopt, std::nullopt, "Computer Science"));
    bookRepo_->addBook(lms::models::Book("World History", "Author H", std::nullopt, std::nullopt, std::nullopt, "History"));

    auto results = bookRepo_->searchByCategory("Computer Science");
    EXPECT_EQ(results.size(), 2);

    results = bookRepo_->searchByCategory("History");
    EXPECT_EQ(results.size(), 1);
}

TEST_F(BookRepositoryTest, UpdateBook) {
    int id = bookRepo_->addBook(lms::models::Book("Old Title", "Old Author", "978-1111111111"));

    lms::models::Book updated(id, "New Title", "New Author", "978-2222222222", "New Pub", 2025, "New Cat", "New Desc");
    EXPECT_TRUE(bookRepo_->updateBook(updated));

    auto retrieved = bookRepo_->getBookById(id);
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->getTitle(), "New Title");
    EXPECT_EQ(retrieved->getAuthor(), "New Author");
    ASSERT_TRUE(retrieved->getIsbn().has_value());
    EXPECT_EQ(*retrieved->getIsbn(), "978-2222222222");
    EXPECT_EQ(retrieved->getPublisher().value_or(""), "New Pub");
    EXPECT_EQ(retrieved->getPublicationYear().value_or(0), 2025);

    // Update non-existent book
    lms::models::Book nonexistent(9999, "Title", "Author");
    EXPECT_FALSE(bookRepo_->updateBook(nonexistent));
}

TEST_F(BookRepositoryTest, MissingBookReturnsNullopt) {
    EXPECT_FALSE(bookRepo_->getBookById(9999).has_value());
    EXPECT_FALSE(bookRepo_->getBookByIsbn("999-9999999999").has_value());
}

TEST_F(BookRepositoryTest, RejectDuplicateIsbn) {
    bookRepo_->addBook(lms::models::Book("Book 1", "Author 1", "978-1234567890"));

    EXPECT_THROW(
        bookRepo_->addBook(lms::models::Book("Book 2", "Author 2", "978-1234567890")),
        lms::database::DatabaseException);
}

TEST_F(BookRepositoryTest, AllowMultipleBooksWithNullIsbn) {
    EXPECT_NO_THROW(bookRepo_->addBook(lms::models::Book("Book A", "Author A", std::nullopt)));
    EXPECT_NO_THROW(bookRepo_->addBook(lms::models::Book("Book B", "Author B", std::nullopt)));

    auto all = bookRepo_->getAllBooks();
    EXPECT_EQ(all.size(), 2);
}

TEST_F(BookRepositoryTest, NullableFieldsHandledCorrectly) {
    int id = bookRepo_->addBook(lms::models::Book("Minimal Book", "Author"));
    auto retrieved = bookRepo_->getBookById(id);
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_FALSE(retrieved->getIsbn().has_value());
    EXPECT_FALSE(retrieved->getPublisher().has_value());
    EXPECT_FALSE(retrieved->getPublicationYear().has_value());
    EXPECT_FALSE(retrieved->getCategory().has_value());
    EXPECT_FALSE(retrieved->getDescription().has_value());
}

TEST_F(BookRepositoryTest, DeleteBookWithoutCopiesSucceeds) {
    int id = bookRepo_->addBook(lms::models::Book("Delete Me", "Author"));
    EXPECT_TRUE(bookRepo_->deleteBook(id));
    EXPECT_FALSE(bookRepo_->getBookById(id).has_value());
    EXPECT_FALSE(bookRepo_->deleteBook(id)); // already deleted
}

TEST_F(BookRepositoryTest, PreventDeleteBookWithCopies) {
    int bookId = bookRepo_->addBook(lms::models::Book("Has Copies", "Author"));
    copyRepo_->addCopy(lms::models::BookCopy(bookId, "ACC-TEST-01"));

    EXPECT_THROW(bookRepo_->deleteBook(bookId), lms::database::DatabaseException);
    EXPECT_TRUE(bookRepo_->getBookById(bookId).has_value());
}

TEST_F(BookRepositoryTest, SpecialCharactersAndSqlLikeInputHandledSafely) {
    std::string sqlInjectionTitle = "Robert'); DROP TABLE books;--";
    std::string specialCharsAuthor = "O'Connor & Sons \"Test\" <tag>";

    int id = bookRepo_->addBook(lms::models::Book(sqlInjectionTitle, specialCharsAuthor));
    auto retrieved = bookRepo_->getBookById(id);
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->getTitle(), sqlInjectionTitle);
    EXPECT_EQ(retrieved->getAuthor(), specialCharsAuthor);

    // Verify database was not dropped
    auto results = bookRepo_->searchByTitle("DROP TABLE");
    EXPECT_EQ(results.size(), 1);
}

} // namespace
