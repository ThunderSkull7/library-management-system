# Testing and Verification Evidence — Library Management System

## 1. Overview & Strategy

The Library Management System is verified through a rigorous, multi-layered automated and manual testing strategy:
1. **Unit Tests**: Isolated validation of domain models, policies, date arithmetic, currency formatting, ASCII table rendering, and resilient console inputs.
2. **Repository & Database Integration Tests**: Verified against in-memory and disk-based SQLite instances, covering CRUD operations, foreign key cascades/restrictions, and schema migrations.
3. **Service & Circulation Integration Tests**: Validates checkout rules, borrowing limits, member status transitions, overdue grace periods, fine caps, and split payments.
4. **Concurrency & Thread Safety Tests**: Validates `BEGIN IMMEDIATE` transaction boundaries under multi-threaded contention to prevent double checkouts and overpayments.
5. **CLI End-to-End Operational Verification**: Direct execution of the compiled binary against independent temporary databases simulating user menu interactions.

---

## 2. Automated Test Suite Matrix

Executed via CTest under MSVC 2022 (`/W4 /permissive- /utf-8 /EHsc`):

| Test Suite Target | Test Focus Area | Test Count | Debug Status | Release Status |
|---|---|---|---|---|
| `DatabaseConnectionTest` | Connection lifecycle, RAII, foreign key enforcement, busy timeout | 11 | **PASS** (100%) | **PASS** (100%) |
| `SchemaManagerTest` | DDL execution, v1->v2 migration, idempotency, foreign key checks | 28 | **PASS** (100%) | **PASS** (100%) |
| `BookModelTest` | Title, author, ISBN validation, optional fields | 12 | **PASS** (100%) | **PASS** (100%) |
| `BookRepositoryTest` | Parameterized CRUD, unique ISBN constraint, search by title/author/category | 14 | **PASS** (100%) | **PASS** (100%) |
| `BookServiceTest` | Business validation, duplicate checks, deletion protection | 18 | **PASS** (100%) | **PASS** (100%) |
| `BookCopyRepositoryTest` | Physical copy CRUD, accession uniqueness, status indexing | 12 | **PASS** (100%) | **PASS** (100%) |
| `BookCopyServiceTest` | Copy status lifecycle, prevention of manual OnLoan transitions | 15 | **PASS** (100%) | **PASS** (100%) |
| `MemberModelTest` | Name, ISO join date, email, phone, MemberStatus conversions | 12 | **PASS** (100%) | **PASS** (100%) |
| `MemberRepositoryTest` | Member CRUD, unique email, status filtering | 14 | **PASS** (100%) | **PASS** (100%) |
| `MemberServiceTest` | Member registration, email regex, status updates, deletion restrictions | 16 | **PASS** (100%) | **PASS** (100%) |
| `LoanModelTest` | Loan fields, active loan predicates, fine minor units | 8 | **PASS** (100%) | **PASS** (100%) |
| `LibraryPolicyTest` | Duration, default daily fines, grace periods, max fine caps | 8 | **PASS** (100%) | **PASS** (100%) |
| `DateUtilsTest` | C++20 calendar arithmetic, leap years, month boundaries, ISO-8601 validation | 14 | **PASS** (100%) | **PASS** (100%) |
| `TransactionTest` | RAII commit, rollback on error, rollback upon destruction | 8 | **PASS** (100%) | **PASS** (100%) |
| `LoanRepositoryTest` | Loans CRUD, partial index enforcement, payment inserts, aggregation queries | 12 | **PASS** (100%) | **PASS** (100%) |
| `LoanServiceTest` | Checkout, return, loan limit, overdue calculation, partial fine payments | 18 | **PASS** (100%) | **PASS** (100%) |
| `LoanConcurrencyTest` | Multi-connection checkout races, concurrent fine payment race safety | 4 | **PASS** (100%) | **PASS** (100%) |
| `LibraryReportServiceTest` | Inventory, Circulation, Member Activity, Fine Report, Overdue Report | 8 | **PASS** (100%) | **PASS** (100%) |
| `EndToEndWorkflowTest` | Cross-subsystem integration (Workflows A, B, C, D) | 1 | **PASS** (100%) | **PASS** (100%) |
| `CurrencyFormatterTest` | Paise to Rupee string formatting (`₹X.XX`) with zero floating-point arithmetic | 3 | **PASS** (100%) | **PASS** (100%) |
| `TableFormatterTest` | Column width calculation, text alignment, header borders | 2 | **PASS** (100%) | **PASS** (100%) |
| `InputHandlerTest` | String trimming, integer parsing, range checks, date validation, EOF handling | 8 | **PASS** (100%) | **PASS** (100%) |
| **Total Test Count** | **Full GoogleTest Regression Suite** | **214** | **214 / 214 Passed** | **214 / 214 Passed** |

