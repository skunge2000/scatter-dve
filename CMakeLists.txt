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
#
# WU-04 adds src/video/chroma.cpp: same target, no new build-graph shape.
#
# WU-05 adds src/io/file_source.cpp and src/io/file_sink.cpp: raw .v210 file
# I/O, plain <fstream>, no Blackmagic SDK — despite living under src/io/,
# which docs/architecture.md section 8's module-layout sketch places under
# the `scatter` application target alongside the DeckLink-linking files
# arriving at WU-14. See ADR-021.
#
# WU-06 adds src/core/lattice.cpp: control lattice, Catmull-Rom expansion,
# analytic Jacobian. Portable — no platform dependency, same as every other
# src/core/ and src/video/ file — so it goes in scatter-core like the rest.
#
# WU-08 adds src/core/binner.cpp: fragment generation and tile binning.
# core/jacobian.hpp (WU-07) stayed header-only; this unit needs a .cpp of
# its own since TileBins carries per-tile storage state, not just pure
# functions of their arguments.
#
# WU-09 adds src/core/splat.cpp: the four-bank splat (architecture.md 4.5).
# Needs a .cpp of its own for the same reason binner.cpp does -- TileAccum
# carries per-tile bank storage state, not just pure functions of Frag
# arguments.
#
# WU-10 adds src/core/resolve.cpp (normalise, composite -- architecture.md
# 4.8) and src/core/pipeline.cpp (single-frame orchestration, this unit's
# own runFrame()/runFrameFile(), declared in core/resolve.hpp rather than a
# separate pipeline.hpp -- see that header and DECISIONS.md ADR-026).
# pipeline.cpp needs no header of its own to compile into scatter-core;
# every scatter-core object file is reachable from tests/test_zoneplate.cpp
# and any future src/app/ caller via #include "core/resolve.hpp" alone.
#
# WU-11 adds src/core/shapes/cylinder.cpp and src/core/shapes/sphere.cpp:
# the first shapes to populate a Lattice's control vertices from a curved
# surface rather than a plane (architecture.md 4.1, DECISIONS.md ADR-027).
# Portable, same as every other src/core/ file -- no platform dependency,
# so scatter-core again, not the scatter application target.
#
# WU-12a adds src/core/shapes/pageturn.cpp: the page-turn surface,
# transparent mode (architecture.md 4.1/4.7, DECISIONS.md ADR-028). Same
# portability as every other shapes/ file -- no core/splat.cpp or
# core/resolve.cpp changes needed; see ADR-028 and HANDOFF.md for why
# priority-tag opaque mode is a separate unit, WU-12b, not yet built.
#
# WU-12b adds compositeLayered() to the existing src/core/resolve.cpp
# (priority-tag opaque mode, architecture.md 4.7 phase 2, DECISIONS.md
# ADR-028/ADR-029) -- no new source file for scatter-core itself, only its
# own new test executable below.
#
# WU-13 adds morphLattice() to the existing src/core/lattice.cpp (keyframed
# lattices, temporal interpolation -- architecture.md 4.1's own "morph",
# DECISIONS.md ADR-030) -- no new source file for scatter-core itself,
# same "extend an existing file" shape as WU-12b, only its own new test
# executable below.
#
# WU-16a adds src/core/pipeline.hpp (new -- ADR-026 already anticipated its
# arrival at this unit) and extends the existing src/core/pipeline.cpp with
# a ThreadPool and a threaded PASS 2 (architecture.md section 6; DECISIONS.md
# ADR-040). Needs the platform thread library -- Threads::Threads, below --
# which no earlier unit has required.
#
# WU-16b extends src/core/binner.hpp/.cpp (a new row-range-aware
# generateFragmentsRowRange(), architecture.md section 6's own row-band
# parallelism; DECISIONS.md ADR-041) and src/core/pipeline.cpp again (PASS 1
# now row-band-parallel too, when PipelineParams::threads > 1) -- no new
# source file for scatter-core itself, only its own new test executable
# below (test_row_band).
#
# WU-17 extends the existing src/video/v210.hpp/.cpp with NEON-suffixed
# siblings of the scalar unpack/pack functions (DECISIONS.md ADR-042) --
# architecture.md section 8's own module-layout sketch already names this
# file pair "scalar reference + NEON", not a separate v210_neon.hpp/.cpp.
# Guarded by #if defined(__ARM_NEON) in both files, so scatter-core's object
# list and every existing test are completely unaffected on a non-NEON
# (x86_64) build -- no new source file for scatter-core itself, only its own
# new test executable below (test_v210_neon), itself gated at the CMake
# level (not by a runtime skip) since its whole body calls functions that do
# not exist to declare or define at all without __ARM_NEON.
#
# WU-18 extends the existing src/video/chroma.hpp/.cpp with NEON-suffixed
# siblings of the scalar upsample/downsample functions (DECISIONS.md
# ADR-043), same shape and same #if defined(__ARM_NEON) guard as WU-17 --
# no new source file for scatter-core itself, only its own new test
# executable below (test_chroma_neon), gated the same way test_v210_neon
# already is.
#
# WU-19a extends the existing src/core/resolve.hpp (one new
# PipelineParams::pool field, a non-owning ThreadPool*, default nullptr)
# and src/core/pipeline.cpp (the threaded PASS-1/PASS-2 body factored into
# a runThreaded() helper, callable against either a fresh per-call pool --
# WU-16a/16b's own unchanged behaviour -- or a caller-supplied persistent
# one, DECISIONS.md ADR-044) -- no new source file for scatter-core
# itself, only its own new test executable below (test_persistent_pool).
# ---------------------------------------------------------------------------
find_package(Threads REQUIRED)

