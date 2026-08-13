cmake_minimum_required(VERSION 3.24)
project(scatter-dve LANGUAGES CXX)

# ---------------------------------------------------------------------------
# Standard and defaults
# ---------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# ---------------------------------------------------------------------------
# scatter-core — portable core. Links no Blackmagic SDK, so the bulk of the
# test suite runs with no hardware connected and no driver installed.
#
# STATIC as of WU-02, which brings the first translation unit. The target name
# and the usage requirements are unchanged from WU-01; only the keyword on each
# of them moved from INTERFACE to PUBLIC.
# ---------------------------------------------------------------------------
add_library(scatter-core STATIC
    src/video/v210.cpp)

target_include_directories(scatter-core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_features(scatter-core PUBLIC cxx_std_20)

# ---------------------------------------------------------------------------
# Warnings and architecture flags
#
# Warnings are errors. A warning in this codebase is usually a sign that an
# invariant is being bent — narrowing conversions in the colour path
# especially.
# ---------------------------------------------------------------------------
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(scatter-core PUBLIC
        -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror)
endif()

include(CheckCXXCompilerFlag)
if(APPLE AND CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
    check_cxx_compiler_flag("-mcpu=apple-m1" HAVE_MCPU_APPLE_M1)
    if(HAVE_MCPU_APPLE_M1)
        target_compile_options(scatter-core PUBLIC -mcpu=apple-m1)
    endif()
endif()

# ---------------------------------------------------------------------------
# Tile size (Q1, open)
#
#   cmake -B build -DSCATTER_TILE_LOG2=4     # 16x16,  32 KB across four banks
#   cmake -B build -DSCATTER_TILE_LOG2=5     # 32x32, 128 KB   (default)
#
# WU-09 benchmarks both. Do not resolve Q1 before then.
# ---------------------------------------------------------------------------
set(SCATTER_TILE_LOG2 "5" CACHE STRING "log2 of tile edge in pixels (4 or 5)")
target_compile_definitions(scatter-core PUBLIC
    SCATTER_TILE_LOG2=${SCATTER_TILE_LOG2})

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------
include(CTest)
enable_testing()

function(scatter_test name)
    add_executable(${name} tests/${name}.cpp ${ARGN})
    target_link_libraries(${name} PRIVATE scatter-core)
    target_include_directories(${name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests)
    add_test(NAME ${name} COMMAND ${name})
endfunction()

scatter_test(test_smoke)
scatter_test(test_v210)

# ---------------------------------------------------------------------------
# make_testpat — WU-03. First executable that is not a test: it writes raw
# .v210 test patterns to disk rather than asserting anything itself, so it
# is its own add_executable rather than going through scatter_test(). Its
# pattern-generation logic lives in tools/testpat.hpp (header-only) so that
# tests/test_testpat.cpp can exercise it directly without linking this
# executable's translation unit.
# ---------------------------------------------------------------------------
add_executable(make_testpat tools/make_testpat.cpp)
target_link_libraries(make_testpat PRIVATE scatter-core)

scatter_test(test_testpat)
target_include_directories(test_testpat PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tools)

# ---------------------------------------------------------------------------
# scatter — the application. Adds src/io, src/diag, src/app and links
# DeckLink. Arrives at WU-14; nothing to build yet.
# ---------------------------------------------------------------------------

message(STATUS "scatter-dve: ${CMAKE_BUILD_TYPE}, tile 2^${SCATTER_TILE_LOG2}")
