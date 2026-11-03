#pragma once

#include "database/DatabaseConnection.hpp"
#include "models/Member.hpp"
#include "models/MemberStatus.hpp"

#include <optional>
#include <string>
#include <vector>

struct sqlite3_stmt;

namespace lms::repositories {

/**
 * @brief Repository for persisting and querying Member entities in SQLite.
 */
class MemberRepository {
public:
    explicit MemberRepository(database::DatabaseConnection& connection);

    /**
     * @brief Inserts a new Member record.
     * @return Generated database ID.
     * @throws DatabaseException on SQL error or unique constraint violation (duplicate email).
     */
    int addMember(const models::Member& member);

    /**
     * @brief Updates an existing Member record.
     * @return True if a record was updated, false if member ID not found.
     * @throws DatabaseException on SQL error.
     */
    bool updateMember(const models::Member& member);

    /**
     * @brief Retrieves a Member by primary key ID.
     */
    [[nodiscard]] std::optional<models::Member> getMemberById(int id);

    /**
     * @brief Retrieves a Member by exact email address.
     */
    [[nodiscard]] std::optional<models::Member> getMemberByEmail(const std::string& email);

    /**
     * @brief Retrieves all registered members ordered by ID ascending.
     */
    [[nodiscard]] std::vector<models::Member> getAllMembers();

    /**
     * @brief Searches members whose full name matches the query substring (case-insensitive ASCII).
     */
    [[nodiscard]] std::vector<models::Member> searchByName(const std::string& nameQuery);

    /**
     * @brief Retrieves members filtered by their membership status.
     */
    [[nodiscard]] std::vector<models::Member> searchByStatus(models::MemberStatus status);

    /**
     * @brief Updates the membership status of a member.
     * @return True if updated, false if member not found.
     */
    bool updateMemberStatus(int memberId, models::MemberStatus newStatus);

    /**
     * @brief Checks if any loans (active or returned) reference this member.
     */
    [[nodiscard]] bool hasLoanHistory(int memberId);

    /**
     * @brief Checks if any active (unreturned) loans exist for this member.
     */
    [[nodiscard]] bool hasActiveLoan(int memberId);

    /**
     * @brief Deletes a member by ID if no loan history exists.
     * @throws DatabaseException if loan history exists or deletion fails.
     * @return True if deleted, false if member not found.
     */
    bool deleteMember(int id);

private:
    database::DatabaseConnection& connection_;

    [[nodiscard]] static models::Member extractMemberFromStatement(sqlite3_stmt* stmt);
};

} // namespace lms::repositories

