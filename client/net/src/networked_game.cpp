#include "client/net/include/networked_game.hpp"

#include <string>
#include <utility>

#include "shared/logic/io/include/wire_message.hpp"

namespace kfc::net {

NetworkedGame::NetworkedGame(const std::string& host, std::uint16_t port,
                             model::Board replica, std::string username,
                             std::string password)
    : client_(host, port),
      boardHeight_(replica.height()),
      username_(std::move(username)),
      password_(std::move(password)),
      remote_(std::move(replica),
              // The colour is irrelevant on a single connection: the server
              // authorizes by the client's own seat, so every command goes out
              // the one socket.
              [this](model::Color, const std::string& message) {
                  client_.send(message);
              }) {
    client_.onMessage(
        [this](const std::string& message) { remote_.receive(message); });
    // Announce who is playing once the connection is up. Sending before then is
    // dropped, so the open handler -- not the constructor body -- is where it goes.
    client_.onOpen([this]() { sendLogin(); });
}

void NetworkedGame::sendLogin() {
    client_.send(io::encode(io::WireMessage{io::Login{username_, password_}},
                            boardHeight_));
}

void NetworkedGame::advance(int /*deltaMs*/) {
    // The server owns the clock; the client only pumps the socket so incoming
    // state and events are applied. The delta is the server's concern, not ours.
    client_.poll();
}

}  // namespace kfc::net
