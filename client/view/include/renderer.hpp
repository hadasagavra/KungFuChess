#pragma once

#include "img.hpp"
#include "shared/logic/model/include/piece.hpp"
#include "client/view/include/game_snapshot.hpp"
#include "client/view/include/lobby_view.hpp"
#include "client/view/include/render_config.hpp"
#include "client/view/include/render_layout.hpp"

namespace kfc::view {

// Display only. The Renderer paints a read-only GameSnapshot onto an image via
// the provided Img/OpenCV wrapper. It holds no game rules and never mutates
// domain state -- it draws the board, each piece's sprite, and the two side
// panels, placing strings the seam already wrote.
class Renderer {
public:
    explicit Renderer(RenderConfig config);

    // Compose one frame from the snapshot and return it. Does not display: the
    // caller decides whether to save or show, keeping the render loop
    // non-blocking (Img::show() blocks on a keypress).
    Img renderFrame(const GameSnapshot& snapshot) const;

    // Compose the Home screen (title, status, buttons, and -- in the dialog -- the
    // room-id text box) as its own frame, sized to the lobby, not the board.
    Img renderLobby(const LobbyView& view) const;

private:
    // Draw the room-id banner and, if an opponent is disconnected, the forfeit
    // countdown, over the top of the board. No-op when neither is set.
    void drawOverlaysOn(Img& frame, const GameSnapshot& snapshot) const;
    // Dim the frame and stamp a large centred "GAME OVER" across it once the
    // game has ended.
    void drawGameOverOn(Img& frame, const FrameLayout& layout) const;
    Img createCanvas(const FrameLayout& layout) const;
    Img loadBoardBackground(int widthCells, int heightCells) const;
    Img loadSprite(model::Kind kind, model::Color color, model::State state,
                   int frame) const;
    void drawBoardOn(Img& frame, const GameSnapshot& snapshot) const;
    void drawPieceOn(Img& frame, const PieceView& piece) const;
    // Shades each legal-move hint cell with a translucent yellow fill. Drawn
    // after the board but before the pieces, so a piece (e.g. a capturable enemy)
    // stays visible on top of its highlight.
    void drawHighlightsOn(Img& frame, const GameSnapshot& snapshot) const;
    // Draws the cooldown indicator over a resting piece: a translucent shade
    // whose top edge descends down the cell as the rest completes. No-op for a
    // piece that is not resting.
    void drawCooldownOn(Img& frame, const PieceView& piece) const;

    // One player's side panel: the name and score block, then the move table.
    void drawPanelOn(Img& frame, const PlayerPanel& panel,
                     const PanelBounds& bounds) const;
    void drawPanelHeaderOn(Img& frame, const PlayerPanel& panel,
                           const PanelBounds& bounds) const;
    void drawMoveTableOn(Img& frame, const PlayerPanel& panel,
                         const PanelBounds& bounds) const;

    RenderConfig config_;
};

}  // namespace kfc::view
