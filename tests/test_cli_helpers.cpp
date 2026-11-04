#include "cli/CurrencyFormatter.hpp"
#include "cli/InputHandler.hpp"
#include "cli/TableFormatter.hpp"

#include <gtest/gtest.h>
#include <sstream>
#include <string>

namespace {

using namespace lms::cli;

// ============================================================================
// CurrencyFormatter Tests
// ============================================================================

TEST(CurrencyFormatterTest, FormatsZeroAndSmallAmounts) {
    EXPECT_EQ(CurrencyFormatter::formatPaiseToRupees(0), "₹0.00");
    EXPECT_EQ(CurrencyFormatter::formatPaiseToRupees(5), "₹0.05");
    EXPECT_EQ(CurrencyFormatter::formatPaiseToRupees(50), "₹0.50");
    EXPECT_EQ(CurrencyFormatter::formatPaiseToRupees(99), "₹0.99");
}

TEST(CurrencyFormatterTest, FormatsStandardAmounts) {
    EXPECT_EQ(CurrencyFormatter::formatPaiseToRupees(100), "₹1.00");
    EXPECT_EQ(CurrencyFormatter::formatPaiseToRupees(1250), "₹12.50");
    EXPECT_EQ(CurrencyFormatter::formatPaiseToRupees(5000), "₹50.00");
    EXPECT_EQ(CurrencyFormatter::formatPaiseToRupees(100000), "₹1000.00");
}

TEST(CurrencyFormatterTest, FormatsNegativeAmounts) {
    EXPECT_EQ(CurrencyFormatter::formatPaiseToRupees(-5), "-₹0.05");
    EXPECT_EQ(CurrencyFormatter::formatPaiseToRupees(-50), "-₹0.50");
    EXPECT_EQ(CurrencyFormatter::formatPaiseToRupees(-100), "-₹1.00");
    EXPECT_EQ(CurrencyFormatter::formatPaiseToRupees(-1250), "-₹12.50");
}

// ============================================================================
// TableFormatter Tests
// ============================================================================

TEST(TableFormatterTest, FormatsEmptyTableWithHeaders) {
    TableFormatter table({
        {"ID", Alignment::Right, 5},
        {"Title", Alignment::Left, 10}
    });

    const std::string output = table.render();
    EXPECT_FALSE(output.empty());
    EXPECT_NE(output.find("ID"), std::string::npos);
    EXPECT_NE(output.find("Title"), std::string::npos);
}

TEST(TableFormatterTest, FormatsRowsWithAlignments) {
    TableFormatter table({
        {"ID", Alignment::Right, 4},
        {"Name", Alignment::Left, 10},
        {"Status", Alignment::Left, 8}
    });

    table.addRow({"1", "Alice", "Active"});
    table.addRow({"2", "Bob", "Suspended"});

    const std::string output = table.render();
    EXPECT_NE(output.find("Alice"), std::string::npos);
    EXPECT_NE(output.find("Bob"), std::string::npos);
    EXPECT_NE(output.find("Suspended"), std::string::npos);
}

// ============================================================================
// InputHandler Tests
// ============================================================================

TEST(InputHandlerTest, ReadsNonEmptyStringWithTrim) {
    std::istringstream in("  \n   Valid Name  \n");
    std::ostringstream out;

    const std::string result = InputHandler::readString(in, out, "Prompt: ");
    EXPECT_EQ(result, "Valid Name");
}

TEST(InputHandlerTest, ReadsAllowEmptyString) {
    std::istringstream in("\nSecond\n");
    std::ostringstream out;

    const std::string emptyRes = InputHandler::readString(in, out, "Prompt: ", true);
    EXPECT_TRUE(emptyRes.empty());

    const std::string filledRes = InputHandler::readString(in, out, "Prompt: ", true);
    EXPECT_EQ(filledRes, "Second");
}

TEST(InputHandlerTest, ReadsIntegerWithRetries) {
    std::istringstream in("abc\n   \n42\n");
    std::ostringstream out;

    const auto value = InputHandler::readInt(in, out, "Enter number: ");
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(*value, 42);
}

TEST(InputHandlerTest, ReadsIntegerInRange) {
    std::istringstream in("0\n15\n7\n");
    std::ostringstream out;

    const auto value = InputHandler::readInt(in, out, "Choose (1-10): ", 1, 10);
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(*value, 7);
}

TEST(InputHandlerTest, ReadsIntegerAllowBlank) {
    std::istringstream in("\n");
    std::ostringstream out;

    const auto value = InputHandler::readInt(in, out, "Optional number: ", std::nullopt, std::nullopt, true);
    EXPECT_FALSE(value.has_value());
}

TEST(InputHandlerTest, ReadsDateWithRetries) {
    std::istringstream in("2026/01/01\n2026-02-30\n2026-02-28\n");
    std::ostringstream out;

    const std::string date = InputHandler::readDate(in, out, "Enter date: ");
    EXPECT_EQ(date, "2026-02-28");
}

TEST(InputHandlerTest, ReadsDateWithDefault) {
    std::istringstream in("\n");
    std::ostringstream out;

    const std::string date = InputHandler::readDate(in, out, "Enter date: ", "2026-03-15");
    EXPECT_EQ(date, "2026-03-15");
}

TEST(InputHandlerTest, ConfirmationHandling) {
    std::istringstream in("maybe\ny\nN\n\n");
    std::ostringstream out;

    EXPECT_TRUE(InputHandler::readConfirmation(in, out, "Confirm?"));
    EXPECT_FALSE(InputHandler::readConfirmation(in, out, "Confirm again?"));
    EXPECT_TRUE(InputHandler::readConfirmation(in, out, "Confirm default true?", true));
}

TEST(InputHandlerTest, HandlesEofGracefully) {
    std::istringstream in("");
    std::ostringstream out;

    EXPECT_THROW((void)InputHandler::readString(in, out, "Prompt: "), EofException);
}

} // namespace
