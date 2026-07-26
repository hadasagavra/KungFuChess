#include "server/app/include/game_session.hpp"

#include <utility>
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
                         RatingSink onRatingChanged)
    : board_(board),
      engine_(board_),
      transport_(transport),
      onRatingChanged_(std::move(onRatingChanged)) {
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

void GameSession::addClient(ClientId client, const std::string& username,
                            int rating) {
    const std::optional<model::Color> color = assignColor();
    roles_[client] = color;
    names_[client] = username;
    ratings_[client] = rating;
    transport_.send(client, io::encode(io::RoleAssignment{color}, board_.height()));
    broadcastRoster();
}

std::optional<GameSession::Seat> GameSession::playerSeat(ClientId client) const {
    const auto role = roles_.find(client);
    if (role == roles_.end() || !role->second) return std::nullopt;  // spectator
    const auto name = names_.find(client);
    const auto rating = ratings_.find(client);
    if (name == names_.end() || rating == ratings_.end()) return std::nullopt;
    return Seat{*role->second, name->second, rating->second};
}

void GameSession::vacate(ClientId client) {
    roles_.erase(client);
    names_.erase(client);
    ratings_.erase(client);
}

void GameSession::seatReturning(ClientId client, model::Color color,
                                const std::string& username, int rating) {
    roles_[client] = color;
    names_[client] = username;
    ratings_[client] = rating;
    transport_.send(client, io::encode(io::RoleAssignment{color}, board_.height()));
    broadcastRoster();
}

void GameSession::handleMessage(ClientId client, const std::string& text) {
    const std::optional<io::WireMessage> message =
        io::decode(text, board_.height());
    if (!message) return;

    std::visit(overloaded{
                   [&](const io::PlayerCommand& command) {
                       handleCommand(client, command);
                   },
                   // Everything else is not a seated session's to act on: a
                   // client's own login and lobby requests are the lobby's, and a
                   // client cannot deal the server its own answers.
                   [](const io::Login&) {},
                   [](const io::RoleAssignment&) {},
                   [](const io::PlayerRoster&) {},
                   [](const io::AuthRejected&) {},
                   [](const model::MoveEvent&) {},
                   [](const model::CapturedPiece&) {},
                   [](const io::StateUpdate&) {},
                   [](const io::SeekGame&) {},
                   [](const io::CancelSeek&) {},
                   [](const io::CreateRoom&) {},
                   [](const io::JoinRoom&) {},
                   [](const io::EnteredRoom&) {},
                   [](const io::NoMatch&) {},
                   [](const io::RoomError&) {},
                   [](const io::OpponentDisconnected&) {},
                   [](const io::OpponentReconnected&) {},
               },
               *message);
}

void GameSession::forfeit(model::Color loser) {
    if (engine_.isGameOver()) return;
    engine_.endGame();
    settleRatings(loser);
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
    if (found == names_.end()) return std::nullopt;
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

    // Both seats must hold players, or there is no pair to rate.
    const std::optional<std::string> winnerName = nameOfColor(winner);
    const std::optional<std::string> loserName = nameOfColor(loser);
    const std::optional<int> winnerRating = ratingOfColor(winner);
    const std::optional<int> loserRating = ratingOfColor(loser);
    if (!winnerName || !loserName || !winnerRating || !loserRating) return;

    const int newWinner =
        game_record::updatedRating(*winnerRating, *loserRating, 1.0);
    const int newLoser =
        game_record::updatedRating(*loserRating, *winnerRating, 0.0);
    if (onRatingChanged_) {
        onRatingChanged_(*winnerName, newWinner);
        onRatingChanged_(*loserName, newLoser);
    }
    ratings_[clientOn(winner)->first] = newWinner;
    ratings_[clientOn(loser)->first] = newLoser;
    broadcastRoster();
}

}  // namespace kfc::server
