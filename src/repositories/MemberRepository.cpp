#include "repositories/MemberRepository.hpp"
#include "database/DatabaseException.hpp"
#include "database/PreparedStatement.hpp"

#include <sqlite3.h>

namespace lms::repositories {

MemberRepository::MemberRepository(database::DatabaseConnection& connection)
    : connection_(connection) {}

int MemberRepository::addMember(const models::Member& member) {
    sqlite3* db = connection_.getRawHandle();
    const std::string sql =
        "INSERT INTO members (full_name, email, phone, joined_on, status) "
        "VALUES (?, ?, ?, ?, ?);";

    auto stmt = database::prepareStatement(db, sql);

    // 1: full_name
    sqlite3_bind_text(stmt.get(), 1, member.getFullName().c_str(), -1, SQLITE_TRANSIENT);

    // 2: email
    if (member.getEmail().has_value()) {
        sqlite3_bind_text(stmt.get(), 2, member.getEmail()->c_str(), -1, SQLITE_TRANSIENT);
    } else {
        sqlite3_bind_null(stmt.get(), 2);
    }

    // 3: phone
    if (member.getPhone().has_value()) {
        sqlite3_bind_text(stmt.get(), 3, member.getPhone()->c_str(), -1, SQLITE_TRANSIENT);
    } else {
        sqlite3_bind_null(stmt.get(), 3);
    }

    // 4: joined_on
    sqlite3_bind_text(stmt.get(), 4, member.getJoinedOn().c_str(), -1, SQLITE_TRANSIENT);

    // 5: status
    std::string statusStr = models::toString(member.getStatus());
    sqlite3_bind_text(stmt.get(), 5, statusStr.c_str(), -1, SQLITE_TRANSIENT);

    int rc = sqlite3_step(stmt.get());
    if (rc != SQLITE_DONE) {
        std::string errMsg = sqlite3_errmsg(db);
        throw database::DatabaseException("Failed to add member: " + errMsg, rc);
    }

    return static_cast<int>(sqlite3_last_insert_rowid(db));
}

bool MemberRepository::updateMember(const models::Member& member) {
    sqlite3* db = connection_.getRawHandle();
    const std::string sql =
        "UPDATE members SET full_name = ?, email = ?, phone = ?, joined_on = ?, status = ? "
        "WHERE id = ?;";

    auto stmt = database::prepareStatement(db, sql);

    // 1: full_name
    sqlite3_bind_text(stmt.get(), 1, member.getFullName().c_str(), -1, SQLITE_TRANSIENT);

    // 2: email
    if (member.getEmail().has_value()) {
        sqlite3_bind_text(stmt.get(), 2, member.getEmail()->c_str(), -1, SQLITE_TRANSIENT);
    } else {
        sqlite3_bind_null(stmt.get(), 2);
    }

    // 3: phone
    if (member.getPhone().has_value()) {
        sqlite3_bind_text(stmt.get(), 3, member.getPhone()->c_str(), -1, SQLITE_TRANSIENT);
    } else {
        sqlite3_bind_null(stmt.get(), 3);
    }

    // 4: joined_on
    sqlite3_bind_text(stmt.get(), 4, member.getJoinedOn().c_str(), -1, SQLITE_TRANSIENT);

    // 5: status
    std::string statusStr = models::toString(member.getStatus());
    sqlite3_bind_text(stmt.get(), 5, statusStr.c_str(), -1, SQLITE_TRANSIENT);

    // 6: id
    sqlite3_bind_int(stmt.get(), 6, member.getId());

    int rc = sqlite3_step(stmt.get());
    if (rc != SQLITE_DONE) {
        std::string errMsg = sqlite3_errmsg(db);
        throw database::DatabaseException("Failed to update member: " + errMsg, rc);
    }

    return sqlite3_changes(db) > 0;
}

std::optional<models::Member> MemberRepository::getMemberById(int id) {
    sqlite3* db = connection_.getRawHandle();
    const std::string sql =
        "SELECT id, full_name, email, phone, joined_on, status FROM members WHERE id = ?;";

    auto stmt = database::prepareStatement(db, sql);
    sqlite3_bind_int(stmt.get(), 1, id);

    int rc = sqlite3_step(stmt.get());
    if (rc == SQLITE_ROW) {
        return extractMemberFromStatement(stmt.get());
    }
    if (rc == SQLITE_DONE) {
        return std::nullopt;
    }

    std::string errMsg = sqlite3_errmsg(db);
    throw database::DatabaseException("Error querying member by ID: " + errMsg, rc);
}

std::optional<models::Member> MemberRepository::getMemberByEmail(const std::string& email) {
    sqlite3* db = connection_.getRawHandle();
    const std::string sql =
        "SELECT id, full_name, email, phone, joined_on, status FROM members WHERE email = ?;";

    auto stmt = database::prepareStatement(db, sql);
    sqlite3_bind_text(stmt.get(), 1, email.c_str(), -1, SQLITE_TRANSIENT);

    int rc = sqlite3_step(stmt.get());
    if (rc == SQLITE_ROW) {
        return extractMemberFromStatement(stmt.get());
    }
    if (rc == SQLITE_DONE) {
        return std::nullopt;
    }

    std::string errMsg = sqlite3_errmsg(db);
    throw database::DatabaseException("Error querying member by email: " + errMsg, rc);
}

std::vector<models::Member> MemberRepository::getAllMembers() {
    sqlite3* db = connection_.getRawHandle();
    const std::string sql =
        "SELECT id, full_name, email, phone, joined_on, status FROM members ORDER BY id ASC;";

    auto stmt = database::prepareStatement(db, sql);
    std::vector<models::Member> members;

    while (true) {
        int rc = sqlite3_step(stmt.get());
        if (rc == SQLITE_ROW) {
            members.push_back(extractMemberFromStatement(stmt.get()));
        } else if (rc == SQLITE_DONE) {
            break;
        } else {
            std::string errMsg = sqlite3_errmsg(db);
            throw database::DatabaseException("Error querying all members: " + errMsg, rc);
        }
    }
    return members;
}

std::vector<models::Member> MemberRepository::searchByName(const std::string& nameQuery) {
    sqlite3* db = connection_.getRawHandle();
    const std::string sql =
        "SELECT id, full_name, email, phone, joined_on, status "
        "FROM members WHERE full_name LIKE ? ORDER BY id ASC;";

    auto stmt = database::prepareStatement(db, sql);
    std::string pattern = "%" + nameQuery + "%";
    sqlite3_bind_text(stmt.get(), 1, pattern.c_str(), -1, SQLITE_TRANSIENT);

    std::vector<models::Member> members;
    while (true) {
        int rc = sqlite3_step(stmt.get());
        if (rc == SQLITE_ROW) {
            members.push_back(extractMemberFromStatement(stmt.get()));
        } else if (rc == SQLITE_DONE) {
            break;
        } else {
            std::string errMsg = sqlite3_errmsg(db);
            throw database::DatabaseException("Error searching members by name: " + errMsg, rc);
        }
    }
    return members;
}

std::vector<models::Member> MemberRepository::searchByStatus(models::MemberStatus status) {
    sqlite3* db = connection_.getRawHandle();
    const std::string sql =
        "SELECT id, full_name, email, phone, joined_on, status "
        "FROM members WHERE status = ? ORDER BY id ASC;";

    auto stmt = database::prepareStatement(db, sql);
    std::string statusStr = models::toString(status);
    sqlite3_bind_text(stmt.get(), 1, statusStr.c_str(), -1, SQLITE_TRANSIENT);

    std::vector<models::Member> members;
    while (true) {
        int rc = sqlite3_step(stmt.get());
        if (rc == SQLITE_ROW) {
            members.push_back(extractMemberFromStatement(stmt.get()));
        } else if (rc == SQLITE_DONE) {
            break;
        } else {
            std::string errMsg = sqlite3_errmsg(db);
            throw database::DatabaseException("Error querying members by status: " + errMsg, rc);
        }
    }
    return members;
}

bool MemberRepository::updateMemberStatus(int memberId, models::MemberStatus newStatus) {
    sqlite3* db = connection_.getRawHandle();
    const std::string sql = "UPDATE members SET status = ? WHERE id = ?;";

    auto stmt = database::prepareStatement(db, sql);
    std::string statusStr = models::toString(newStatus);
    sqlite3_bind_text(stmt.get(), 1, statusStr.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt.get(), 2, memberId);

    int rc = sqlite3_step(stmt.get());
    if (rc != SQLITE_DONE) {
        std::string errMsg = sqlite3_errmsg(db);
        throw database::DatabaseException("Failed to update member status: " + errMsg, rc);
    }

    return sqlite3_changes(db) > 0;
}

bool MemberRepository::hasLoanHistory(int memberId) {
    sqlite3* db = connection_.getRawHandle();
    const std::string sql = "SELECT COUNT(*) FROM loans WHERE member_id = ?;";

    auto stmt = database::prepareStatement(db, sql);
    sqlite3_bind_int(stmt.get(), 1, memberId);

    int rc = sqlite3_step(stmt.get());
    if (rc == SQLITE_ROW) {
        return sqlite3_column_int(stmt.get(), 0) > 0;
    }

    std::string errMsg = sqlite3_errmsg(db);
    throw database::DatabaseException("Error checking member loan history: " + errMsg, rc);
}

bool MemberRepository::hasActiveLoan(int memberId) {
    sqlite3* db = connection_.getRawHandle();
    const std::string sql = "SELECT COUNT(*) FROM loans WHERE member_id = ? AND returned_on IS NULL;";

    auto stmt = database::prepareStatement(db, sql);
    sqlite3_bind_int(stmt.get(), 1, memberId);

    int rc = sqlite3_step(stmt.get());
    if (rc == SQLITE_ROW) {
        return sqlite3_column_int(stmt.get(), 0) > 0;
    }

    std::string errMsg = sqlite3_errmsg(db);
    throw database::DatabaseException("Error checking member active loans: " + errMsg, rc);
}

bool MemberRepository::deleteMember(int id) {
    if (hasLoanHistory(id)) {
        throw database::DatabaseException("Cannot delete member: loan history exists.");
    }

    sqlite3* db = connection_.getRawHandle();
    const std::string sql = "DELETE FROM members WHERE id = ?;";

    auto stmt = database::prepareStatement(db, sql);
    sqlite3_bind_int(stmt.get(), 1, id);

    int rc = sqlite3_step(stmt.get());
    if (rc != SQLITE_DONE) {
        std::string errMsg = sqlite3_errmsg(db);
        throw database::DatabaseException("Failed to delete member: " + errMsg, rc);
    }

    return sqlite3_changes(db) > 0;
}

models::Member MemberRepository::extractMemberFromStatement(sqlite3_stmt* stmt) {
    int id = sqlite3_column_int(stmt, 0);
    std::string fullName = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));

    std::optional<std::string> email = std::nullopt;
    if (sqlite3_column_type(stmt, 2) != SQLITE_NULL) {
        email = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
    }

    std::optional<std::string> phone = std::nullopt;
    if (sqlite3_column_type(stmt, 3) != SQLITE_NULL) {
        phone = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
    }

    std::string joinedOn = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));

    std::string statusStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
    models::MemberStatus status = models::memberStatusFromString(statusStr);

    return models::Member(id, std::move(fullName), std::move(joinedOn), std::move(email),
                          std::move(phone), status);
}

} // namespace lms::repositories

