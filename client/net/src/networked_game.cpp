#include "client/net/include/networked_game.hpp"

#include <string>
#include <utility>

namespace kfc::net {

NetworkedGame::NetworkedGame(const std::string& host, std::uint16_t port,
                             model::Board replica)
    : client_(host, port),
      remote_(std::move(replica),
              // The colour is irrelevant on a single connection: the server
              // authorizes by the client's own seat, so every command goes out
              // the one socket.
              [this](model::Color, const std::string& message) {
                  client_.send(message);
              }) {
    client_.onMessage(
        [this](const std::string& message) { remote_.receive(message); });
}

void NetworkedGame::advance(int /*deltaMs*/) {
    // The server owns the clock; the client only pumps the socket so incoming
    // state and events are applied. The delta is the server's concern, not ours.
    client_.poll();
}

}  // namespace kfc::net
