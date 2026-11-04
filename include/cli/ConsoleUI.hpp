#pragma once

#include "services/BookCopyService.hpp"
#include "services/BookService.hpp"
#include "services/LibraryReportService.hpp"
#include "services/LoanService.hpp"
#include "services/MemberService.hpp"

#include <iostream>

namespace lms::cli {

/**
 * @brief Interactive command-line interface orchestrating menus, user interaction, and service workflows.
 */
class ConsoleUI {
public:
    ConsoleUI(services::BookService& bookService,
              services::BookCopyService& copyService,
              services::MemberService& memberService,
              services::LoanService& loanService,
              services::LibraryReportService& reportService,
              std::istream& in = std::cin,
              std::ostream& out = std::cout);

    /**
     * @brief Starts the main application menu loop. Returns when the user chooses Exit or EOF is reached.
     */
    void run();

private:
    services::BookService& bookService_;
    services::BookCopyService& copyService_;
    services::MemberService& memberService_;
    services::LoanService& loanService_;
    services::LibraryReportService& reportService_;
    std::istream& in_;
    std::ostream& out_;

    // Top-level menu routers
    void runBookManagement();
    void runCopyManagement();
    void runMemberManagement();
    void runCirculationManagement();
    void runSearchManagement();
    void runReportsManagement();
    void runFinePaymentsManagement();

    // Book actions
    void handleAddBook();
    void handleUpdateBook();
    void handleViewBook();
    void handleListBooks();
    void handleDeleteBook();

    // Copy actions
    void handleAddCopy();
    void handleViewCopy();
    void handleListCopiesForBook();
    void handleUpdateCopyStatus();

    // Member actions
    void handleRegisterMember();
    void handleUpdateMember();
    void handleViewMember();
    void handleListMembers();
    void handleUpdateMemberStatus();

    // Issue / Return actions
    void handleCheckoutBook();
    void handleReturnBook();

    // Search actions
    void handleSearchBookByTitle();
    void handleSearchBookByAuthor();
    void handleSearchBookByIsbn();
    void handleSearchBookByCategory();
    void handleSearchBookById();
    void handleSearchCopyByAccession();
    void handleFilterCopiesByStatus();
    void handleSearchMemberByName();
    void handleSearchMemberByEmail();
    void handleSearchMemberById();
    void handleFilterMembersByStatus();
    void handleSearchLoanById();
    void handleMemberLoanHistory();
    void handleCopyLoanHistory();
    void handleListActiveLoans();
    void handleListOverdueLoans();

    // Report actions
    void handleInventoryReport();
    void handleCirculationReport();
    void handleMemberActivityReport();
    void handleFineReport();
    void handleOverdueReport();

    // Fine payment actions
    void handleViewOutstandingFines();
    void handleViewLoanPaymentHistory();
    void handleRecordPayment();

    // Helpers
    void printHeader(const std::string& title);
    void printSuccess(const std::string& message);
    void printError(const std::string& message);
};

} // namespace lms::cli

