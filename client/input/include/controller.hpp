#pragma once

#include <optional>

#include "client/input/include/board_mapper.hpp"
#include "client/input/include/game_access.hpp"
#include "shared/logic/model/include/position.hpp"

namespace kfc::input {

// Translates raw user clicks into game commands. It decides nothing about chess
// legality: it maps pixels to cells via BoardMapper, tracks the selected cell,
// and forwards completed source->destination moves through a GameAccess. It
// speaks to that interface, not to a concrete engine, so the same Controller
// drives a local engine or a remote server without change.
class Controller {
public:
    Controller(GameAccess& game, BoardMapper mapper);

    void handleClick(int x, int y);
    void handleJump(int x, int y);
    const std::optional<model::Position>& selection() const;

private:
    void handleFirstClick(std::optional<model::Position> cell);
    void handleSecondClick(std::optional<model::Position> cell);


    GameAccess& game_;
    BoardMapper mapper_;
    std::optional<model::Position> selected_;
};

}  // namespace kfc::input
