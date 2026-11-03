#include "services/MemberService.hpp"
#include "utils/ValidationException.hpp"
#include "utils/ValidationUtils.hpp"

namespace lms::services {

MemberService::MemberService(repositories::MemberRepository& memberRepository)
    : memberRepository_(memberRepository) {}

models::Member MemberService::validateAndNormalize(const models::Member& member) {
    std::string fullName = utils::trim(member.getFullName());
    if (fullName.empty()) {
        throw utils::ValidationException("Member full name cannot be empty or whitespace-only.");
    }

    std::string joinedOn = utils::trim(member.getJoinedOn());
    if (joinedOn.empty() || !utils::isValidIsoDate(joinedOn)) {
        throw utils::ValidationException(
            "Joined date must be a valid calendar date in YYYY-MM-DD format. Received: '" + joinedOn + "'");
    }

    std::optional<std::string> email = std::nullopt;
    if (member.getEmail().has_value()) {
        std::string trimmedEmail = utils::trim(*member.getEmail());
        if (trimmedEmail.empty()) {
            throw utils::ValidationException("Email, if provided, cannot be empty or whitespace-only.");
        }
        if (!utils::isValidEmail(trimmedEmail)) {
            throw utils::ValidationException(
                "Invalid email format: '" + trimmedEmail + "'. Expected format: user@domain.tld");
        }
        email = trimmedEmail;
    }

    std::optional<std::string> phone = std::nullopt;
    if (member.getPhone().has_value()) {
        std::string trimmedPhone = utils::trim(*member.getPhone());
        if (trimmedPhone.empty()) {
            throw utils::ValidationException("Phone number, if provided, cannot be empty or whitespace-only.");
        }
        phone = trimmedPhone;
    }

    return models::Member(member.getId(), std::move(fullName), std::move(joinedOn),
                          std::move(email), std::move(phone), member.getStatus());
}

int MemberService::registerMember(const std::string& fullName,
                                 const std::string& joinedOn,
                                 const std::optional<std::string>& email,
                                 const std::optional<std::string>& phone,
                                 models::MemberStatus status) {
    models::Member member(fullName, joinedOn, email, phone, status);
    return registerMember(member);
}

int MemberService::registerMember(const models::Member& member) {
    models::Member normalized = validateAndNormalize(member);

    if (normalized.getEmail().has_value()) {
        auto existing = memberRepository_.getMemberByEmail(*normalized.getEmail());
        if (existing.has_value()) {
            throw utils::ValidationException(
                "A member with email '" + *normalized.getEmail() + "' already exists.");
        }
    }

    return memberRepository_.addMember(normalized);
}

void MemberService::updateMember(const models::Member& member) {
    auto existing = memberRepository_.getMemberById(member.getId());
    if (!existing.has_value()) {
        throw utils::NotFoundException("Member with ID " + std::to_string(member.getId()) + " does not exist.");
    }

    models::Member normalized = validateAndNormalize(member);

    if (normalized.getEmail().has_value()) {
        auto duplicateCheck = memberRepository_.getMemberByEmail(*normalized.getEmail());
        if (duplicateCheck.has_value() && duplicateCheck->getId() != member.getId()) {
            throw utils::ValidationException(
                "A member with email '" + *normalized.getEmail() + "' already exists.");
        }
    }

    memberRepository_.updateMember(normalized);
}

models::Member MemberService::getMember(int id) {
    auto member = memberRepository_.getMemberById(id);
    if (!member.has_value()) {
        throw utils::NotFoundException("Member with ID " + std::to_string(id) + " does not exist.");
    }
    return *member;
}

std::optional<models::Member> MemberService::getMemberByEmail(const std::string& email) {
    std::string trimmed = utils::trim(email);
    if (trimmed.empty()) {
        throw utils::ValidationException("Email search query cannot be empty or whitespace-only.");
    }
    return memberRepository_.getMemberByEmail(trimmed);
}

std::vector<models::Member> MemberService::getAllMembers() {
    return memberRepository_.getAllMembers();
}

std::vector<models::Member> MemberService::searchMembersByName(const std::string& nameQuery) {
    return memberRepository_.searchByName(utils::trim(nameQuery));
}

std::vector<models::Member> MemberService::filterMembersByStatus(models::MemberStatus status) {
    return memberRepository_.searchByStatus(status);
}

void MemberService::updateStatus(int memberId, models::MemberStatus newStatus) {
    auto existing = memberRepository_.getMemberById(memberId);
    if (!existing.has_value()) {
        throw utils::NotFoundException("Member with ID " + std::to_string(memberId) + " does not exist.");
    }
    memberRepository_.updateMemberStatus(memberId, newStatus);
}

void MemberService::deleteMember(int id) {
    auto existing = memberRepository_.getMemberById(id);
    if (!existing.has_value()) {
        throw utils::NotFoundException("Member with ID " + std::to_string(id) + " does not exist.");
    }

    if (memberRepository_.hasActiveLoan(id)) {
        throw utils::ValidationException(
            "Cannot delete member with ID " + std::to_string(id) +
            ": member currently has active unreturned loans.");
    }

    if (memberRepository_.hasLoanHistory(id)) {
        throw utils::ValidationException(
            "Cannot delete member with ID " + std::to_string(id) +
            ": member loan history exists and must be preserved.");
    }

    memberRepository_.deleteMember(id);
}

} // namespace lms::services

