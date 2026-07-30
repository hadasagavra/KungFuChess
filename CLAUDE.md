# CLAUDE.md

Guidance for Claude Code (claude.ai/code) when working in this repository.

Part 1 (**Orientation**) describes what the project *is*. Part 2 (**Guidelines**) states how to
extend it. Read Orientation to find your way around; follow Guidelines when writing code.

---

# Part 1 — Orientation

## The game

KungFuChess is a real-time, simultaneous chess variant — no turns, both players move at once. The
rules the logic enforces: after any move a piece must rest (cooldown) before moving again; winning
is capturing the king, with no check and no checkmate; and one extra command, Jump (move + short
pause), which travels the wire as a move with `from == to`. Layered on top: moves log, score (sum of
the cost of captured pieces), player names, Elo ratings.

The Quadcopter piece (slower cooldown, may move anywhere in `[±2, ±2]`) is specified but **not
implemented** — `model::Kind` holds the six standard kinds only and there are no sprites. Its letter
in `io::piece_codec` cannot be `'Q'`; the queen already has it.

## The three layers

Layer separation is the most important design rule here, and the primary thing the project is graded
on — quality of design, not merely "does it work."

- **Business Logic** — piece rules, movement, cooldown, capture, win detection. Zero dependency on
  display or networking. These rules live here and nowhere else.
- **GUI** — display only. Renders board state, collects input, holds no game rules.
- **Server** — networking and coordination. Owns the authoritative engine, seats clients, routes
  messages. Holds no game rules either: it decides *may this client move that colour*, never *is that
  move legal*.

A leak of game rules into display or network code is a design defect to correct, not accommodate.
Scalability toward very large player counts is aspirational — architectural pressure to keep the
layers clean, not a mandate to implement at that scale.

## Directory structure

Grouped by **deployment target** (`shared/`, `client/`, `server/`), and within that by module, each
module mapping onto one layer. The two axes are independent and both must hold: the target says *who
ships this code*, the layer says *what it may know*. `shared/` is not a synonym for Business Logic —
`bus` and `log` are shared and are neither — and being client-only never licenses a game rule.

```
shared/                      # what a client and a server both need
  error.hpp  frame_step.hpp  # kfc::Error; clampedStepMs (the clamped frame step)
  bus/event_bus              # generic pub/sub, names no game concept (header-only)
  log/logger                 # timestamped, mutex-guarded writes (header-only)
  logic/                     # Business Logic — the whole of it
    model/      position piece board board_errors game_state
                game_event         # MoveEvent / CapturedPiece
    rules/      piece_rules        # per-kind movement strategies + ruleFor(Kind)
                rule_engine        # the rules -> one legality decision
    realtime/   motion             # travel time, cooldown, jump durations
                real_time_arbiter  # pending moves, the clock, arrival resolution
    engine/     game_engine        # the logic entry point; owns an EventBus
    game_record/ move_log score_board rating   # listeners; rating = Elo
    io/         board_parser board_printer piece_codec   # piece_codec: letters <-> Kind/Color
                move_notation      # MoveEvent -> "Nxc6"
                command_notation   # PlayerCommand <-> "WQe2e5"
                event_codec state_codec text
                wire_message       # the WireMessage variant + the encode/decode seam
client/                      # the GUI plus the client's networking
  app/     game_app          # the frame loop: Home screen vs board (graphics)
           lobby_controller cli script_mode      # all pure
           client_app  src/client_main.cpp       # runClient; executable root (a shim)
  input/   board_mapper game_access local_game_access controller
  net/     lobby_access client_game    # the two seams the frame loop drives
           remote_game       # replica-backed view; sends commands, applies state
           endpoint  loopback_transport  loopback_game   # loopback hosts a session
           websocket_client  networked_game             # the same seam over a socket
  view/    scene_translator  # the GUI<-Logic seam: engine snapshot -> GameSnapshot
           game_snapshot animator animation_config animation_config_store
           asset_paths render_config render_layout lobby_view
           renderer image_view demo/render_demo         # graphics
  assets/                    # board and piece sprites
server/
  net/     message_transport # abstract send/broadcast seam (header-only)
           websocket_server  # MessageTransport over a real WebSocket (pimpl)
  app/     game_session      # authoritative host: owns the engine, seats by colour
           seat_table rating_settler                    # one Occupant/client; Elo policy
           room room_manager room_transport matchmaker  # the lobby
           src/server_main.cpp                          # root — a RoomManager loop
  store/   user_store in_memory_user_store sqlite_user_store password_hasher
texttests/         script_parser script_runner   # the --script harness
config/            start_position.txt            # loaded by both roots
third_party/       doctest, img (Img + MouseWindow), opencv, asio + websocketpp
tests/             test_main.cpp + unit/ integration/ socket/
```

