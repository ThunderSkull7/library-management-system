# Library Management System

A modern, high-performance Library Management System built with **C++20**, **SQLite3**, **CMake**, and **GoogleTest**.

> **Production Release**: Complete, audited, and hardened implementation encompassing the database architecture, physical copy management, patron account lifecycles, atomic circulation workflows, concurrency protections, catalog discovery, integer paise financial accounting, comprehensive administrative reporting, and an interactive command-line interface.

---

## 1. Project Overview

The Library Management System is architected as a modular, extensible C++ application designed for physical library operations:

- **Phase 1: Foundation & Database Architecture**: RAII SQLite3 connection management, foreign-key enforcement, busy timeouts, transactional migrations via `user_version`, normalized tables (`books`, `book_copies`, `members`, `loans`), and partial unique indexes.
- **Phase 2: Book Catalog & Physical Copy Management**:
  - Independent domain models (`Book`, `BookCopy`, `BookCopyStatus`).
  - Parameterized SQLite repositories (`BookRepository`, `BookCopyRepository`) with RAII prepared statement handling.
  - Business services (`BookService`, `BookCopyService`) validating non-empty fields, publication year, ISO dates, duplicate checks, and status safety.
- **Phase 3: Member Registration & Management**:
  - Independent domain model (`Member`) and status enum (`MemberStatus`: `Active`, `Suspended`, `Closed`).
  - Parameterized SQLite repository (`MemberRepository`) handling nullable emails and phones, unique constraint violations, and status updates.
  - Business service (`MemberService`) validating full names, basic email syntax (`user@domain.tld`), ISO calendar join dates, status lifecycles, and loan history preservation.
- **Phase 4 & 4.1: Circulation, Fines & Concurrency Safety**:
  - Independent domain models: `Loan`, `LoanPayment`, and `LibraryPolicy`.
  - Date calculations using C++20 `<chrono>` calendar (`DateUtils`).
  - Schema migration to Version 2 introducing the `loan_payments` table and foreign key indexes.
  - RAII Transaction manager (`Transaction`) guaranteeing atomic operations (`BEGIN IMMEDIATE`, `COMMIT`, `ROLLBACK`) with database reads encapsulated strictly inside transaction boundaries to eliminate race conditions.
  - Business service (`LoanService`) enforcing borrowing limits, copy availability, member status verification, overdue fines in integer minor units (paise: ₹2 = 200 paise/day), grace periods, maximum fine caps, and partial/full fine payments.
- **Phase 5: Search, Reporting & Complete CLI Integration**:
  - Interactive CLI application (`ConsoleUI`) with formatted menus, breadcrumbs, resilient input validation (`InputHandler`), and aligned ASCII tabular displays (`TableFormatter`).
  - Flexible catalog search: book title, author, ISBN, category, copy accession number, member name/email/ID, and active loans.
  - Five comprehensive administrative, circulation, and financial reports (`LibraryReportService`):
    1. **Inventory Report**: Catalog titles, copy availability, on-loan, lost, damaged, and withdrawn counts.
    2. **Circulation Report**: Total, active, returned, and overdue loans with optional date-range filtering.
    3. **Member Activity Report**: Member status breakdown, active borrowing statistics, and configurable top borrowers ranking.
    4. **Fine Report**: Total fines assessed, payments received, and net outstanding balance in integer paise (formatted as `₹X.XX`).
    5. **Overdue Report**: Detailed line items of active overdue loans as of any arbitrary date with days overdue and accrued fines capped at policy limits.
- **Phase 6: Final Audit, Hardening & Verification**:
  - Comprehensive static analysis and code audit ensuring zero resource leaks and strict exception safety.
  - Verified full backwards compatibility of Schema V1 to V2 migration under foreign key constraints.
  - Real end-to-end operational execution of Workflows A through F through the CLI executable.
  - 214 automated GoogleTests passing with 100% success rate across Debug and Release configurations.

---

## 2. Technology Stack

- **Language Standard**: C++20 (`/std:c++20`)
- **Target OS**: Windows 11 (MSVC 2022 x64 toolchain)
- **Build System**: CMake 3.20+ with CMake Presets
- **Dependency Manager**: vcpkg in manifest mode (`vcpkg.json`)
- **Database Engine**: SQLite 3
- **Unit Testing**: GoogleTest (gtest) via CTest

