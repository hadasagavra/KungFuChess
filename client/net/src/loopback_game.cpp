#include "client/net/include/loopback_game.hpp"

#include <string>
#include <utility>

namespace kfc::net {

LoopbackGame::LoopbackGame(model::Board startPosition)
    : serverBoard_(startPosition),
      session_(serverBoard_, transport_, store_),
      remote_(startPosition,
              [this](model::Color mover, const std::string& message) {
                  // Route the command to the seat that owns its colour, so the
                  // server accepts either colour from this one window.
                  session_.handleMessage(
                      mover == model::Color::White ? whiteSeat : blackSeat,
                      message);
              }) {
    // The server delivers everything it emits to the one RemoteGame.
    transport_.setClientSink(
        [this](const std::string& message) { remote_.receive(message); });

    session_.addClient(whiteSeat);
    session_.addClient(blackSeat);

    // Seed a first frame so the replica holds the real board before the first
    // render, without advancing the clock.
    session_.tick(0);
}

void LoopbackGame::advance(int deltaMs) {
    // Ticking the session advances the authoritative clock and broadcasts the new
    // frame, which the transport hands straight to the RemoteGame's replica.
    session_.tick(deltaMs);
}

}  // namespace kfc::net
