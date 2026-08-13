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
# INTERFACE for now because WU-01 adds no translation units. WU-02 converts
# this to a STATIC library when v210.cpp arrives; the target name and the
# usage requirements below do not change.
# ---------------------------------------------------------------------------

add_library(scatter-core INTERFACE)
target_include_directories(scatter-core INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_features(scatter-core INTERFACE cxx_std_20)

# ---------------------------------------------------------------------------
# Warnings and architecture flags
#
# Warnings are errors. A warning in this codebase is usually a sign that an
# invariant is being bent — narrowing conversions in the colour path
# especially.
# ---------------------------------------------------------------------------

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(scatter-core INTERFACE
        -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror)
endif()

include(CheckCXXCompilerFlag)
if(APPLE AND CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
    check_cxx_compiler_flag("-mcpu=apple-m1" HAVE_MCPU_APPLE_M1)
    if(HAVE_MCPU_APPLE_M1)
        target_compile_options(scatter-core INTERFACE -mcpu=apple-m1)
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
target_compile_definitions(scatter-core INTERFACE
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

# ---------------------------------------------------------------------------
# scatter — the application. Adds src/io, src/diag, src/app and links
# DeckLink. Arrives at WU-14; nothing to build yet.
# ---------------------------------------------------------------------------

message(STATUS "scatter-dve: ${CMAKE_BUILD_TYPE}, tile 2^${SCATTER_TILE_LOG2}")