add_library(scatter-core STATIC
    src/video/v210.cpp
    src/video/chroma.cpp
    src/io/file_source.cpp
    src/io/file_sink.cpp
    src/core/lattice.cpp
    src/core/binner.cpp
    src/core/splat.cpp
    src/core/resolve.cpp
    src/core/pipeline.cpp
    src/core/shapes/cylinder.cpp
    src/core/shapes/sphere.cpp
    src/core/shapes/pageturn.cpp)
target_include_directories(scatter-core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_features(scatter-core PUBLIC cxx_std_20)
target_link_libraries(scatter-core PUBLIC Threads::Threads)

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
# Tile size (Q1, settled — WU-19b, DECISIONS.md ADR-045)
#
#   cmake -B build -DSCATTER_TILE_LOG2=4     # 16x16,  32 KB across four banks
#   cmake -B build -DSCATTER_TILE_LOG2=5     # 32x32, 128 KB   (default, confirmed)
#
# WU-19b measured both, whole-pipeline, on the real M1 Max, across every
# worker count architecture.md 6 names: 2^5 (32x32) wins at all of them, by
# roughly 8-10%, not a one-off. Still both configurable here (nothing about
# the build breaks at 2^4), but 2^5 is this project's own settled choice
# going forward, not merely an unexamined default.
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
scatter_test(test_chroma)
scatter_test(test_ramp_roundtrip)
scatter_test(test_jacobian)
scatter_test(test_ewa)
scatter_test(test_binner)
scatter_test(test_splat)
scatter_test(test_zoneplate)
target_include_directories(test_zoneplate PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tools)
scatter_test(test_shapes)
scatter_test(test_pageturn)
scatter_test(test_layered_composite)
scatter_test(test_morph)
scatter_test(test_threading)
target_include_directories(test_threading PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tools)
scatter_test(test_row_band)
# WU-19a -- persistent, caller-owned ThreadPool (DECISIONS.md ADR-044).
# Needs tools/testpat.hpp the same way test_threading/test_zoneplate do
# (WarpedFrame is built from a zone plate).
scatter_test(test_persistent_pool)
target_include_directories(test_persistent_pool PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tools)

# ---------------------------------------------------------------------------
# test_v210_neon -- WU-17. The NEON-suffixed functions it calls are declared
# in src/video/v210.hpp only under #if defined(__ARM_NEON) (AArch64 always
# defines it; no -mfpu flag needed, unlike 32-bit ARM); this executable would
# therefore fail to *compile*, not just fail to run, on a processor this
# guard doesn't match, so it is gated here rather than left for a runtime
# skip -- the same "gate the whole executable at CMake level" shape
# BLACKMAGIC_SDK_DIR already uses below for test_decklink_device/_output
# (ADR-031), applied here for a portable-toolchain reason (a processor
# target) rather than a missing vendor SDK. Deliberately not folded into the
# neighbouring `if(APPLE AND CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")`
# -mcpu=apple-m1 block above: that block is Apple-specific tuning, unrelated
# to whether NEON intrinsics themselves are available, and this guard must
# also match a non-Apple AArch64 Linux build (e.g. a cross-compiled
# verification build, see ADR-042) that the APPLE-gated block never would.
#
# test_chroma_neon -- WU-18, same guard, same reasoning, reused rather than
# re-derived (DECISIONS.md ADR-043): chroma.hpp's own NEON-suffixed
# functions are equally declared only under #if defined(__ARM_NEON).
# ---------------------------------------------------------------------------
if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm64|aarch64)$")
    scatter_test(test_v210_neon)
    scatter_test(test_chroma_neon)
    message(STATUS "scatter-dve: CMAKE_SYSTEM_PROCESSOR=${CMAKE_SYSTEM_PROCESSOR} -- test_v210_neon, test_chroma_neon configured")
