#pragma once

#include <cstdint>
#include <iosfwd>

#include "shared/logic/model/include/position.hpp"

namespace kfc::model {

enum class Color { White, Black };
enum class Kind { King, Queen, Rook, Bishop, Knight, Pawn };
enum class State { Idle, Moving, Airborne, Resting, Captured };

// A Piece is a domain entity: a stable identity (id) plus descriptive
// attributes and a lifecycle flag. It knows only where it currently sits
// (cell); pathing, destination, elapsed time, speed and arrival belong to
// Motion/RealTimeArbiter, not here. It has no knowledge of rendering, input,
// or id generation (BoardParser/PieceFactory owns that).
class Piece {
public:
    Piece(std::uint32_t id, Color color, Kind kind, Position cell,
          State state = State::Idle)
        : id_(id), color_(color), kind_(kind), cell_(cell), state_(state) {}

    std::uint32_t getId() const { return id_; }
    Color getColor() const { return color_; }
    Kind getKind() const { return kind_; }
    Position getCell() const { return cell_; }
    State getState() const { return state_; }

    void setCell(const Position& cell) { cell_ = cell; }
    void setState(State state) { state_ = state; }
    void setKind(Kind kind) { kind_ = kind; }  // promotion

private:
    std::uint32_t id_;
    Color color_;
    Kind kind_;
    Position cell_;
    State state_;
};

// Identity-based: two pieces are the same iff they share an id, regardless of
// their current color/kind/cell/state.
bool operator==(const Piece& a, const Piece& b);
bool operator!=(const Piece& a, const Piece& b);

// The other side. The one definition of "the opposing colour", so no caller
// re-derives it with its own conditional.
Color opponentOf(Color color);

// What capturing a piece of this kind is worth. A property of the KIND, not of
// the instance: deriving it on demand is what keeps a promoted pawn's worth
// correct without anyone having to remember to update a stored field.
int costOf(Kind kind);

std::ostream& operator<<(std::ostream& os, Color c);
std::ostream& operator<<(std::ostream& os, Kind k);
std::ostream& operator<<(std::ostream& os, State s);
std::ostream& operator<<(std::ostream& os, const Piece& p);

}  // namespace kfc::model
