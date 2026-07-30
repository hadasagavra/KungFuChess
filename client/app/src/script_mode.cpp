#include "client/app/include/script_mode.hpp"

#include <optional>
#include <string>

#include "client/input/include/board_mapper.hpp"
#include "client/input/include/controller.hpp"
#include "client/input/include/local_game_access.hpp"
#include "client/view/include/render_config.hpp"
#include "shared/logic/engine/include/game_engine.hpp"
#include "shared/logic/io/include/board_parser.hpp"
#include "texttests/include/script_parser.hpp"
#include "texttests/include/script_runner.hpp"

namespace kfc::app {

int runScript(std::istream& in, std::ostream& out) {
    kfc::io::ParsedInput parsed = kfc::io::parseInput(in);

    kfc::engine::GameEngine engine{parsed.board};
    kfc::input::LocalGameAccess access{engine};
    // The text harness draws nothing, so the board sits at the frame origin with
    // no panels beside it.
    kfc::input::BoardMapper mapper{parsed.board.width(), parsed.board.height(),
                                   kfc::view::defaultCellPx, 0, 0};
    kfc::input::Controller controller{access, mapper};
    kfc::texttests::ScriptRunner runner{controller, engine, out};

    for (const std::string& line : parsed.commands) {
        if (std::optional<kfc::texttests::Command> command =
                kfc::texttests::parseCommand(line)) {
            runner.run(*command);
        }
    }
    return 0;
}

}  // namespace kfc::app
