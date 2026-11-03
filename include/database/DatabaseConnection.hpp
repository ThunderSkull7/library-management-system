#pragma once

#include <filesystem>
#include <string>

struct sqlite3;

namespace lms::database {

/**
 * @brief Manages the lifecycle of an SQLite3 database connection using RAII.
 *
 * Enforces PRAGMA foreign_keys = ON, configures busy timeouts, and provides safe access
 * to the underlying SQLite handle. Non-copyable, movable.
 */
class DatabaseConnection {
public:
    DatabaseConnection() = default;
    explicit DatabaseConnection(const std::filesystem::path& dbPath);
    ~DatabaseConnection();

    // Prevent copy semantics
    DatabaseConnection(const DatabaseConnection&) = delete;
    DatabaseConnection& operator=(const DatabaseConnection&) = delete;

    // Support safe move semantics
    DatabaseConnection(DatabaseConnection&& other) noexcept;
    DatabaseConnection& operator=(DatabaseConnection&& other) noexcept;

    /**
     * @brief Opens a database at the specified path (or ":memory:" for in-memory).
     * @throws DatabaseException if opening, pragma configuration, or foreign key verification fails.
     */
    void open(const std::filesystem::path& dbPath);

    /**
     * @brief Safely closes the database connection.
     */
    void close() noexcept;

    /**
     * @brief Checks if the database connection is currently open.
     */
    [[nodiscard]] bool isOpen() const noexcept;

    /**
     * @brief Returns the raw SQLite3 database handle.
     */
    [[nodiscard]] sqlite3* getRawHandle() const noexcept;

    /**
     * @brief Returns the path of the opened database.
     */
    [[nodiscard]] const std::filesystem::path& getPath() const noexcept;

    /**
     * @brief Executes a non-parameterized SQL script or single statement.
     * @throws DatabaseException on SQL execution failure.
     */
    void execute(const std::string& sql);

private:
    sqlite3* db_{nullptr};
    std::filesystem::path path_{};

    void configurePragmas();
    void verifyForeignKeysEnabled();
};

} // namespace lms::database

