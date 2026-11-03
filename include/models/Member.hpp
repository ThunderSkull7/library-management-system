#pragma once

#include "models/MemberStatus.hpp"

#include <optional>
#include <string>

namespace lms::models {

/**
 * @brief Represents a library patron / cardholder account.
 */
class Member {
public:
    Member() = default;

    /**
     * @brief Constructs a new Member entity (typically before persistence, id = 0).
     */
    Member(std::string fullName,
           std::string joinedOn,
           std::optional<std::string> email = std::nullopt,
           std::optional<std::string> phone = std::nullopt,
           MemberStatus status = MemberStatus::Active);

    /**
     * @brief Constructs a Member entity with a persisted database identifier.
     */
    Member(int id,
           std::string fullName,
           std::string joinedOn,
           std::optional<std::string> email = std::nullopt,
           std::optional<std::string> phone = std::nullopt,
           MemberStatus status = MemberStatus::Active);

    // Getters
    [[nodiscard]] int getId() const noexcept { return id_; }
    [[nodiscard]] const std::string& getFullName() const noexcept { return fullName_; }
    [[nodiscard]] const std::optional<std::string>& getEmail() const noexcept { return email_; }
    [[nodiscard]] const std::optional<std::string>& getPhone() const noexcept { return phone_; }
    [[nodiscard]] const std::string& getJoinedOn() const noexcept { return joinedOn_; }
    [[nodiscard]] MemberStatus getStatus() const noexcept { return status_; }

    // Setters
    void setId(int id) noexcept { id_ = id; }
    void setFullName(std::string fullName) { fullName_ = std::move(fullName); }
    void setEmail(std::optional<std::string> email) { email_ = std::move(email); }
    void setPhone(std::optional<std::string> phone) { phone_ = std::move(phone); }
    void setJoinedOn(std::string joinedOn) { joinedOn_ = std::move(joinedOn); }
    void setStatus(MemberStatus status) noexcept { status_ = status; }

    // Equality comparison
    bool operator==(const Member& other) const = default;

private:
    int id_{0};
    std::string fullName_{};
    std::string joinedOn_{};
    std::optional<std::string> email_{std::nullopt};
    std::optional<std::string> phone_{std::nullopt};
    MemberStatus status_{MemberStatus::Active};
};

} // namespace lms::models

