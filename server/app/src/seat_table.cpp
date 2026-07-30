#include "server/app/include/seat_table.hpp"

namespace kfc::server {

void SeatTable::seat(ClientId client, std::optional<model::Color> color,
                     const std::string& username, int rating) {
    occupants_[client] = Occupant{color, username, rating};
}

void SeatTable::remove(ClientId client) { occupants_.erase(client); }

void SeatTable::setRating(ClientId client, int rating) {
    const auto found = occupants_.find(client);
    if (found != occupants_.end()) found->second.rating = rating;
}

std::optional<model::Color> SeatTable::assignColor() const {
    bool whiteTaken = false;
    bool blackTaken = false;
    for (const auto& [id, occupant] : occupants_) {
        if (occupant.color == model::Color::White) whiteTaken = true;
        if (occupant.color == model::Color::Black) blackTaken = true;
    }
    if (!whiteTaken) return model::Color::White;
    if (!blackTaken) return model::Color::Black;
    return std::nullopt;  // both seats taken: a spectator
}

std::optional<model::Color> SeatTable::colorOf(ClientId client) const {
    const auto found = occupants_.find(client);
    if (found == occupants_.end()) return std::nullopt;
    return found->second.color;
}

std::optional<ClientId> SeatTable::clientOn(model::Color color) const {
    for (const auto& [id, occupant] : occupants_) {
        if (occupant.color == color) return id;
    }
    return std::nullopt;
}

std::optional<Occupant> SeatTable::occupantOf(model::Color color) const {
    for (const auto& [id, occupant] : occupants_) {
        if (occupant.color == color) return occupant;
    }
    return std::nullopt;
}

}  // namespace kfc::server
