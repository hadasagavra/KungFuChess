#include <algorithm>
#include <chrono>
#include <cstddef>
#include <iostream>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "engine/include/game_engine.hpp"
#include "input/include/board_mapper.hpp"
#include "input/include/controller.hpp"
#include "game_record/include/move_log.hpp"
#include "game_record/include/score_board.hpp"
#include "io/include/board_parser.hpp"
#include "texttests/include/script_parser.hpp"
#include "texttests/include/script_runner.hpp"
#include "view/include/image_view.hpp"
#include "view/include/render_config.hpp"
#include "view/include/render_layout.hpp"
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

// Who is playing. Names are shown beside each player's moves table; no game rule
// depends on them, so they stay here in the composition root rather than in the
// Business Logic.
const char* const whitePlayerName = "White";
const char* const blackPlayerName = "Black";

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
    // The text harness draws nothing, so the board sits at the frame origin with
    // no panels beside it.
    kfc::input::BoardMapper mapper{parsed.board.width(), parsed.board.height(),
                                   kfc::view::defaultCellPx, 0, 0};
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

    // The moves log and the score are Business Logic that listens: the engine
    // publishes what happened and never learns who is recording it. Subscribing
    // here is the whole cost of a new feature of this kind -- a sound player or
    // an end-of-game animation would join the same bus without the engine, the
    // log, or the score changing at all.
    kfc::game_record::MoveLog moveLog;
    kfc::game_record::ScoreBoard scoreBoard;
    engine.events().subscribe<kfc::model::MoveEvent>(
        [&moveLog](const kfc::model::MoveEvent& event) {
            moveLog.record(event);
            
        });
    engine.events().subscribe<kfc::model::CapturedPiece>(
        [&scoreBoard](const kfc::model::CapturedPiece& captured) {
            scoreBoard.record(captured);
        });

    const kfc::view::RenderConfig config =
        kfc::view::defaultRenderConfig(assetsRoot, cellPx);
    // One layout answer, shared: the renderer draws the board at this origin and
    // the mapper reads clicks against it, so the two cannot disagree.
    const kfc::view::FrameLayout layout = kfc::view::computeLayout(
        config, parsed.board.width(), parsed.board.height());

    kfc::input::BoardMapper mapper{parsed.board.width(), parsed.board.height(),
                                   cellPx, layout.boardOrigin.x,
                                   layout.boardOrigin.y};
    kfc::input::Controller controller{engine, mapper};

    kfc::view::ImageView view{config};
    view.open();

    auto last = std::chrono::steady_clock::now();
    while (view.isOpen()) {
        const auto now = std::chrono::steady_clock::now();
        const int deltaMs = elapsedMsSince(last, now);
        engine.wait(deltaMs);
        last = now;

        // Highlight the selected piece's legal destinations, if any. The
        // Controller owns the selection; the engine (Business Logic) answers
        // where that piece may move. The composition root only plumbs the two
        // together -- no game rules live here.
        std::set<kfc::model::Position> highlights;
        if (const std::optional<kfc::model::Position>& selected =
                controller.selection()) {
            highlights = engine.legalDestinationsFor(*selected);
        }
        const kfc::engine::GameSnapshot state = engine.getSnapshot();
        const kfc::view::SceneInput scene{
            state,      moveLog,          scoreBoard,
            highlights, whitePlayerName,  blackPlayerName};
        view.render(kfc::view::buildSnapshot(scene, config, layout), deltaMs);

        // A double-click requests a jump; a single click drives the ordinary
        // select/move flow. A double-click also delivers the opening click that
        // began it, so a click immediately followed by a double-click on the same
        // spot is dropped -- acting on it too would leave a stray selection on
        // the piece that just jumped.
        const std::vector<kfc::view::MouseAction> actions = view.takeMouseActions();
        for (std::size_t i = 0; i < actions.size(); ++i) {
            const kfc::view::MouseAction& action = actions[i];
            if (action.type == kfc::view::MouseAction::Type::DoubleClick) {
                controller.handleJump(action.position.x, action.position.y);
                continue;
            }
            const bool opensDoubleClick =
                i + 1 < actions.size() &&
                actions[i + 1].type == kfc::view::MouseAction::Type::DoubleClick &&
                actions[i + 1].position == action.position;
            if (!opensDoubleClick) {
                controller.handleClick(action.position.x, action.position.y);
            }

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
