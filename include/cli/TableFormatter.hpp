#pragma once

#include <iostream>
#include <string>
#include <vector>

namespace lms::cli {

enum class Alignment {
    Left,
    Right
};

struct Column {
    std::string header;
    Alignment alignment{Alignment::Left};
    size_t minWidth{0};
};

/**
 * @brief Utility for rendering clean, aligned ASCII tables for terminal output.
 */
class TableFormatter {
public:
    explicit TableFormatter(std::vector<Column> columns);

    void addRow(std::vector<std::string> row);
    void clearRows();

    [[nodiscard]] std::string render() const;
    void print(std::ostream& os = std::cout) const;

    [[nodiscard]] size_t rowCount() const noexcept { return rows_.size(); }

private:
    std::vector<Column> columns_;
    std::vector<std::vector<std::string>> rows_;

    [[nodiscard]] std::vector<size_t> computeWidths() const;
    [[nodiscard]] std::string formatSeparator(const std::vector<size_t>& widths) const;
    [[nodiscard]] std::string formatRow(const std::vector<std::string>& row, const std::vector<size_t>& widths) const;
};

} // namespace lms::cli

