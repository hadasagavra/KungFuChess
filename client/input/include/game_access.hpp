#pragma once

#include <optional>

#include "shared/logic/model/include/piece.hpp"
#include "shared/logic/model/include/position.hpp"

namespace kfc::input {

// The whole of the game as the input side sees it: what sits on a cell, and the
// two commands a click can issue. The Controller depends on this and nothing
// more, so it neither knows nor cares whether the game it drives is a local
// engine (the script harness) or a remote server reached over a socket. Requests
// return nothing -- a command is a request, and its outcome comes back as new
// state, not as a return value.
class GameAccess {
public:
    virtual ~GameAccess() = default;

    virtual std::optional<model::Piece> pieceAt(model::Position cell) const = 0;
    virtual void requestMove(model::Position from, model::Position to) = 0;
    virtual void requestJump(model::Position cell) = 0;
};

}  // namespace kfc::input
