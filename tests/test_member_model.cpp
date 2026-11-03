#include "models/Member.hpp"
#include "models/MemberStatus.hpp"

#include <gtest/gtest.h>
#include <stdexcept>
#include <string>

namespace {

TEST(MemberModelTest, ConstructValidMemberDefault) {
    lms::models::Member member;
    EXPECT_EQ(member.getId(), 0);
    EXPECT_TRUE(member.getFullName().empty());
    EXPECT_TRUE(member.getJoinedOn().empty());
    EXPECT_FALSE(member.getEmail().has_value());
    EXPECT_FALSE(member.getPhone().has_value());
    EXPECT_EQ(member.getStatus(), lms::models::MemberStatus::Active);
}

TEST(MemberModelTest, ConstructValidMemberWithoutId) {
    lms::models::Member member("Alice Smith", "2026-01-15", "alice@example.com", "555-1234", lms::models::MemberStatus::Active);

    EXPECT_EQ(member.getId(), 0);
    EXPECT_EQ(member.getFullName(), "Alice Smith");
    EXPECT_EQ(member.getJoinedOn(), "2026-01-15");
    ASSERT_TRUE(member.getEmail().has_value());
    EXPECT_EQ(*member.getEmail(), "alice@example.com");
    ASSERT_TRUE(member.getPhone().has_value());
    EXPECT_EQ(*member.getPhone(), "555-1234");
    EXPECT_EQ(member.getStatus(), lms::models::MemberStatus::Active);
}

TEST(MemberModelTest, ConstructValidMemberWithId) {
    lms::models::Member member(101, "Bob Jones", "2026-02-01", std::nullopt, std::nullopt, lms::models::MemberStatus::Suspended);

    EXPECT_EQ(member.getId(), 101);
    EXPECT_EQ(member.getFullName(), "Bob Jones");
    EXPECT_EQ(member.getJoinedOn(), "2026-02-01");
    EXPECT_FALSE(member.getEmail().has_value());
    EXPECT_FALSE(member.getPhone().has_value());
    EXPECT_EQ(member.getStatus(), lms::models::MemberStatus::Suspended);
}

TEST(MemberModelTest, SettersAndEquality) {
    lms::models::Member m1(1, "Charlie Brown", "2026-01-01");
    lms::models::Member m2(1, "Charlie Brown", "2026-01-01");
    EXPECT_EQ(m1, m2);

    m1.setFullName("Charlie B.");
    EXPECT_NE(m1, m2);

    m2.setFullName("Charlie B.");
    EXPECT_EQ(m1, m2);

    m1.setEmail("charlie@peanuts.com");
    m1.setPhone("123-4567");
    m1.setJoinedOn("2026-03-01");
    m1.setStatus(lms::models::MemberStatus::Closed);

    EXPECT_EQ(*m1.getEmail(), "charlie@peanuts.com");
    EXPECT_EQ(*m1.getPhone(), "123-4567");
    EXPECT_EQ(m1.getJoinedOn(), "2026-03-01");
    EXPECT_EQ(m1.getStatus(), lms::models::MemberStatus::Closed);
}

TEST(MemberStatusTest, EnumToStringConversion) {
    EXPECT_EQ(lms::models::toString(lms::models::MemberStatus::Active), "Active");
    EXPECT_EQ(lms::models::toString(lms::models::MemberStatus::Suspended), "Suspended");
    EXPECT_EQ(lms::models::toString(lms::models::MemberStatus::Closed), "Closed");
}

TEST(MemberStatusTest, StringToEnumConversion) {
    EXPECT_EQ(lms::models::memberStatusFromString("Active"), lms::models::MemberStatus::Active);
    EXPECT_EQ(lms::models::memberStatusFromString("Suspended"), lms::models::MemberStatus::Suspended);
    EXPECT_EQ(lms::models::memberStatusFromString("Closed"), lms::models::MemberStatus::Closed);
}

TEST(MemberStatusTest, InvalidStringToEnumThrows) {
    EXPECT_THROW(lms::models::memberStatusFromString("active"), std::invalid_argument);
    EXPECT_THROW(lms::models::memberStatusFromString("ACTIVE"), std::invalid_argument);
    EXPECT_THROW(lms::models::memberStatusFromString("Pending"), std::invalid_argument);
    EXPECT_THROW(lms::models::memberStatusFromString(""), std::invalid_argument);
}

} // namespace