**Layer mapping.** Everything under `shared/logic/` is Business Logic; `client/input` + `client/view`
are the GUI. `client/net` is the client's networking runtime, the counterpart to `server/` — it holds
no game rules, keeping a replica the server fills and reusing `RuleEngine` only to hint legal-move
highlights. `shared/logic/io` is a serialization boundary. `shared/bus` and `shared/log` are neutral
infrastructure: they name no game concept, so every layer may depend on them and they depend on none.

## How events flow

`GameEngine` exposes an `EventBus` (`engine.events()`) and publishes `model::MoveEvent` and
`model::CapturedPiece` onto it; `MoveLog` and `ScoreBoard` subscribe. Subscription happens in the
orchestration layer — `GameApp` on the client, `GameSession` on the server — so the engine never
learns who is listening and a new listener costs one `subscribe` call. Events carry domain data only:
turning a `MoveEvent` into `"Nxc6"` is `io::move_notation`'s job, at the boundary.

Three properties of the bus: the event type is explicit at the call site (`subscribe<MoveEvent>`),
there is **no unsubscribe** (a subscriber must outlive the bus), and it is **not thread-safe**.

## Build

MSVC is required. The vendored OpenCV under `third_party/opencv` is a prebuilt `/MD` MSVC binary with
no `OpenCVConfig.cmake`, so it is wired by hand in `CMakeLists.txt`; MinGW cannot link it.

```
cmake -B build
cmake --build build
ctest --test-dir build          # unit_tests + socket_tests
./build/kfc                     # graphical play (local, over an in-process loopback)
./build/kfc_server [port]       # run a server (default port 9000)
./build/kfc --connect host:port # play against a server (default 127.0.0.1:9000)
./build/kfc --script            # text harness, reads a board + script from stdin
```

`kfc` and `render_demo` resolve `client/assets` relative to the working directory — run them from the
repository root, or pass an assets root as the first argument.

| Target | Sources | Links | OpenCV |
|---|---|---|---|
| `kfc_lib` | globbed: `shared/logic/*`, `client/input`, `texttests` | — | no |
| `kfc_websocket` | INTERFACE: vendored asio + websocketpp, defines, Winsock | — | no |
| `kfc_server_lib` | globbed: `server/{net,app,store}` minus `server_main.cpp` | `kfc_lib`; PRIVATE `kfc_websocket`, `winsqlite3` | no |
| `kfc_client_net` | globbed: `client/net` | `kfc_lib`, `kfc_server_lib`; PRIVATE `kfc_websocket` | no |
| `kfc_view_core` | explicit: seam, animator, asset paths, config store, layout, lobby_view | `kfc_lib` | no |
| `kfc_view` | explicit: `renderer`, `image_view`, `third_party/img` | `kfc_view_core` | **yes** |
| `kfc_app_core` | explicit: `cli`, `lobby_controller`, `script_mode` | `kfc_client_net`, `kfc_view_core` | no |
| `kfc_app` | explicit: `client_app`, `game_app` | `kfc_app_core`, `kfc_view` | via `kfc_view` |
| `kfc` | `client/app/src/client_main.cpp` | `kfc_app` | via `kfc_app` |
| `kfc_server` | `server/app/src/server_main.cpp` | `kfc_server_lib` | no |
| `render_demo` | `client/view/demo/render_demo.cpp` | `kfc_view` | via `kfc_view` |
| `unit_tests` | `test_main.cpp` + globbed `tests/{unit,integration}` | `kfc_lib`, `kfc_view_core`, `kfc_server_lib`, `kfc_client_net`, `kfc_app_core` | no |
| `socket_tests` | `test_main.cpp` + globbed `tests/socket` | `kfc_server_lib`, `kfc_client_net` | no |

