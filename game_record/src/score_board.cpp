#include "game_record/include/score_board.hpp"

namespace kfc::game_record {

using model::CapturedPiece;
using model::Color;

void ScoreBoard::onCapture(const CapturedPiece& captured) {
    // The victim's owner lost the piece, so the other player is the one who
    // gained its worth.
    totalFor(model::opponentOf(captured.color)) += model::costOf(captured.kind);
}

int ScoreBoard::scoreFor(Color player) const {
    return player == Color::White ? white_ : black_;
}

int& ScoreBoard::totalFor(Color player) {
    return player == Color::White ? white_ : black_;
}

}  // namespace kfc::game_record
