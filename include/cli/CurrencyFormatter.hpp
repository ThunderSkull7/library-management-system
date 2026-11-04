#pragma once

#include <cstdio>
#include <string>

namespace lms::cli {

/**
 * @brief Utilities for formatting integer paise minor units into Indian Rupee currency strings.
 */
class CurrencyFormatter {
public:
    /**
     * @brief Formats an integer amount of paise into a human-readable rupee string (e.g. 200 -> "₹2.00", 50 -> "₹0.50").
     * Guaranteed never to use floating point arithmetic to prevent precision issues.
     */
    static std::string formatPaiseToRupees(int paise) {
        bool negative = false;
        long long absPaise = paise;
        if (absPaise < 0) {
            negative = true;
            absPaise = -absPaise;
        }

        long long rupees = absPaise / 100;
        long long remainingPaise = absPaise % 100;

        char buf[64];
        std::snprintf(buf, sizeof(buf), "%s₹%lld.%02lld", negative ? "-" : "", rupees, remainingPaise);
        return std::string(buf);
    }
};

} // namespace lms::cli

