#include "cli/TableFormatter.hpp"

#include <algorithm>
#include <sstream>

namespace lms::cli {

TableFormatter::TableFormatter(std::vector<Column> columns)
    : columns_(std::move(columns)) {}

void TableFormatter::addRow(std::vector<std::string> row) {
    if (row.size() < columns_.size()) {
        row.resize(columns_.size(), "");
    }
    rows_.push_back(std::move(row));
}

void TableFormatter::clearRows() {
    rows_.clear();
}

std::vector<size_t> TableFormatter::computeWidths() const {
    std::vector<size_t> widths(columns_.size(), 0);

    for (size_t i = 0; i < columns_.size(); ++i) {
        widths[i] = std::max(columns_[i].header.length(), columns_[i].minWidth);
    }

    for (const auto& row : rows_) {
        for (size_t i = 0; i < columns_.size() && i < row.size(); ++i) {
            widths[i] = std::max(widths[i], row[i].length());
        }
    }

    return widths;
}

std::string TableFormatter::formatSeparator(const std::vector<size_t>& widths) const {
    std::ostringstream oss;
    oss << '+';
    for (size_t w : widths) {
        oss << std::string(w + 2, '-') << '+';
    }
    oss << '\n';
    return oss.str();
}

std::string TableFormatter::formatRow(const std::vector<std::string>& row, const std::vector<size_t>& widths) const {
    std::ostringstream oss;
    oss << '|';
    for (size_t i = 0; i < columns_.size(); ++i) {
        std::string val = (i < row.size()) ? row[i] : "";
        size_t pad = widths[i] >= val.length() ? widths[i] - val.length() : 0;

        oss << ' ';
        if (columns_[i].alignment == Alignment::Right) {
            oss << std::string(pad, ' ') << val;
        } else {
            oss << val << std::string(pad, ' ');
        }
        oss << " |";
    }
    oss << '\n';
    return oss.str();
}

std::string TableFormatter::render() const {
    if (columns_.empty()) {
        return "";
    }

    auto widths = computeWidths();
    std::string sep = formatSeparator(widths);

    std::ostringstream oss;
    oss << sep;

    // Header row
    std::vector<std::string> headers;
    headers.reserve(columns_.size());
    for (const auto& col : columns_) {
        headers.push_back(col.header);
    }
    oss << formatRow(headers, widths);
    oss << sep;

    if (rows_.empty()) {
        size_t totalWidth = 1;
        for (size_t w : widths) {
            totalWidth += w + 3;
        }
        std::string msg = "(No records found)";
        if (totalWidth > msg.length() + 2) {
            size_t pad = (totalWidth - 2 - msg.length()) / 2;
            oss << '|' << std::string(pad, ' ') << msg << std::string(totalWidth - 2 - msg.length() - pad, ' ') << "|\n";
        } else {
            oss << "| " << msg << " |\n";
        }
        oss << sep;
    } else {
        for (const auto& row : rows_) {
            oss << formatRow(row, widths);
        }
        oss << sep;
    }

    return oss.str();
}

void TableFormatter::print(std::ostream& os) const {
    os << render();
}

} // namespace lms::cli

