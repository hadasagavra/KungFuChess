#include "shared/logic/io/include/board_printer.hpp"

#include "shared/logic/io/include/piece_codec.hpp"
#include "shared/logic/model/include/position.hpp"

namespace kfc::io {

void printBoard(const engine::GameSnapshot& snapshot, std::ostream& out) {
    for (int row = 0; row < snapshot.height(); ++row) {
        for (int col = 0; col < snapshot.width(); ++col) {
            if (col > 0) out << ' ';
            out << encodeCell(snapshot.pieceAt(model::Position{row, col}));
        }
        out << '\n';
    }
}

}  // namespace kfc::io
