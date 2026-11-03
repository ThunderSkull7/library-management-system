#include "models/Loan.hpp"
#include "models/LoanPayment.hpp"

#include <gtest/gtest.h>
#include <optional>
#include <string>

namespace {

TEST(LoanModelTest, DefaultConstructor) {
    lms::models::Loan loan;
    EXPECT_EQ(loan.getId(), 0);
    EXPECT_EQ(loan.getCopyId(), 0);
    EXPECT_EQ(loan.getMemberId(), 0);
    EXPECT_TRUE(loan.getIssuedOn().empty());
    EXPECT_TRUE(loan.getDueOn().empty());
    EXPECT_FALSE(loan.getReturnedOn().has_value());
    EXPECT_EQ(loan.getFineMinorUnits(), 0);
    EXPECT_FALSE(loan.isFinePaid());
    EXPECT_TRUE(loan.isActive());
    EXPECT_FALSE(loan.isReturned());
}

TEST(LoanModelTest, ParameterizedConstructorWithoutId) {
    lms::models::Loan loan(10, 20, "2026-03-01", "2026-03-15");
    EXPECT_EQ(loan.getId(), 0);
    EXPECT_EQ(loan.getCopyId(), 10);
    EXPECT_EQ(loan.getMemberId(), 20);
    EXPECT_EQ(loan.getIssuedOn(), "2026-03-01");
    EXPECT_EQ(loan.getDueOn(), "2026-03-15");
    EXPECT_FALSE(loan.getReturnedOn().has_value());
    EXPECT_EQ(loan.getFineMinorUnits(), 0);
    EXPECT_FALSE(loan.isFinePaid());
    EXPECT_TRUE(loan.isActive());
    EXPECT_FALSE(loan.isReturned());
}

TEST(LoanModelTest, ParameterizedConstructorWithIdAndReturnDate) {
    lms::models::Loan loan(1, 10, 20, "2026-03-01", "2026-03-15", "2026-03-18", 600, true);
    EXPECT_EQ(loan.getId(), 1);
    EXPECT_EQ(loan.getCopyId(), 10);
    EXPECT_EQ(loan.getMemberId(), 20);
    EXPECT_EQ(loan.getIssuedOn(), "2026-03-01");
    EXPECT_EQ(loan.getDueOn(), "2026-03-15");
    ASSERT_TRUE(loan.getReturnedOn().has_value());
    EXPECT_EQ(*loan.getReturnedOn(), "2026-03-18");
    EXPECT_EQ(loan.getFineMinorUnits(), 600); // 600 paise = ₹6.00
    EXPECT_TRUE(loan.isFinePaid());
    EXPECT_FALSE(loan.isActive());
    EXPECT_TRUE(loan.isReturned());
}

TEST(LoanModelTest, SettersAndEquality) {
    lms::models::Loan l1(1, 10, 20, "2026-03-01", "2026-03-15");
    lms::models::Loan l2(1, 10, 20, "2026-03-01", "2026-03-15");
    EXPECT_EQ(l1, l2);

    l1.setReturnedOn("2026-03-16");
    l1.setFineMinorUnits(200);
    l1.setFinePaid(true);

    EXPECT_NE(l1, l2);
    ASSERT_TRUE(l1.getReturnedOn().has_value());
    EXPECT_EQ(*l1.getReturnedOn(), "2026-03-16");
    EXPECT_EQ(l1.getFineMinorUnits(), 200);
    EXPECT_TRUE(l1.isFinePaid());
}

TEST(LoanPaymentModelTest, ConstructorsAndAccessors) {
    lms::models::LoanPayment p1(5, 500, "2026-03-20");
    EXPECT_EQ(p1.getId(), 0);
    EXPECT_EQ(p1.getLoanId(), 5);
    EXPECT_EQ(p1.getAmountMinorUnits(), 500);
    EXPECT_EQ(p1.getPaidOn(), "2026-03-20");

    lms::models::LoanPayment p2(1, 5, 500, "2026-03-20");
    EXPECT_EQ(p2.getId(), 1);
    EXPECT_EQ(p2.getLoanId(), 5);
    EXPECT_EQ(p2.getAmountMinorUnits(), 500);
    EXPECT_EQ(p2.getPaidOn(), "2026-03-20");
}

} // namespace