---

## 3. Prerequisites

Before building the project, ensure the following are installed:

- **Visual Studio 2022** (or Visual Studio Build Tools 2022) with the "Desktop development with C++" workload (MSVC v143 toolchain).
- **CMake** (v3.20 or newer).
- **vcpkg**: Microsoft C++ Library Manager. Ensure the environment variable `VCPKG_ROOT` points to your vcpkg installation (e.g., `C:\vcpkg`).
- **Git** for version control.

---

## 4. vcpkg Setup

The project uses **vcpkg manifest mode** via `vcpkg.json`. When CMake configures the project, vcpkg automatically resolves, downloads, and compiles all declared dependencies (`sqlite3` and `gtest` for the test feature).

Ensure `VCPKG_ROOT` is configured in your environment:
```powershell
$env:VCPKG_ROOT = "C:\vcpkg"
```

---

## 5. CMake Configuration Instructions

Configure using the provided CMake presets:

### Debug Configuration
```powershell
cmake --preset windows-msvc-debug
```

### Release Configuration
```powershell
cmake --preset windows-msvc-release
```

---

## 6. Build Instructions

### Debug Build
```powershell
cmake --build --preset windows-msvc-debug
```

### Release Build
```powershell
cmake --build --preset windows-msvc-release
```

---

## 7. How to Run the Application

Execute the compiled executable from the build directory. By default, it manages `data/library.db`. You may also provide an optional custom database path:

### Default Database (`data/library.db`)
```powershell
.\build\release\Release\library_management_system.exe
```

### Custom / Test Database
```powershell
.\build\release\Release\library_management_system.exe path\to\custom_library.db
```

---

## 8. Default Circulation Policy

The library operates under configurable business rules managed by `LibraryPolicy`:

| Policy Parameter | Default Value | Description |
|---|---|---|
| **Loan Duration** | 14 days | Standard borrowing period for checked-out physical copies |
| **Overdue Fine** | 200 paise / day (`₹2.00`) | Assessed per overdue calendar day past the due date |
| **Max Active Loans** | 5 loans | Maximum number of concurrent active loans per member |
| **Grace Period** | 0 days | Days past due date before fines begin accruing |
| **Max Fine Cap** | Uncapped (`std::nullopt`) | Optional ceiling on accumulated overdue fines per loan |

---

## 9. CLI Menu Hierarchy & Features

```text
========================================
  LIBRARY MANAGEMENT SYSTEM
========================================
1. Book Management
2. Physical Copy Management
3. Member Management
4. Issue / Return Books
5. Search
6. Reports
7. Fine Payments
0. Exit
```

### Submenu Capabilities:
- **1. Book Management**: Add book title, update book details, delete book title, view book details by ID, list all books.
- **2. Physical Copy Management**: Add physical copy to book, update copy status (`Available`, `Lost`, `Damaged`, `Withdrawn`), list all copies for a book, view copy details by accession number.
- **3. Member Management**: Register new member, update profile, change status (`Active`, `Suspended`, `Closed`), delete member, view member details, list all members.
- **4. Issue / Return Books**: Checkout book copy to member with loan limit enforcement, return book copy with automatic fine assessment.
- **5. Search**: Search books (by title, author, ISBN, category, ID, or list all), search members (by name, email, ID, or list all), search loans (active loans, loans by member, loans by copy).
- **6. Reports**:
  - Inventory Summary Report
  - Circulation Summary Report (with optional ISO start/end date range filter)
  - Member Activity Report (with configurable top borrower limit)
  - Fine & Revenue Report (assessed, collected, outstanding balance)
  - Overdue Loans Report (with custom as-of date and fine calculation)
- **7. Fine Payments**: View unpaid fines, view member fine history, record full or partial fine payment.

---

## 10. Core Operational Workflows

