#include "server/app/include/game_session.hpp"

#include <optional>
#include <string>
#include <utility>
#include <variant>

#include "shared/logic/io/include/state_codec.hpp"
#include "shared/logic/model/include/piece.hpp"

namespace kfc::server {

GameSession::GameSession(model::Board& board, MessageTransport& transport,
                         RatingSink onRatingChanged)
    : board_(board),
      engine_(board_),
      transport_(transport),
      settler_(seats_, std::move(onRatingChanged),
               [this]() { broadcastRoster(); }) {
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
            // Taking the king ends the game: the captured king's colour lost, so
            // the settler rates both players. The engine decides the game is over;
            // the session only routes that fact to the rating rule.
            if (captured.kind == model::Kind::King) settler_.settle(captured.color);
        });
}

void GameSession::addClient(ClientId client, const std::string& username,
                            int rating) {
    const std::optional<model::Color> color = seats_.assignColor();
    seats_.seat(client, color, username, rating);
    sendRole(client, color);
    broadcastRoster();
}

std::optional<Occupant> GameSession::playerSeat(ClientId client) const {
    const std::optional<model::Color> color = seats_.colorOf(client);
    if (!color) return std::nullopt;  // spectator or unknown
    return seats_.occupantOf(*color);  // the occupant seated on that colour
}

void GameSession::vacate(ClientId client) { seats_.remove(client); }

void GameSession::seatReturning(ClientId client, model::Color color,
                                const std::string& username, int rating) {
    seats_.seat(client, color, username, rating);
    sendRole(client, color);
    broadcastRoster();
}

void GameSession::applyCommand(ClientId client, const io::PlayerCommand& command) {
    // Authorization, not chess: you may only command your own colour. Whether the
    // move itself is legal is the engine's call, made inside request*.
    const std::optional<model::Color> role = seats_.colorOf(client);
    if (!role || *role != command.player) return;

    if (command.from == command.to) {
        engine_.requestJump(command.from);
    } else {
        engine_.requestMove(command.from, command.to);
    }
}

void GameSession::handleMessage(ClientId client, const std::string& text) {
    const std::optional<io::WireMessage> message =
        io::decode(text, board_.height());
    if (!message) return;
    // Only a command means anything to a seated session; a client's own login and
    // the server's answers are not its to act on.
    if (const auto* command = std::get_if<io::PlayerCommand>(&*message)) {
        applyCommand(client, *command);
    }
}

void GameSession::forfeit(model::Color loser) {
    if (engine_.isGameOver()) return;
    engine_.endGame();
    settler_.settle(loser);
    // Push the final frame so every client sees the game is over at once.
    transport_.broadcast(
        io::encode(io::StateUpdate{io::encodeState(engine_.getSnapshot())},
                   board_.height()));
}

void GameSession::tick(int deltaMs) {
    engine_.wait(deltaMs);
    transport_.broadcast(
        io::encode(io::StateUpdate{io::encodeState(engine_.getSnapshot())},
                   board_.height()));
}

bool GameSession::isOver() const { return engine_.isGameOver(); }

void GameSession::sendRole(ClientId client, std::optional<model::Color> color) {
    transport_.send(client, io::encode(io::RoleAssignment{color}, board_.height()));
}

void GameSession::broadcastRoster() {
    const std::optional<Occupant> white = seats_.occupantOf(model::Color::White);
    const std::optional<Occupant> black = seats_.occupantOf(model::Color::Black);
    const auto nameOf = [](const std::optional<Occupant>& o) {
        return o ? std::optional<std::string>(o->username) : std::nullopt;
    };
    const auto ratingOf = [](const std::optional<Occupant>& o) {
        return o ? std::optional<int>(o->rating) : std::nullopt;
    };
    const io::PlayerRoster roster{nameOf(white), ratingOf(white), nameOf(black),
                                  ratingOf(black)};
    transport_.broadcast(io::encode(io::WireMessage{roster}, board_.height()));
}

}  // namespace kfc::server
