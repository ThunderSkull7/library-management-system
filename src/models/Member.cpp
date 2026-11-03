#include "models/Member.hpp"

#include <utility>

namespace lms::models {

Member::Member(std::string fullName,
               std::string joinedOn,
               std::optional<std::string> email,
               std::optional<std::string> phone,
               MemberStatus status)
    : id_(0),
      fullName_(std::move(fullName)),
      joinedOn_(std::move(joinedOn)),
      email_(std::move(email)),
      phone_(std::move(phone)),
      status_(status) {}

Member::Member(int id,
               std::string fullName,
               std::string joinedOn,
               std::optional<std::string> email,
               std::optional<std::string> phone,
               MemberStatus status)
    : id_(id),
      fullName_(std::move(fullName)),
      joinedOn_(std::move(joinedOn)),
      email_(std::move(email)),
      phone_(std::move(phone)),
      status_(status) {}

} // namespace lms::models

