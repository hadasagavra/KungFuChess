#pragma once

#include <optional>

#include "shared/logic/engine/include/game_engine.hpp"
#include "client/input/include/board_mapper.hpp"
#include "shared/logic/model/include/position.hpp"

namespace kfc::input {

// Translates raw user clicks into game commands. It decides nothing about chess
// legality: it maps pixels to cells via BoardMapper, tracks the selected cell,
// and forwards completed source->destination moves to the GameEngine. It never
// calls Board::movePiece or RuleEngine directly.
class Controller {
public:
    Controller(engine::GameEngine& engine, BoardMapper mapper);

    void handleClick(int x, int y);
    void handleJump(int x, int y);
    const std::optional<model::Position>& selection() const;

private:
    void handleFirstClick(std::optional<model::Position> cell);
    void handleSecondClick(std::optional<model::Position> cell);


    engine::GameEngine& engine_;
    BoardMapper mapper_;
    std::optional<model::Position> selected_;
};

}  // namespace kfc::input
