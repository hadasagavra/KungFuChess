#include "texttests/include/script_runner.hpp"

#include <variant>

#include "shared/logic/io/include/board_printer.hpp"
namespace kfc::texttests {
namespace {

template <typename... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};
template <typename... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

}  // namespace

ScriptRunner::ScriptRunner(input::Controller& controller,
                           engine::GameEngine& engine, std::ostream& out)
    : controller_(controller), engine_(engine), out_(out) {}

void ScriptRunner::run(const Command& command) {
    std::visit(overloaded{
                   [&](const ClickCommand& c) { controller_.handleClick(c.x, c.y); },
                   [&](const JumpCommand& c) { controller_.handleJump(c.x, c.y); },
                   [&](const WaitCommand& c) { engine_.wait(c.ms); },
                   [&](const PrintBoardCommand&) {
                       io::printBoard(engine_.getSnapshot(), out_);
                   },
               },
               command);
}

}