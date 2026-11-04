#include "cli/ConsoleUI.hpp"
#include "database/DatabaseConnection.hpp"
#include "database/DatabaseException.hpp"
#include "database/SchemaManager.hpp"
#include "repositories/BookCopyRepository.hpp"
#include "repositories/BookRepository.hpp"
#include "repositories/LoanRepository.hpp"
#include "repositories/MemberRepository.hpp"
#include "services/BookCopyService.hpp"
#include "services/BookService.hpp"
#include "services/LibraryReportService.hpp"
#include "services/LoanService.hpp"
#include "services/MemberService.hpp"

#include <exception>
#include <filesystem>
#include <iostream>

int main(int argc, char* argv[]) {
    try {
        std::filesystem::path dbPath;
        if (argc > 1 && argv[1] != nullptr && argv[1][0] != '\0') {
            dbPath = argv[1];
        } else {
            const std::filesystem::path dataDir = "data";
            if (!std::filesystem::exists(dataDir)) {
                std::filesystem::create_directories(dataDir);
            }
            dbPath = dataDir / "library.db";
        }

        if (dbPath.has_parent_path() && !std::filesystem::exists(dbPath.parent_path())) {
            std::filesystem::create_directories(dbPath.parent_path());
        }

        lms::database::DatabaseConnection connection(dbPath);
        lms::database::SchemaManager schemaManager(connection);
        schemaManager.initializeSchema();

        lms::repositories::BookRepository bookRepo(connection);
        lms::repositories::BookCopyRepository copyRepo(connection);
        lms::repositories::MemberRepository memberRepo(connection);
        lms::repositories::LoanRepository loanRepo(connection);

        lms::services::BookService bookService(bookRepo);
        lms::services::BookCopyService copyService(copyRepo, bookRepo);
        lms::services::MemberService memberService(memberRepo);
        lms::services::LoanService loanService(loanRepo, copyRepo, memberRepo, connection);
        lms::services::LibraryReportService reportService(bookRepo, copyRepo, memberRepo, loanRepo);

        lms::cli::ConsoleUI ui(bookService, copyService, memberService, loanService, reportService);
        ui.run();

        return 0;
    } catch (const lms::database::DatabaseException& ex) {
        std::cerr << "Fatal Database Error (code " << ex.getErrorCode() << "): " << ex.what() << '\n';
        return 1;
    } catch (const std::exception& ex) {
        std::cerr << "Fatal Application Error: " << ex.what() << '\n';
        return 1;
    }
}
