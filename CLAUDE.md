# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Status

The restructuring around explicit design patterns and strict layer separation is essentially done: every module described under Project Structure exists at the repository root as `<module>/include` + `<module>/src`, and the CMake build compiles only those. The legacy `Business Logic/` directory (PascalCase files: `Board.cpp`, `Game.cpp`, `MoveRules.cpp`, …) is dead code — nothing includes it and the build never touches it. It stays only as a reference during the tail of the migration; do not add to it, do not fix bugs in it, and prefer deleting it once nothing is being cross-checked against it any more.

Two structural decisions landed after the original layout and are now the rule:

- **`bus/` replaced the observer interface.** There was a `GameSubject` / `GameObserver` pair in `engine/`; it is gone. Game events now travel over a generic `EventBus` that names no game concept, so a new kind of event costs a struct plus whoever subscribes, instead of an edit to a contract every listener shares.
- **`game_record/` holds the product-feature records.** The moves log and the score live in their own Business Logic module rather than inside the engine — they are listeners, not part of the rules.

## Game Overview

KungFuChess is a real-time, simultaneous chess variant — there are no turns. Both players move at the same time. Core rules that the logic must enforce:

- After any move, a piece must rest (cooldown) before it can move again.
- Winning = capturing the king. There is no concept of check or checkmate.
- Extra piece commands: Jump (move + short pause).
- Extra piece types beyond standard chess, e.g. Quadcopter: slower cooldown than other pieces, but can move to any square in the [±2, ±2] range.
- Product features layered on top: moves log, score (sum of the "cost" of captured pieces), and player name display.

## Architecture — the central constraint

The project is organized into three layers with strict separation. This is the single most important design rule and the primary thing the project is graded on — quality of design and layer separation, not merely "does it work."

- **Business Logic** — the heart of the game, with zero dependency on display or networking: piece rules, movement, cooldown, capture logic, and win detection live here and nowhere else.
- **GUI** — display only. Renders board state and collects input. Contains no game rules.
- **Server** — the networking / coordination layer between players. Not implemented yet.

The guiding principle: Business Logic must be completely decoupled from GUI and Server. Never mix game rules into display or network code. If the layers are designed this way from the start, the test structure and separation fall into place almost automatically. When making changes, treat any leak of game rules into the GUI or Server layer as a design defect to be corrected, not accommodated.

A non-functional/aspirational goal is scalability toward millions of concurrent players. This is architectural pressure to keep layers cleanly separated and the server well-designed — not a mandate to actually implement at that scale.

## Project Structure

The source modules live at the repository root, each mapping onto one of the architectural layers; `tests/` mirrors them. Add new code to the module that matches its responsibility — never widen a module's job to avoid creating the right one.

```
model/                # Business Logic — pure domain data & state
  position            # a board coordinate
  piece               # piece identity (type, color)
  board               # the grid of pieces + safe accessors
  board_errors        # the board's error/result vocabulary
  game_state          # authoritative game state (no orchestration)
  game_event          # MoveEvent / CapturedPiece — what happened, in domain terms
rules/                # Business Logic — the rules of the game
  piece_rules         # per-type movement/legality rules
  rule_engine         # orchestrates the rules into a legality decision
realtime/             # Business Logic — the real-time / simultaneous mechanics
  motion              # travel time, cooldown, jump durations
  real_time_arbiter   # pending moves/jumps, the clock, arrival resolution
engine/               # Business Logic — top-level coordination
  game_engine         # drives state + rules + realtime; the logic entry point.
                      # Owns an EventBus and publishes onto it; knows no listener.
game_record/          # Business Logic — the product-feature records (listeners)
  move_log            # the moves log, fed by MoveEvent
  score_board         # captured-piece cost totals, fed by CapturedPiece
bus/                  # layer-neutral infrastructure — knows no game concept
  event_bus           # generic pub/sub; any layer may depend on it (header-only)
input/                # GUI (input side) — no game rules
  board_mapper        # pixel <-> cell mapping (display-coupled)
  controller          # turns raw input into engine commands; owns the selection
io/                   # serialization — text in/out (not display, not rules)
  board_parser        # text -> board + commands
  board_printer       # board -> text
  piece_codec         # the single place letters <-> Kind/Color is encoded
  move_notation       # MoveEvent -> "Nxc6"
  text                # shared text tokens
view/                 # GUI (output side) — display only
  scene_translator    # the GUI<-Logic seam: engine snapshot -> GameSnapshot
  game_snapshot       # the flat, drawable description of one frame
  animator            # sprite animation state machine (display-only)
  animation_config    # + animation_config_store: graphics half of config.json
  asset_paths         # where a sprite for a piece/state lives
  render_config       # colors, cell size, panel sizes
  render_layout       # one layout answer shared by renderer and mapper
  renderer            # draws a GameSnapshot (graphics)
  image_view          # window + mouse loop (graphics)
  demo/render_demo    # renders a starting position to PNG; visual check only
texttests/            # scripted end-to-end test harness
  script_parser
  script_runner
third_party/          # vendored: doctest, img (Img + MouseWindow), opencv
assets/               # board and piece sprites
main.cpp              # composition root wiring the layers together

tests/
  test_main.cpp       # doctest entry point
  unit/               # Business Logic + seams, each unit tested in isolation
    test_position  test_piece  test_board  test_piece_rules  test_rule_engine
    test_real_time_arbiter  test_game_engine  test_event_bus
    test_move_log  test_score_board
    test_board_mapper  test_controller
    test_board_parser  test_board_printer  test_move_notation
    test_scene_translator  test_animator  test_render_layout
    test_script_parser
```

