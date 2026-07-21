#pragma once

#include <ostream>

#include "shared/logic/engine/include/game_engine.hpp"
#include "client/input/include/controller.hpp"
#include "texttests/include/script_parser.hpp"

namespace kfc::texttests {

class ScriptRunner {
public:
    ScriptRunner(input::Controller& controller, engine::GameEngine& engine,
                 std::ostream& out);

    void run(const Command& command);

private:
    input::Controller& controller_;
    engine::GameEngine& engine_;
    std::ostream& out_;
};

}