Two rules the split encodes. **The core→graphics dependency runs one way** (`kfc_view` → `kfc_view_core`,
`kfc_app` → `kfc_app_core`), so "graphics only via `Img`" is build-enforced: a core source that reaches
for `opencv2/` fails to compile because the OpenCV include directory is on `kfc_view` alone. **Split
targets list their sources explicitly** rather than globbing, because which half a file lands on is a
design decision about whether it may touch graphics.

---

# Part 2 — Guidelines

## Naming and structure

- **Files & directories:** snake_case, one module per name, paired `.hpp`/`.cpp` (header-only modules
  may omit the `.cpp`). Test files are `.cpp` only, named `test_<module>`.
- **Module layout:** every module is strictly `include/` (all `.hpp`) + `src/` (all `.cpp`). Never put
  a `.hpp` or `.cpp` in a module folder's root.
- **Types** (classes, structs, enums) and **enum values:** PascalCase, even though the filename is
  snake_case — `game_engine.hpp` declares `class GameEngine`; `Kind::Knight`, `MoveReason::Ok`.
- **Functions & variables:** camelCase; private members carry a trailing underscore (`board_`).
- **Constants:** camelCase with **no prefix** — `squareTravelMs`, `msPerCell`, `defaultCellPx`. Never
  a `k` prefix; `kSquareTravelMs` is wrong.
- **Getters:** get-prefixed camelCase (`getKind()`, `getState()`); conversions are `toString()`; board
  dimensions are `width()`/`height()`.
- **Namespaces:** `kfc` at the root, one nested namespace per module — `model`, `rules`, `realtime`,
  `engine`, `game_record`, `io`, `bus`, `log`, `input`, `view`, `net`, `app`, `server`, `texttests`.
  `kfc::server` covers all three server sub-modules; `kfc::net` is the *client's* networking.
  Business Logic namespaces must never `#include` from `input`/`view`/`net`/`server`.
- **Includes** are repo-root relative and spell the full path through `include/`:
  `#include "shared/logic/model/include/piece.hpp"`. Never a relative `../` path.

## Extension directives

Find the row for what you are about to do. Each names the principle it enforces, so the instruction
and its reason travel together. The "not this" column is the failure mode to avoid.

