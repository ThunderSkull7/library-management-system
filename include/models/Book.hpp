#pragma once

#include <optional>
#include <string>

namespace lms::models {

/**
 * @brief Represents a book title record in the library catalog.
 */
class Book {
public:
    Book() = default;

    /**
     * @brief Constructs a new Book entity (typically before database insertion, id = 0).
     */
    Book(std::string title,
         std::string author,
         std::optional<std::string> isbn = std::nullopt,
         std::optional<std::string> publisher = std::nullopt,
         std::optional<int> publicationYear = std::nullopt,
         std::optional<std::string> category = std::nullopt,
         std::optional<std::string> description = std::nullopt);

    /**
     * @brief Constructs a Book entity with a persisted database identifier.
     */
    Book(int id,
         std::string title,
         std::string author,
         std::optional<std::string> isbn = std::nullopt,
         std::optional<std::string> publisher = std::nullopt,
         std::optional<int> publicationYear = std::nullopt,
         std::optional<std::string> category = std::nullopt,
         std::optional<std::string> description = std::nullopt);

    // Getters
    [[nodiscard]] int getId() const noexcept { return id_; }
    [[nodiscard]] const std::string& getTitle() const noexcept { return title_; }
    [[nodiscard]] const std::string& getAuthor() const noexcept { return author_; }
    [[nodiscard]] const std::optional<std::string>& getIsbn() const noexcept { return isbn_; }
    [[nodiscard]] const std::optional<std::string>& getPublisher() const noexcept { return publisher_; }
    [[nodiscard]] std::optional<int> getPublicationYear() const noexcept { return publicationYear_; }
    [[nodiscard]] const std::optional<std::string>& getCategory() const noexcept { return category_; }
    [[nodiscard]] const std::optional<std::string>& getDescription() const noexcept { return description_; }

    // Setters
    void setId(int id) noexcept { id_ = id; }
    void setTitle(std::string title) { title_ = std::move(title); }
    void setAuthor(std::string author) { author_ = std::move(author); }
    void setIsbn(std::optional<std::string> isbn) { isbn_ = std::move(isbn); }
    void setPublisher(std::optional<std::string> publisher) { publisher_ = std::move(publisher); }
    void setPublicationYear(std::optional<int> publicationYear) noexcept { publicationYear_ = publicationYear; }
    void setCategory(std::optional<std::string> category) { category_ = std::move(category); }
    void setDescription(std::optional<std::string> description) { description_ = std::move(description); }

    // Equality comparison
    bool operator==(const Book& other) const = default;

private:
    int id_{0};
    std::string title_{};
    std::string author_{};
    std::optional<std::string> isbn_{std::nullopt};
    std::optional<std::string> publisher_{std::nullopt};
    std::optional<int> publicationYear_{std::nullopt};
    std::optional<std::string> category_{std::nullopt};
    std::optional<std::string> description_{std::nullopt};
};

} // namespace lms::models
