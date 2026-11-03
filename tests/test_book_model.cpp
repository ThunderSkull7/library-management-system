#include "models/Book.hpp"
#include "models/BookCopy.hpp"
#include "models/BookCopyStatus.hpp"

#include <gtest/gtest.h>
#include <stdexcept>
#include <string>

namespace {

TEST(BookModelTest, DefaultConstructor) {
    lms::models::Book book;
    EXPECT_EQ(book.getId(), 0);
    EXPECT_TRUE(book.getTitle().empty());
    EXPECT_TRUE(book.getAuthor().empty());
    EXPECT_FALSE(book.getIsbn().has_value());
    EXPECT_FALSE(book.getPublisher().has_value());
    EXPECT_FALSE(book.getPublicationYear().has_value());
    EXPECT_FALSE(book.getCategory().has_value());
    EXPECT_FALSE(book.getDescription().has_value());
}

TEST(BookModelTest, ParameterizedConstructorWithoutId) {
    lms::models::Book book("Modern C++", "Bjarne Stroustrup", "978-0134997834",
                           "Addison-Wesley", 2020, "Programming", "Comprehensive guide");

    EXPECT_EQ(book.getId(), 0);
    EXPECT_EQ(book.getTitle(), "Modern C++");
    EXPECT_EQ(book.getAuthor(), "Bjarne Stroustrup");
    ASSERT_TRUE(book.getIsbn().has_value());
    EXPECT_EQ(*book.getIsbn(), "978-0134997834");
    ASSERT_TRUE(book.getPublisher().has_value());
    EXPECT_EQ(*book.getPublisher(), "Addison-Wesley");
    ASSERT_TRUE(book.getPublicationYear().has_value());
    EXPECT_EQ(*book.getPublicationYear(), 2020);
    ASSERT_TRUE(book.getCategory().has_value());
    EXPECT_EQ(*book.getCategory(), "Programming");
    ASSERT_TRUE(book.getDescription().has_value());
    EXPECT_EQ(*book.getDescription(), "Comprehensive guide");
}

TEST(BookModelTest, ParameterizedConstructorWithId) {
    lms::models::Book book(42, "Design Patterns", "Gang of Four", std::nullopt,
                           std::nullopt, 1994, std::nullopt, std::nullopt);

    EXPECT_EQ(book.getId(), 42);
    EXPECT_EQ(book.getTitle(), "Design Patterns");
    EXPECT_EQ(book.getAuthor(), "Gang of Four");
    EXPECT_FALSE(book.getIsbn().has_value());
    EXPECT_FALSE(book.getPublisher().has_value());
    ASSERT_TRUE(book.getPublicationYear().has_value());
    EXPECT_EQ(*book.getPublicationYear(), 1994);
}

TEST(BookModelTest, SettersAndEquality) {
    lms::models::Book book1(1, "Title A", "Author A");
    lms::models::Book book2(1, "Title A", "Author A");
    EXPECT_EQ(book1, book2);

    book1.setTitle("New Title");
    EXPECT_NE(book1, book2);

    book2.setTitle("New Title");
    EXPECT_EQ(book1, book2);

    book1.setIsbn("12345");
    book1.setPublisher("Pub");
    book1.setPublicationYear(2021);
    book1.setCategory("Tech");
    book1.setDescription("Desc");

    EXPECT_EQ(*book1.getIsbn(), "12345");
    EXPECT_EQ(*book1.getPublisher(), "Pub");
    EXPECT_EQ(*book1.getPublicationYear(), 2021);
    EXPECT_EQ(*book1.getCategory(), "Tech");
    EXPECT_EQ(*book1.getDescription(), "Desc");
}

TEST(BookCopyStatusTest, EnumToStringConversion) {
    EXPECT_EQ(lms::models::toString(lms::models::BookCopyStatus::Available), "Available");
    EXPECT_EQ(lms::models::toString(lms::models::BookCopyStatus::OnLoan), "On Loan");
    EXPECT_EQ(lms::models::toString(lms::models::BookCopyStatus::Lost), "Lost");
    EXPECT_EQ(lms::models::toString(lms::models::BookCopyStatus::Damaged), "Damaged");
    EXPECT_EQ(lms::models::toString(lms::models::BookCopyStatus::Withdrawn), "Withdrawn");
}

TEST(BookCopyStatusTest, StringToEnumConversion) {
    EXPECT_EQ(lms::models::bookCopyStatusFromString("Available"), lms::models::BookCopyStatus::Available);
    EXPECT_EQ(lms::models::bookCopyStatusFromString("On Loan"), lms::models::BookCopyStatus::OnLoan);
    EXPECT_EQ(lms::models::bookCopyStatusFromString("Lost"), lms::models::BookCopyStatus::Lost);
    EXPECT_EQ(lms::models::bookCopyStatusFromString("Damaged"), lms::models::BookCopyStatus::Damaged);
    EXPECT_EQ(lms::models::bookCopyStatusFromString("Withdrawn"), lms::models::BookCopyStatus::Withdrawn);
}

TEST(BookCopyStatusTest, InvalidStringToEnumThrows) {
    EXPECT_THROW(lms::models::bookCopyStatusFromString("OnLoan"), std::invalid_argument);
    EXPECT_THROW(lms::models::bookCopyStatusFromString("available"), std::invalid_argument);
    EXPECT_THROW(lms::models::bookCopyStatusFromString("UNKNOWN"), std::invalid_argument);
    EXPECT_THROW(lms::models::bookCopyStatusFromString(""), std::invalid_argument);
}

TEST(BookCopyModelTest, ConstructorsAndAccessors) {
    lms::models::BookCopy copy1(10, "ACC-001");
    EXPECT_EQ(copy1.getId(), 0);
    EXPECT_EQ(copy1.getBookId(), 10);
    EXPECT_EQ(copy1.getAccessionNumber(), "ACC-001");
    EXPECT_EQ(copy1.getStatus(), lms::models::BookCopyStatus::Available);
    EXPECT_FALSE(copy1.getAcquiredOn().has_value());

    lms::models::BookCopy copy2(5, 10, "ACC-002", lms::models::BookCopyStatus::Damaged, "2026-03-15");
    EXPECT_EQ(copy2.getId(), 5);
    EXPECT_EQ(copy2.getBookId(), 10);
    EXPECT_EQ(copy2.getAccessionNumber(), "ACC-002");
    EXPECT_EQ(copy2.getStatus(), lms::models::BookCopyStatus::Damaged);
    ASSERT_TRUE(copy2.getAcquiredOn().has_value());
    EXPECT_EQ(*copy2.getAcquiredOn(), "2026-03-15");

    copy2.setStatus(lms::models::BookCopyStatus::Withdrawn);
    EXPECT_EQ(copy2.getStatus(), lms::models::BookCopyStatus::Withdrawn);
}

} // namespace
