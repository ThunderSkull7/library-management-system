#include "models/LibraryPolicy.hpp"
#include "utils/ValidationException.hpp"

#include <gtest/gtest.h>

namespace {

using lms::models::LibraryPolicy;
using lms::utils::ValidationException;

TEST(LibraryPolicyTest, DefaultValues) {
    LibraryPolicy policy;
    EXPECT_EQ(policy.getLoanDurationDays(), 14);
    EXPECT_EQ(policy.getFinePerDayMinorUnits(), 200);
    EXPECT_EQ(policy.getMaxActiveLoansPerMember(), 5);
    EXPECT_EQ(policy.getGracePeriodDays(), 0);
    EXPECT_FALSE(policy.getMaxFineMinorUnits().has_value());
}

TEST(LibraryPolicyTest, CustomValuesConstructor) {
    LibraryPolicy policy(21, 500, 10, 2, 5000);
    EXPECT_EQ(policy.getLoanDurationDays(), 21);
    EXPECT_EQ(policy.getFinePerDayMinorUnits(), 500);
    EXPECT_EQ(policy.getMaxActiveLoansPerMember(), 10);
    EXPECT_EQ(policy.getGracePeriodDays(), 2);
    ASSERT_TRUE(policy.getMaxFineMinorUnits().has_value());
    EXPECT_EQ(*policy.getMaxFineMinorUnits(), 5000);
}

TEST(LibraryPolicyTest, SettersAndEquality) {
    LibraryPolicy p1;
    LibraryPolicy p2;
    EXPECT_EQ(p1, p2);

    p1.setLoanDurationDays(28);
    EXPECT_NE(p1, p2);
    EXPECT_EQ(p1.getLoanDurationDays(), 28);

    p1.setFinePerDayMinorUnits(300);
    EXPECT_EQ(p1.getFinePerDayMinorUnits(), 300);

    p1.setMaxActiveLoansPerMember(7);
    EXPECT_EQ(p1.getMaxActiveLoansPerMember(), 7);

    p1.setGracePeriodDays(3);
    EXPECT_EQ(p1.getGracePeriodDays(), 3);

    p1.setMaxFineMinorUnits(10000);
    ASSERT_TRUE(p1.getMaxFineMinorUnits().has_value());
    EXPECT_EQ(*p1.getMaxFineMinorUnits(), 10000);

    p1.setMaxFineMinorUnits(std::nullopt);
    EXPECT_FALSE(p1.getMaxFineMinorUnits().has_value());
}

TEST(LibraryPolicyTest, RejectInvalidLoanDuration) {
    EXPECT_THROW((void)LibraryPolicy(0, 200, 5), ValidationException);
    EXPECT_THROW((void)LibraryPolicy(-5, 200, 5), ValidationException);

    LibraryPolicy policy;
    EXPECT_THROW(policy.setLoanDurationDays(0), ValidationException);
    EXPECT_THROW(policy.setLoanDurationDays(-1), ValidationException);
}

TEST(LibraryPolicyTest, RejectInvalidFinePerDay) {
    EXPECT_THROW((void)LibraryPolicy(14, -1, 5), ValidationException);

    LibraryPolicy policy;
    EXPECT_THROW(policy.setFinePerDayMinorUnits(-100), ValidationException);
    // 0 paise per day is allowed (free library)
    EXPECT_NO_THROW(policy.setFinePerDayMinorUnits(0));
}

TEST(LibraryPolicyTest, RejectInvalidMaxLoans) {
    EXPECT_THROW((void)LibraryPolicy(14, 200, 0), ValidationException);
    EXPECT_THROW((void)LibraryPolicy(14, 200, -2), ValidationException);

    LibraryPolicy policy;
    EXPECT_THROW(policy.setMaxActiveLoansPerMember(0), ValidationException);
    EXPECT_THROW(policy.setMaxActiveLoansPerMember(-3), ValidationException);
}

TEST(LibraryPolicyTest, RejectInvalidGracePeriod) {
    EXPECT_THROW((void)LibraryPolicy(14, 200, 5, -1), ValidationException);

    LibraryPolicy policy;
    EXPECT_THROW(policy.setGracePeriodDays(-1), ValidationException);
    EXPECT_NO_THROW(policy.setGracePeriodDays(0));
}

TEST(LibraryPolicyTest, RejectInvalidMaxFine) {
    EXPECT_THROW((void)LibraryPolicy(14, 200, 5, 0, -100), ValidationException);

    LibraryPolicy policy;
    EXPECT_THROW(policy.setMaxFineMinorUnits(-1), ValidationException);
    EXPECT_NO_THROW(policy.setMaxFineMinorUnits(0));
}

} // namespace
