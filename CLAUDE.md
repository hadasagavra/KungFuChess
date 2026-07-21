CLAUDE.md
This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

Status
The project is currently undergoing a major refactoring of the entire folder/directory structure, restructuring the codebase around explicit design patterns and the strict layer separation described below. Expect files and directories to be reorganized; when adding or moving code, follow the intended architecture and design-pattern conventions rather than the previous layout. The sections below describe the intended architecture and rules the implementation must follow.

The target layout (see Project Structure below) has been scaffolded with each module directory at the repository root, alongside tests/. During migration it coexists with the legacy Business Logic/ directories; new work goes into the new layout, and the legacy directories are deleted only once everything has been migrated across.

Game Overview
KungFuChess is a real-time, simultaneous chess variant — there are no turns. Both players move at the same time. Core rules that the logic must enforce:

After any move, a piece must rest (cooldown) before it can move again.
Winning = capturing the king. There is no concept of check or checkmate.
Extra piece commands: Jump (move + short pause).
Extra piece types beyond standard chess, e.g. Quadcopter: slower cooldown than other pieces, but can move to any square in the [±2, ±2] range.
Product features layered on top: moves log, score (sum of the "cost" of captured pieces), and player name display.
Architecture — the central constraint
The project is organized into three layers with strict separation. This is the single most important design rule and the primary thing the project is graded on — quality of design and layer separation, not merely "does it work."

Business Logic — the heart of the game, with zero dependency on display or networking: piece rules, movement, cooldown, capture logic, and win detection live here and nowhere else.
GUI — display only. Renders board state and collects input. Contains no game rules.
Server — the networking / coordination layer between players.
The guiding principle: Business Logic must be completely decoupled from GUI and Server. Never mix game rules into display or network code. If the layers are designed this way from the start, the test structure and separation fall into place almost automatically. When making changes, treat any leak of game rules into the GUI or Server layer as a design defect to be corrected, not accommodated.

A non-functional/aspirational goal is scalability toward millions of concurrent players. This is architectural pressure to keep layers cleanly separated and the server well-designed — not a mandate to actually implement at that scale.

Project Structure
The codebase is being migrated to the layout below. The source modules live at the repository root, each mapping onto one of the architectural layers; tests/ mirrors them. Add new code to the module that matches its responsibility — never widen a module's job to avoid creating the right one.

model/                # Business Logic — pure domain data & state
  position            # a board coordinate
  piece               # piece identity (type, color)
  board               # the grid of pieces + safe accessors
  game_state          # authoritative game state (no orchestration)
rules/                # Business Logic — the rules of the game
  piece_rules         # per-type movement/legality rules
  rule_engine         # orchestrates the rules into a legality decision
realtime/             # Business Logic — the real-time / simultaneous mechanics
  motion              # travel time, cooldown, jump durations
  real_time_arbiter   # pending moves/jumps, the clock, arrival resolution
engine/               # Business Logic — top-level coordination
  game_engine         # drives state + rules + realtime; the logic entry point
input/                # GUI (input side) — no game rules
  board_mapper        # pixel <-> cell mapping (display-coupled)
  controller          # turns raw input into engine commands
io/                   # serialization — board text in/out (not display, not rules)
  board_parser        # text -> board + commands
  board_printer       # board -> text
view/                 # GUI (output side) — display only
  renderer            # renders board state
  image_view          # graphical view
texttests/            # scripted end-to-end test harness
  script_parser
  script_runner
main.cpp              # composition root wiring the layers together

tests/
  unit/               # Business Logic + seams, each unit tested in isolation
    test_position  test_board  test_piece_rules  test_rule_engine
    test_real_time_arbiter  test_game_engine  test_board_mapper
    test_controller  test_board_parser  test_board_printer
Layer mapping: model + rules + realtime + engine are Business Logic; input + view are the GUI. io is a serialization boundary, not a rules or display layer.

Naming conventions
Files & directories: snake_case, one module per name, paired .hpp / .cpp (header-only modules may omit the .cpp). Test files are .cpp only, named test_<module>.
Types (classes, structs, enums): PascalCase.
Enum values: PascalCase (e.g. Kind::Knight, MoveReason::Ok).
Functions & variables: camelCase; private members carry a trailing underscore (board_).
Constants: camelCase, with no prefix — e.g. squareTravelMs, msPerCell, defaultCellPx. Do NOT use a k prefix (kSquareTravelMs is wrong).
Getters: get-prefixed camelCase (getKind(), getState()); conversions are toString(). Board dimensions are width() / height().
Namespaces: kfc at the root, one nested namespace per layer (kfc::model, kfc::rules, kfc::realtime, kfc::engine, kfc::input, kfc::view, kfc::io). Business Logic namespaces must never #include from input/view.
Module Internal Structure:
Every architectural module (e.g., model/, rules/, realtime/) must be strictly divided into two sub-directories:
1. `include/` - This will contain all `.hpp` header files for the module.
2. `src/` - This will contain all `.cpp` implementation files for the module.
Do not place .hpp or .cpp files directly in the root of the module folder.

Important C++ Naming Rules:

Files: use snake_case (e.g., game_engine.hpp).

Classes/Types inside those files: use PascalCase (e.g., class GameEngine).

Please ensure all classes follow the PascalCase convention, even if the filename is snake_case

Code Quality Rules
Beyond layer separation, all code — Business Logic above all — must follow:

DRY — every piece of logic/domain knowledge is implemented in exactly one place. If a fact (e.g. "which letters are valid piece types") is needed in two places, define it once and reference it from both; do not let two independent switch/if chains encode the same rule, since they will drift out of sync.
SRP — every function does exactly one thing. A function that validates, parses, transforms, and mutates state in one body should be split into named steps, each doing one of those things.
No hardcoded constants or strings in Business Logic — piece-type letters, color markers, the empty-cell token, board dimensions, cooldown durations, piece costs, etc. must live in named constants/enums/config, never as inline literals ('K', ".", 'w', 100) scattered through the logic.
Encapsulation — classes and functions expose behavior, not internal representation. Don't return raw internal containers (e.g. a vector<vector<string>> board) from an accessor and let callers pattern-match against the encoding; expose purpose-built methods instead.
Treat a violation of any of these as a design defect to fix, not a style nit — review new/changed code against this list before considering a change complete.

Testing strategy
The test framework is doctest — all unit tests under tests/ are written with it.

Each layer is verified by tests appropriate to it:

Unit Tests — Business Logic in isolation.
GUI–Logic Unit Tests — the seam between GUI and Logic.
Integration Tests — at the server level.
Acceptance Tests — product requirements verified end-to-end (Logic + GUI).
System Tests — the full system including the server.
When implementing a feature, add or update tests at the layer(s) it touches; a change to a game rule belongs in Business Logic and its unit tests, never validated only through the GUI.