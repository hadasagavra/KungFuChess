#pragma once

#include <optional>
#include <set>

#include "client/net/include/client_game.hpp"
#include "client/net/include/loopback_transport.hpp"
#include "client/net/include/remote_game.hpp"
#include "server/app/include/game_session.hpp"
#include "shared/bus/include/event_bus.hpp"
#include "shared/logic/engine/include/game_engine.hpp"
#include "shared/logic/model/include/board.hpp"
#include "shared/logic/model/include/piece.hpp"
#include "shared/logic/model/include/position.hpp"

namespace kfc::net {

// A whole game running in one process over the real server protocol. It hosts an
// authoritative GameSession and a RemoteGame, wired to each other through a
// LoopbackTransport, and seats two clients -- one white, one black -- so a single
// window may command either colour while every command still passes the server's
// ownership check. Nothing here shortcuts the protocol: local play exercises the
// same encode/decode and the same session as a networked game would.
//
// The ClientGame surface is served by delegating to the RemoteGame; advancing the
// game is ticking the session, whose broadcast refreshes the RemoteGame's replica.
class LoopbackGame : public ClientGame {
public:
    explicit LoopbackGame(model::Board startPosition);

    void advance(int deltaMs) override;
    engine::GameSnapshot getSnapshot() const override {
        return remote_.getSnapshot();
    }
    std::set<model::Position> legalDestinationsFor(
        model::Position source) const override {
        return remote_.legalDestinationsFor(source);
    }
    bus::EventBus& events() override { return remote_.events(); }

    std::optional<model::Piece> pieceAt(model::Position cell) const override {
        return remote_.pieceAt(cell);
    }
    void requestMove(model::Position from, model::Position to) override {
        remote_.requestMove(from, to);
    }
    void requestJump(model::Position cell) override {
        remote_.requestJump(cell);
    }

private:
    static constexpr server::ClientId whiteSeat = 1;
    static constexpr server::ClientId blackSeat = 2;

    model::Board serverBoard_;
    LoopbackTransport transport_;
    server::GameSession session_;
    RemoteGame remote_;
};

}  // namespace kfc::net
