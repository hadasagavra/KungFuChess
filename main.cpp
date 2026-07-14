#include <iostream>
#include <optional>
#include <string>
#include "engine/include/game_engine.hpp"
#include "input/include/board_mapper.hpp"
#include "input/include/controller.hpp"
#include "io/include/board_parser.hpp"
#include "texttests/include/script_parser.hpp"
#include "texttests/include/script_runner.hpp"

//https://github.com/hadasagavra/KungFuChess

int main() {
    try {
        kfc::io::ParsedInput parsed = kfc::io::parseInput(std::cin);

        kfc::engine::GameEngine engine{parsed.board};
        kfc::input::BoardMapper mapper{parsed.board.width(), parsed.board.height()};
        kfc::input::Controller controller{engine, mapper};
        kfc::texttests::ScriptRunner runner{controller, engine, std::cout};

        for (const std::string& line : parsed.commands) {
            if (std::optional<kfc::texttests::Command> command =
                    kfc::texttests::parseCommand(line)) {
                runner.run(*command);
            }
        }
    } catch (const kfc::io::ParseError& e) {
        std::cout << "ERROR " << e.code << "\n";
        return 1;
    }

    return 0;
}
