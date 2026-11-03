#pragma once

#include "models/Member.hpp"
#include "models/MemberStatus.hpp"
#include "repositories/MemberRepository.hpp"

#include <optional>
#include <string>
#include <vector>

namespace lms::services {

/**
 * @brief Service responsible for patron account registration, profile updates, and lifecycle management.
 */
class MemberService {
public:
    explicit MemberService(repositories::MemberRepository& memberRepository);

    /**
     * @brief Registers a new library member.
     * @throws ValidationException on empty name, invalid date, invalid email format, or duplicate email.
     * @return Generated ID of the registered member.
     */
    int registerMember(const std::string& fullName,
                       const std::string& joinedOn,
                       const std::optional<std::string>& email = std::nullopt,
                       const std::optional<std::string>& phone = std::nullopt,
                       models::MemberStatus status = models::MemberStatus::Active);

    /**
     * @brief Registers a new member from a Member entity object.
     */
    int registerMember(const models::Member& member);

    /**
     * @brief Updates an existing member's account information.
     * @throws NotFoundException if member ID does not exist.
     * @throws ValidationException on invalid name, invalid date, malformed email, or duplicate email.
     */
    void updateMember(const models::Member& member);

    /**
     * @brief Retrieves a member by ID.
     * @throws NotFoundException if member does not exist.
     */
    [[nodiscard]] models::Member getMember(int id);

    /**
     * @brief Retrieves a member by exact email address.
     * @throws ValidationException if email parameter is empty or whitespace.
     */
    [[nodiscard]] std::optional<models::Member> getMemberByEmail(const std::string& email);

    /**
     * @brief Retrieves all registered members ordered by ID ascending.
     */
    [[nodiscard]] std::vector<models::Member> getAllMembers();

    /**
     * @brief Searches members by full name substring.
     */
    [[nodiscard]] std::vector<models::Member> searchMembersByName(const std::string& nameQuery);

    /**
     * @brief Filters members by account status.
     */
    [[nodiscard]] std::vector<models::Member> filterMembersByStatus(models::MemberStatus status);

    /**
     * @brief Changes the account status of a member (Active, Suspended, Closed).
     * @throws NotFoundException if member does not exist.
     */
    void updateStatus(int memberId, models::MemberStatus newStatus);

    /**
     * @brief Safely deletes a member if no loan circulation records exist.
     * @throws NotFoundException if member does not exist.
     * @throws ValidationException if the member has active or historical loans.
     */
    void deleteMember(int id);

private:
    repositories::MemberRepository& memberRepository_;

    static models::Member validateAndNormalize(const models::Member& member);
};

} // namespace lms::services

