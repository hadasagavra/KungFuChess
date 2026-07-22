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
                   // A client is told its role and its own commands echo nowhere;
                   // neither changes what it draws, so both are ignored here.
                   [](const io::RoleAssignment&) {},
                   [](const io::PlayerCommand&) {},
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
    if (!replica_.getPieceAt(source)) return {};
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
    const io::PlayerCommand command{mover, piece->getKind(), from, to};
    sink_(mover, io::encode(io::WireMessage{command}, replica_.height()));
}

void RemoteGame::requestMove(model::Position from, model::Position to) {
    sendCommand(from, to);
}

void RemoteGame::requestJump(model::Position cell) { sendCommand(cell, cell); }

}  // namespace kfc::net
