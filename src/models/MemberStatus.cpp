#include "models/MemberStatus.hpp"

#include <stdexcept>

namespace lms::models {

std::string toString(MemberStatus status) {
    switch (status) {
        case MemberStatus::Active:
            return "Active";
        case MemberStatus::Suspended:
            return "Suspended";
        case MemberStatus::Closed:
            return "Closed";
    }
    throw std::invalid_argument("Unknown MemberStatus enum value");
}

MemberStatus memberStatusFromString(const std::string& str) {
    if (str == "Active") {
        return MemberStatus::Active;
    }
    if (str == "Suspended") {
        return MemberStatus::Suspended;
    }
    if (str == "Closed") {
        return MemberStatus::Closed;
    }
    throw std::invalid_argument("Invalid MemberStatus string: '" + str + "'");
}

} // namespace lms::models

