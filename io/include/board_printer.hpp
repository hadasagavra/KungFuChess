#pragma once

#include <ostream>

#include "engine/include/game_engine.hpp"

namespace kfc::io {

void printBoard(const engine::GameSnapshot& snapshot, std::ostream& out);

}