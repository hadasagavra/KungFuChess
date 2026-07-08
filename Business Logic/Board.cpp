#include "Board.h"

#include <utility>

namespace kfc::logic {

Board::Board(std::vector<Row> rows) : rows_(std::move(rows)) {}

int Board::height() const {
    return static_cast<int>(rows_.size());
}

int Board::width() const {
    return rows_.empty() ? 0 : static_cast<int>(rows_.front().size());
}

void Board::print(std::ostream& out) const {
    for (const auto& row : rows_) {
        for (size_t col = 0; col < row.size(); ++col) {
            if (col > 0) out << ' ';
            out << row[col];
        }
        out << '\n';
    }
}

}  // namespace kfc::logic