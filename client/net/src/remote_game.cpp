#include "client/net/include/remote_game.hpp"

#include <utility>
#include <variant>

#include "shared/logic/io/include/state_codec.hpp"
#include "shared/logic/io/include/wire_message.hpp"

namespace kfc::net {
namespace {

template <class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

}  // namespace

RemoteGame::RemoteGame(model::Board replica, CommandSink sink)
    : replica_(std::move(replica)), sink_(std::move(sink)) {}

void RemoteGame::receive(const std::string& message) {
    const std::optional<io::WireMessage> decoded =
        io::decode(message, replica_.height());
    if (!decoded) return;

    std::visit(overloaded{
                   [this](const io::StateUpdate& update) {
                       if (const std::optional<io::DecodedState> state =
                               io::decodeState(update.frame, replica_)) {
                           isOver_ = state->isOver;
                           motions_ = state->motions;
                           cooldowns_ = state->cooldowns;
                       }
                   },
                   [this](const model::MoveEvent& event) { bus_.publish(event); },
                   [this](const model::CapturedPiece& captured) {
                       bus_.publish(captured);
                   },
                   // The colour(s) this client may command; a spectator's role
                   // carries none, leaving the set empty.
                   [this](const io::RoleAssignment& role) {
                       if (role.color) ownColors_.insert(*role.color);
                   },
                   // The player names and ratings, for the side panels to show.
                   [this](const io::PlayerRoster& roster) {
                       whiteName_ = roster.whiteName.value_or("");
                       blackName_ = roster.blackName.value_or("");
                       whiteRating_ = roster.whiteRating;
                       blackRating_ = roster.blackRating;
                   },
                   // The server refused this client's login; the reason is kept
                   // for the shell to report.
                   [this](const io::AuthRejected& rejected) {
                       authError_ = rejected.reason;
                   },
                   // The lobby's answers: which room we are in, why a request
                   // failed, and an opponent's disconnect countdown.
                   [this](const io::EnteredRoom& entered) {
                       inGame_ = true;
                       searching_ = false;
                       roomId_ = entered.roomId;
                       statusMessage_.clear();
                   },
                   [this](const io::NoMatch&) {
                       searching_ = false;
                       statusMessage_ = "Couldn't find a match";
                   },
                   [this](const io::RoomError& error) {
                       searching_ = false;
                       statusMessage_ = error.reason;
                   },
                   [this](const io::OpponentDisconnected& gone) {
                       opponentCountdown_ = gone.secondsLeft;
                   },
                   [this](const io::OpponentReconnected&) {
                       opponentCountdown_.reset();
                   },
                   // A client never receives its own requests or login back;
                   // none change what it draws, so all are ignored here.
                   [](const io::PlayerCommand&) {},
                   [](const io::Login&) {},
                   [](const io::SeekGame&) {},
                   [](const io::CancelSeek&) {},
                   [](const io::CreateRoom&) {},
                   [](const io::JoinRoom&) {},
               },
               *decoded);
}

engine::GameSnapshot RemoteGame::getSnapshot() const {
    return engine::GameSnapshot{replica_, isOver_, motions_, cooldowns_};
}

bool RemoteGame::isBusy(model::Position cell) const {
    for (const realtime::MotionState& motion : motions_) {
        if (motion.from == cell) return true;
    }
    for (const realtime::CooldownState& cooldown : cooldowns_) {
        if (cooldown.cell == cell) return true;
    }
    return false;
}

std::set<model::Position> RemoteGame::legalDestinationsFor(
    model::Position source) const {
    // A display aid, mirroring the engine's gate as closely as the replica allows:
    // no highlights once the game is over or while the piece is busy. The replica
    // carries no piece state, so "busy" is read from the live motions/cooldowns.
    // The server remains the authority; a stray highlight only offers a move it
    // would refuse.
    if (isOver_ || isBusy(source)) return {};
    const std::shared_ptr<model::Piece> piece = replica_.getPieceAt(source);
    if (!piece) return {};
    // Only a piece this client may command is worth highlighting: a spectator
    // highlights nothing, and a player never highlights the opponent's pieces.
    if (!mayCommand(piece->getColor())) return {};
    return ruleEngine_.getLegalDestinations(replica_, source);
}

std::optional<model::Piece> RemoteGame::pieceAt(model::Position cell) const {
    const std::shared_ptr<model::Piece> piece = replica_.getPieceAt(cell);
    if (!piece) return std::nullopt;
    return *piece;
}

void RemoteGame::sendCommand(model::Position from, model::Position to) {
    const std::shared_ptr<model::Piece> piece = replica_.getPieceAt(from);
    if (!piece) return;  // nothing there to name the mover
    const model::Color mover = piece->getColor();
    // View-only unless this client owns the colour: a spectator sends nothing,
    // and a player sends no command for the opponent's pieces. The server would
    // refuse either way; this spares it the round trip.
    if (!mayCommand(mover)) return;
    const io::PlayerCommand command{mover, piece->getKind(), from, to};
    sink_(mover, io::encode(io::WireMessage{command}, replica_.height()));
}

void RemoteGame::requestMove(model::Position from, model::Position to) {
    sendCommand(from, to);
}

void RemoteGame::requestJump(model::Position cell) { sendCommand(cell, cell); }

void RemoteGame::sendLobby(const io::WireMessage& message) {
    sink_(model::Color::White, io::encode(message, replica_.height()));
}

void RemoteGame::seekGame() {
    searching_ = true;
    statusMessage_ = "Searching for an opponent...";
    sendLobby(io::WireMessage{io::SeekGame{}});
}

void RemoteGame::cancelSeek() {
    searching_ = false;
    statusMessage_.clear();
    sendLobby(io::WireMessage{io::CancelSeek{}});
}

void RemoteGame::createRoom() { sendLobby(io::WireMessage{io::CreateRoom{}}); }

void RemoteGame::joinRoom(const std::string& roomId) {
    sendLobby(io::WireMessage{io::JoinRoom{roomId}});
}

}  // namespace kfc::net
