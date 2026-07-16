#include <algorithm>
#include <chrono>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>

#include "engine/include/game_engine.hpp"
#include "input/include/board_mapper.hpp"
#include "input/include/controller.hpp"
#include "io/include/board_parser.hpp"
#include "texttests/include/script_parser.hpp"
#include "texttests/include/script_runner.hpp"
#include "view/include/image_view.hpp"
#include "view/include/render_config.hpp"
#include "view/include/scene_translator.hpp"

//https://github.com/hadasagavra/KungFuChess

namespace {

// Standard chess starting position in the BoardParser text format (color letter
// + kind letter; '.' is an empty cell). This is composition-root configuration,
// not Business Logic -- the parser turns it into a Board and assigns piece ids.
const char* const startingBoardText =
    "Board:\n"
    "bR bN bB bQ bK bB bN bR\n"
    "bP bP bP bP bP bP bP bP\n"
    ". . . . . . . .\n"
    ". . . . . . . .\n"
    ". . . . . . . .\n"
    ". . . . . . . .\n"
    "wP wP wP wP wP wP wP wP\n"
    "wR wN wB wQ wK wB wN wR\n"
    "Commands:\n";

// Largest real-time step fed to the engine in one frame. Clamping keeps a
// paused/janky frame (or the first frame) from jumping the clock forward.
constexpr int maxStepMs = 100;

int elapsedMsSince(std::chrono::steady_clock::time_point last,
                   std::chrono::steady_clock::time_point now) {
    const auto ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(now - last).count();
    return std::clamp(static_cast<int>(ms), 0, maxStepMs);
}

// Text harness: read a board + a script from `in`, replay the commands through
// the public command path, and print board dumps to `out`. Unchanged behavior
// from the original entry point -- kept behind the --script flag.
int runScript(std::istream& in, std::ostream& out) {
    kfc::io::ParsedInput parsed = kfc::io::parseInput(in);

    kfc::engine::GameEngine engine{parsed.board};
    kfc::input::BoardMapper mapper{parsed.board.width(), parsed.board.height()};
    kfc::input::Controller controller{engine, mapper};
    kfc::texttests::ScriptRunner runner{controller, engine, out};

    for (const std::string& line : parsed.commands) {
        if (std::optional<kfc::texttests::Command> command =
                kfc::texttests::parseCommand(line)) {
            runner.run(*command);
        }
    }
    return 0;
}

// Graphical play: a persistent window renders the engine's live state every
// frame, real time advances by wall-clock, and each click is forwarded to the
// Controller (source -> destination moves). The window speaks pixels; the
// Controller owns the pixel->cell mapping, so no board logic lives here.
int runGraphical(const std::string& assetsRoot, int cellPx) {
    std::istringstream boardText{startingBoardText};
    kfc::io::ParsedInput parsed = kfc::io::parseInput(boardText);

    kfc::engine::GameEngine engine{parsed.board};
    kfc::input::BoardMapper mapper{parsed.board.width(), parsed.board.height()};
    kfc::input::Controller controller{engine, mapper};

    kfc::view::ImageView view{kfc::view::RenderConfig{assetsRoot, cellPx}};
    view.open();

    auto last = std::chrono::steady_clock::now();
    while (view.isOpen()) {
        const auto now = std::chrono::steady_clock::now();
        const int deltaMs = elapsedMsSince(last, now);
        engine.wait(deltaMs);
        last = now;

        view.render(kfc::view::buildSnapshot(engine.getSnapshot(), cellPx),
                    deltaMs);

        if (std::optional<kfc::view::PixelPoint> click = view.pollClick()) {
            controller.handleClick(click->x, click->y);
        }
    }
    return 0;
}

}  // namespace

int main(int argc, char** argv) {
    try {
        if (argc > 1 && std::string(argv[1]) == "--script") {
            return runScript(std::cin, std::cout);
        }
        const std::string assetsRoot = (argc > 1) ? argv[1] : "assets";
        return runGraphical(assetsRoot, kfc::view::defaultCellPx);
    } catch (const kfc::io::ParseError& e) {
        std::cout << "ERROR " << e.code << "\n";
        return 1;
    }
}
