#pragma once

#include <ostream>

#include "engine/include/game_engine.hpp"
#include "input/include/controller.hpp"
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