#pragma once

#include <ostream>
#include <string>
#include <vector>

namespace kfc::logic {

using Row = std::vector<std::string>;

class Board {
public:
    explicit Board(std::vector<Row> rows);

    int height() const;
    int width() const;
    void print(std::ostream& out) const;

private:
    std::vector<Row> rows_;
};

}  // namespace kfc::logic