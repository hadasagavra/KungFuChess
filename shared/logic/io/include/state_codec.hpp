#pragma once

#include <optional>
#include <string>
#include <vector>

#include "shared/logic/engine/include/game_engine.hpp"
#include "shared/logic/model/include/board.hpp"
#include "shared/logic/realtime/include/motion.hpp"

namespace kfc::io {

// The wire form of a whole frame of game state: the board, the game-over flag,
// and the in-flight motions and cooldowns a display needs to interpolate. The
// server encodes its GameSnapshot each tick; a client decodes it into a replica
// board it owns, then builds its own GameSnapshot from that. A full frame, not a
// diff, so a dropped or garbled message is corrected by the very next one.
struct DecodedState {
    bool isOver;
    std::vector<realtime::MotionState> motions;
    std::vector<realtime::CooldownState> cooldowns;
};

std::string encodeState(const engine::GameSnapshot& snapshot);

// Rebuild a frame into `board` (its contents are replaced) and return the rest.
// nullopt when the text is not a well-formed frame -- the board section reuses
// board_parser, so the same malformed grid that fails a file fails here too.
std::optional<DecodedState> decodeState(const std::string& text,
                                        model::Board& board);

}  // namespace kfc::io