else()
    message(STATUS "scatter-dve: CMAKE_SYSTEM_PROCESSOR=${CMAKE_SYSTEM_PROCESSOR}, not arm64/aarch64 -- skipping test_v210_neon, test_chroma_neon (WU-17/WU-18's NEON paths are not declared at all without __ARM_NEON)")
endif()

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
# scatter — the application. Adds the DeckLink-linking parts of src/io/
# (com_ptr.hpp, decklink_*.cpp — WU-14) plus src/diag and src/app. file_source
# .cpp and file_sink.cpp are also physically under src/io/ but, having no SDK
# dependency, already compile into scatter-core above (ADR-021); this target
# will link scatter-core and add only the files that actually need the SDK.
#
# WU-14 adds the first of those files: src/io/com_ptr.hpp and
# src/io/decklink_device.hpp/.cpp (device enumeration only — architecture.md
# 7's entry point, no stream opened; DECISIONS.md ADR-031). No
# src/app/main.cpp yet, so there is no `scatter` executable to link yet —
# the block below builds a `scatter-decklink` static library and its own
# test executable instead, both gated on the Blackmagic SDK actually being
# found (see BLACKMAGIC_SDK_DIR below), since this cannot be built in the
# Linux cloud sandbox at all.
# ---------------------------------------------------------------------------
message(STATUS "scatter-dve: ${CMAKE_BUILD_TYPE}, tile 2^${SCATTER_TILE_LOG2}")

# ---------------------------------------------------------------------------
# scatter-decklink — WU-14. Requires the Blackmagic DeckLink SDK's own
# headers and its DeckLinkAPIDispatch.cpp (architecture.md section 7:
# "Compile DeckLinkAPIDispatch.cpp into the project; it loads
# DeckLinkAPI.bundle via CFPlugIn at runtime"), plus CoreFoundation.framework.
# No DeckLink dylib/framework is linked at build time — the SDK's own
# dispatch file CFBundle-loads the real implementation from
# /Library/Frameworks/DeckLinkAPI.framework at first call, at runtime, so
# only the SDK's *headers* (and that one dispatch .cpp) are a build-time
# dependency at all.
#
# BLACKMAGIC_SDK_DIR is a cache variable, not a hardcoded path — same
# pattern SCATTER_TILE_LOG2 already uses above, and for the same kind of
# reason: a path under the developer's own home directory, with a version
# number baked into its own folder name (liable to change on the next SDK
# update), has no business being committed into source, and the SDK's own
# EULA (shipped alongside its headers) is a further reason not to vendor it
# into this repository at all. Point it at the SDK root — the directory
# that itself contains Mac/, Win/, Linux/ — e.g.:
#
#   cmake -B build -DBLACKMAGIC_SDK_DIR="/Users/stephenneal/src/Blackmagic DeckLink SDK 16.0"
#
# (quoted: the folder name has a space in it). Left unset, or set to a path
# that does not contain Mac/include/DeckLinkAPI.h, this whole block is
# skipped with a STATUS message rather than a hard configure error —
# ADR-013/ADR-021's own "no unit leaves the tree unbuildable" property: the
# Linux cloud sandbox's existing scatter-core/test matrix must keep
# configuring and building with no SDK present at all, exactly as it always
# has for every unit through WU-13.
#
# See DECISIONS.md ADR-031 for the full design this compiles. UNVERIFIED as
# of WU-14's own session: drafted and written via the device bridge with no
# Blackmagic SDK and no AppleClang/Xcode toolchain available to check it
# against (the Linux cloud sandbox has neither) — needs building and running
# at the real terminal before this unit can go green. See HANDOFF.md.
# ---------------------------------------------------------------------------
set(BLACKMAGIC_SDK_DIR "" CACHE PATH
    "Root of the Blackmagic DeckLink SDK (the directory containing Mac/, Win/, Linux/). Empty skips all DeckLink-dependent targets.")

