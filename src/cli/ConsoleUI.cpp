#include "cli/ConsoleUI.hpp"
#include "cli/CurrencyFormatter.hpp"
#include "cli/InputHandler.hpp"
#include "cli/TableFormatter.hpp"
#include "database/DatabaseException.hpp"
#include "models/Book.hpp"
#include "models/BookCopy.hpp"
#include "models/BookCopyStatus.hpp"
#include "models/Member.hpp"
#include "models/MemberStatus.hpp"
#include "utils/DateUtils.hpp"
#include "utils/ValidationException.hpp"

#include <iomanip>
#include <sstream>

namespace lms::cli {

ConsoleUI::ConsoleUI(services::BookService& bookService,
                     services::BookCopyService& copyService,
                     services::MemberService& memberService,
                     services::LoanService& loanService,
                     services::LibraryReportService& reportService,
                     std::istream& in,
                     std::ostream& out)
    : bookService_(bookService),
      copyService_(copyService),
      memberService_(memberService),
      loanService_(loanService),
      reportService_(reportService),
      in_(in),
      out_(out) {}

void ConsoleUI::printHeader(const std::string& title) {
    out_ << "\n========================================\n"
         << "  " << title << "\n"
         << "========================================\n";
}

void ConsoleUI::printSuccess(const std::string& message) {
    out_ << "\n[✓] " << message << "\n";
}

void ConsoleUI::printError(const std::string& message) {
    out_ << "\n[!] Error: " << message << "\n";
}

void ConsoleUI::run() {
    try {
        while (true) {
            printHeader("LIBRARY MANAGEMENT SYSTEM");
            out_ << "1. Book Management\n"
                 << "2. Physical Copy Management\n"
                 << "3. Member Management\n"
                 << "4. Issue / Return Books\n"
                 << "5. Search\n"
                 << "6. Reports\n"
                 << "7. Fine Payments\n"
                 << "0. Exit\n\n";

            auto choice = InputHandler::readInt(in_, out_, "Select an option: ", 0, 7);
            if (!choice.has_value()) {
                continue;
            }

            switch (*choice) {
                case 1: runBookManagement(); break;
                case 2: runCopyManagement(); break;
                case 3: runMemberManagement(); break;
                case 4: runCirculationManagement(); break;
                case 5: runSearchManagement(); break;
                case 6: runReportsManagement(); break;
                case 7: runFinePaymentsManagement(); break;
                case 0:
                    out_ << "\nThank you for using Library Management System. Goodbye!\n";
                    return;
            }
        }
    } catch (const EofException&) {
        out_ << "\nSession terminated (EOF reached). Goodbye!\n";
    }
}

// -----------------------------------------------------------------------------
// Book Management
// -----------------------------------------------------------------------------
void ConsoleUI::runBookManagement() {
    while (true) {
        printHeader("BOOK MANAGEMENT");
        out_ << "1. Add a Book\n"
             << "2. Update Book Details\n"
             << "3. View Book Details\n"
             << "4. List All Books\n"
             << "5. Delete a Book\n"
             << "0. Back to Main Menu\n\n";

        auto choice = InputHandler::readInt(in_, out_, "Select an option: ", 0, 5);
        if (!choice.has_value() || *choice == 0) {
            break;
        }

        switch (*choice) {
            case 1: handleAddBook(); break;
            case 2: handleUpdateBook(); break;
            case 3: handleViewBook(); break;
            case 4: handleListBooks(); break;
            case 5: handleDeleteBook(); break;
        }
    }
}

void ConsoleUI::handleAddBook() {
    out_ << "\n--- Add New Catalog Book ---\n";
    try {
        std::string title = InputHandler::readString(in_, out_, "Enter Title: ", false);
        std::string author = InputHandler::readString(in_, out_, "Enter Author: ", false);
        std::string isbn = InputHandler::readString(in_, out_, "Enter ISBN (optional, press Enter to skip): ", true);
        std::string publisher = InputHandler::readString(in_, out_, "Enter Publisher (optional, press Enter to skip): ", true);
        auto year = InputHandler::readInt(in_, out_, "Enter Publication Year (optional, press Enter to skip): ", 1000, 2100, true);
        std::string category = InputHandler::readString(in_, out_, "Enter Category (optional, press Enter to skip): ", true);
        std::string desc = InputHandler::readString(in_, out_, "Enter Description (optional, press Enter to skip): ", true);

        models::Book book(title, author,
                          isbn.empty() ? std::nullopt : std::make_optional(isbn),
                          publisher.empty() ? std::nullopt : std::make_optional(publisher),
                          year,
                          category.empty() ? std::nullopt : std::make_optional(category),
                          desc.empty() ? std::nullopt : std::make_optional(desc));

        int newId = bookService_.addBook(book);
        printSuccess("Book successfully added with ID: " + std::to_string(newId));
    } catch (const utils::ValidationException& ex) {
        printError(ex.what());
    } catch (const database::DatabaseException& ex) {
        printError("Database error adding book: " + std::string(ex.what()));
    }
    InputHandler::waitForEnter(in_, out_);
}

void ConsoleUI::handleUpdateBook() {
    out_ << "\n--- Update Book Details ---\n";
    try {
        auto bookId = InputHandler::readInt(in_, out_, "Enter Book ID to update: ", 1);
        if (!bookId) return;

        auto existing = bookService_.getBook(*bookId);
        out_ << "Current Title: " << existing.getTitle() << "\n";
        std::string title = InputHandler::readString(in_, out_, "New Title (press Enter to keep current): ", true);
        if (title.empty()) title = existing.getTitle();

        out_ << "Current Author: " << existing.getAuthor() << "\n";
        std::string author = InputHandler::readString(in_, out_, "New Author (press Enter to keep current): ", true);
        if (author.empty()) author = existing.getAuthor();

        out_ << "Current ISBN: " << existing.getIsbn().value_or("(none)") << "\n";
        std::string isbn = InputHandler::readString(in_, out_, "New ISBN (press Enter to keep current): ", true);
        std::optional<std::string> optIsbn = isbn.empty() ? existing.getIsbn() : std::make_optional(isbn);

        out_ << "Current Publisher: " << existing.getPublisher().value_or("(none)") << "\n";
        std::string pub = InputHandler::readString(in_, out_, "New Publisher (press Enter to keep current): ", true);
        std::optional<std::string> optPub = pub.empty() ? existing.getPublisher() : std::make_optional(pub);

        out_ << "Current Publication Year: " << (existing.getPublicationYear() ? std::to_string(*existing.getPublicationYear()) : "(none)") << "\n";
        auto year = InputHandler::readInt(in_, out_, "New Publication Year (press Enter to keep current): ", 1000, 2100, true);
        std::optional<int> optYear = year.has_value() ? year : existing.getPublicationYear();

        out_ << "Current Category: " << existing.getCategory().value_or("(none)") << "\n";
        std::string cat = InputHandler::readString(in_, out_, "New Category (press Enter to keep current): ", true);
        std::optional<std::string> optCat = cat.empty() ? existing.getCategory() : std::make_optional(cat);

        models::Book updated(*bookId, std::move(title), std::move(author),
                             std::move(optIsbn), std::move(optPub), optYear,
                             std::move(optCat), existing.getDescription());

        bookService_.updateBook(updated);
        printSuccess("Book ID " + std::to_string(*bookId) + " successfully updated.");
    } catch (const utils::NotFoundException& ex) {
        printError(ex.what());
    } catch (const utils::ValidationException& ex) {
        printError(ex.what());
    } catch (const database::DatabaseException& ex) {
        printError("Database update error: " + std::string(ex.what()));
    }
    InputHandler::waitForEnter(in_, out_);
}

void ConsoleUI::handleViewBook() {
    out_ << "\n--- View Book Details ---\n";
    try {
        auto bookId = InputHandler::readInt(in_, out_, "Enter Book ID: ", 1);
        if (!bookId) return;

        auto book = bookService_.getBook(*bookId);
        auto copies = copyService_.getCopiesByBook(*bookId);
        int availableCount = copyService_.getAvailableCopiesCountForBook(*bookId);

        out_ << "\n=======================================================\n"
             << " ID               : " << book.getId() << "\n"
             << " Title            : " << book.getTitle() << "\n"
             << " Author           : " << book.getAuthor() << "\n"
             << " ISBN             : " << book.getIsbn().value_or("(none)") << "\n"
             << " Publisher        : " << book.getPublisher().value_or("(none)") << "\n"
             << " Publication Year : " << (book.getPublicationYear() ? std::to_string(*book.getPublicationYear()) : "(none)") << "\n"
             << " Category         : " << book.getCategory().value_or("(none)") << "\n"
             << " Description      : " << book.getDescription().value_or("(none)") << "\n"
             << " Total Copies     : " << copies.size() << "\n"
             << " Available Copies : " << availableCount << "\n"
             << "=======================================================\n";

        if (!copies.empty()) {
            out_ << "\nPhysical Copies:\n";
            TableFormatter tf({
                {"Copy ID", Alignment::Right, 8},
                {"Accession Number", Alignment::Left, 18},
                {"Status", Alignment::Left, 12},
                {"Acquired Date", Alignment::Left, 14}
            });
            for (const auto& c : copies) {
                tf.addRow({
                    std::to_string(c.getId()),
                    c.getAccessionNumber(),
                    models::toString(c.getStatus()),
                    c.getAcquiredOn().value_or("-")
                });
            }
            tf.print(out_);
        }
    } catch (const utils::NotFoundException& ex) {
        printError(ex.what());
    }
    InputHandler::waitForEnter(in_, out_);
}

void ConsoleUI::handleListBooks() {
    auto books = bookService_.getAllBooks();
    out_ << "\nTotal Catalog Books: " << books.size() << "\n";

    TableFormatter tf({
        {"ID", Alignment::Right, 5},
        {"Title", Alignment::Left, 32},
        {"Author", Alignment::Left, 22},
        {"ISBN", Alignment::Left, 14},
        {"Category", Alignment::Left, 14},
        {"Available", Alignment::Right, 10}
    });

    for (const auto& b : books) {
        int avail = copyService_.getAvailableCopiesCountForBook(b.getId());
        tf.addRow({
            std::to_string(b.getId()),
            b.getTitle(),
            b.getAuthor(),
            b.getIsbn().value_or("-"),
            b.getCategory().value_or("-"),
            std::to_string(avail)
        });
    }

    tf.print(out_);
    InputHandler::waitForEnter(in_, out_);
}

void ConsoleUI::handleDeleteBook() {
    out_ << "\n--- Delete Book ---\n";
    try {
        auto bookId = InputHandler::readInt(in_, out_, "Enter Book ID to delete: ", 1);
        if (!bookId) return;

        auto book = bookService_.getBook(*bookId);
        out_ << "Selected: \"" << book.getTitle() << "\" by " << book.getAuthor() << "\n";

        if (!InputHandler::readConfirmation(in_, out_, "Are you sure you want to delete this book?", false)) {
            out_ << "Deletion canceled.\n";
            InputHandler::waitForEnter(in_, out_);
            return;
        }

        bookService_.deleteBook(*bookId);
        printSuccess("Book ID " + std::to_string(*bookId) + " successfully deleted.");
    } catch (const utils::NotFoundException& ex) {
        printError(ex.what());
    } catch (const utils::ValidationException& ex) {
        printError(ex.what());
    } catch (const database::DatabaseException& ex) {
        printError("Database error during deletion: " + std::string(ex.what()));
    }
    InputHandler::waitForEnter(in_, out_);
}

// -----------------------------------------------------------------------------
// Physical Copy Management
// -----------------------------------------------------------------------------
void ConsoleUI::runCopyManagement() {
    while (true) {
        printHeader("PHYSICAL COPY MANAGEMENT");
        out_ << "1. Add a Physical Copy\n"
             << "2. View Copy Details\n"
             << "3. List Copies for a Book\n"
             << "4. Change Copy Status\n"
             << "0. Back to Main Menu\n\n";

        auto choice = InputHandler::readInt(in_, out_, "Select an option: ", 0, 4);
        if (!choice.has_value() || *choice == 0) {
            break;
        }

        switch (*choice) {
            case 1: handleAddCopy(); break;
            case 2: handleViewCopy(); break;
            case 3: handleListCopiesForBook(); break;
            case 4: handleUpdateCopyStatus(); break;
        }
    }
}

void ConsoleUI::handleAddCopy() {
    out_ << "\n--- Add Physical Copy ---\n";
    try {
        auto bookId = InputHandler::readInt(in_, out_, "Enter Book ID: ", 1);
        if (!bookId) return;

        auto book = bookService_.getBook(*bookId);
        out_ << "Adding copy for: \"" << book.getTitle() << "\"\n";

        std::string acc = InputHandler::readString(in_, out_, "Enter Unique Accession Number / Barcode: ", false);
        std::string today = utils::DateUtils::getTodayIsoDate();
        std::string acqDate = InputHandler::readDate(in_, out_, "Acquired Date", today);

        out_ << "\nInitial Status:\n"
             << "1. Available\n"
             << "2. Lost\n"
             << "3. Damaged\n"
             << "4. Withdrawn\n";
        auto statusChoice = InputHandler::readInt(in_, out_, "Select Status (default 1): ", 1, 4, true);

        models::BookCopyStatus status = models::BookCopyStatus::Available;
        if (statusChoice.has_value()) {
            switch (*statusChoice) {
                case 1: status = models::BookCopyStatus::Available; break;
                case 2: status = models::BookCopyStatus::Lost; break;
                case 3: status = models::BookCopyStatus::Damaged; break;
                case 4: status = models::BookCopyStatus::Withdrawn; break;
            }
        }

        int copyId = copyService_.addCopy(*bookId, acc, acqDate.empty() ? std::nullopt : std::make_optional(acqDate), status);
        printSuccess("Physical copy added successfully with Copy ID: " + std::to_string(copyId));
    } catch (const utils::NotFoundException& ex) {
        printError(ex.what());
    } catch (const utils::ValidationException& ex) {
        printError(ex.what());
    } catch (const database::DatabaseException& ex) {
        printError("Database error adding copy: " + std::string(ex.what()));
    }
    InputHandler::waitForEnter(in_, out_);
}

void ConsoleUI::handleViewCopy() {
    out_ << "\n--- View Physical Copy ---\n";
    try {
        auto copyId = InputHandler::readInt(in_, out_, "Enter Copy ID: ", 1);
        if (!copyId) return;

        auto copy = copyService_.getCopy(*copyId);
        auto book = bookService_.getBook(copy.getBookId());
        auto history = loanService_.getLoanHistoryForCopy(*copyId);

        out_ << "\n=======================================================\n"
             << " Copy ID          : " << copy.getId() << "\n"
             << " Accession Number : " << copy.getAccessionNumber() << "\n"
             << " Status           : " << models::toString(copy.getStatus()) << "\n"
             << " Acquired Date    : " << copy.getAcquiredOn().value_or("(none)") << "\n"
             << " Book ID          : " << book.getId() << "\n"
             << " Title            : " << book.getTitle() << "\n"
             << " Author           : " << book.getAuthor() << "\n"
             << " Loan History     : " << history.size() << " records\n"
             << "=======================================================\n";
    } catch (const utils::NotFoundException& ex) {
        printError(ex.what());
    }
    InputHandler::waitForEnter(in_, out_);
}

void ConsoleUI::handleListCopiesForBook() {
    out_ << "\n--- List Copies for a Book ---\n";
    try {
        auto bookId = InputHandler::readInt(in_, out_, "Enter Book ID: ", 1);
        if (!bookId) return;

        auto book = bookService_.getBook(*bookId);
        auto copies = copyService_.getCopiesByBook(*bookId);

        out_ << "\nCopies for Book ID " << *bookId << " (\"" << book.getTitle() << "\"): " << copies.size() << " total\n";

        TableFormatter tf({
            {"Copy ID", Alignment::Right, 8},
            {"Accession Number", Alignment::Left, 18},
            {"Status", Alignment::Left, 12},
            {"Acquired Date", Alignment::Left, 14}
        });

        for (const auto& c : copies) {
            tf.addRow({
                std::to_string(c.getId()),
                c.getAccessionNumber(),
                models::toString(c.getStatus()),
                c.getAcquiredOn().value_or("-")
            });
        }
        tf.print(out_);
    } catch (const utils::NotFoundException& ex) {
        printError(ex.what());
    }
    InputHandler::waitForEnter(in_, out_);
}

void ConsoleUI::handleUpdateCopyStatus() {
    out_ << "\n--- Change Physical Copy Status ---\n";
    try {
        auto copyId = InputHandler::readInt(in_, out_, "Enter Copy ID: ", 1);
        if (!copyId) return;

        auto copy = copyService_.getCopy(*copyId);
        out_ << "Current Status: " << models::toString(copy.getStatus()) << "\n";

        out_ << "\nNew Status:\n"
             << "1. Available\n"
             << "2. Lost\n"
             << "3. Damaged\n"
             << "4. Withdrawn\n"
             << "(Note: Setting status to 'On Loan' manually is prohibited)\n";

        auto statusChoice = InputHandler::readInt(in_, out_, "Select new status: ", 1, 4);
        if (!statusChoice) return;

        models::BookCopyStatus newStatus = models::BookCopyStatus::Available;
        switch (*statusChoice) {
            case 1: newStatus = models::BookCopyStatus::Available; break;
            case 2: newStatus = models::BookCopyStatus::Lost; break;
            case 3: newStatus = models::BookCopyStatus::Damaged; break;
            case 4: newStatus = models::BookCopyStatus::Withdrawn; break;
        }

        copyService_.updateStatus(*copyId, newStatus);
        printSuccess("Copy ID " + std::to_string(*copyId) + " status updated to '" + models::toString(newStatus) + "'.");
    } catch (const utils::NotFoundException& ex) {
        printError(ex.what());
    } catch (const utils::ValidationException& ex) {
        printError(ex.what());
    }
    InputHandler::waitForEnter(in_, out_);
}

// -----------------------------------------------------------------------------
// Member Management
// -----------------------------------------------------------------------------
void ConsoleUI::runMemberManagement() {
    while (true) {
        printHeader("MEMBER MANAGEMENT");
        out_ << "1. Register a Member\n"
             << "2. Update Member Details\n"
             << "3. View Member Details\n"
             << "4. List All Members\n"
             << "5. Suspend / Reactivate / Close Member\n"
             << "0. Back to Main Menu\n\n";

        auto choice = InputHandler::readInt(in_, out_, "Select an option: ", 0, 5);
        if (!choice.has_value() || *choice == 0) {
            break;
        }

        switch (*choice) {
            case 1: handleRegisterMember(); break;
            case 2: handleUpdateMember(); break;
            case 3: handleViewMember(); break;
            case 4: handleListMembers(); break;
            case 5: handleUpdateMemberStatus(); break;
        }
    }
}

void ConsoleUI::handleRegisterMember() {
    out_ << "\n--- Register New Library Member ---\n";
    try {
        std::string name = InputHandler::readString(in_, out_, "Enter Full Name: ", false);
        std::string email = InputHandler::readString(in_, out_, "Enter Email Address (optional, press Enter to skip): ", true);
        std::string phone = InputHandler::readString(in_, out_, "Enter Phone Number (optional, press Enter to skip): ", true);
        std::string today = utils::DateUtils::getTodayIsoDate();
        std::string joined = InputHandler::readDate(in_, out_, "Joined Date", today);

        int newId = memberService_.registerMember(
            name, joined,
            email.empty() ? std::nullopt : std::make_optional(email),
            phone.empty() ? std::nullopt : std::make_optional(phone)
        );
        printSuccess("Member registered successfully with Member ID: " + std::to_string(newId));
    } catch (const utils::ValidationException& ex) {
        printError(ex.what());
    } catch (const database::DatabaseException& ex) {
        printError("Database registration error: " + std::string(ex.what()));
    }
    InputHandler::waitForEnter(in_, out_);
}

void ConsoleUI::handleUpdateMember() {
    out_ << "\n--- Update Member Details ---\n";
    try {
        auto memberId = InputHandler::readInt(in_, out_, "Enter Member ID to update: ", 1);
        if (!memberId) return;

        auto existing = memberService_.getMember(*memberId);
        out_ << "Current Full Name: " << existing.getFullName() << "\n";
        std::string name = InputHandler::readString(in_, out_, "New Full Name (press Enter to keep current): ", true);
        if (name.empty()) name = existing.getFullName();

        out_ << "Current Email: " << existing.getEmail().value_or("(none)") << "\n";
        std::string email = InputHandler::readString(in_, out_, "New Email (press Enter to keep current): ", true);
        std::optional<std::string> optEmail = email.empty() ? existing.getEmail() : std::make_optional(email);

        out_ << "Current Phone: " << existing.getPhone().value_or("(none)") << "\n";
        std::string phone = InputHandler::readString(in_, out_, "New Phone (press Enter to keep current): ", true);
        std::optional<std::string> optPhone = phone.empty() ? existing.getPhone() : std::make_optional(phone);

        models::Member updated(*memberId, std::move(name), existing.getJoinedOn(),
                               std::move(optEmail), std::move(optPhone), existing.getStatus());

        memberService_.updateMember(updated);
        printSuccess("Member ID " + std::to_string(*memberId) + " successfully updated.");
    } catch (const utils::NotFoundException& ex) {
        printError(ex.what());
    } catch (const utils::ValidationException& ex) {
        printError(ex.what());
    } catch (const database::DatabaseException& ex) {
        printError("Database update error: " + std::string(ex.what()));
    }
    InputHandler::waitForEnter(in_, out_);
}

void ConsoleUI::handleViewMember() {
    out_ << "\n--- View Member Details ---\n";
    try {
        auto memberId = InputHandler::readInt(in_, out_, "Enter Member ID: ", 1);
        if (!memberId) return;

        auto member = memberService_.getMember(*memberId);
        auto activeLoans = loanService_.getActiveLoansForMember(*memberId);
        auto allLoans = loanService_.getLoanHistoryForMember(*memberId);

        out_ << "\n=======================================================\n"
             << " Member ID      : " << member.getId() << "\n"
             << " Full Name      : " << member.getFullName() << "\n"
             << " Status         : " << models::toString(member.getStatus()) << "\n"
             << " Email          : " << member.getEmail().value_or("(none)") << "\n"
             << " Phone          : " << member.getPhone().value_or("(none)") << "\n"
             << " Joined On      : " << member.getJoinedOn() << "\n"
             << " Active Loans   : " << activeLoans.size() << " books\n"
             << " Total History  : " << allLoans.size() << " loans recorded\n"
             << "=======================================================\n";

        if (!activeLoans.empty()) {
            out_ << "\nCurrently Borrowed Books:\n";
            TableFormatter tf({
                {"Loan ID", Alignment::Right, 8},
                {"Copy ID", Alignment::Right, 8},
                {"Book Title", Alignment::Left, 26},
                {"Issued On", Alignment::Left, 12},
                {"Due On", Alignment::Left, 12}
            });
            for (const auto& l : activeLoans) {
                std::string title = "Copy #" + std::to_string(l.getCopyId());
                auto copy = copyService_.getCopy(l.getCopyId());
                auto book = bookService_.getBook(copy.getBookId());
                title = book.getTitle();

                tf.addRow({
                    std::to_string(l.getId()),
                    std::to_string(l.getCopyId()),
                    title,
                    l.getIssuedOn(),
                    l.getDueOn()
                });
            }
            tf.print(out_);
        }
    } catch (const utils::NotFoundException& ex) {
        printError(ex.what());
    }
    InputHandler::waitForEnter(in_, out_);
}

void ConsoleUI::handleListMembers() {
    auto members = memberService_.getAllMembers();
    out_ << "\nTotal Registered Members: " << members.size() << "\n";

    TableFormatter tf({
        {"ID", Alignment::Right, 6},
        {"Full Name", Alignment::Left, 24},
        {"Email", Alignment::Left, 24},
        {"Status", Alignment::Left, 10},
        {"Joined On", Alignment::Left, 12},
        {"Active Loans", Alignment::Right, 12}
    });

    for (const auto& m : members) {
        auto activeLoans = loanService_.getActiveLoansForMember(m.getId());
        tf.addRow({
            std::to_string(m.getId()),
            m.getFullName(),
            m.getEmail().value_or("-"),
            models::toString(m.getStatus()),
            m.getJoinedOn(),
            std::to_string(activeLoans.size())
        });
    }

    tf.print(out_);
    InputHandler::waitForEnter(in_, out_);
}

void ConsoleUI::handleUpdateMemberStatus() {
    out_ << "\n--- Change Member Account Status ---\n";
    try {
        auto memberId = InputHandler::readInt(in_, out_, "Enter Member ID: ", 1);
        if (!memberId) return;

        auto member = memberService_.getMember(*memberId);
        out_ << "Member: " << member.getFullName() << " (Current Status: " << models::toString(member.getStatus()) << ")\n";

        out_ << "\nSelect New Status:\n"
             << "1. Active\n"
             << "2. Suspended\n"
             << "3. Closed\n";

        auto statusChoice = InputHandler::readInt(in_, out_, "Select status option: ", 1, 3);
        if (!statusChoice) return;

        models::MemberStatus newStatus = models::MemberStatus::Active;
        switch (*statusChoice) {
            case 1: newStatus = models::MemberStatus::Active; break;
            case 2: newStatus = models::MemberStatus::Suspended; break;
            case 3: newStatus = models::MemberStatus::Closed; break;
        }

        memberService_.updateStatus(*memberId, newStatus);
        printSuccess("Member status successfully updated to '" + models::toString(newStatus) + "'.");
    } catch (const utils::NotFoundException& ex) {
        printError(ex.what());
    } catch (const utils::ValidationException& ex) {
        printError(ex.what());
    }
    InputHandler::waitForEnter(in_, out_);
}

// -----------------------------------------------------------------------------
// Issue / Return Books
// -----------------------------------------------------------------------------
void ConsoleUI::runCirculationManagement() {
    while (true) {
        printHeader("ISSUE / RETURN BOOKS");
        out_ << "1. Issue a Book Copy to Member\n"
             << "2. Return a Book Copy\n"
             << "0. Back to Main Menu\n\n";

        auto choice = InputHandler::readInt(in_, out_, "Select an option: ", 0, 2);
        if (!choice.has_value() || *choice == 0) {
            break;
        }

        switch (*choice) {
            case 1: handleCheckoutBook(); break;
            case 2: handleReturnBook(); break;
        }
    }
}

void ConsoleUI::handleCheckoutBook() {
    out_ << "\n--- Issue Book Copy to Member ---\n";
    try {
        auto memberId = InputHandler::readInt(in_, out_, "Enter Member ID: ", 1);
        if (!memberId) return;
        auto member = memberService_.getMember(*memberId);
        out_ << "Borrower: " << member.getFullName() << " (" << models::toString(member.getStatus()) << ")\n";

        auto copyId = InputHandler::readInt(in_, out_, "Enter Copy ID: ", 1);
        if (!copyId) return;
        auto copy = copyService_.getCopy(*copyId);
        auto book = bookService_.getBook(copy.getBookId());
        out_ << "Copy: #" << copy.getAccessionNumber() << " - \"" << book.getTitle() << "\"\n";

        std::string today = utils::DateUtils::getTodayIsoDate();
        std::string issueDate = InputHandler::readDate(in_, out_, "Issue Date", today);

        auto loan = loanService_.checkoutBook(*memberId, *copyId, issueDate);

        printSuccess("Book successfully checked out!");
        out_ << "=======================================================\n"
             << " Loan ID   : " << loan.getId() << "\n"
             << " Member    : " << member.getFullName() << " (ID #" << *memberId << ")\n"
             << " Book      : \"" << book.getTitle() << "\"\n"
             << " Copy Barcode: " << copy.getAccessionNumber() << " (Copy #" << *copyId << ")\n"
             << " Issued On : " << loan.getIssuedOn() << "\n"
             << " Due Date  : " << loan.getDueOn() << "\n"
             << "=======================================================\n";
    } catch (const utils::NotFoundException& ex) {
        printError(ex.what());
    } catch (const utils::ValidationException& ex) {
        printError(ex.what());
    } catch (const database::DatabaseException& ex) {
        printError("Circulation database error: " + std::string(ex.what()));
    }
    InputHandler::waitForEnter(in_, out_);
}

void ConsoleUI::handleReturnBook() {
    out_ << "\n--- Return Book Copy ---\n";
    try {
        auto copyId = InputHandler::readInt(in_, out_, "Enter Copy ID to return: ", 1);
        if (!copyId) return;

        auto copy = copyService_.getCopy(*copyId);
        auto book = bookService_.getBook(copy.getBookId());
        out_ << "Returning: Copy #" << copy.getAccessionNumber() << " - \"" << book.getTitle() << "\"\n";

        std::string today = utils::DateUtils::getTodayIsoDate();
        std::string returnDate = InputHandler::readDate(in_, out_, "Return Date", today);

        auto returnedLoan = loanService_.returnBook(*copyId, returnDate);

        printSuccess("Book successfully returned!");
        out_ << "=======================================================\n"
             << " Loan ID       : " << returnedLoan.getId() << "\n"
             << " Book Title    : \"" << book.getTitle() << "\"\n"
             << " Copy Barcode  : " << copy.getAccessionNumber() << "\n"
             << " Issued On     : " << returnedLoan.getIssuedOn() << "\n"
             << " Due Date      : " << returnedLoan.getDueOn() << "\n"
             << " Returned On   : " << returnedLoan.getReturnedOn().value_or("-") << "\n"
             << " Assessed Fine : " << CurrencyFormatter::formatPaiseToRupees(returnedLoan.getFineMinorUnits()) << "\n"
             << " Fine Status   : " << (returnedLoan.isFinePaid() ? "Paid / Settled" : "UNPAID") << "\n"
             << " Copy Status   : Available\n"
             << "=======================================================\n";
    } catch (const utils::NotFoundException& ex) {
        printError(ex.what());
    } catch (const utils::ValidationException& ex) {
        printError(ex.what());
    } catch (const database::DatabaseException& ex) {
        printError("Circulation database error: " + std::string(ex.what()));
    }
    InputHandler::waitForEnter(in_, out_);
}

// -----------------------------------------------------------------------------
// Search Management
// -----------------------------------------------------------------------------
void ConsoleUI::runSearchManagement() {
    while (true) {
        printHeader("SEARCH & DISCOVERY");
        out_ << " 1. Search Books by Title\n"
             << " 2. Search Books by Author\n"
             << " 3. Search Books by ISBN\n"
             << " 4. Search Books by Category\n"
             << " 5. Search Books by ID\n"
             << " 6. Search Physical Copy by Accession Number\n"
             << " 7. Filter Physical Copies by Status\n"
             << " 8. Search Members by Name\n"
             << " 9. Search Members by Email\n"
             << "10. Search Members by ID\n"
             << "11. Filter Members by Status\n"
             << "12. Search Loan by ID\n"
             << "13. View Member's Complete Loan History\n"
             << "14. View Copy's Complete Loan History\n"
             << "15. List All Active Loans\n"
             << "16. List All Overdue Loans\n"
             << " 0. Back to Main Menu\n\n";

        auto choice = InputHandler::readInt(in_, out_, "Select a search option: ", 0, 16);
        if (!choice.has_value() || *choice == 0) {
            break;
        }

        switch (*choice) {
            case 1: handleSearchBookByTitle(); break;
            case 2: handleSearchBookByAuthor(); break;
            case 3: handleSearchBookByIsbn(); break;
            case 4: handleSearchBookByCategory(); break;
            case 5: handleSearchBookById(); break;
            case 6: handleSearchCopyByAccession(); break;
            case 7: handleFilterCopiesByStatus(); break;
            case 8: handleSearchMemberByName(); break;
            case 9: handleSearchMemberByEmail(); break;
            case 10: handleSearchMemberById(); break;
            case 11: handleFilterMembersByStatus(); break;
            case 12: handleSearchLoanById(); break;
            case 13: handleMemberLoanHistory(); break;
            case 14: handleCopyLoanHistory(); break;
            case 15: handleListActiveLoans(); break;
            case 16: handleListOverdueLoans(); break;
        }
    }
}

void ConsoleUI::handleSearchBookByTitle() {
    std::string query = InputHandler::readString(in_, out_, "Enter Title substring: ", false);
    auto matches = bookService_.searchBooksByTitle(query);
    out_ << "\nMatching Books (" << matches.size() << " found):\n";

    TableFormatter tf({
        {"ID", Alignment::Right, 5},
        {"Title", Alignment::Left, 30},
        {"Author", Alignment::Left, 20},
        {"Category", Alignment::Left, 14},
        {"Available", Alignment::Right, 10}
    });

    for (const auto& b : matches) {
        int avail = copyService_.getAvailableCopiesCountForBook(b.getId());
        tf.addRow({std::to_string(b.getId()), b.getTitle(), b.getAuthor(), b.getCategory().value_or("-"), std::to_string(avail)});
    }
    tf.print(out_);
    InputHandler::waitForEnter(in_, out_);
}

void ConsoleUI::handleSearchBookByAuthor() {
    std::string query = InputHandler::readString(in_, out_, "Enter Author substring: ", false);
    auto matches = bookService_.searchBooksByAuthor(query);
    out_ << "\nMatching Books (" << matches.size() << " found):\n";

    TableFormatter tf({
        {"ID", Alignment::Right, 5},
        {"Title", Alignment::Left, 30},
        {"Author", Alignment::Left, 20},
        {"Category", Alignment::Left, 14},
        {"Available", Alignment::Right, 10}
    });

    for (const auto& b : matches) {
        int avail = copyService_.getAvailableCopiesCountForBook(b.getId());
        tf.addRow({std::to_string(b.getId()), b.getTitle(), b.getAuthor(), b.getCategory().value_or("-"), std::to_string(avail)});
    }
    tf.print(out_);
    InputHandler::waitForEnter(in_, out_);
}

void ConsoleUI::handleSearchBookByIsbn() {
    std::string isbn = InputHandler::readString(in_, out_, "Enter exact ISBN: ", false);
    auto book = bookService_.getBookByIsbn(isbn);
    if (!book.has_value()) {
        out_ << "\nNo book found with ISBN '" << isbn << "'.\n";
    } else {
        int avail = copyService_.getAvailableCopiesCountForBook(book->getId());
        out_ << "\nBook Found:\n"
             << " ID        : " << book->getId() << "\n"
             << " Title     : " << book->getTitle() << "\n"
             << " Author    : " << book->getAuthor() << "\n"
             << " ISBN      : " << book->getIsbn().value_or("-") << "\n"
             << " Available : " << avail << " copies\n";
    }
    InputHandler::waitForEnter(in_, out_);
}

void ConsoleUI::handleSearchBookByCategory() {
    std::string query = InputHandler::readString(in_, out_, "Enter Category substring: ", false);
    auto matches = bookService_.searchBooksByCategory(query);
    out_ << "\nMatching Books in Category '" << query << "' (" << matches.size() << " found):\n";

    TableFormatter tf({
        {"ID", Alignment::Right, 5},
        {"Title", Alignment::Left, 30},
        {"Author", Alignment::Left, 20},
        {"Category", Alignment::Left, 14},
        {"Available", Alignment::Right, 10}
    });

    for (const auto& b : matches) {
        int avail = copyService_.getAvailableCopiesCountForBook(b.getId());
        tf.addRow({std::to_string(b.getId()), b.getTitle(), b.getAuthor(), b.getCategory().value_or("-"), std::to_string(avail)});
    }
    tf.print(out_);
    InputHandler::waitForEnter(in_, out_);
}

void ConsoleUI::handleSearchBookById() {
    auto id = InputHandler::readInt(in_, out_, "Enter Book ID: ", 1);
    if (!id) return;

    try {
        auto book = bookService_.getBook(*id);
        int avail = copyService_.getAvailableCopiesCountForBook(book.getId());
        out_ << "\nBook Found:\n"
             << " ID        : " << book.getId() << "\n"
             << " Title     : " << book.getTitle() << "\n"
             << " Author    : " << book.getAuthor() << "\n"
             << " Available : " << avail << " copies\n";
    } catch (const utils::NotFoundException& ex) {
        printError(ex.what());
    }
    InputHandler::waitForEnter(in_, out_);
}

void ConsoleUI::handleSearchCopyByAccession() {
    std::string acc = InputHandler::readString(in_, out_, "Enter Accession Number / Barcode: ", false);
    auto copy = copyService_.searchByAccessionNumber(acc);
    if (!copy.has_value()) {
        out_ << "\nNo physical copy found with accession number '" << acc << "'.\n";
    } else {
        auto book = bookService_.getBook(copy->getBookId());
        out_ << "\nCopy Details:\n"
             << " Copy ID          : " << copy->getId() << "\n"
             << " Accession Number : " << copy->getAccessionNumber() << "\n"
             << " Status           : " << models::toString(copy->getStatus()) << "\n"
             << " Book Title       : " << book.getTitle() << " (Book ID #" << book.getId() << ")\n"
             << " Author           : " << book.getAuthor() << "\n";
    }
    InputHandler::waitForEnter(in_, out_);
}

void ConsoleUI::handleFilterCopiesByStatus() {
    out_ << "\nSelect Status to Filter By:\n"
         << "1. Available\n"
         << "2. OnLoan\n"
         << "3. Lost\n"
         << "4. Damaged\n"
         << "5. Withdrawn\n";
    auto opt = InputHandler::readInt(in_, out_, "Choose status: ", 1, 5);
    if (!opt) return;

    models::BookCopyStatus st = models::BookCopyStatus::Available;
    switch (*opt) {
        case 1: st = models::BookCopyStatus::Available; break;
        case 2: st = models::BookCopyStatus::OnLoan; break;
        case 3: st = models::BookCopyStatus::Lost; break;
        case 4: st = models::BookCopyStatus::Damaged; break;
        case 5: st = models::BookCopyStatus::Withdrawn; break;
    }

    auto matches = copyService_.filterCopiesByStatus(st);
    out_ << "\nCopies with status '" << models::toString(st) << "' (" << matches.size() << " found):\n";

    TableFormatter tf({
        {"Copy ID", Alignment::Right, 8},
        {"Accession", Alignment::Left, 16},
        {"Book ID", Alignment::Right, 8},
        {"Book Title", Alignment::Left, 30},
        {"Acquired Date", Alignment::Left, 14}
    });

    for (const auto& c : matches) {
        std::string title = "-";
        try {
            auto b = bookService_.getBook(c.getBookId());
            title = b.getTitle();
        } catch (...) {}

        tf.addRow({
            std::to_string(c.getId()),
            c.getAccessionNumber(),
            std::to_string(c.getBookId()),
            title,
            c.getAcquiredOn().value_or("-")
        });
    }
    tf.print(out_);
    InputHandler::waitForEnter(in_, out_);
}

void ConsoleUI::handleSearchMemberByName() {
    std::string query = InputHandler::readString(in_, out_, "Enter Member Name substring: ", false);
    auto matches = memberService_.searchMembersByName(query);
    out_ << "\nMatching Members (" << matches.size() << " found):\n";

    TableFormatter tf({
        {"ID", Alignment::Right, 6},
        {"Full Name", Alignment::Left, 24},
        {"Status", Alignment::Left, 10},
        {"Email", Alignment::Left, 24},
        {"Active Loans", Alignment::Right, 12}
    });

    for (const auto& m : matches) {
        auto activeLoans = loanService_.getActiveLoansForMember(m.getId());
        tf.addRow({
            std::to_string(m.getId()),
            m.getFullName(),
            models::toString(m.getStatus()),
            m.getEmail().value_or("-"),
            std::to_string(activeLoans.size())
        });
    }
    tf.print(out_);
    InputHandler::waitForEnter(in_, out_);
}

void ConsoleUI::handleSearchMemberByEmail() {
    std::string email = InputHandler::readString(in_, out_, "Enter exact Email Address: ", false);
    auto member = memberService_.getMemberByEmail(email);
    if (!member.has_value()) {
        out_ << "\nNo member found with email '" << email << "'.\n";
    } else {
        auto activeLoans = loanService_.getActiveLoansForMember(member->getId());
        out_ << "\nMember Details:\n"
             << " ID           : " << member->getId() << "\n"
             << " Full Name    : " << member->getFullName() << "\n"
             << " Status       : " << models::toString(member->getStatus()) << "\n"
             << " Email        : " << member->getEmail().value_or("(none)") << "\n"
             << " Phone        : " << member->getPhone().value_or("(none)") << "\n"
             << " Active Loans : " << activeLoans.size() << " books\n";
    }
    InputHandler::waitForEnter(in_, out_);
}

void ConsoleUI::handleSearchMemberById() {
    auto id = InputHandler::readInt(in_, out_, "Enter Member ID: ", 1);
    if (!id) return;

    try {
        auto member = memberService_.getMember(*id);
        auto activeLoans = loanService_.getActiveLoansForMember(member.getId());
        out_ << "\nMember Details:\n"
             << " ID           : " << member.getId() << "\n"
             << " Full Name    : " << member.getFullName() << "\n"
             << " Status       : " << models::toString(member.getStatus()) << "\n"
             << " Email        : " << member.getEmail().value_or("(none)") << "\n"
             << " Phone        : " << member.getPhone().value_or("(none)") << "\n"
             << " Active Loans : " << activeLoans.size() << " books\n";
    } catch (const utils::NotFoundException& ex) {
        printError(ex.what());
    }
    InputHandler::waitForEnter(in_, out_);
}

void ConsoleUI::handleFilterMembersByStatus() {
    out_ << "\nSelect Member Status:\n"
         << "1. Active\n"
         << "2. Suspended\n"
         << "3. Closed\n";
    auto opt = InputHandler::readInt(in_, out_, "Choose option: ", 1, 3);
    if (!opt) return;

    models::MemberStatus st = models::MemberStatus::Active;
    switch (*opt) {
        case 1: st = models::MemberStatus::Active; break;
        case 2: st = models::MemberStatus::Suspended; break;
        case 3: st = models::MemberStatus::Closed; break;
    }

    auto matches = memberService_.filterMembersByStatus(st);
    out_ << "\nMembers with status '" << models::toString(st) << "' (" << matches.size() << " found):\n";

    TableFormatter tf({
        {"ID", Alignment::Right, 6},
        {"Full Name", Alignment::Left, 24},
        {"Status", Alignment::Left, 10},
        {"Email", Alignment::Left, 24},
        {"Active Loans", Alignment::Right, 12}
    });

    for (const auto& m : matches) {
        auto activeLoans = loanService_.getActiveLoansForMember(m.getId());
        tf.addRow({
            std::to_string(m.getId()),
            m.getFullName(),
            models::toString(m.getStatus()),
            m.getEmail().value_or("-"),
            std::to_string(activeLoans.size())
        });
    }
    tf.print(out_);
    InputHandler::waitForEnter(in_, out_);
}

void ConsoleUI::handleSearchLoanById() {
    auto id = InputHandler::readInt(in_, out_, "Enter Loan ID: ", 1);
    if (!id) return;

    try {
        auto loan = loanService_.getLoan(*id);
        std::string borrowerName = "Member #" + std::to_string(loan.getMemberId());
        try {
            borrowerName = memberService_.getMember(loan.getMemberId()).getFullName();
        } catch (...) {}

        std::string title = "Copy #" + std::to_string(loan.getCopyId());
        std::string acc = "-";
        try {
            auto copy = copyService_.getCopy(loan.getCopyId());
            acc = copy.getAccessionNumber();
            auto book = bookService_.getBook(copy.getBookId());
            title = book.getTitle();
        } catch (...) {}

        out_ << "\n=======================================================\n"
             << " Loan ID       : " << loan.getId() << "\n"
             << " Borrower      : " << borrowerName << " (ID #" << loan.getMemberId() << ")\n"
             << " Book Title    : " << title << "\n"
             << " Copy Barcode  : " << acc << " (Copy #" << loan.getCopyId() << ")\n"
             << " Issued On     : " << loan.getIssuedOn() << "\n"
             << " Due Date      : " << loan.getDueOn() << "\n"
             << " Returned On   : " << loan.getReturnedOn().value_or("(Active / Not returned)") << "\n"
             << " Assessed Fine : " << CurrencyFormatter::formatPaiseToRupees(loan.getFineMinorUnits()) << "\n"
             << " Fine Status   : " << (loan.isFinePaid() ? "Paid / Settled" : "UNPAID") << "\n"
             << "=======================================================\n";
    } catch (const utils::NotFoundException& ex) {
        printError(ex.what());
    }
    InputHandler::waitForEnter(in_, out_);
}

void ConsoleUI::handleMemberLoanHistory() {
    auto memberId = InputHandler::readInt(in_, out_, "Enter Member ID: ", 1);
    if (!memberId) return;

    try {
        auto member = memberService_.getMember(*memberId);
        auto loans = loanService_.getLoanHistoryForMember(*memberId);

        out_ << "\nLoan History for " << member.getFullName() << " (" << loans.size() << " records):\n";

        TableFormatter tf({
            {"Loan ID", Alignment::Right, 8},
            {"Copy ID", Alignment::Right, 8},
            {"Issued On", Alignment::Left, 12},
            {"Due On", Alignment::Left, 12},
            {"Returned On", Alignment::Left, 14},
            {"Fine", Alignment::Right, 10},
            {"Status", Alignment::Left, 10}
        });

        for (const auto& l : loans) {
            tf.addRow({
                std::to_string(l.getId()),
                std::to_string(l.getCopyId()),
                l.getIssuedOn(),
                l.getDueOn(),
                l.getReturnedOn().value_or("Active"),
                CurrencyFormatter::formatPaiseToRupees(l.getFineMinorUnits()),
                l.isActive() ? "Active" : (l.isFinePaid() ? "Returned" : "Fine Due")
            });
        }
        tf.print(out_);
    } catch (const utils::NotFoundException& ex) {
        printError(ex.what());
    }
    InputHandler::waitForEnter(in_, out_);
}

void ConsoleUI::handleCopyLoanHistory() {
    auto copyId = InputHandler::readInt(in_, out_, "Enter Copy ID: ", 1);
    if (!copyId) return;

    try {
        auto copy = copyService_.getCopy(*copyId);
        auto book = bookService_.getBook(copy.getBookId());
        auto loans = loanService_.getLoanHistoryForCopy(*copyId);

        out_ << "\nCirculation History for Copy #" << copy.getAccessionNumber() << " (\"" << book.getTitle() << "\"): " << loans.size() << " records\n";

        TableFormatter tf({
            {"Loan ID", Alignment::Right, 8},
            {"Member ID", Alignment::Right, 10},
            {"Issued On", Alignment::Left, 12},
            {"Due On", Alignment::Left, 12},
            {"Returned On", Alignment::Left, 14},
            {"Fine", Alignment::Right, 10}
        });

        for (const auto& l : loans) {
            tf.addRow({
                std::to_string(l.getId()),
                std::to_string(l.getMemberId()),
                l.getIssuedOn(),
                l.getDueOn(),
                l.getReturnedOn().value_or("Active"),
                CurrencyFormatter::formatPaiseToRupees(l.getFineMinorUnits())
            });
        }
        tf.print(out_);
    } catch (const utils::NotFoundException& ex) {
        printError(ex.what());
    }
    InputHandler::waitForEnter(in_, out_);
}

void ConsoleUI::handleListActiveLoans() {
    auto activeLoans = loanService_.getActiveLoans();
    out_ << "\nActive Loans across Library: " << activeLoans.size() << " records\n";

    TableFormatter tf({
        {"Loan ID", Alignment::Right, 8},
        {"Member ID", Alignment::Right, 10},
        {"Copy ID", Alignment::Right, 8},
        {"Issued On", Alignment::Left, 12},
        {"Due On", Alignment::Left, 12}
    });

    for (const auto& l : activeLoans) {
        tf.addRow({
            std::to_string(l.getId()),
            std::to_string(l.getMemberId()),
            std::to_string(l.getCopyId()),
            l.getIssuedOn(),
            l.getDueOn()
        });
    }
    tf.print(out_);
    InputHandler::waitForEnter(in_, out_);
}

void ConsoleUI::handleListOverdueLoans() {
    std::string today = utils::DateUtils::getTodayIsoDate();
    std::string asOfDate = InputHandler::readDate(in_, out_, "Check overdue loans as of date", today);

    auto overdue = loanService_.getOverdueLoans(asOfDate);
    out_ << "\nOverdue Loans as of " << asOfDate << ": " << overdue.size() << " found\n";

    TableFormatter tf({
        {"Loan ID", Alignment::Right, 8},
        {"Member ID", Alignment::Right, 10},
        {"Copy ID", Alignment::Right, 8},
        {"Issued On", Alignment::Left, 12},
        {"Due On", Alignment::Left, 12},
        {"Days Overdue", Alignment::Right, 12}
    });

    for (const auto& l : overdue) {
        int days = std::max(0, utils::DateUtils::daysBetween(l.getDueOn(), asOfDate));
        tf.addRow({
            std::to_string(l.getId()),
            std::to_string(l.getMemberId()),
            std::to_string(l.getCopyId()),
            l.getIssuedOn(),
            l.getDueOn(),
            std::to_string(days)
        });
    }
    tf.print(out_);
    InputHandler::waitForEnter(in_, out_);
}

// -----------------------------------------------------------------------------
// Reports
// -----------------------------------------------------------------------------
void ConsoleUI::runReportsManagement() {
    while (true) {
        printHeader("LIBRARY REPORTS");
        out_ << "1. Inventory Report\n"
             << "2. Circulation Report\n"
             << "3. Member Activity Report\n"
             << "4. Financial & Fine Report\n"
             << "5. Overdue Loans Report\n"
             << "0. Back to Main Menu\n\n";

        auto choice = InputHandler::readInt(in_, out_, "Select a report: ", 0, 5);
        if (!choice.has_value() || *choice == 0) {
            break;
        }

        switch (*choice) {
            case 1: handleInventoryReport(); break;
            case 2: handleCirculationReport(); break;
            case 3: handleMemberActivityReport(); break;
            case 4: handleFineReport(); break;
            case 5: handleOverdueReport(); break;
        }
    }
}

void ConsoleUI::handleInventoryReport() {
    auto r = reportService_.generateInventoryReport();

    printHeader("INVENTORY REPORT");
    out_ << " Total Catalog Titles  : " << r.totalTitles << "\n"
         << " Total Physical Copies : " << r.totalCopies << "\n"
         << "----------------------------------------\n"
         << " Copies by Status:\n"
         << "   • Available         : " << r.availableCopies << "\n"
         << "   • On Loan           : " << r.onLoanCopies << "\n"
         << "   • Lost              : " << r.lostCopies << "\n"
         << "   • Damaged           : " << r.damagedCopies << "\n"
         << "   • Withdrawn         : " << r.withdrawnCopies << "\n"
         << "========================================\n";
    InputHandler::waitForEnter(in_, out_);
}

void ConsoleUI::handleCirculationReport() {
    std::string today = utils::DateUtils::getTodayIsoDate();
    std::string asOfDate = InputHandler::readDate(in_, out_, "Enter as-of date for overdue calculations", today);

    bool filterRange = InputHandler::readConfirmation(in_, out_, "Filter loans issued within a specific date range?", false);
    std::optional<std::string> start = std::nullopt;
    std::optional<std::string> end = std::nullopt;

    if (filterRange) {
        start = InputHandler::readDate(in_, out_, "Range Start Date");
        end = InputHandler::readDate(in_, out_, "Range End Date");
    }

    try {
        auto r = reportService_.generateCirculationReport(asOfDate, start, end);

        printHeader("CIRCULATION REPORT");
        out_ << " As-of Date             : " << asOfDate << "\n"
             << " Total Loans Recorded   : " << r.totalLoans << "\n"
             << " Currently Active Loans : " << r.activeLoans << "\n"
             << " Returned Loans         : " << r.returnedLoans << "\n"
             << " Currently Overdue Loans: " << r.overdueLoans << "\n"
             << "========================================\n";

        if (r.rangeStart.has_value() && r.rangeEnd.has_value()) {
            out_ << "\nLoans Issued Between " << *r.rangeStart << " and " << *r.rangeEnd << " (" << r.rangeLoans.size() << " records):\n";
            TableFormatter tf({
                {"Loan ID", Alignment::Right, 8},
                {"Member ID", Alignment::Right, 10},
                {"Copy ID", Alignment::Right, 8},
                {"Issued On", Alignment::Left, 12},
                {"Due On", Alignment::Left, 12},
                {"Returned On", Alignment::Left, 14}
            });
            for (const auto& l : r.rangeLoans) {
                tf.addRow({
                    std::to_string(l.getId()),
                    std::to_string(l.getMemberId()),
                    std::to_string(l.getCopyId()),
                    l.getIssuedOn(),
                    l.getDueOn(),
                    l.getReturnedOn().value_or("Active")
                });
            }
            tf.print(out_);
        }
    } catch (const utils::ValidationException& ex) {
        printError(ex.what());
    }
    InputHandler::waitForEnter(in_, out_);
}

void ConsoleUI::handleMemberActivityReport() {
    auto r = reportService_.generateMemberActivityReport(10);

    printHeader("MEMBER ACTIVITY REPORT");
    out_ << " Total Registered Members : " << r.totalMembers << "\n"
         << "   • Active Members       : " << r.activeMembers << "\n"
         << "   • Suspended Members    : " << r.suspendedMembers << "\n"
         << "   • Closed Members       : " << r.closedMembers << "\n"
         << " Members Borrowing Books  : " << r.membersWithActiveLoans << "\n"
         << "========================================\n";

    out_ << "\nTop Active Borrowers:\n";
    TableFormatter tf({
        {"Member ID", Alignment::Right, 10},
        {"Full Name", Alignment::Left, 26},
        {"Status", Alignment::Left, 12},
        {"Active Loans", Alignment::Right, 12}
    });

    for (const auto& tb : r.topBorrowers) {
        tf.addRow({
            std::to_string(tb.memberId),
            tb.memberName,
            tb.status,
            std::to_string(tb.activeLoanCount)
        });
    }
    tf.print(out_);
    InputHandler::waitForEnter(in_, out_);
}

void ConsoleUI::handleFineReport() {
    auto r = reportService_.generateFineReport();

    printHeader("FINANCIAL & FINE REPORT");
    out_ << " Total Fines Assessed     : " << CurrencyFormatter::formatPaiseToRupees(r.totalFinesAssessedMinorUnits) << "\n"
         << " Total Payments Received  : " << CurrencyFormatter::formatPaiseToRupees(r.totalPaymentsReceivedMinorUnits) << "\n"
         << " Outstanding Balance      : " << CurrencyFormatter::formatPaiseToRupees(r.outstandingBalanceMinorUnits) << "\n"
         << "----------------------------------------\n"
         << " Unpaid Fine Records      : " << r.unpaidFineCount << "\n"
         << " Fully Paid Fine Records  : " << r.fullyPaidFineCount << "\n"
         << "========================================\n";
    InputHandler::waitForEnter(in_, out_);
}

void ConsoleUI::handleOverdueReport() {
    std::string today = utils::DateUtils::getTodayIsoDate();
    std::string asOfDate = InputHandler::readDate(in_, out_, "Enter as-of date for overdue report", today);

    try {
        auto r = reportService_.generateOverdueReport(asOfDate);

        printHeader("OVERDUE LOANS REPORT (As of " + asOfDate + ")");
        out_ << "Total Overdue Loans: " << r.totalOverdueLoans << "\n\n";

        TableFormatter tf({
            {"Loan ID", Alignment::Right, 8},
            {"Borrower", Alignment::Left, 20},
            {"Book Title", Alignment::Left, 24},
            {"Accession", Alignment::Left, 12},
            {"Due Date", Alignment::Left, 12},
            {"Days Late", Alignment::Right, 10},
            {"Accrued Fine", Alignment::Right, 12},
            {"Status", Alignment::Left, 8}
        });

        for (const auto& item : r.items) {
            tf.addRow({
                std::to_string(item.loanId),
                item.memberName + " (#" + std::to_string(item.memberId) + ")",
                item.bookTitle,
                item.accessionNumber,
                item.dueOn,
                std::to_string(item.overdueDays),
                CurrencyFormatter::formatPaiseToRupees(item.assessedFineMinorUnits),
                item.isActive ? "Active" : "Returned"
            });
        }
        tf.print(out_);
    } catch (const utils::ValidationException& ex) {
        printError(ex.what());
    }
    InputHandler::waitForEnter(in_, out_);
}

// -----------------------------------------------------------------------------
// Fine Payments
// -----------------------------------------------------------------------------
void ConsoleUI::runFinePaymentsManagement() {
    while (true) {
        printHeader("FINE PAYMENTS & REVENUE");
        out_ << "1. View Outstanding Unpaid Fines\n"
             << "2. View Loan Payment History\n"
             << "3. Record a Fine Payment\n"
             << "0. Back to Main Menu\n\n";

        auto choice = InputHandler::readInt(in_, out_, "Select an option: ", 0, 3);
        if (!choice.has_value() || *choice == 0) {
            break;
        }

        switch (*choice) {
            case 1: handleViewOutstandingFines(); break;
            case 2: handleViewLoanPaymentHistory(); break;
            case 3: handleRecordPayment(); break;
        }
    }
}

void ConsoleUI::handleViewOutstandingFines() {
    auto outstanding = loanService_.getOutstandingFines();
    out_ << "\nOutstanding Unpaid Fines: " << outstanding.size() << " records\n";

    TableFormatter tf({
        {"Loan ID", Alignment::Right, 8},
        {"Member ID", Alignment::Right, 10},
        {"Copy ID", Alignment::Right, 8},
        {"Returned On", Alignment::Left, 12},
        {"Assessed Fine", Alignment::Right, 14},
        {"Paid Amount", Alignment::Right, 14},
        {"Balance Due", Alignment::Right, 14}
    });

    for (const auto& l : outstanding) {
        int rem = loanService_.getRemainingFine(l.getId());
        int paid = l.getFineMinorUnits() - rem;
        tf.addRow({
            std::to_string(l.getId()),
            std::to_string(l.getMemberId()),
            std::to_string(l.getCopyId()),
            l.getReturnedOn().value_or("Active"),
            CurrencyFormatter::formatPaiseToRupees(l.getFineMinorUnits()),
            CurrencyFormatter::formatPaiseToRupees(paid),
            CurrencyFormatter::formatPaiseToRupees(rem)
        });
    }
    tf.print(out_);
    InputHandler::waitForEnter(in_, out_);
}

void ConsoleUI::handleViewLoanPaymentHistory() {
    auto loanId = InputHandler::readInt(in_, out_, "Enter Loan ID: ", 1);
    if (!loanId) return;

    try {
        auto loan = loanService_.getLoan(*loanId);
        auto payments = loanService_.getPaymentsForLoan(*loanId);
        int rem = loanService_.getRemainingFine(*loanId);

        out_ << "\nPayment Summary for Loan ID " << *loanId << ":\n"
             << " Total Fine Assessed : " << CurrencyFormatter::formatPaiseToRupees(loan.getFineMinorUnits()) << "\n"
             << " Total Paid So Far   : " << CurrencyFormatter::formatPaiseToRupees(loan.getFineMinorUnits() - rem) << "\n"
             << " Outstanding Balance : " << CurrencyFormatter::formatPaiseToRupees(rem) << "\n"
             << " Settlement Status   : " << (loan.isFinePaid() ? "Fully Settled" : "UNPAID") << "\n\n";

        TableFormatter tf({
            {"Payment ID", Alignment::Right, 10},
            {"Amount Paid", Alignment::Right, 14},
            {"Payment Date", Alignment::Left, 14}
        });

        for (const auto& p : payments) {
            tf.addRow({
                std::to_string(p.getId()),
                CurrencyFormatter::formatPaiseToRupees(p.getAmountMinorUnits()),
                p.getPaidOn()
            });
        }
        tf.print(out_);
    } catch (const utils::NotFoundException& ex) {
        printError(ex.what());
    }
    InputHandler::waitForEnter(in_, out_);
}

void ConsoleUI::handleRecordPayment() {
    out_ << "\n--- Record Fine Payment ---\n";
    try {
        auto loanId = InputHandler::readInt(in_, out_, "Enter Loan ID: ", 1);
        if (!loanId) return;

        auto loan = loanService_.getLoan(*loanId);
        if (loan.isActive()) {
            printError("Cannot accept payment for active loan. Book must be returned first.");
            InputHandler::waitForEnter(in_, out_);
            return;
        }

        int remaining = loanService_.getRemainingFine(*loanId);
        if (remaining <= 0 || loan.isFinePaid()) {
            printSuccess("Fine for Loan ID " + std::to_string(*loanId) + " is already fully settled.");
            InputHandler::waitForEnter(in_, out_);
            return;
        }

        out_ << "Assessed Fine   : " << CurrencyFormatter::formatPaiseToRupees(loan.getFineMinorUnits()) << "\n"
             << "Outstanding Due : " << CurrencyFormatter::formatPaiseToRupees(remaining) << " (" << remaining << " paise)\n";

        auto amount = InputHandler::readInt(in_, out_, "Enter payment amount in paise: ", 1, remaining);
        if (!amount) return;

        std::string today = utils::DateUtils::getTodayIsoDate();
        std::string payDate = InputHandler::readDate(in_, out_, "Payment Date", today);

        loanService_.payFine(*loanId, *amount, payDate);

        int updatedRem = loanService_.getRemainingFine(*loanId);
        auto updatedLoan = loanService_.getLoan(*loanId);

        printSuccess("Payment of " + CurrencyFormatter::formatPaiseToRupees(*amount) + " recorded successfully!");
        out_ << "Remaining Balance : " << CurrencyFormatter::formatPaiseToRupees(updatedRem) << "\n"
             << "Settlement Status : " << (updatedLoan.isFinePaid() ? "FULLY SETTLED [✓]" : "PARTIALLY PAID") << "\n";
    } catch (const utils::NotFoundException& ex) {
        printError(ex.what());
    } catch (const utils::ValidationException& ex) {
        printError(ex.what());
    } catch (const database::DatabaseException& ex) {
        printError("Database error recording payment: " + std::string(ex.what()));
    }
    InputHandler::waitForEnter(in_, out_);
}

} // namespace lms::cli

