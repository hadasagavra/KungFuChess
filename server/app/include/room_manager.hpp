#pragma once

#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>

#include "server/app/include/matchmaker.hpp"
#include "server/app/include/room.hpp"
#include "server/net/include/message_transport.hpp"
#include "server/store/include/user_store.hpp"
#include "shared/logic/io/include/wire_message.hpp"
#include "shared/logic/model/include/board.hpp"

namespace kfc::server {

// The lobby: the single-process coordinator above all rooms. A client's life is
// connect -> login -> Home -> create/join a room (or Play) -> seated in that
// room's game. The manager authenticates once here (it owns the user store),
// keeps every room, and routes each message either to lobby handling or to the
// client's room. It holds no game rules -- a room's GameSession does -- and no
// sockets: it speaks to a MessageTransport, so it is testable without one.
class RoomManager {
public:
    // Makes a fresh board for each new room. Rooms must not share pieces, and a
    // Board copy would (it holds shared_ptrs), so the caller supplies a factory
    // that builds one from the opening position each time.
    using BoardFactory = std::function<model::Board()>;

    RoomManager(MessageTransport& transport, UserStore& users,
                BoardFactory boardFactory);

    // A message arrived from a client. Lobby requests (login, create, join, seek)
    // are handled here; a command is forwarded to the client's room. Malformed or
    // out-of-place messages are dropped.
    void handleMessage(ClientId client, const std::string& text);

    // A client's connection dropped: cancel any search, and let its room start a
    // forfeit countdown (a player) or free it (a spectator).
    void handleDisconnected(ClientId client);

    // Advance every room's clock (and -- later -- matchmaking timers).
    void tick(int deltaMs);

private:
    // What the lobby knows about one connected client.
    struct Client {
        std::string username;
        int rating = 0;
        bool authed = false;
        std::optional<RoomId> room;
    };

    void handleLogin(ClientId client, const io::Login& login);
    void handleCreateRoom(ClientId client);
    void handleJoinRoom(ClientId client, const io::JoinRoom& join);
    // Play: queue the client, or -- if a compatible opponent is waiting -- put
    // the two in a fresh room together.
    void handleSeek(ClientId client);
    void routeCommand(ClientId client, const io::PlayerCommand& command);

    // Seat an authenticated client into a room and tell it which room it is in.
    void enterRoom(ClientId client, Room& room);
    // A fresh room with a unique id, registered and returned.
    Room& createRoom();
    // A random room id not currently in use.
    RoomId mintRoomId() const;

    void sendTo(ClientId client, const io::WireMessage& message);

    MessageTransport& transport_;
    UserStore& users_;
    BoardFactory boardFactory_;
    // The board height, cached so lobby replies can be encoded without a game.
    // Lobby messages carry no squares, so the value only has to be consistent.
    int boardHeight_;
    std::map<ClientId, Client> clients_;
    std::map<RoomId, std::unique_ptr<Room>> rooms_;
    Matchmaker matchmaker_;
};

}  // namespace kfc::server