if(APPLE AND BLACKMAGIC_SDK_DIR AND EXISTS "${BLACKMAGIC_SDK_DIR}/Mac/include/DeckLinkAPI.h")
    add_library(scatter-decklink STATIC
        src/io/decklink_device.cpp
        src/io/decklink_output.cpp
        "${BLACKMAGIC_SDK_DIR}/Mac/include/DeckLinkAPIDispatch.cpp")
    target_include_directories(scatter-decklink PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        "${BLACKMAGIC_SDK_DIR}/Mac/include")
    target_compile_features(scatter-decklink PUBLIC cxx_std_20)
    target_link_libraries(scatter-decklink PUBLIC "-framework CoreFoundation")

    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(scatter-decklink PUBLIC
            -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror)
    endif()
    if(APPLE AND CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
        check_cxx_compiler_flag("-mcpu=apple-m1" HAVE_MCPU_APPLE_M1_DECKLINK)
        if(HAVE_MCPU_APPLE_M1_DECKLINK)
            target_compile_options(scatter-decklink PUBLIC -mcpu=apple-m1)
        endif()
    endif()

    # DeckLinkAPIDispatch.cpp is generated, third-party SDK code, not this
    # project's own — its warnings are not this project's to fix, so this
    # one file (and only this file) is exempted from the -Werror set above.
    # A trailing -w on the same file's own compile command overrides the
    # target-level -Werror that precedes it; standard for a vendored file
    # like this, but UNVERIFIED here — confirm the build actually treats it
    # this way rather than still failing on it.
    set_source_files_properties(
        "${BLACKMAGIC_SDK_DIR}/Mac/include/DeckLinkAPIDispatch.cpp"
        PROPERTIES COMPILE_OPTIONS "-w")

    add_executable(test_decklink_device tests/test_decklink_device.cpp)
    target_link_libraries(test_decklink_device PRIVATE scatter-decklink)
    target_include_directories(test_decklink_device PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests)
    add_test(NAME test_decklink_device COMMAND test_decklink_device)

    # WU-15a — test_decklink_output.cpp exercises both scatter-decklink
    # (LoopedFramePlayback, device enumeration) and scatter-core (runFrameFile,
    # buildCylinderLattice, testpat helpers, v210::rowBytesMin — building and
    # writing the real .v210 file it plays back), so it links both static
    # libraries, unlike test_decklink_device above. See DECISIONS.md ADR-032.
    add_executable(test_decklink_output tests/test_decklink_output.cpp)
    target_link_libraries(test_decklink_output PRIVATE scatter-decklink scatter-core)
    target_include_directories(test_decklink_output PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tools)
    add_test(NAME test_decklink_output COMMAND test_decklink_output)

    message(STATUS "scatter-dve: DeckLink SDK found at ${BLACKMAGIC_SDK_DIR} — scatter-decklink, test_decklink_device and test_decklink_output configured")
else()
    message(STATUS "scatter-dve: BLACKMAGIC_SDK_DIR not set (or not Apple, or SDK not found there) — skipping scatter-decklink and test_decklink_device. WU-14 needs this set to build.")
endif()
