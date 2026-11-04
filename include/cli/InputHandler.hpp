#pragma once

#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>

namespace lms::cli {

/**
 * @brief Exception thrown when standard input reaches end-of-file (e.g. Ctrl+Z / Ctrl+D).
 */
class EofException : public std::runtime_error {
public:
    EofException() : std::runtime_error("End of input stream reached.") {}
};

/**
 * @brief Reusable helper for resilient, user-friendly interactive CLI input reading and parsing.
 */
class InputHandler {
public:
    /**
     * @brief Reads a line of text, automatically trimming leading and trailing whitespace.
     * Re-prompts if empty and allowEmpty is false.
     * @throws EofException on EOF.
     */
    static std::string readString(std::istream& in,
                                  std::ostream& out,
                                  const std::string& prompt,
                                  bool allowEmpty = false);

    /**
     * @brief Reads an integer within [minVal, maxVal]. Re-prompts on malformed or out-of-range input.
     * If allowBlank is true and user presses Enter, returns std::nullopt.
     * @throws EofException on EOF.
     */
    static std::optional<int> readInt(std::istream& in,
                                      std::ostream& out,
                                      const std::string& prompt,
                                      std::optional<int> minVal = std::nullopt,
                                      std::optional<int> maxVal = std::nullopt,
                                      bool allowBlank = false);

    /**
     * @brief Reads an ISO date (YYYY-MM-DD). If defaultDate is non-empty and user presses enter,
     * defaultDate is returned. Re-prompts on invalid date.
     * @throws EofException on EOF.
     */
    static std::string readDate(std::istream& in,
                                std::ostream& out,
                                const std::string& prompt,
                                const std::string& defaultDate = "");

    /**
     * @brief Prompts for confirmation (y/n).
     * @throws EofException on EOF.
     */
    static bool readConfirmation(std::istream& in,
                                 std::ostream& out,
                                 const std::string& prompt,
                                 bool defaultYes = false);

    /**
     * @brief Prompts the user to press Enter to continue.
     */
    static void waitForEnter(std::istream& in,
                             std::ostream& out,
                             const std::string& prompt = "Press [Enter] to continue...");
};

} // namespace lms::cli

