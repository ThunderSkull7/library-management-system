# Library Management System

A modern, high-performance Library Management System built with **C++20**, **SQLite3**, **CMake**, and **GoogleTest**.

> **Note**: This repository represents **Phase 4: Loan Checkout, Returns & Fine Calculation** (built upon the Phase 1 Foundation, Phase 2 Book Catalog, and Phase 3 Member Management). It provides domain models, SQLite repositories, atomic RAII transactions, and business services for circulation checkouts, returns, fine calculations in integer paise, payment tracking, and policy enforcement. Search, reporting, and complete interactive CLI menus are planned for subsequent phases.

---

## 1. Project Overview

The Library Management System is architected as a modular, extensible C++ application designed for physical library operations:
- **Phase 1: Foundation & Database Architecture**: RAII SQLite3 connection management, foreign-key enforcement, busy timeouts, transactional migrations via `user_version`, normalized tables (`books`, `book_copies`, `members`, `loans`), and partial unique indexes.
- **Phase 2: Book Catalog & Physical Copy Management**:
  - Independent domain models (`Book`, `BookCopy`, `BookCopyStatus`).
  - Parameterized SQLite repositories (`BookRepository`, `BookCopyRepository`) with RAII prepared statement handling.
  - Business services (`BookService`, `BookCopyService`) validating non-empty fields, publication year, ISO dates, duplicate checks, and status safety (preventing manual `OnLoan` transitions and preserving copies with loan history).
- **Phase 3: Member Registration & Management**:
  - Independent domain model (`Member`) and status enum (`MemberStatus`: `Active`, `Suspended`, `Closed`).
  - Parameterized SQLite repository (`MemberRepository`) handling nullable emails and phones, unique constraint violations, and status updates.
  - Business service (`MemberService`) validating full names, basic email syntax (`user@domain.tld`), ISO calendar join dates, status lifecycles, and loan history preservation.
- **Phase 4: Loan Checkout, Returns & Fine Calculation**:
  - Independent domain models: `Loan`, `LoanPayment`, and `LibraryPolicy`.
  - Date calculations using C++20 `<chrono>` calendar (`DateUtils`).
  - RAII Transaction manager (`Transaction`) guaranteeing atomic operations (`BEGIN IMMEDIATE`, `COMMIT`, `ROLLBACK`).
  - Schema migration to Version 2 introducing the `loan_payments` table and foreign key index.
  - Repository (`LoanRepository`) for loans, active loan queries, overdue queries, outstanding fines, and payment records.
  - Business service (`LoanService`) enforcing borrowing limits, copy availability, member status verification, overdue fines in integer minor units (paise: ₹2 = 200 paise/day), grace periods, maximum fine caps, and partial/full fine payments.
  - 181 automated GoogleTests covering all models, policies, repositories, services, and transactions.

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

Execute the compiled executable from the build directory:

### Debug Mode
```powershell
.\build\debug\Debug\library_management_system.exe
```

### Release Mode
```powershell
.\build\release\Release\library_management_system.exe
```

Upon successful startup, the application verifies the connection, ensures the `data/` directory and `data/library.db` database exist, initializes the schema to version 2, and outputs:
```text
========================================
       LIBRARY MANAGEMENT SYSTEM        
========================================

Database connection: Successful
Database schema: Initialized
Schema version: 2

Library Management System foundation
is ready.

========================================
```

---

## 8. How to Run Tests

Run the full GoogleTest suite using CTest:

### Run Debug Tests
```powershell
ctest --preset windows-msvc-debug
```
or directly run the test binary:
```powershell
.\build\debug\tests\Debug\library_management_tests.exe
```

### Run Release Tests
```powershell
ctest --preset windows-msvc-release
```

---

## 9. Database Schema Overview (Schema Version 2)

The database schema manages five core tables:

1. **`books`**: Stores catalog metadata (id, title, author, publisher, publication_year, category, description, and unique nullable ISBN).
2. **`book_copies`**: Tracks individual physical copies (id, book_id FK, accession_number UNIQUE, status CHECK, acquired_on).
   - Valid statuses: `Available`, `On Loan`, `Lost`, `Damaged`, `Withdrawn`.
3. **`members`**: Tracks registered patrons (id, full_name, email UNIQUE, phone, joined_on, status CHECK).
   - Valid statuses: `Active`, `Suspended`, `Closed`.
4. **`loans`**: Tracks checkouts and returns (id, copy_id FK, member_id FK, issued_on, due_on, returned_on, fine_minor_units CHECK, fine_paid CHECK).
   - Partial unique index: `one_active_loan_per_copy` (`loans(copy_id) WHERE returned_on IS NULL`) guarantees that a physical copy cannot be checked out multiple times simultaneously.
5. **`loan_payments`** (introduced in Schema V2): Tracks partial and full fine payments made against returned loans (id, loan_id FK, amount_minor_units CHECK, paid_on).
   - Index: `idx_loan_payments_loan_id` for efficient payment aggregation.

Detailed documentation is available in [docs/database_architecture.md](docs/database_architecture.md).

---

## 10. Project Directory Structure

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
├── include/
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
│   │   └── MemberStatus.hpp
│   ├── repositories/
│   │   ├── BookCopyRepository.hpp
│   │   ├── BookRepository.hpp
│   │   ├── LoanRepository.hpp
│   │   └── MemberRepository.hpp
│   ├── services/
│   │   ├── BookCopyService.hpp
│   │   ├── BookService.hpp
│   │   ├── LoanService.hpp
│   │   └── MemberService.hpp
│   └── utils/
│       ├── DateUtils.hpp
│       ├── ValidationException.hpp
│       └── ValidationUtils.hpp
│
├── src/
│   ├── main.cpp                # Application entry point
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
│   └── test_loan_service.cpp
│
├── docs/
│   └── database_architecture.md # Detailed database design specification
│
└── data/
    └── .gitkeep                # Application runtime database directory
```