**Layer mapping:** `model` + `rules` + `realtime` + `engine` + `game_record` are Business Logic; `input` + `view` are the GUI. `io` is a serialization boundary, not a rules or display layer. `bus` is neutral infrastructure: it names no game concept, so every layer — including a future server — may depend on it, and it depends on none.

### How events flow

`GameEngine` exposes an `EventBus` (`engine.events()`) and publishes `model::MoveEvent` and `model::CapturedPiece` onto it. `MoveLog` and `ScoreBoard` subscribe. The subscription itself happens in `main.cpp` — the engine never learns who is listening, and a new listener (sound, end-of-game animation, a server relay) is a subscribe call and nothing else. Events carry domain data only: no notation string, no formatted clock. Turning a `MoveEvent` into `"Nxc6"` is `io::move_notation`'s job, at the boundary.

The bus has three properties worth knowing: the event type is written explicitly at the call site (`subscribe<MoveEvent>(...)`), there is no unsubscribe (a subscriber must outlive the bus), and it is not thread-safe — a networked server would need to revisit that.

## Build

MSVC is required. The vendored OpenCV under `third_party/opencv` is a prebuilt `/MD` MSVC binary with no `OpenCVConfig.cmake`, so it is wired by hand in `CMakeLists.txt` and MinGW cannot link it.

```
cmake -B build
cmake --build build
ctest --test-dir build     # run the unit tests
./build/kfc                # graphical play
./build/kfc --script       # text harness, reads a board + script from stdin
```

Targets, and why they are split:

- `kfc_lib` — all Business Logic + `input` + `io` + `texttests`. Globbed; no graphics.
- `kfc_view_core` — the pure half of `view`: the seam, the Animator, asset paths, the config loader. Links `kfc_lib`, **not** OpenCV, so the unit tests can link it.
- `kfc_view` — the drawing half: `Renderer`, `ImageView`, and the `Img`/`MouseWindow` wrapper. The only target that links OpenCV. Sources are listed explicitly, not globbed, because which side a file lands on is a design decision.
- `kfc` — the app; `render_demo` — the PNG visual check; `unit_tests` — the doctest suite.

The one-way dependency (`kfc_view` → `kfc_view_core`) makes "graphics only via `Img`" a build-enforced rule: a core source that reaches for `img.hpp` or `opencv2/` fails to compile, because neither include directory is on `kfc_view_core`'s path.

## Naming conventions

- **Files & directories:** snake_case, one module per name, paired `.hpp` / `.cpp` (header-only modules may omit the `.cpp` — e.g. `bus/event_bus`). Test files are `.cpp` only, named `test_<module>`.
- **Types** (classes, structs, enums): PascalCase — even though the filename is snake_case (`game_engine.hpp` → `class GameEngine`).
- **Enum values:** PascalCase (e.g. `Kind::Knight`, `MoveReason::Ok`).
- **Functions & variables:** camelCase; private members carry a trailing underscore (`board_`).
- **Constants:** camelCase, with no prefix — e.g. `squareTravelMs`, `msPerCell`, `defaultCellPx`. Do NOT use a `k` prefix (`kSquareTravelMs` is wrong).
- **Getters:** get-prefixed camelCase (`getKind()`, `getState()`); conversions are `toString()`. Board dimensions are `width()` / `height()`.
- **Namespaces:** `kfc` at the root, one nested namespace per module (`kfc::model`, `kfc::rules`, `kfc::realtime`, `kfc::engine`, `kfc::game_record`, `kfc::input`, `kfc::view`, `kfc::io`, `kfc::bus`, `kfc::texttests`). Business Logic namespaces must never `#include` from `input`/`view`.
- **Includes** are repo-root relative, including the sub-directory: `#include "model/include/piece.hpp"`.

**Module internal structure:** every architectural module is strictly divided into `include/` (all `.hpp`) and `src/` (all `.cpp`). Do not place `.hpp` or `.cpp` files directly in the root of a module folder.

## Code Quality Rules

Beyond layer separation, all code — Business Logic above all — must follow:

- **DRY** — every piece of logic/domain knowledge is implemented in exactly one place. If a fact (e.g. "which letters are valid piece types") is needed in two places, define it once and reference it from both (that fact lives in `io/piece_codec`); do not let two independent switch/if chains encode the same rule, since they will drift out of sync.
- **SRP** — every function does exactly one thing. A function that validates, parses, transforms, and mutates state in one body should be split into named steps, each doing one of those things.
- **No hardcoded constants or strings in Business Logic** — piece-type letters, color markers, the empty-cell token, board dimensions, cooldown durations, piece costs, etc. must live in named constants/enums/config, never as inline literals (`'K'`, `"."`, `'w'`, `100`) scattered through the logic.
- **Encapsulation** — classes and functions expose behavior, not internal representation. Don't return raw internal containers (e.g. a `vector<vector<string>>` board) from an accessor and let callers pattern-match against the encoding; expose purpose-built methods instead.

Treat a violation of any of these as a design defect to fix, not a style nit — review new/changed code against this list before considering a change complete.

## Testing strategy

The test framework is doctest — all unit tests under `tests/` are written with it, and `unit_tests` links `kfc_lib` + `kfc_view_core` (never OpenCV).

Each layer is verified by tests appropriate to it:

- **Unit Tests** — Business Logic in isolation.
- **GUI–Logic Unit Tests** — the seam between GUI and Logic (`test_scene_translator`, `test_board_mapper`, `test_controller`).
- **Integration Tests** — at the server level.
- **Acceptance Tests** — product requirements verified end-to-end (Logic + GUI).
- **System Tests** — the full system including the server.

When implementing a feature, add or update tests at the layer(s) it touches; a change to a game rule belongs in Business Logic and its unit tests, never validated only through the GUI.
