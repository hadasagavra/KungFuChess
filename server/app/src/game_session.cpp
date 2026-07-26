#include "server/app/include/game_session.hpp"

#include <variant>

#include "shared/logic/game_record/include/rating.hpp"
#include "shared/logic/io/include/state_codec.hpp"
#include "shared/logic/model/include/piece.hpp"

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

GameSession::GameSession(model::Board& board, MessageTransport& transport,
                         UserStore& users)
    : board_(board), engine_(board_), transport_(transport), users_(users) {
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
            // settle both players' ratings. The engine decides the game is over;
            // the session only turns that into a rating change.
            if (captured.kind == model::Kind::King) settleRatings(captured.color);
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
                   [&](const io::Login& login) { handleLogin(client, login); },
                   // A client cannot deal the server its own answers. These are
                   // well-formed messages arriving from the wrong direction, so
                   // they are simply not acted on.
                   [](const io::RoleAssignment&) {},
                   [](const io::PlayerRoster&) {},
                   [](const io::AuthRejected&) {},
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

void GameSession::handleLogin(ClientId client, const io::Login& login) {
    // Credentials are the store's business, not a game rule. An unknown name is
    // registered; a known one must match its password. A refusal is told only to
    // the client that sent it -- there is nothing to broadcast.
    const AuthResult auth = users_.authenticate(login.username, login.password);
    if (!auth.accepted) {
        transport_.send(client,
                        io::encode(io::WireMessage{io::AuthRejected{
                                       "incorrect password for " + login.username}},
                                   board_.height()));
        return;
    }
    // Accepted: record the name and current rating, and tell everyone the roster.
    // A spectator may log in too; its name is kept but names no seat.
    names_[client] = login.username;
    ratings_[client] = auth.rating;
    broadcastRoster();
}

std::map<ClientId, std::optional<model::Color>>::const_iterator
GameSession::clientOn(model::Color color) const {
    for (auto it = roles_.begin(); it != roles_.end(); ++it) {
        if (it->second == color) return it;
    }
    return roles_.end();
}

std::optional<std::string> GameSession::nameOfColor(model::Color color) const {
    const auto seat = clientOn(color);
    if (seat == roles_.end()) return std::nullopt;  // no client on this seat
    const auto found = names_.find(seat->first);
    if (found == names_.end()) return std::nullopt;  // seated, not logged in yet
    return found->second;
}

std::optional<int> GameSession::ratingOfColor(model::Color color) const {
    const auto seat = clientOn(color);
    if (seat == roles_.end()) return std::nullopt;
    const auto found = ratings_.find(seat->first);
    if (found == ratings_.end()) return std::nullopt;
    return found->second;
}

void GameSession::broadcastRoster() {
    const io::PlayerRoster roster{
        nameOfColor(model::Color::White), ratingOfColor(model::Color::White),
        nameOfColor(model::Color::Black), ratingOfColor(model::Color::Black)};
    transport_.broadcast(io::encode(io::WireMessage{roster}, board_.height()));
}

void GameSession::settleRatings(model::Color loser) {
    const model::Color winner =
        loser == model::Color::White ? model::Color::Black : model::Color::White;

    // Both seats must hold logged-in players, or there is no pair to rate.
    const std::optional<std::string> winnerName = nameOfColor(winner);
    const std::optional<std::string> loserName = nameOfColor(loser);
    const std::optional<int> winnerRating = ratingOfColor(winner);
    const std::optional<int> loserRating = ratingOfColor(loser);
    if (!winnerName || !loserName || !winnerRating || !loserRating) return;

    const int newWinner =
        game_record::updatedRating(*winnerRating, *loserRating, 1.0);
    const int newLoser =
        game_record::updatedRating(*loserRating, *winnerRating, 0.0);
    users_.updateRating(*winnerName, newWinner);
    users_.updateRating(*loserName, newLoser);
    ratings_[clientOn(winner)->first] = newWinner;
    ratings_[clientOn(loser)->first] = newLoser;
    broadcastRoster();
}

}  // namespace kfc::server
