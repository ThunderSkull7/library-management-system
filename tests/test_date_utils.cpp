#include "utils/DateUtils.hpp"
#include "utils/ValidationException.hpp"

#include <gtest/gtest.h>

namespace {

using lms::utils::DateUtils;
using lms::utils::ValidationException;

TEST(DateUtilsTest, ValidIsoDate) {
    EXPECT_TRUE(DateUtils::isValidIsoDate("2026-03-15"));
    EXPECT_TRUE(DateUtils::isValidIsoDate("2024-02-29")); // Leap year
    EXPECT_TRUE(DateUtils::isValidIsoDate("2000-02-29")); // Century leap year
    EXPECT_TRUE(DateUtils::isValidIsoDate("2026-04-30")); // 30-day month
    EXPECT_TRUE(DateUtils::isValidIsoDate("2026-12-31")); // 31-day month
}

TEST(DateUtilsTest, InvalidIsoDate) {
    EXPECT_FALSE(DateUtils::isValidIsoDate(""));
    EXPECT_FALSE(DateUtils::isValidIsoDate("2023-02-29")); // Non-leap year
    EXPECT_FALSE(DateUtils::isValidIsoDate("1900-02-29")); // Non-leap century year
    EXPECT_FALSE(DateUtils::isValidIsoDate("2026-04-31")); // April has 30 days
    EXPECT_FALSE(DateUtils::isValidIsoDate("2026-13-01")); // Month 13
    EXPECT_FALSE(DateUtils::isValidIsoDate("2026-00-10")); // Month 0
    EXPECT_FALSE(DateUtils::isValidIsoDate("2026-01-00")); // Day 0
    EXPECT_FALSE(DateUtils::isValidIsoDate("2026-01-32")); // Day 32
    EXPECT_FALSE(DateUtils::isValidIsoDate("2026/03/15")); // Slashes
    EXPECT_FALSE(DateUtils::isValidIsoDate("15-03-2026")); // DD-MM-YYYY
    EXPECT_FALSE(DateUtils::isValidIsoDate("2026-3-15"));  // Single digit month
    EXPECT_FALSE(DateUtils::isValidIsoDate("2026-03-5"));  // Single digit day
    EXPECT_FALSE(DateUtils::isValidIsoDate("abcd-ef-gh")); // Non-numeric
}

TEST(DateUtilsTest, AddDays) {
    // Normal same month
    EXPECT_EQ(DateUtils::addDays("2026-03-01", 14), "2026-03-15");

    // Month boundary
    EXPECT_EQ(DateUtils::addDays("2026-01-25", 10), "2026-02-04");

    // Leap year boundary
    EXPECT_EQ(DateUtils::addDays("2024-02-28", 1), "2024-02-29");
    EXPECT_EQ(DateUtils::addDays("2024-02-28", 2), "2024-03-01");

    // Non-leap year boundary
    EXPECT_EQ(DateUtils::addDays("2023-02-28", 1), "2023-03-01");

    // Year boundary
    EXPECT_EQ(DateUtils::addDays("2026-12-25", 10), "2027-01-04");

    // Zero days
    EXPECT_EQ(DateUtils::addDays("2026-05-10", 0), "2026-05-10");

    // Negative days
    EXPECT_EQ(DateUtils::addDays("2026-03-05", -10), "2026-02-23");
    EXPECT_EQ(DateUtils::addDays("2026-01-05", -10), "2025-12-26");

    // Invalid date throws
    EXPECT_THROW((void)DateUtils::addDays("invalid-date", 5), ValidationException);
    EXPECT_THROW((void)DateUtils::addDays("2023-02-29", 5), ValidationException);
}

TEST(DateUtilsTest, DaysBetween) {
    // Same day
    EXPECT_EQ(DateUtils::daysBetween("2026-03-15", "2026-03-15"), 0);

    // Forward difference
    EXPECT_EQ(DateUtils::daysBetween("2026-03-01", "2026-03-15"), 14);

    // Backward difference
    EXPECT_EQ(DateUtils::daysBetween("2026-03-15", "2026-03-01"), -14);

    // Across leap day
    EXPECT_EQ(DateUtils::daysBetween("2024-02-28", "2024-03-01"), 2);

    // Across non-leap day
    EXPECT_EQ(DateUtils::daysBetween("2023-02-28", "2023-03-01"), 1);

    // Across year boundary
    EXPECT_EQ(DateUtils::daysBetween("2025-12-30", "2026-01-02"), 3);

    // Invalid dates throw
    EXPECT_THROW((void)DateUtils::daysBetween("invalid", "2026-03-15"), ValidationException);
    EXPECT_THROW((void)DateUtils::daysBetween("2026-03-15", "invalid"), ValidationException);
}

TEST(DateUtilsTest, DateComparisons) {
    EXPECT_TRUE(DateUtils::isDateBefore("2026-03-01", "2026-03-02"));
    EXPECT_FALSE(DateUtils::isDateBefore("2026-03-02", "2026-03-01"));
    EXPECT_FALSE(DateUtils::isDateBefore("2026-03-01", "2026-03-01"));

    EXPECT_TRUE(DateUtils::isDateAfter("2026-03-02", "2026-03-01"));
    EXPECT_FALSE(DateUtils::isDateAfter("2026-03-01", "2026-03-02"));
    EXPECT_FALSE(DateUtils::isDateAfter("2026-03-01", "2026-03-01"));

    EXPECT_TRUE(DateUtils::isDateEqual("2026-03-01", "2026-03-01"));
    EXPECT_FALSE(DateUtils::isDateEqual("2026-03-01", "2026-03-02"));

    // Throw on invalid
    EXPECT_THROW((void)DateUtils::isDateBefore("bad", "2026-03-01"), ValidationException);
    EXPECT_THROW((void)DateUtils::isDateAfter("2026-03-01", "bad"), ValidationException);
    EXPECT_THROW((void)DateUtils::isDateEqual("bad", "bad"), ValidationException);
}

} // namespace
