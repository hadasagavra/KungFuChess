#include "server/app/include/room_manager.hpp"

#include <array>
#include <random>
#include <utility>
#include <variant>

#include "shared/log/include/logger.hpp"

namespace kfc::server {
namespace {

constexpr const char* logComponent = "server";
constexpr int roomIdLength = 6;

template <class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

}  // namespace

RoomManager::RoomManager(MessageTransport& transport, UserStore& users,
                         BoardFactory boardFactory)
    : transport_(transport),
      users_(users),
      boardFactory_(std::move(boardFactory)),
      boardHeight_(boardFactory_().height()) {}

void RoomManager::handleMessage(ClientId client, const std::string& text) {
    const std::optional<io::WireMessage> message = io::decode(text, boardHeight_);
    if (!message) {
        log::write(logComponent, "client " + std::to_string(client) +
                                     " sent malformed text, dropped");
        return;
    }

    std::visit(
        overloaded{
            [&](const io::Login& login) { handleLogin(client, login); },
            [&](const io::CreateRoom&) { handleCreateRoom(client); },
            [&](const io::JoinRoom& join) { handleJoinRoom(client, join); },
            [&](const io::PlayerCommand&) { routeCommand(client, text); },
            [&](const io::SeekGame&) { handleSeek(client); },
            [&](const io::CancelSeek&) { matchmaker_.cancel(client); },
            // A client cannot send the server its own answers; these are ignored.
            [](const io::RoleAssignment&) {},
            [](const io::PlayerRoster&) {},
            [](const io::AuthRejected&) {},
            [](const io::EnteredRoom&) {},
            [](const io::NoMatch&) {},
            [](const io::RoomError&) {},
            [](const io::OpponentDisconnected&) {},
            [](const io::OpponentReconnected&) {},
            [](const model::MoveEvent&) {},
            [](const model::CapturedPiece&) {},
            [](const io::StateUpdate&) {},
        },
        *message);
}

void RoomManager::handleLogin(ClientId client, const io::Login& login) {
    const AuthResult auth = users_.authenticate(login.username, login.password);
    if (!auth.accepted) {
        log::write(logComponent, "login refused for " + login.username +
                                     " (client " + std::to_string(client) + ")");
        sendTo(client, io::AuthRejected{"incorrect password for " + login.username});
        return;
    }
    clients_[client] = Client{login.username, auth.rating, true, std::nullopt};
    log::write(logComponent, "login ok: " + login.username + " (rating " +
                                 std::to_string(auth.rating) + ", client " +
                                 std::to_string(client) + ")");
}

void RoomManager::handleCreateRoom(ClientId client) {
    const auto found = clients_.find(client);
    if (found == clients_.end() || !found->second.authed) {
        sendTo(client, io::RoomError{"log in before creating a room"});
        return;
    }
    Room& room = createRoom();
    log::write(logComponent, found->second.username + " created room " + room.id());
    enterRoom(client, room);
}

void RoomManager::handleJoinRoom(ClientId client, const io::JoinRoom& join) {
    const auto found = clients_.find(client);
    if (found == clients_.end() || !found->second.authed) {
        sendTo(client, io::RoomError{"log in before joining a room"});
        return;
    }
    const auto room = rooms_.find(join.roomId);
    if (room == rooms_.end()) {
        log::write(logComponent, found->second.username + " tried to join unknown room " +
                                     join.roomId);
        sendTo(client, io::RoomError{"no room " + join.roomId});
        return;
    }
    log::write(logComponent, found->second.username + " joined room " + join.roomId);
    enterRoom(client, *room->second);
}

void RoomManager::handleSeek(ClientId client) {
    const auto found = clients_.find(client);
    if (found == clients_.end() || !found->second.authed) {
        sendTo(client, io::RoomError{"log in before searching for a game"});
        return;
    }
    const std::optional<ClientId> opponent =
        matchmaker_.seek(client, found->second.rating);
    if (!opponent) {
        log::write(logComponent, found->second.username + " is searching for a game");
        return;  // queued; the client already knows it is searching
    }
    // A match: the waiting player takes white, the arriving player black.
    Room& room = createRoom();
    log::write(logComponent, "matched " + clients_.at(*opponent).username + " vs " +
                                 found->second.username + " in room " + room.id());
    enterRoom(*opponent, room);
    enterRoom(client, room);
}

void RoomManager::routeCommand(ClientId client, const std::string& text) {
    const auto found = clients_.find(client);
    if (found == clients_.end() || !found->second.room) return;
    const auto room = rooms_.find(*found->second.room);
    if (room == rooms_.end()) return;
    room->second->handleMessage(client, text);
}

void RoomManager::enterRoom(ClientId client, Room& room) {
    Client& state = clients_.at(client);
    state.room = room.id();
    room.addClient(client, state.username, state.rating);
    sendTo(client, io::EnteredRoom{room.id()});
}

Room& RoomManager::createRoom() {
    const RoomId id = mintRoomId();
    auto room = std::make_unique<Room>(
        id, boardFactory_(), transport_,
        // Persist each settled rating to the account store.
        [this](const std::string& username, int rating) {
            users_.updateRating(username, rating);
        });
    Room& ref = *room;
    rooms_.emplace(id, std::move(room));
    return ref;
}

RoomId RoomManager::mintRoomId() const {
    static const std::array<char, 36> alphabet{
        'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L',
        'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
        'Y', 'Z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};
    static std::mt19937 rng{std::random_device{}()};
    std::uniform_int_distribution<std::size_t> pick(0, alphabet.size() - 1);

    RoomId id;
    do {
        id.clear();
        for (int i = 0; i < roomIdLength; ++i) id.push_back(alphabet[pick(rng)]);
    } while (rooms_.count(id) != 0);  // vanishingly rare, but never collide
    return id;
}

void RoomManager::handleDisconnected(ClientId client) {
    matchmaker_.cancel(client);  // if it was searching, stop
    const auto found = clients_.find(client);
    if (found != clients_.end() && found->second.room) {
        const auto room = rooms_.find(*found->second.room);
        if (room != rooms_.end()) {
            log::write(logComponent, found->second.username +
                                         " disconnected from room " + room->first);
            room->second->handleDisconnect(client);
        }
    }
    clients_.erase(client);
}

void RoomManager::tick(int deltaMs) {
    // Searches that ran out of time are told no opponent was found.
    for (const ClientId client : matchmaker_.tick(deltaMs)) {
        log::write(logComponent, "no match for client " + std::to_string(client));
        sendTo(client, io::NoMatch{});
    }
    for (const auto& [id, room] : rooms_) room->tick(deltaMs);
}

void RoomManager::sendTo(ClientId client, const io::WireMessage& message) {
    transport_.send(client, io::encode(message, boardHeight_));
}

}  // namespace kfc::server
