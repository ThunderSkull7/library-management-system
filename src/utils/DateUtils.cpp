#include "utils/DateUtils.hpp"
#include "utils/ValidationException.hpp"

#include <chrono>
#include <cstdio>

namespace lms::utils {

namespace {

bool parseIsoDate(const std::string& dateStr, std::chrono::year_month_day& ymd) {
    if (dateStr.length() != 10 || dateStr[4] != '-' || dateStr[7] != '-') {
        return false;
    }

    for (size_t i = 0; i < 10; ++i) {
        if (i == 4 || i == 7) {
            continue;
        }
        if (dateStr[i] < '0' || dateStr[i] > '9') {
            return false;
        }
    }

    int y = (dateStr[0] - '0') * 1000 + (dateStr[1] - '0') * 100 + (dateStr[2] - '0') * 10 + (dateStr[3] - '0');
    int m = (dateStr[5] - '0') * 10 + (dateStr[6] - '0');
    int d = (dateStr[8] - '0') * 10 + (dateStr[9] - '0');

    if (y < 1000 || y > 9999 || m < 1 || m > 12) {
        return false;
    }

    ymd = std::chrono::year_month_day{
        std::chrono::year{y},
        std::chrono::month{static_cast<unsigned>(m)},
        std::chrono::day{static_cast<unsigned>(d)}
    };

    return ymd.ok();
}

std::chrono::year_month_day getValidYmd(const std::string& dateStr) {
    std::chrono::year_month_day ymd;
    if (!parseIsoDate(dateStr, ymd)) {
        throw ValidationException("Invalid ISO date: '" + dateStr + "'. Expected format: YYYY-MM-DD");
    }
    return ymd;
}

std::string formatYmd(const std::chrono::year_month_day& ymd) {
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%04d-%02u-%02u",
                  static_cast<int>(ymd.year()),
                  static_cast<unsigned>(ymd.month()),
                  static_cast<unsigned>(ymd.day()));
    return std::string(buf);
}

} // namespace

bool DateUtils::isValidIsoDate(const std::string& dateStr) {
    std::chrono::year_month_day ymd;
    return parseIsoDate(dateStr, ymd);
}

std::string DateUtils::addDays(const std::string& dateStr, int days) {
    auto ymd = getValidYmd(dateStr);
    std::chrono::sys_days sd{ymd};
    sd += std::chrono::days{days};
    std::chrono::year_month_day result{sd};
    return formatYmd(result);
}

int DateUtils::daysBetween(const std::string& fromDateStr, const std::string& toDateStr) {
    auto ymdFrom = getValidYmd(fromDateStr);
    auto ymdTo = getValidYmd(toDateStr);

    std::chrono::sys_days sdFrom{ymdFrom};
    std::chrono::sys_days sdTo{ymdTo};

    return static_cast<int>((sdTo - sdFrom).count());
}

bool DateUtils::isDateBefore(const std::string& d1, const std::string& d2) {
    return daysBetween(d1, d2) > 0;
}

bool DateUtils::isDateAfter(const std::string& d1, const std::string& d2) {
    return daysBetween(d1, d2) < 0;
}

bool DateUtils::isDateEqual(const std::string& d1, const std::string& d2) {
    auto ymd1 = getValidYmd(d1);
    auto ymd2 = getValidYmd(d2);
    return ymd1 == ymd2;
}

} // namespace lms::utils
