# KungFuChess build.
#
#   cmake -B build            # configure (add -G "Ninja" or "MinGW Makefiles" if needed)
#   cmake --build build       # build the app (kfc) and the tests (unit_tests)
#   ctest --test-dir build    # run the unit tests
#   ./build/kfc               # run the app (reads a script from stdin)

cmake_minimum_required(VERSION 3.16)
project(KungFuChess LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---------------------------------------------------------------------------
# kfc_lib: all Business Logic + GUI + io + texttests sources (everything except
# main.cpp and the tests). The app and the tests both link this, so each source
# is compiled once.
#
# The sources are grouped by deployment target: shared/logic is the code a
# future server and the client both need, client/ is client-only.
# ---------------------------------------------------------------------------
file(GLOB_RECURSE KFC_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/shared/logic/model/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/shared/logic/rules/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/shared/logic/realtime/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/shared/logic/engine/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/shared/logic/game_record/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/shared/logic/io/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/client/input/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/texttests/src/*.cpp"
)

add_library(kfc_lib STATIC ${KFC_SOURCES})

# Repo root on the include path so headers resolve as "model/include/...".
# PUBLIC: propagates to the app and tests that link kfc_lib.
target_include_directories(kfc_lib PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

# Warnings for everything that links kfc_lib (library, app, and tests).
if(MSVC)
    target_compile_options(kfc_lib PUBLIC /W4)
else()
    target_compile_options(kfc_lib PUBLIC -Wall -Wextra)
endif()

# On MinGW/MSYS2, statically link the GCC/libstdc++ runtimes so the produced
# .exe runs standalone (no missing libstdc++-6.dll / libwinpthread-1.dll).
if(WIN32 AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_link_options(kfc_lib PUBLIC -static -static-libgcc -static-libstdc++)
endif()

# ---------------------------------------------------------------------------
# kfc_websocket: the vendored, header-only WebSocket stack -- standalone asio
# (third_party/asio) under websocketpp (third_party/websocketpp). An INTERFACE
# library: it carries the include dirs and the defines, added as SYSTEM includes
# so their warnings stay out of our /W4. Only the .cpp wrappers that actually
# speak WebSocket link it (PRIVATE), so the heavy headers never leak into a
# module's own headers or into the tests. On Windows asio needs Winsock.
# ---------------------------------------------------------------------------
add_library(kfc_websocket INTERFACE)
target_include_directories(kfc_websocket SYSTEM INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/asio/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/websocketpp")
target_compile_definitions(kfc_websocket INTERFACE
    ASIO_STANDALONE
    _WEBSOCKETPP_CPP11_STRICT_
    _WIN32_WINNT=0x0601)
if(WIN32)
    target_link_libraries(kfc_websocket INTERFACE ws2_32)
endif()

# ---------------------------------------------------------------------------
# kfc_server_lib: the Server layer -- the networking/coordination code. It links
# kfc_lib (it drives a GameEngine and speaks the io wire protocol) and NEVER
# OpenCV: a server has no display. Kept a separate target so the tests can link
# the session without pulling in the app's socket main.
#
# Globbed like kfc_lib. server/net today is header-only (the transport interface),
# so the glob picks up server/app's sources; the pattern is ready for when the
# net side gains a .cpp (the WebSocket transport).
# ---------------------------------------------------------------------------
file(GLOB_RECURSE KFC_SERVER_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/server/net/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/server/app/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/server/store/src/*.cpp"
)
# server_main.cpp is the server executable's composition root, not library code:
# keep its main() out of the lib the tests link (mirrors main.cpp vs kfc_lib).
list(REMOVE_ITEM KFC_SERVER_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/server/app/src/server_main.cpp")

add_library(kfc_server_lib STATIC ${KFC_SERVER_SOURCES})
# kfc_websocket is PRIVATE: only websocket_server.cpp includes the WebSocket
# headers, so they compile into this lib but are invisible to anything that
# links it (its own headers stay clean via pimpl). winsqlite3 is the SQLite that
# ships with Windows (header + import lib from the Windows SDK); only
# sqlite_user_store.cpp touches it, so it too stays behind a pimpl. Both are
# PRIVATE, but a static lib cannot resolve its own externals, so consumers link
# them transitively -- harmless system libraries either way.
target_link_libraries(kfc_server_lib PUBLIC kfc_lib PRIVATE kfc_websocket winsqlite3)

# ---------------------------------------------------------------------------
# kfc_client_net: the client's networking runtime -- RemoteGame (a client's view
# of the game), the ClientGame seam the frame loop drives, and the loopback that
# hosts a GameSession in-process for local play. It links kfc_server_lib because
# loopback IS the server running inside the client; no graphics, so the tests can
# link it. Globbed.
# ---------------------------------------------------------------------------
file(GLOB_RECURSE KFC_CLIENT_NET_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/client/net/src/*.cpp"
)

add_library(kfc_client_net STATIC ${KFC_CLIENT_NET_SOURCES})
# Same as the server lib: only websocket_client.cpp sees the WebSocket headers.
target_link_libraries(kfc_client_net PUBLIC kfc_lib kfc_server_lib
                      PRIVATE kfc_websocket)

# ---------------------------------------------------------------------------
# OpenCV: a vendored, prebuilt MSVC /MD build under third_party/opencv. It ships
# no OpenCVConfig.cmake, so find_package won't work -- we wire include/lib/dll by
# hand. ONLY the view layer links OpenCV; the Business Logic never sees it.
# Because the prebuilt is /MD, the whole project must be built with MSVC (the
# CMake default MSVC runtime is /MD[d], which matches). MinGW cannot link it.
# ---------------------------------------------------------------------------
set(OPENCV_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/opencv/OpenCV_451")
set(OPENCV_INCLUDE_DIR "${OPENCV_DIR}/include")
# Debug links the *d libs/dll (also /MDd); every other config links release.
set(OPENCV_LIB "$<IF:$<CONFIG:Debug>,${OPENCV_DIR}/bin/opencv_world451d.lib,${OPENCV_DIR}/bin/opencv_world451.lib>")
set(OPENCV_DLL "$<IF:$<CONFIG:Debug>,${OPENCV_DIR}/bin/opencv_world451d.dll,${OPENCV_DIR}/bin/opencv_world451.dll>")

# ---------------------------------------------------------------------------
# The view/ module is one architectural layer but splits into two build targets,
# because only half of it needs the graphics stack:
#
#   kfc_view_core -- the pure half: the GUI<-Logic seam, the animation state
#                    machine, the asset-path scheme, the config loader. No
#                    OpenCV, no Img, so the unit tests can link it directly.
#   kfc_view      -- the drawing half: Renderer + ImageView + the Img/MouseWindow
#                    wrapper. The ONLY target that links OpenCV.
#
# The dependency runs one way (kfc_view -> kfc_view_core), so the graphics stack
# is unreachable from the core and from the tests. That makes "graphics only via
# Img, never in the seam" a build-enforced rule rather than a convention: a core
# source that reaches for img.hpp or opencv2/ fails to compile, since neither
# include directory is on kfc_view_core's path.
#
# Sources are listed explicitly rather than globbed: which side a file lands on
# is a design decision about whether it may touch graphics, so it is stated here
# instead of being inferred from a wildcard.
# ---------------------------------------------------------------------------
add_library(kfc_view_core STATIC
    "${CMAKE_CURRENT_SOURCE_DIR}/client/view/src/animation_config_store.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/client/view/src/animator.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/client/view/src/asset_paths.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/client/view/src/lobby_view.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/client/view/src/render_layout.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/client/view/src/scene_translator.cpp")

# kfc_lib supplies the domain types a GameSnapshot references (Piece, Position,
# the engine snapshot the seam reads). Nothing graphical is added here -- that
# omission is the point of this target.
target_link_libraries(kfc_view_core PUBLIC kfc_lib)

# ---------------------------------------------------------------------------
# kfc_view: the drawing half, plus the OpenCV wrapper (Img bitmap + MouseWindow
# window/input). OpenCV's headers are added as SYSTEM includes so their warnings
# stay out of our /W4.
# ---------------------------------------------------------------------------
add_library(kfc_view STATIC
    "${CMAKE_CURRENT_SOURCE_DIR}/client/view/src/image_view.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/client/view/src/renderer.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/img/img.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/img/mouse_window.cpp")

target_link_libraries(kfc_view PUBLIC kfc_view_core "${OPENCV_LIB}")
target_include_directories(kfc_view PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/img")
target_include_directories(kfc_view SYSTEM PUBLIC "${OPENCV_INCLUDE_DIR}")

# ---------------------------------------------------------------------------
# kfc: the application (composition root).
# ---------------------------------------------------------------------------
add_executable(kfc "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp")
target_link_libraries(kfc PRIVATE kfc_lib kfc_view kfc_client_net)

# The prebuilt OpenCV is a DLL: drop it next to kfc.exe so the app runs from the
# build tree without needing OpenCV on PATH.
add_custom_command(TARGET kfc POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${OPENCV_DLL}" "$<TARGET_FILE_DIR:kfc>"
    VERBATIM)

# ---------------------------------------------------------------------------
# kfc_server: the server application (composition root). No window, no sprites,
# no OpenCV -- the real test that the Server layer needs none of the GUI. It
# links the server lib (and, transitively, Winsock); the WebSocket headers are
# reached only inside kfc_server_lib.
# ---------------------------------------------------------------------------
add_executable(kfc_server "${CMAKE_CURRENT_SOURCE_DIR}/server/app/src/server_main.cpp")
target_link_libraries(kfc_server PRIVATE kfc_server_lib)

# ---------------------------------------------------------------------------
# render_demo: a small driver that builds a starting-position GameSnapshot and
# renders it to a PNG. Visual check for the view layer; not part of the app.
# ---------------------------------------------------------------------------
add_executable(render_demo "${CMAKE_CURRENT_SOURCE_DIR}/client/view/demo/render_demo.cpp")
target_link_libraries(render_demo PRIVATE kfc_view)
add_custom_command(TARGET render_demo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${OPENCV_DLL}" "$<TARGET_FILE_DIR:render_demo>"
    VERBATIM)

# ---------------------------------------------------------------------------
# unit_tests: the doctest suite. test_main.cpp provides doctest's entry point;
# every tests/unit/*.cpp registers TEST_CASEs.
# ---------------------------------------------------------------------------
# Unit tests plus the server-level integration tests (tests/integration): both
# are doctest cases, so one binary runs them all. The folders keep the levels
# apart -- unit tests a module in isolation, integration drives a GameSession.
file(GLOB TEST_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/unit/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/integration/*.cpp")

add_executable(unit_tests
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_main.cpp"
    ${TEST_SOURCES}
)
# kfc_view_core carries the pure view modules (the seam, the Animator, the asset
# paths, the config loader). kfc_server_lib carries the GameSession and
# kfc_client_net the RemoteGame/loopback the integration tests drive. None drags
# OpenCV into the test binary.
target_link_libraries(unit_tests PRIVATE kfc_lib kfc_view_core kfc_server_lib
                      kfc_client_net)

# doctest is vendored under third_party/. The tests include it two ways:
#   "third_party/doctest/doctest.h"  (resolved via the repo root, already set)
#   <doctest/doctest.h>              (resolved via third_party/)
target_include_directories(unit_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/third_party")

# ---------------------------------------------------------------------------
# CTest: `ctest --test-dir build` runs the whole suite.
# ---------------------------------------------------------------------------
# ---------------------------------------------------------------------------
# socket_tests: the one test that opens a real WebSocket (server + client in this
# process over localhost). It lives in its own binary and its own CTest entry so
# its timing and its need to bind a port never destabilise the deterministic
# unit suite -- run or skip it on its own. Reuses doctest's entry point.
# ---------------------------------------------------------------------------
file(GLOB SOCKET_TEST_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/socket/*.cpp")

add_executable(socket_tests
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_main.cpp"
    ${SOCKET_TEST_SOURCES}
)
target_link_libraries(socket_tests PRIVATE kfc_server_lib kfc_client_net)
target_include_directories(socket_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/third_party")

# ---------------------------------------------------------------------------
# CTest: `ctest --test-dir build` runs the whole suite.
# ---------------------------------------------------------------------------
enable_testing()
add_test(NAME unit_tests COMMAND unit_tests)
add_test(NAME socket_tests COMMAND socket_tests)
set_tests_properties(socket_tests PROPERTIES LABELS socket TIMEOUT 60)
