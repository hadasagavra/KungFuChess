#pragma once

#include "engine/include/game_engine.hpp"
#include "view/include/game_snapshot.hpp"

namespace kfc::view {

// The GUI<-Logic output seam: projects the engine's live state onto a read-only
// display DTO. It is the graphical twin of io::printBoard (engine state -> text):
// here engine state -> pixels. It walks every cell and places each piece at
// pixel (col * cellPx, row * cellPx). It holds no game rules, mutates nothing,
// and never touches OpenCV -- so the Renderer keeps receiving only the pure
// view::GameSnapshot it is allowed to know, and this seam stays unit-testable.
//
// Header-only: it depends solely on two plain snapshot types, so both the app
// and the unit tests can use it without linking the OpenCV-backed view library.
inline GameSnapshot buildSnapshot(const engine::GameSnapshot& state, int cellPx) {
    GameSnapshot snapshot;
    snapshot.boardWidth = state.width();
    snapshot.boardHeight = state.height();

    for (int row = 0; row < state.height(); ++row) {
        for (int col = 0; col < state.width(); ++col) {
            const std::optional<model::Piece> piece =
                state.pieceAt(model::Position{row, col});
            if (!piece) {
                continue;
            }
            snapshot.pieces.push_back(
                PieceView{piece->getKind(), piece->getColor(), piece->getState(),
                          PixelPoint{col * cellPx, row * cellPx}, piece->getId()});
        }
    }
    return snapshot;
}

}  // namespace kfc::view
