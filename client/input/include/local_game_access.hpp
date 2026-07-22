#pragma once

#include <optional>

#include "client/input/include/game_access.hpp"
#include "shared/logic/engine/include/game_engine.hpp"
#include "shared/logic/model/include/piece.hpp"
#include "shared/logic/model/include/position.hpp"

namespace kfc::input {

// A GameAccess backed by a GameEngine in the same process. It is the direct,
// no-network path: the script harness and the controller unit tests drive a real
// engine through it. Real graphical play does NOT use this -- it goes over the
// server protocol (loopback or socket) so both colours obey the same authority.
class LocalGameAccess : public GameAccess {
public:
    explicit LocalGameAccess(engine::GameEngine& engine);

    std::optional<model::Piece> pieceAt(model::Position cell) const override;
    void requestMove(model::Position from, model::Position to) override;
    void requestJump(model::Position cell) override;

private:
    engine::GameEngine& engine_;
};

}  // namespace kfc::input
