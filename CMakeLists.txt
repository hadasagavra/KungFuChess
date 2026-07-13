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
# kfc: the application (composition root).
# ---------------------------------------------------------------------------
add_executable(kfc "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp")
target_link_libraries(kfc PRIVATE kfc_lib)

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
target_link_libraries(unit_tests PRIVATE kfc_lib)

# doctest is vendored under third_party/. The tests include it two ways:
#   "third_party/doctest/doctest.h"  (resolved via the repo root, already set)
#   <doctest/doctest.h>              (resolved via third_party/)
target_include_directories(unit_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/third_party")

# ---------------------------------------------------------------------------
# CTest: `ctest --test-dir build` runs the whole suite.
# ---------------------------------------------------------------------------
enable_testing()
add_test(NAME unit_tests COMMAND unit_tests)
