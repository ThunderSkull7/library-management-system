#include "database/DatabaseConnection.hpp"
#include "database/DatabaseException.hpp"
#include "database/SchemaManager.hpp"

#include <exception>
#include <filesystem>
#include <iostream>

int main() {
    try {
        const std::filesystem::path dataDir = "data";
        if (!std::filesystem::exists(dataDir)) {
            std::filesystem::create_directories(dataDir);
        }

        const std::filesystem::path dbPath = dataDir / "library.db";

        lms::database::DatabaseConnection connection(dbPath);
        lms::database::SchemaManager schemaManager(connection);
        schemaManager.initializeSchema();

        const int version = schemaManager.getSchemaVersion();

        std::cout << "========================================\n"
                  << "       LIBRARY MANAGEMENT SYSTEM        \n"
                  << "========================================\n\n"
                  << "Database connection: Successful\n"
                  << "Database schema: Initialized\n"
                  << "Schema version: " << version << "\n\n"
                  << "Library Management System foundation\n"
                  << "is ready.\n\n"
                  << "========================================\n";

        return 0;
    } catch (const lms::database::DatabaseException& ex) {
        std::cerr << "Database Error (code " << ex.getErrorCode() << "): " << ex.what() << '\n';
        return 1;
    } catch (const std::exception& ex) {
        std::cerr << "Initialization Error: " << ex.what() << '\n';
        return 1;
    }
}

