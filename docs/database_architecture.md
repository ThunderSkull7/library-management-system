# Database Architecture — Library Management System

## 1. Overview

The Library Management System foundation employs an embedded **SQLite3** database accessed through an idiomatic, modern C++20 resource-management architecture.

The database layer is divided into two primary components:
1. **`DatabaseConnection`**: Responsible for database connection lifecycle management (RAII), runtime configuration, busy timeout handling, and foreign-key constraint enforcement.
2. **`SchemaManager`**: Responsible for transactional DDL execution, schema migration tracking via SQLite's `PRAGMA user_version`, idempotent table and index creation, and rejecting unsupported future versions.

---

## 2. Component Design

### 2.1 DatabaseConnection

`DatabaseConnection` acts as an RAII wrapper around the native SQLite3 handle (`sqlite3*`).

- **Lifecycle & RAII**: Opening a database allocates the underlying SQLite handle and validates its state. The connection is cleanly closed in the destructor or when explicitly invoking `close()`.
- **Copy & Move Semantics**: Copy operations are deleted to prevent duplicate ownership of SQLite connection pointers. Move operations (move constructor and move assignment) are fully supported with `std::exchange`.
- **Foreign Key Enforcement**: SQLite does not enable foreign keys by default. `DatabaseConnection` executes `PRAGMA foreign_keys = ON;` on every connection opened and immediately verifies that `PRAGMA foreign_keys;` returns `1`. If foreign keys cannot be verified, an exception is thrown and the connection is closed.
- **Busy Timeout**: Configured to 5,000 milliseconds to handle concurrent contention scenarios gracefully.
- **Error Propagation**: Low-level SQLite return codes and error strings are wrapped and surfaced as `DatabaseException` (derived from `std::runtime_error`), preserving both the error code and error message.

### 2.2 SchemaManager

`SchemaManager` ensures that the database schema is brought up to date safely and idempotently.

- **Transaction Isolation**: Schema creation and migrations execute within an explicit `BEGIN IMMEDIATE;` ... `COMMIT;` transaction block, with automatic `ROLLBACK;` upon any failure.
- **Schema Versioning**: SQLite's `PRAGMA user_version` tracks the current schema version.
  - Initial baseline version: `1`.
  - Up-to-date checks: If the database is already at version 1, initialization verifies tables/indexes idempotently.
  - Future version safety: If `user_version > CURRENT_SCHEMA_VERSION`, `SchemaManager` throws a descriptive `DatabaseException` refusing to operate on incompatible newer schemas.
- **Idempotency**: DDL statements utilize `IF NOT EXISTS` constructs so that repeated executions preserve existing data without corruption or duplication.

---

## 3. Database Schema Specification

### 3.1 Tables

#### A. `books`
Represents the bibliographic entity in the catalog.

| Column | Type | Constraints | Description |
|---|---|---|---|
| `id` | INTEGER | PRIMARY KEY AUTOINCREMENT | Unique book record identifier |
| `isbn` | TEXT | UNIQUE, NULLABLE | ISBN-10 or ISBN-13 |
| `title` | TEXT | NOT NULL | Title of the book |
| `author` | TEXT | NOT NULL | Primary author or contributor |
| `publisher` | TEXT | NULLABLE | Publishing entity |
| `publication_year`| INTEGER | NULLABLE | Year of publication |
| `category` | TEXT | NULLABLE | Genre / subject classification |
| `description` | TEXT | NULLABLE | Summary or synopsis |

#### B. `book_copies`
Represents specific physical copies owned by the library.

| Column | Type | Constraints | Description |
|---|---|---|---|
| `id` | INTEGER | PRIMARY KEY AUTOINCREMENT | Unique physical copy identifier |
| `book_id` | INTEGER | NOT NULL, FK -> `books(id)` | Associated book record |
| `accession_number`| TEXT | NOT NULL, UNIQUE | Barcode / accession identifier |
| `status` | TEXT | NOT NULL, CHECK in set | Current physical copy state |
| `acquired_on` | TEXT | NULLABLE | Acquisition date (ISO-8601) |

*Allowed `status` values*: `'Available'`, `'On Loan'`, `'Lost'`, `'Damaged'`, `'Withdrawn'`.

#### C. `members`
Represents registered patrons / library card holders.

| Column | Type | Constraints | Description |
|---|---|---|---|
| `id` | INTEGER | PRIMARY KEY AUTOINCREMENT | Unique member identifier |
| `full_name` | TEXT | NOT NULL | Full name of the member |
| `email` | TEXT | UNIQUE, NULLABLE | Contact email address |
| `phone` | TEXT | NULLABLE | Contact telephone number |
| `joined_on` | TEXT | NOT NULL | Registration date (ISO-8601) |
| `status` | TEXT | NOT NULL, CHECK in set | Account membership status |

*Allowed `status` values*: `'Active'`, `'Suspended'`, `'Closed'`.

#### D. `loans`
Represents circulation checkout events and return history.

| Column | Type | Constraints | Description |
|---|---|---|---|
| `id` | INTEGER | PRIMARY KEY AUTOINCREMENT | Unique loan identifier |
| `copy_id` | INTEGER | NOT NULL, FK -> `book_copies(id)` | Physical copy loaned |
| `member_id` | INTEGER | NOT NULL, FK -> `members(id)` | Borrowing member |
| `issued_on` | TEXT | NOT NULL | Date loaned (ISO-8601) |
| `due_on` | TEXT | NOT NULL, CHECK(`due_on >= issued_on`) | Scheduled due date |
| `returned_on` | TEXT | NULLABLE, CHECK(`returned_on >= issued_on`) | Return date |
| `fine_minor_units`| INTEGER| NOT NULL DEFAULT 0, CHECK(`>= 0`) | Overdue fine in cents/pennies |
| `fine_paid` | INTEGER | NOT NULL DEFAULT 0, CHECK(`IN (0, 1)`) | Fine payment boolean flag |

---

## 4. Constraints and Indexing Strategy

### 4.1 Partial Unique Index on Active Loans
To enforce physical real-world consistency, a book copy can never be checked out to multiple members simultaneously:

```sql
CREATE UNIQUE INDEX IF NOT EXISTS one_active_loan_per_copy
ON loans(copy_id)
WHERE returned_on IS NULL;
```
When a book is returned (`returned_on` set to a timestamp), the partial constraint ceases to index that row, allowing the copy to be loaned again.

### 4.2 Query Optimization Indexes
Indexes are strategically placed without redundant duplication:
- **`books`**:
  - `idx_books_title ON books(title)`
  - `idx_books_author ON books(author)`
  - (*`isbn` is uniquely indexed by SQLite auto-index via its `UNIQUE` constraint*)
- **`book_copies`**:
  - `idx_book_copies_book_id ON book_copies(book_id)`
  - `idx_book_copies_status ON book_copies(status)`
  - (*`accession_number` is uniquely indexed by SQLite auto-index via its `UNIQUE` constraint*)
- **`members`**:
  - `idx_members_full_name ON members(full_name)`
  - (*`email` is uniquely indexed by SQLite auto-index via its `UNIQUE` constraint*)
- **`loans`**:
  - `idx_loans_member_id ON loans(member_id)`
  - `idx_loans_copy_id ON loans(copy_id)`
  - `idx_loans_due_on ON loans(due_on)`
  - `idx_loans_active_due ON loans(due_on) WHERE returned_on IS NULL` (for fast overdue and active loan searches)