### Workflow A: Book Creation, Copies, Member Registration & Checkout
1. Select `1. Book Management` -> `1. Add Book Title`. Enter title, author, ISBN, publisher, publication year, category.
2. Select `2. Physical Copy Management` -> `1. Add Physical Copy`. Enter the Book ID and a unique accession number (e.g., `ACC-001`).
3. Select `3. Member Management` -> `1. Register New Member`. Enter full name, join date (`YYYY-MM-DD`), email, and phone number.
4. Select `4. Issue / Return Books` -> `1. Issue (Checkout) Book`. Enter the Member ID, Copy ID, and Issue Date.
5. Verify the copy transitions to `OnLoan` and the active loan appears in `5. Search` -> `3. Search Loans`.

### Workflow B: Rejection of Disallowed Checkouts
1. Attempt to check out an already-loaned copy: Select `4. Issue / Return Books` -> `1. Issue Book` with the loaned Copy ID. The system safely rolls back and reports: `Copy is not available for checkout (current status: On Loan)`.
2. Suspend a member: Select `3. Member Management` -> `3. Change Member Status` and set the status to `Suspended`.
3. Attempt checkout for the suspended member: The system rejects the transaction with: `Cannot checkout book to inactive member (current status: Suspended)`.
4. Verify physical copy and database state remain completely uncorrupted.

### Workflow C: Overdue Return, Fine Assessment & Payment
1. Select `4. Issue / Return Books` -> `2. Return Book`. Enter the Copy ID and a return date after the due date.
2. The system calculates the overdue duration and assesses fines in integer paise (₹2.00 / 200 paise per day):
   - Example: 10 days late = 2000 paise = `₹20.00`.
   - The copy is safely returned to `Available` status.
3. Select `7. Fine Payments` -> `3. Record Fine Payment`. Enter the Loan ID and payment date.
4. Make a partial payment (e.g., `₹8.00` = 800 paise). The system records the payment in `loan_payments`, leaves `fine_paid = false`, and displays the remaining balance (`₹12.00`).
5. Make the remaining payment (`₹12.00` = 1200 paise). The system marks `fine_paid = true`.

