#include "repositories/LoanRepository.hpp"
#include "database/DatabaseException.hpp"
#include "database/PreparedStatement.hpp"

#include <sqlite3.h>

namespace lms::repositories {

LoanRepository::LoanRepository(database::DatabaseConnection& connection)
    : connection_(connection) {}

int LoanRepository::addLoan(const models::Loan& loan) {
    sqlite3* db = connection_.getRawHandle();
    const std::string sql =
        "INSERT INTO loans (copy_id, member_id, issued_on, due_on, returned_on, fine_minor_units, fine_paid) "
        "VALUES (?, ?, ?, ?, ?, ?, ?);";

    auto stmt = database::prepareStatement(db, sql);

    // 1: copy_id
    sqlite3_bind_int(stmt.get(), 1, loan.getCopyId());

    // 2: member_id
    sqlite3_bind_int(stmt.get(), 2, loan.getMemberId());

    // 3: issued_on
    sqlite3_bind_text(stmt.get(), 3, loan.getIssuedOn().c_str(), -1, SQLITE_TRANSIENT);

    // 4: due_on
    sqlite3_bind_text(stmt.get(), 4, loan.getDueOn().c_str(), -1, SQLITE_TRANSIENT);

    // 5: returned_on
    if (loan.getReturnedOn().has_value()) {
        sqlite3_bind_text(stmt.get(), 5, loan.getReturnedOn()->c_str(), -1, SQLITE_TRANSIENT);
    } else {
        sqlite3_bind_null(stmt.get(), 5);
    }

    // 6: fine_minor_units
    sqlite3_bind_int(stmt.get(), 6, loan.getFineMinorUnits());

    // 7: fine_paid
    sqlite3_bind_int(stmt.get(), 7, loan.isFinePaid() ? 1 : 0);

    int rc = sqlite3_step(stmt.get());
    if (rc != SQLITE_DONE) {
        std::string errMsg = sqlite3_errmsg(db);
        throw database::DatabaseException("Failed to add loan: " + errMsg, rc);
    }

    return static_cast<int>(sqlite3_last_insert_rowid(db));
}

bool LoanRepository::updateLoan(const models::Loan& loan) {
    sqlite3* db = connection_.getRawHandle();
    const std::string sql =
        "UPDATE loans SET copy_id = ?, member_id = ?, issued_on = ?, due_on = ?, "
        "returned_on = ?, fine_minor_units = ?, fine_paid = ? WHERE id = ?;";

    auto stmt = database::prepareStatement(db, sql);

    // 1: copy_id
    sqlite3_bind_int(stmt.get(), 1, loan.getCopyId());

    // 2: member_id
    sqlite3_bind_int(stmt.get(), 2, loan.getMemberId());

    // 3: issued_on
    sqlite3_bind_text(stmt.get(), 3, loan.getIssuedOn().c_str(), -1, SQLITE_TRANSIENT);

    // 4: due_on
    sqlite3_bind_text(stmt.get(), 4, loan.getDueOn().c_str(), -1, SQLITE_TRANSIENT);

    // 5: returned_on
    if (loan.getReturnedOn().has_value()) {
        sqlite3_bind_text(stmt.get(), 5, loan.getReturnedOn()->c_str(), -1, SQLITE_TRANSIENT);
    } else {
        sqlite3_bind_null(stmt.get(), 5);
    }

    // 6: fine_minor_units
    sqlite3_bind_int(stmt.get(), 6, loan.getFineMinorUnits());

    // 7: fine_paid
    sqlite3_bind_int(stmt.get(), 7, loan.isFinePaid() ? 1 : 0);

    // 8: id
    sqlite3_bind_int(stmt.get(), 8, loan.getId());

    int rc = sqlite3_step(stmt.get());
    if (rc != SQLITE_DONE) {
        std::string errMsg = sqlite3_errmsg(db);
        throw database::DatabaseException("Failed to update loan: " + errMsg, rc);
    }

    return sqlite3_changes(db) > 0;
}

std::optional<models::Loan> LoanRepository::getLoanById(int id) {
    sqlite3* db = connection_.getRawHandle();
    const std::string sql =
        "SELECT id, copy_id, member_id, issued_on, due_on, returned_on, fine_minor_units, fine_paid "
        "FROM loans WHERE id = ?;";

    auto stmt = database::prepareStatement(db, sql);
    sqlite3_bind_int(stmt.get(), 1, id);

    int rc = sqlite3_step(stmt.get());
    if (rc == SQLITE_ROW) {
        return extractLoanFromStatement(stmt.get());
    }
    if (rc == SQLITE_DONE) {
        return std::nullopt;
    }

    std::string errMsg = sqlite3_errmsg(db);
    throw database::DatabaseException("Error querying loan by ID: " + errMsg, rc);
}

std::optional<models::Loan> LoanRepository::getActiveLoanByCopyId(int copyId) {
    sqlite3* db = connection_.getRawHandle();
    const std::string sql =
        "SELECT id, copy_id, member_id, issued_on, due_on, returned_on, fine_minor_units, fine_paid "
        "FROM loans WHERE copy_id = ? AND returned_on IS NULL;";

    auto stmt = database::prepareStatement(db, sql);
    sqlite3_bind_int(stmt.get(), 1, copyId);

    int rc = sqlite3_step(stmt.get());
    if (rc == SQLITE_ROW) {
        return extractLoanFromStatement(stmt.get());
    }
    if (rc == SQLITE_DONE) {
        return std::nullopt;
    }

    std::string errMsg = sqlite3_errmsg(db);
    throw database::DatabaseException("Error querying active loan by copy ID: " + errMsg, rc);
}

std::vector<models::Loan> LoanRepository::getActiveLoansByMemberId(int memberId) {
    sqlite3* db = connection_.getRawHandle();
    const std::string sql =
        "SELECT id, copy_id, member_id, issued_on, due_on, returned_on, fine_minor_units, fine_paid "
        "FROM loans WHERE member_id = ? AND returned_on IS NULL ORDER BY due_on ASC;";

    auto stmt = database::prepareStatement(db, sql);
    sqlite3_bind_int(stmt.get(), 1, memberId);

    std::vector<models::Loan> loans;
    while (true) {
        int rc = sqlite3_step(stmt.get());
        if (rc == SQLITE_ROW) {
            loans.push_back(extractLoanFromStatement(stmt.get()));
        } else if (rc == SQLITE_DONE) {
            break;
        } else {
            std::string errMsg = sqlite3_errmsg(db);
            throw database::DatabaseException("Error querying active loans by member ID: " + errMsg, rc);
        }
    }
    return loans;
}

std::vector<models::Loan> LoanRepository::getLoanHistoryByMemberId(int memberId) {
    sqlite3* db = connection_.getRawHandle();
    const std::string sql =
        "SELECT id, copy_id, member_id, issued_on, due_on, returned_on, fine_minor_units, fine_paid "
        "FROM loans WHERE member_id = ? ORDER BY id DESC;";

    auto stmt = database::prepareStatement(db, sql);
    sqlite3_bind_int(stmt.get(), 1, memberId);

    std::vector<models::Loan> loans;
    while (true) {
        int rc = sqlite3_step(stmt.get());
        if (rc == SQLITE_ROW) {
            loans.push_back(extractLoanFromStatement(stmt.get()));
        } else if (rc == SQLITE_DONE) {
            break;
        } else {
            std::string errMsg = sqlite3_errmsg(db);
            throw database::DatabaseException("Error querying loan history by member ID: " + errMsg, rc);
        }
    }
    return loans;
}

std::vector<models::Loan> LoanRepository::getLoanHistoryByCopyId(int copyId) {
    sqlite3* db = connection_.getRawHandle();
    const std::string sql =
        "SELECT id, copy_id, member_id, issued_on, due_on, returned_on, fine_minor_units, fine_paid "
        "FROM loans WHERE copy_id = ? ORDER BY id DESC;";

    auto stmt = database::prepareStatement(db, sql);
    sqlite3_bind_int(stmt.get(), 1, copyId);

    std::vector<models::Loan> loans;
    while (true) {
        int rc = sqlite3_step(stmt.get());
        if (rc == SQLITE_ROW) {
            loans.push_back(extractLoanFromStatement(stmt.get()));
        } else if (rc == SQLITE_DONE) {
            break;
        } else {
            std::string errMsg = sqlite3_errmsg(db);
            throw database::DatabaseException("Error querying loan history by copy ID: " + errMsg, rc);
        }
    }
    return loans;
}

std::vector<models::Loan> LoanRepository::getAllLoans() {
    sqlite3* db = connection_.getRawHandle();
    const std::string sql =
        "SELECT id, copy_id, member_id, issued_on, due_on, returned_on, fine_minor_units, fine_paid "
        "FROM loans ORDER BY id ASC;";

    auto stmt = database::prepareStatement(db, sql);
    std::vector<models::Loan> loans;

    while (true) {
        int rc = sqlite3_step(stmt.get());
        if (rc == SQLITE_ROW) {
            loans.push_back(extractLoanFromStatement(stmt.get()));
        } else if (rc == SQLITE_DONE) {
            break;
        } else {
            std::string errMsg = sqlite3_errmsg(db);
            throw database::DatabaseException("Error querying all loans: " + errMsg, rc);
        }
    }
    return loans;
}

std::vector<models::Loan> LoanRepository::getOverdueLoans(const std::string& currentDate) {
    sqlite3* db = connection_.getRawHandle();
    const std::string sql =
        "SELECT id, copy_id, member_id, issued_on, due_on, returned_on, fine_minor_units, fine_paid "
        "FROM loans WHERE returned_on IS NULL AND due_on < ? ORDER BY due_on ASC;";

    auto stmt = database::prepareStatement(db, sql);
    sqlite3_bind_text(stmt.get(), 1, currentDate.c_str(), -1, SQLITE_TRANSIENT);

    std::vector<models::Loan> loans;
    while (true) {
        int rc = sqlite3_step(stmt.get());
        if (rc == SQLITE_ROW) {
            loans.push_back(extractLoanFromStatement(stmt.get()));
        } else if (rc == SQLITE_DONE) {
            break;
        } else {
            std::string errMsg = sqlite3_errmsg(db);
            throw database::DatabaseException("Error querying overdue loans: " + errMsg, rc);
        }
    }
    return loans;
}

std::vector<models::Loan> LoanRepository::getOutstandingFines() {
    sqlite3* db = connection_.getRawHandle();
    const std::string sql =
        "SELECT id, copy_id, member_id, issued_on, due_on, returned_on, fine_minor_units, fine_paid "
        "FROM loans WHERE fine_minor_units > 0 AND fine_paid = 0 ORDER BY id ASC;";

    auto stmt = database::prepareStatement(db, sql);
    std::vector<models::Loan> loans;

    while (true) {
        int rc = sqlite3_step(stmt.get());
        if (rc == SQLITE_ROW) {
            loans.push_back(extractLoanFromStatement(stmt.get()));
        } else if (rc == SQLITE_DONE) {
            break;
        } else {
            std::string errMsg = sqlite3_errmsg(db);
            throw database::DatabaseException("Error querying outstanding fines: " + errMsg, rc);
        }
    }
    return loans;
}

int LoanRepository::getActiveLoanCountForMember(int memberId) {
    sqlite3* db = connection_.getRawHandle();
    const std::string sql =
        "SELECT COUNT(*) FROM loans WHERE member_id = ? AND returned_on IS NULL;";

    auto stmt = database::prepareStatement(db, sql);
    sqlite3_bind_int(stmt.get(), 1, memberId);

    int rc = sqlite3_step(stmt.get());
    if (rc == SQLITE_ROW) {
        return sqlite3_column_int(stmt.get(), 0);
    }

    std::string errMsg = sqlite3_errmsg(db);
    throw database::DatabaseException("Error counting active loans for member: " + errMsg, rc);
}

bool LoanRepository::hasActiveLoanForCopy(int copyId) {
    sqlite3* db = connection_.getRawHandle();
    const std::string sql =
        "SELECT COUNT(*) FROM loans WHERE copy_id = ? AND returned_on IS NULL;";

    auto stmt = database::prepareStatement(db, sql);
    sqlite3_bind_int(stmt.get(), 1, copyId);

    int rc = sqlite3_step(stmt.get());
    if (rc == SQLITE_ROW) {
        return sqlite3_column_int(stmt.get(), 0) > 0;
    }

    std::string errMsg = sqlite3_errmsg(db);
    throw database::DatabaseException("Error checking active loan for copy: " + errMsg, rc);
}

int LoanRepository::addPayment(int loanId, int amountMinorUnits, const std::string& paymentDate) {
    sqlite3* db = connection_.getRawHandle();
    const std::string sql =
        "INSERT INTO loan_payments (loan_id, amount_minor_units, paid_on) VALUES (?, ?, ?);";

    auto stmt = database::prepareStatement(db, sql);
    sqlite3_bind_int(stmt.get(), 1, loanId);
    sqlite3_bind_int(stmt.get(), 2, amountMinorUnits);
    sqlite3_bind_text(stmt.get(), 3, paymentDate.c_str(), -1, SQLITE_TRANSIENT);

    int rc = sqlite3_step(stmt.get());
    if (rc != SQLITE_DONE) {
        std::string errMsg = sqlite3_errmsg(db);
        throw database::DatabaseException("Failed to add loan payment: " + errMsg, rc);
    }

    return static_cast<int>(sqlite3_last_insert_rowid(db));
}

int LoanRepository::getTotalPaidForLoan(int loanId) {
    sqlite3* db = connection_.getRawHandle();
    const std::string sql =
        "SELECT COALESCE(SUM(amount_minor_units), 0) FROM loan_payments WHERE loan_id = ?;";

    auto stmt = database::prepareStatement(db, sql);
    sqlite3_bind_int(stmt.get(), 1, loanId);

    int rc = sqlite3_step(stmt.get());
    if (rc == SQLITE_ROW) {
        return sqlite3_column_int(stmt.get(), 0);
    }

    std::string errMsg = sqlite3_errmsg(db);
    throw database::DatabaseException("Error querying total paid for loan: " + errMsg, rc);
}

std::vector<models::LoanPayment> LoanRepository::getPaymentsForLoan(int loanId) {
    sqlite3* db = connection_.getRawHandle();
    const std::string sql =
        "SELECT id, loan_id, amount_minor_units, paid_on FROM loan_payments WHERE loan_id = ? ORDER BY id ASC;";

    auto stmt = database::prepareStatement(db, sql);
    sqlite3_bind_int(stmt.get(), 1, loanId);

    std::vector<models::LoanPayment> payments;
    while (true) {
        int rc = sqlite3_step(stmt.get());
        if (rc == SQLITE_ROW) {
            payments.push_back(extractPaymentFromStatement(stmt.get()));
        } else if (rc == SQLITE_DONE) {
            break;
        } else {
            std::string errMsg = sqlite3_errmsg(db);
            throw database::DatabaseException("Error querying loan payments: " + errMsg, rc);
        }
    }
    return payments;
}

bool LoanRepository::setFinePaidStatus(int loanId, bool paid) {
    sqlite3* db = connection_.getRawHandle();
    const std::string sql = "UPDATE loans SET fine_paid = ? WHERE id = ?;";

    auto stmt = database::prepareStatement(db, sql);
    sqlite3_bind_int(stmt.get(), 1, paid ? 1 : 0);
    sqlite3_bind_int(stmt.get(), 2, loanId);

    int rc = sqlite3_step(stmt.get());
    if (rc != SQLITE_DONE) {
        std::string errMsg = sqlite3_errmsg(db);
        throw database::DatabaseException("Failed to set fine paid status: " + errMsg, rc);
    }

    return sqlite3_changes(db) > 0;
}

models::Loan LoanRepository::extractLoanFromStatement(sqlite3_stmt* stmt) {
    int id = sqlite3_column_int(stmt, 0);
    int copyId = sqlite3_column_int(stmt, 1);
    int memberId = sqlite3_column_int(stmt, 2);
    std::string issuedOn = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
    std::string dueOn = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));

    std::optional<std::string> returnedOn = std::nullopt;
    if (sqlite3_column_type(stmt, 5) != SQLITE_NULL) {
        returnedOn = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
    }

    int fineMinorUnits = sqlite3_column_int(stmt, 6);
    bool finePaid = sqlite3_column_int(stmt, 7) != 0;

    return models::Loan(id, copyId, memberId, std::move(issuedOn), std::move(dueOn),
                        std::move(returnedOn), fineMinorUnits, finePaid);
}

models::LoanPayment LoanRepository::extractPaymentFromStatement(sqlite3_stmt* stmt) {
    int id = sqlite3_column_int(stmt, 0);
    int loanId = sqlite3_column_int(stmt, 1);
    int amount = sqlite3_column_int(stmt, 2);
    std::string paidOn = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));

    return models::LoanPayment(id, loanId, amount, std::move(paidOn));
}

} // namespace lms::repositories
