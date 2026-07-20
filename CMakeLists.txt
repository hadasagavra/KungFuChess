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
# ---------------------------------------------------------------------------
file(GLOB_RECURSE KFC_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/model/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/rules/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/realtime/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/engine/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/input/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/io/src/*.cpp"
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
    "${CMAKE_CURRENT_SOURCE_DIR}/view/src/animation_config_store.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/view/src/animator.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/view/src/asset_paths.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/view/src/scene_translator.cpp")

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
    "${CMAKE_CURRENT_SOURCE_DIR}/view/src/image_view.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/view/src/renderer.cpp"
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
target_link_libraries(kfc PRIVATE kfc_lib kfc_view)

# The prebuilt OpenCV is a DLL: drop it next to kfc.exe so the app runs from the
# build tree without needing OpenCV on PATH.
add_custom_command(TARGET kfc POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${OPENCV_DLL}" "$<TARGET_FILE_DIR:kfc>"
    VERBATIM)

# ---------------------------------------------------------------------------
# render_demo: a small driver that builds a starting-position GameSnapshot and
# renders it to a PNG. Visual check for the view layer; not part of the app.
# ---------------------------------------------------------------------------
add_executable(render_demo "${CMAKE_CURRENT_SOURCE_DIR}/view/demo/render_demo.cpp")
target_link_libraries(render_demo PRIVATE kfc_view)
add_custom_command(TARGET render_demo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${OPENCV_DLL}" "$<TARGET_FILE_DIR:render_demo>"
    VERBATIM)

# ---------------------------------------------------------------------------
# unit_tests: the doctest suite. test_main.cpp provides doctest's entry point;
# every tests/unit/*.cpp registers TEST_CASEs.
# ---------------------------------------------------------------------------
file(GLOB TEST_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/unit/*.cpp")

add_executable(unit_tests
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_main.cpp"
    ${TEST_SOURCES}
)
# kfc_view_core carries the pure view modules (the seam, the Animator, the asset
# paths, the config loader). Linking it here keeps them under unit test without
# dragging OpenCV into the test binary.
target_link_libraries(unit_tests PRIVATE kfc_lib kfc_view_core)

# doctest is vendored under third_party/. The tests include it two ways:
#   "third_party/doctest/doctest.h"  (resolved via the repo root, already set)
#   <doctest/doctest.h>              (resolved via third_party/)
target_include_directories(unit_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/third_party")

# ---------------------------------------------------------------------------
# CTest: `ctest --test-dir build` runs the whole suite.
# ---------------------------------------------------------------------------
enable_testing()
add_test(NAME unit_tests COMMAND unit_tests)
