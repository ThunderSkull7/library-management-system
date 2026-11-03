#pragma once

#include "database/DatabaseConnection.hpp"

namespace lms::database {

/**
 * @brief Manages the creation, verification, and versioning of the database schema.
 */
class SchemaManager {
public:
    static constexpr int CURRENT_SCHEMA_VERSION = 1;

    explicit SchemaManager(DatabaseConnection& connection);

    /**
     * @brief Initializes the database schema idempotently inside a transaction.
     * @throws DatabaseException if future schema version is detected or on SQL error.
     */
    void initializeSchema();

    /**
     * @brief Retrieves the current schema version from SQLite (PRAGMA user_version).
     */
    [[nodiscard]] int getSchemaVersion() const;

private:
    DatabaseConnection& connection_;

    void setSchemaVersion(int version);
    void createTablesAndIndexes();
};

} // namespace lms::database