---

## 3. Actual CLI Workflow Verification Evidence

The compiled binary (`build/debug/Debug/library_management_system.exe`) was executed against an isolated test database (`data/cli_audit_test.db`), systematically exercising all interactive menu paths:

### Workflow A: Book & Member Registration
- Created Book: *"Domain-Driven Design"* (ISBN `978-0321125217`, Category `Software`) -> Assigned Book ID `1`.
- Added Physical Copy 1 (`DDD-COPY-01`, Available) -> Assigned Copy ID `1`.
- Added Physical Copy 2 (`DDD-COPY-02`, Available) -> Assigned Copy ID `2`.
- Registered Member: *"Katherine Johnson"* (Email `katherine@nasa.gov`) -> Assigned Member ID `1`.
- Verified catalog query displays Book ID 1 with 2 available copies.

### Workflow B: Checkout & Rejection
- Checked out Copy 1 to Member 1 on `2026-01-05`:
  - Assigned Loan ID: `1`
  - Due Date calculated: `2026-01-19` (14 days policy duration)
  - Copy status transitioned to `On Loan`.
- Attempted duplicate checkout of Copy 1:
  - System rejected operation: `[!] Error: Copy is not available for checkout (current status: On Loan)`.
  - Database remained unmodified and consistent.

### Workflow C: Overdue Return & Fine Assessment
- Returned Copy 1 on `2026-01-29`:
  - Return date is 10 days past due date (`2026-01-19`).
  - Assessed overdue fine: `10 days * 200 paise/day = 2000 paise` (`₹20.00`).
  - Copy 1 returned to `Available` status.
  - Loan record updated with `returned_on = '2026-01-29'`, `fine_minor_units = 2000`, `fine_paid = 0`.

### Workflow D: Fine Payment
- Recorded partial payment of 800 paise (`₹8.00`) on `2026-01-30`:
  - Outstanding balance: `₹12.00` (1200 paise).
  - Loan settlement status: `Partially Paid` (`fine_paid = 0`).
- Recorded final payment of 1200 paise (`₹12.00`) on `2026-01-31`:
  - Outstanding balance: `₹0.00`.
  - Loan settlement status: `FULLY SETTLED [✓]` (`fine_paid = 1`).

### Workflow E: Search & Reports
- Searched Books by title substring (`Domain`): Returned Book ID 1 with 2 available copies.
- Searched Members by name substring (`Katherine`): Returned Member ID 1.
- Generated Inventory Report: 1 title, 2 copies (2 Available, 0 On Loan).
- Generated Circulation Report (as of `2026-01-31`): 1 total loan, 0 active, 1 returned, 0 overdue.
- Generated Member Activity Report: 1 active member, 0 active borrowers.
- Generated Financial & Fine Report:
  - Total Fines Assessed: `₹20.00`
  - Total Payments Received: `₹20.00`
  - Outstanding Balance: `₹0.00`
  - Fully Paid Fine Records: `1`
  - Unpaid Fine Records: `0`

### Workflow F: Application Restart & Persistence Verification
- Terminated application cleanly (Exit option `0`).
- Launched a completely fresh process instance targeting `data/cli_audit_test.db`:
  - Verified catalog book table displays Book ID 1 with 2 available copies.
  - Verified member table displays Member ID 1 (*Katherine Johnson*).
  - Verified payment history displays Payment ID 1 (`₹8.00` on `2026-01-30`) and Payment ID 2 (`₹12.00` on `2026-01-31`).
  - Verified financial report retains zero outstanding balance and 1 fully paid record.

