#pragma once

#include <cstdint>
#include <optional>
#include <set>
#include <string>

#include "client/net/include/client_game.hpp"
#include "client/net/include/remote_game.hpp"
#include "client/net/include/websocket_client.hpp"
#include "shared/bus/include/event_bus.hpp"
#include "shared/logic/engine/include/game_engine.hpp"
#include "shared/logic/model/include/board.hpp"
#include "shared/logic/model/include/piece.hpp"
#include "shared/logic/model/include/position.hpp"

namespace kfc::net {

// A game played against a real server over a socket. It is the networked sibling
// of LoopbackGame and fits the same ClientGame seam, so the frame loop drives it
// with no idea a socket is involved. It owns a WebSocketClient and a RemoteGame,
// wired to each other: commands go out through the socket, the server's messages
// come in and update the replica.
//
// Here the client controls only what the server lets it (its assigned colour);
// there is no per-colour seat routing as in loopback -- every command rides one
// connection and the server's ownership check settles it. Advancing the game is
// just pumping the socket: the server owns the clock.
class NetworkedGame : public ClientGame {
public:
    NetworkedGame(const std::string& host, std::uint16_t port,
                  model::Board replica);

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
    WebSocketClient client_;
    RemoteGame remote_;
};

}  // namespace kfc::net
