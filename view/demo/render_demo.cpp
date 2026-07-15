#include <iostream>
#include <string>

#include <opencv2/opencv.hpp>

#include "img.hpp"
#include "input/include/board_mapper.hpp"
#include "model/include/piece.hpp"
#include "view/include/game_snapshot.hpp"
#include "view/include/image_view.hpp"
#include "view/include/render_config.hpp"
#include "view/include/renderer.hpp"

using namespace kfc;

namespace {

// Standard chess back-rank order, files a..h.
const model::Kind backRank[8] = {
    model::Kind::Rook,   model::Kind::Knight, model::Kind::Bishop,
    model::Kind::Queen,  model::Kind::King,   model::Kind::Bishop,
    model::Kind::Knight, model::Kind::Rook};

// Build a standard 8x8 starting-position snapshot. This plays the trivial role
// of the seam: it turns each cell (row, col) into a pixel position using cellPx.
// Black occupies the top rows (0, 1); White the bottom rows (6, 7).
view::GameSnapshot startingPosition(int cellPx) {
    view::GameSnapshot snap;
    snap.boardWidth = 8;
    snap.boardHeight = 8;

    auto addPiece = [&](model::Kind kind, model::Color color, int row, int col) {
        snap.pieces.push_back(
            view::PieceView{kind, color, model::State::Idle,
                            view::PixelPoint{col * cellPx, row * cellPx}});
    };

    for (int col = 0; col < 8; ++col) {
        addPiece(backRank[col], model::Color::Black, 0, col);
        addPiece(model::Kind::Pawn, model::Color::Black, 1, col);
        addPiece(model::Kind::Pawn, model::Color::White, 6, col);
        addPiece(backRank[col], model::Color::White, 7, col);
    }
    return snap;
}

}  // namespace

int main(int argc, char** argv) {
    const std::string assetsRoot = (argc > 1) ? argv[1] : "assets";
    const bool showWindow = (argc > 2 && std::string(argv[2]) == "--show");

    view::RenderConfig config{assetsRoot, view::defaultCellPx};
    view::GameSnapshot snapshot = startingPosition(config.cellPx);

    if (showWindow) {
        // On-screen path: a persistent, non-blocking window that reports clicks.
        // The view speaks pixels; BoardMapper (input layer) is the seam that
        // turns a click pixel into a board cell. This proves the chain
        // window -> pixel -> cell that will later feed engine move commands.
        view::ImageView view{config};
        input::BoardMapper mapper{snapshot.boardWidth, snapshot.boardHeight};

        view.open();
        while (view.isOpen()) {
            view.render(snapshot);
            if (auto px = view.pollClick()) {
                if (auto cell = mapper.toCell(px->x, px->y)) {
                    std::cout << "cell: row=" << cell->row
                              << " col=" << cell->col << "\n";
                } else {
                    std::cout << "click outside board\n";
                }
            }
        }
        return 0;
    }

    // Dev verification path: dump the composed frame to a PNG for eyeballing.
    // This is a test harness, not on-screen game graphics -- it uses the
    // provided Img::get_mat() accessor with cv::imwrite. The on-screen path
    // above stays pure Img::show().
    view::Renderer renderer{config};
    Img frame = renderer.renderFrame(snapshot);
    const std::string out = assetsRoot + "/_render_demo.png";
    cv::imwrite(out, frame.get_mat());
    std::cout << "wrote " << out << "\n";
    return 0;
}
