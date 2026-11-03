#pragma once

#include <optional>
#include <string>

namespace lms::models {

/**
 * @brief Represents a book circulation loan record issuing a physical copy to a library member.
 */
class Loan {
public:
    Loan() = default;

    /**
     * @brief Constructs a new Loan entity before persistence (id = 0).
     */
    Loan(int copyId,
         int memberId,
         std::string issuedOn,
         std::string dueOn,
         std::optional<std::string> returnedOn = std::nullopt,
         int fineMinorUnits = 0,
         bool finePaid = false);

    /**
     * @brief Constructs a Loan entity with a persisted primary key ID.
     */
    Loan(int id,
         int copyId,
         int memberId,
         std::string issuedOn,
         std::string dueOn,
         std::optional<std::string> returnedOn = std::nullopt,
         int fineMinorUnits = 0,
         bool finePaid = false);

    // Getters
    [[nodiscard]] int getId() const noexcept { return id_; }
    [[nodiscard]] int getCopyId() const noexcept { return copyId_; }
    [[nodiscard]] int getMemberId() const noexcept { return memberId_; }
    [[nodiscard]] const std::string& getIssuedOn() const noexcept { return issuedOn_; }
    [[nodiscard]] const std::string& getDueOn() const noexcept { return dueOn_; }
    [[nodiscard]] const std::optional<std::string>& getReturnedOn() const noexcept { return returnedOn_; }
    [[nodiscard]] int getFineMinorUnits() const noexcept { return fineMinorUnits_; }
    [[nodiscard]] bool isFinePaid() const noexcept { return finePaid_; }

    [[nodiscard]] bool isActive() const noexcept { return !returnedOn_.has_value(); }
    [[nodiscard]] bool isReturned() const noexcept { return returnedOn_.has_value(); }

    // Setters
    void setId(int id) noexcept { id_ = id; }
    void setCopyId(int copyId) noexcept { copyId_ = copyId; }
    void setMemberId(int memberId) noexcept { memberId_ = memberId; }
    void setIssuedOn(std::string issuedOn) { issuedOn_ = std::move(issuedOn); }
    void setDueOn(std::string dueOn) { dueOn_ = std::move(dueOn); }
    void setReturnedOn(std::optional<std::string> returnedOn) { returnedOn_ = std::move(returnedOn); }
    void setFineMinorUnits(int fineMinorUnits) noexcept { fineMinorUnits_ = fineMinorUnits; }
    void setFinePaid(bool finePaid) noexcept { finePaid_ = finePaid; }

    bool operator==(const Loan& other) const = default;

private:
    int id_{0};
    int copyId_{0};
    int memberId_{0};
    std::string issuedOn_{};
    std::string dueOn_{};
    std::optional<std::string> returnedOn_{std::nullopt};
    int fineMinorUnits_{0};
    bool finePaid_{false};
};

} // namespace lms::models
