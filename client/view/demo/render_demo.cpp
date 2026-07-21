#include <iostream>
#include <string>

#include <opencv2/opencv.hpp>

#include "img.hpp"
#include "client/input/include/board_mapper.hpp"
#include "shared/logic/model/include/piece.hpp"
#include "client/view/include/game_snapshot.hpp"
#include "client/view/include/image_view.hpp"
#include "client/view/include/render_config.hpp"
#include "client/view/include/render_layout.hpp"
#include "client/view/include/renderer.hpp"

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
view::GameSnapshot startingPosition(int cellPx, view::PixelPoint boardOrigin) {
    view::GameSnapshot snap;
    snap.boardWidth = 8;
    snap.boardHeight = 8;
    snap.boardOrigin = boardOrigin;

    // Sample panel contents. This harness exists to eyeball the view layer, so
    // it supplies its own display strings rather than running a game -- the same
    // trivial stand-in role it already plays for the seam.
    snap.whitePanel.name = "White";
    snap.whitePanel.score = 4;
    snap.whitePanel.moves = {{"00:02.314", "e4"},
                             {"00:06.927", "Nf3"},
                             {"00:13.401", "Bb5"},
                             {"00:23.589", "Re1"},
                             {"00:40.102", "Bxc6"},
                             {"01:08.577", "dxe5"}};
    snap.blackPanel.name = "Black";
    snap.blackPanel.score = 7;
    snap.blackPanel.moves = {{"00:04.105", "e5"},
                             {"00:09.642", "Nc6"},
                             {"00:15.998", "a6"},
                             {"00:20.781", "Nf6"},
                             {"01:04.756", "exd4"},
                             {"01:18.662", "JNe4"}};

    auto addPiece = [&](model::Kind kind, model::Color color, int row, int col) {
        snap.pieces.push_back(view::PieceView{
            kind, color, model::State::Idle,
            view::PixelPoint{boardOrigin.x + col * cellPx,
                             boardOrigin.y + row * cellPx}});
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
    const std::string assetsRoot = (argc > 1) ? argv[1] : "client/assets";
    const bool showWindow = (argc > 2 && std::string(argv[2]) == "--show");

    const view::RenderConfig config =
        view::defaultRenderConfig(assetsRoot, view::defaultCellPx);
    const view::FrameLayout layout = view::computeLayout(config, 8, 8);
    view::GameSnapshot snapshot =
        startingPosition(config.cellPx, layout.boardOrigin);

    if (showWindow) {
        // On-screen path: a persistent, non-blocking window that reports clicks.
        // The view speaks pixels; BoardMapper (input layer) is the seam that
        // turns a click pixel into a board cell. This proves the chain
        // window -> pixel -> cell that will later feed engine move commands.
        view::ImageView view{config};
        input::BoardMapper mapper{snapshot.boardWidth, snapshot.boardHeight,
                                  config.cellPx, layout.boardOrigin.x,
                                  layout.boardOrigin.y};

        view.open();
        while (view.isOpen()) {
            // Static demo: a zero delta holds every piece on its first frame.
            view.render(snapshot, 0);
            for (const view::MouseAction& action : view.takeMouseActions()) {
                const char* const kind =
                    action.type == view::MouseAction::Type::DoubleClick
                        ? "double-click"
                        : "click";
                if (auto cell = mapper.toCell(action.position.x,
                                              action.position.y)) {
                    std::cout << kind << ": row=" << cell->row
                              << " col=" << cell->col << "\n";
                } else {
                    std::cout << kind << " outside board\n";
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
