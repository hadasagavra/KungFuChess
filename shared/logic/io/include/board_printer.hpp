#pragma once

#include <ostream>

#include "shared/logic/engine/include/game_engine.hpp"

namespace kfc::io {

void printBoard(const engine::GameSnapshot& snapshot, std::ostream& out);

}