### Workflow D: Reporting & Analytics
1. Select `6. Reports` -> `1. Inventory Summary`: Displays total titles, total copies, available copies, on-loan copies, lost, damaged, and withdrawn copies.
2. Select `6. Reports` -> `2. Circulation Summary`: Enter an as-of date (or accept today's date) and optional start/end dates. Displays total loans, active loans, returned loans, and overdue counts.
3. Select `6. Reports` -> `3. Member Activity Report`: Enter top borrower limit. Displays member count by status, patrons with active loans, and ranked top borrowers.
4. Select `6. Reports` -> `4. Overdue Loans Report`: Enter an evaluation date. Displays all active loans that have passed their due date with days overdue and accrued fine (capped at policy max).
5. Select `6. Reports` -> `5. Fine & Revenue Report`: Displays total assessed fines, collected revenue, and outstanding receivables in `₹X.XX`.

---

## 11. How to Run Tests

Run the full GoogleTest suite using CTest:

### Run Debug Tests
```powershell
ctest --preset windows-msvc-debug
```

### Run Release Tests
```powershell
ctest --preset windows-msvc-release
```

Or execute the test binary directly:
```powershell
.\build\release\tests\Release\library_management_tests.exe
```

---

## 12. Known Limitations

- **Single-Machine Embedded Database**: Built on SQLite3 with file locking. It is optimized for single-machine library branch deployments or desktop use, not high-concurrency client-server web architectures.
- **Calendar Boundaries**: All dates must be formatted strictly in ISO-8601 (`YYYY-MM-DD`).
- **Terminal Display**: Tabular reports are optimized for terminals of at least 80–120 characters in width.

---

## 13. Documentation Sitemap

- [System Architecture](docs/architecture.md): Layered design, transaction isolation, RAII, and exception safety.
- [Database Architecture](docs/database_architecture.md): Full schema (V2), Mermaid ER diagram, constraint definitions, and indexes.
- [Testing & Verification Evidence](docs/testing_and_verification.md): Automated test matrix and manual CLI workflow execution logs.

---

## 14. Project Directory Structure

```text
LibraryManagementSystem/
│
├── CMakeLists.txt              # Root build configuration
├── CMakePresets.json           # Presets for Windows MSVC Debug & Release
├── vcpkg.json                  # Dependency manifest (sqlite3, gtest)
├── .gitignore                  # Git ignore rules for build and runtime data
├── .clang-format               # Code formatting configuration
├── README.md                   # Project documentation
│
├── docs/
│   ├── architecture.md         # Detailed architectural documentation
│   ├── database_architecture.md # Database schema, ER diagram & index design
│   └── testing_and_verification.md # Test evidence and CLI workflow execution logs
│
├── include/
│   ├── cli/
│   │   ├── ConsoleUI.hpp       # Interactive CLI menu orchestrator
│   │   ├── CurrencyFormatter.hpp # Integer paise currency formatter (₹X.XX)
│   │   ├── InputHandler.hpp    # Resilient user input reader and validator
│   │   └── TableFormatter.hpp  # Aligned ASCII tabular output renderer
│   ├── database/
│   │   ├── DatabaseConnection.hpp
│   │   ├── DatabaseException.hpp
│   │   ├── PreparedStatement.hpp
│   │   ├── SchemaManager.hpp
│   │   └── Transaction.hpp
│   ├── models/
│   │   ├── Book.hpp
│   │   ├── BookCopy.hpp
│   │   ├── BookCopyStatus.hpp
│   │   ├── LibraryPolicy.hpp
│   │   ├── Loan.hpp
│   │   ├── LoanPayment.hpp
│   │   ├── Member.hpp
│   │   ├── MemberStatus.hpp
│   │   └── ReportModels.hpp    # Domain DTOs for administrative reports
│   ├── repositories/
│   │   ├── BookCopyRepository.hpp
│   │   ├── BookRepository.hpp
│   │   ├── LoanRepository.hpp
│   │   └── MemberRepository.hpp
│   ├── services/
│   │   ├── BookCopyService.hpp
│   │   ├── BookService.hpp
│   │   ├── LibraryReportService.hpp # Reporting service for analytics
│   │   ├── LoanService.hpp
│   │   └── MemberService.hpp
│   └── utils/
│       ├── DateUtils.hpp
│       ├── ValidationException.hpp
│       └── ValidationUtils.hpp
│
├── src/
│   ├── main.cpp                # Application entry point with ConsoleUI launch
│   ├── cli/
│   │   ├── ConsoleUI.cpp
│   │   ├── InputHandler.cpp
│   │   └── TableFormatter.cpp
│   ├── database/
│   │   ├── DatabaseConnection.cpp
│   │   ├── SchemaManager.cpp
│   │   └── Transaction.cpp
│   ├── models/
│   │   ├── Book.cpp
│   │   ├── BookCopy.cpp
│   │   ├── BookCopyStatus.cpp
│   │   ├── LibraryPolicy.cpp
│   │   ├── Loan.cpp
│   │   ├── LoanPayment.cpp
│   │   ├── Member.cpp
│   │   └── MemberStatus.cpp
│   ├── repositories/
│   │   ├── BookCopyRepository.cpp
│   │   ├── BookRepository.cpp
│   │   ├── LoanRepository.cpp
│   │   └── MemberRepository.cpp
│   ├── services/
│   │   ├── BookCopyService.cpp
│   │   ├── BookService.cpp
│   │   ├── LibraryReportService.cpp
│   │   ├── LoanService.cpp
│   │   └── MemberService.cpp
│   └── utils/
│       ├── DateUtils.cpp
│       └── ValidationUtils.cpp
│
├── tests/
│   ├── CMakeLists.txt          # Test build configuration
│   ├── test_database_connection.cpp
│   ├── test_schema_manager.cpp
│   ├── test_book_model.cpp
│   ├── test_book_repository.cpp
│   ├── test_book_service.cpp
│   ├── test_book_copy_repository.cpp
│   ├── test_book_copy_service.cpp
│   ├── test_member_model.cpp
│   ├── test_member_repository.cpp
│   ├── test_member_service.cpp
│   ├── test_loan_model.cpp
│   ├── test_library_policy.cpp
│   ├── test_date_utils.cpp
│   ├── test_transaction.cpp
│   ├── test_loan_repository.cpp
│   ├── test_loan_service.cpp
│   ├── test_report_service.cpp # Reports & end-to-end workflow verification
│   └── test_cli_helpers.cpp    # CLI InputHandler, TableFormatter, CurrencyFormatter
│
└── data/
    └── .gitkeep                # Application runtime database directory
```
