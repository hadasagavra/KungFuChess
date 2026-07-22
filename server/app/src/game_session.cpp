#include "server/app/include/game_session.hpp"

#include <variant>

#include "shared/logic/io/include/state_codec.hpp"

namespace kfc::server {
namespace {

// std::visit over a set of lambdas: one operator() per message kind, so the
// compiler still forces every WireMessage alternative to be handled.
template <class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

}  // namespace

GameSession::GameSession(model::Board& board, MessageTransport& transport)
    : board_(board), engine_(board_), transport_(transport) {
    // The engine announces what happened; the session forwards it to the clients.
    // The engine never learns a network is listening -- this is the network
    // publisher its EventBus was built to accept, added without touching it.
    engine_.events().subscribe<model::MoveEvent>(
        [this](const model::MoveEvent& event) {
            transport_.broadcast(io::encode(event, board_.height()));
        });
    engine_.events().subscribe<model::CapturedPiece>(
        [this](const model::CapturedPiece& captured) {
            transport_.broadcast(io::encode(captured, board_.height()));
        });
}

void GameSession::addClient(ClientId client) {
    const std::optional<model::Color> color = assignColor();
    roles_[client] = color;
    transport_.send(client, io::encode(io::RoleAssignment{color}, board_.height()));
}

void GameSession::handleMessage(ClientId client, const std::string& text) {
    const std::optional<io::WireMessage> message =
        io::decode(text, board_.height());
    if (!message) return;

    std::visit(overloaded{
                   [&](const io::PlayerCommand& command) {
                       handleCommand(client, command);
                   },
                   // A client cannot deal the server its own answers. These are
                   // well-formed messages arriving from the wrong direction, so
                   // they are simply not acted on.
                   [](const io::RoleAssignment&) {},
                   [](const model::MoveEvent&) {},
                   [](const model::CapturedPiece&) {},
                   [](const io::StateUpdate&) {},
               },
               *message);
}

void GameSession::tick(int deltaMs) {
    engine_.wait(deltaMs);
    transport_.broadcast(
        io::encode(io::StateUpdate{io::encodeState(engine_.getSnapshot())},
                   board_.height()));
}

std::optional<model::Color> GameSession::colorOf(ClientId client) const {
    const auto found = roles_.find(client);
    if (found == roles_.end()) return std::nullopt;
    return found->second;
}

std::optional<model::Color> GameSession::assignColor() const {
    bool whiteTaken = false;
    bool blackTaken = false;
    for (const auto& [id, color] : roles_) {
        if (color == model::Color::White) whiteTaken = true;
        if (color == model::Color::Black) blackTaken = true;
    }
    if (!whiteTaken) return model::Color::White;
    if (!blackTaken) return model::Color::Black;
    return std::nullopt;  // both seats taken: a spectator
}

void GameSession::handleCommand(ClientId client, const io::PlayerCommand& command) {
    // Authorization, not chess: you may only command your own colour. Whether the
    // move itself is legal is the engine's call, made inside request*.
    const std::optional<model::Color> role = colorOf(client);
    if (!role || *role != command.player) return;

    if (command.from == command.to) {
        engine_.requestJump(command.from);
    } else {
        engine_.requestMove(command.from, command.to);
    }
}

}  // namespace kfc::server
