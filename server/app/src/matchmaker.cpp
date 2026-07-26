#include "server/app/include/matchmaker.hpp"

#include <cstdlib>

#include "shared/logic/game_record/include/rating.hpp"

namespace kfc::server {
namespace {

// How long a seeker waits before the search gives up ("can't find") -- one minute.
constexpr int matchmakingTimeoutMs = 60000;

}  // namespace

std::optional<ClientId> Matchmaker::seek(ClientId client, int rating) {
    if (isWaiting(client)) return std::nullopt;  // already looking

    for (auto it = queue_.begin(); it != queue_.end(); ++it) {
        if (std::abs(it->rating - rating) <= game_record::eloMatchRange) {
            const ClientId opponent = it->client;
            queue_.erase(it);
            return opponent;
        }
    }
    queue_.push_back(Seeker{client, rating, 0});
    return std::nullopt;
}

void Matchmaker::cancel(ClientId client) {
    for (auto it = queue_.begin(); it != queue_.end(); ++it) {
        if (it->client == client) {
            queue_.erase(it);
            return;
        }
    }
}

std::vector<ClientId> Matchmaker::tick(int deltaMs) {
    std::vector<ClientId> timedOut;
    for (auto it = queue_.begin(); it != queue_.end();) {
        it->waitedMs += deltaMs;
        if (it->waitedMs >= matchmakingTimeoutMs) {
            timedOut.push_back(it->client);
            it = queue_.erase(it);
        } else {
            ++it;
        }
    }
    return timedOut;
}

bool Matchmaker::isWaiting(ClientId client) const {
    for (const Seeker& seeker : queue_) {
        if (seeker.client == client) return true;
    }
    return false;
}

}  // namespace kfc::server