| When you need to… | Principle / Pattern | Do this | Not this |
|---|---|---|---|
| Add a piece kind | Strategy + OCP | Subclass `rules::PieceRules`, add the `Kind`, add a `ruleFor` case | A movement `if` chain outside `rules/` |
| Add a message on the wire (either direction) | Command + trait specialization (OCP) | Struct in `wire_message.hpp` → add to the `WireMessage` variant → specialize `WireCodec<T>` in `wire_message.cpp` → add a `std::visit` arm in `RoomManager::handleMessage` (client→server) or `RemoteGame::receive` (server→client) | An encode switch plus a matching decode switch; or passing a raw string and re-parsing it downstream |
| React to a game event | Observer | `engine.events().subscribe<E>(...)` in the orchestration layer | Give the engine a pointer to the listener |
| Add a server responsibility | SRP (extract collaborator) | A new collaborator alongside `SeatTable`/`Matchmaker`/`RatingSettler` | Another member and method on `GameSession` |
| Track per-client data | SRP (whole-value object) | Extend `SeatTable::Occupant` | A second `map<ClientId, …>` kept in sync by hand |
| Depend on I/O — socket, DB, clock | DIP | Take an interface reference: `MessageTransport`, `UserStore`, or time injected via `tick(deltaMs)` | Construct the concrete socket or DB inside the class |
| Wrap a third-party header | Pimpl | `struct Impl` + `unique_ptr` + out-of-line destructor, as `WebSocketServer`/`SqliteUserStore` do | Let vendored headers into a public header |
| Add a GUI seam | Adapter | A pure function or DTO in `view/` — see `buildSnapshot`, `GameSnapshot` | Read engine internals from `Renderer` |
| Give the GUI a narrower view | ISP | A new seam beside `GameAccess`/`LobbyAccess`; `ClientGame` may inherit both | Widen `GameAccess` for one caller |
| Swap local play for networked | LSP | Implement `ClientGame`, as `LoopbackGame`/`NetworkedGame` do | Branch on "am I online?" inside the frame loop |
| Change a scoring or matching rule | Policy object | Edit `RatingSettler`/`Matchmaker`; keep the formula in `game_record/rating.hpp` | Inline the rule at its call site |
| Scope a broadcast to a subset | Decorator | Wrap `MessageTransport`, as `RoomTransport` does | Filter recipients inside `GameSession` |
| Construct a per-room `Board` | Factory | Call the injected `RoomManager::BoardFactory` | Copy a `Board` — it shares its `shared_ptr` pieces |
| Add runtime logic to a root | SRP (thin composition root) | Put it in `client/app` or `server/app` and unit-test it | Grow `client_main.cpp`/`server_main.cpp` past a shim |
| State a domain fact needed in two places | DRY (single source of truth) | Define it once — `piece_codec` for letters, `rating.hpp` for Elo — and reference it | Two switch chains that must agree |
| Draw anything | Layer separation (facade over `Img`) | Go through `Img`/`MouseWindow`, in `kfc_view` | Include `opencv2/` from a `*_core` target |

## Code quality rules

Review new and changed code against these before considering a change complete. A violation is a
design defect to fix, not a style nit.

- **DRY** — every fact is implemented in exactly one place. Two independent switch/if chains encoding
  the same rule will drift out of sync.
- **SRP** — every function does one thing. A body that validates, parses, transforms, and mutates
  should be split into named steps. The same applies to classes: reach for a collaborator before a
  fourth member.
- **No hardcoded constants or strings in Business Logic** — piece letters, colour markers, the
  empty-cell token, board dimensions, cooldowns, piece costs all live in named constants, enums, or
  config; never as inline `'K'`, `"."`, `'w'`, `100`.
- **Encapsulation** — expose behavior, not representation. Don't return a raw internal container and
  let callers pattern-match against the encoding.

## Testing

doctest is the framework. `unit_tests` links no OpenCV, so anything it covers must be reachable
without graphics.

- `tests/unit/` — Business Logic in isolation, **and** the GUI↔Logic seams (`test_scene_translator`,
  `test_board_mapper`, `test_controller`, `test_lobby_controller`).
- `tests/integration/` — server and client-runtime level over a fake or loopback transport
  (`test_game_session`, `test_room_manager`, `test_loopback_game`, `test_remote_game`,
  `test_sqlite_user_store`).
- `tests/socket/` — the full stack over a real socket. Its own binary, `socket_tests`, with its own
  CTest entry (`LABELS socket`, `TIMEOUT 60`) so its timing never destabilises the deterministic suite.

Add or update tests at the layer a change touches. **A change to a game rule is tested in Business
Logic, never only through the GUI.**

`texttests/` is the `--script` end-to-end harness (`script_parser`, `script_runner`), driven by
`kfc --script`. It ships no checked-in script fixtures.
