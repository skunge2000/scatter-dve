# Handoff

Overwritten at the end of every session. This is the first thing to read.

---

**Session:** 3
**Tag:** `wu-03-green` (pending — see "Still to do" below)
**Phase:** 1 — portable core, file to file, 576p25, single-threaded

**Tests:** Not yet run on the M1 Max. Verified in a Linux sandbox instead —
Clang 18 and GCC 13, both under the project's exact warning set (`-Wall
-Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror`), Release and
Debug, `SCATTER_TILE_LOG2` 4 and 5. All four configurations built clean and
`ctest` passed all three suites: `test_smoke` 2076 checks, `test_v210` 635
checks, `test_testpat` 10995 checks (unchanged for the first two — WU-03
touched neither). Not run under ASan/UBSan anywhere; that check remains
yours, as it always has been, since it's not wired into `CMakeLists.txt`.
**Run `./tools/close.sh 03` to get the authoritative result and tag.**

**Build:** clean under `-Werror -Wconversion -Wsign-conversion` on the two
compilers available in the sandbox. Not yet built with AppleClang.

## Where we are

WU-03 done, pending your local confirmation. `tools/testpat.hpp` (new,
header-only) holds `makeRamp`, `makeZonePlate`, `makeExcursion` and
`writeV210`. `tools/make_testpat.cpp` is a thin CLI wrapper around it, built
as its own `add_executable` per the WU-03 scope note. `tests/test_testpat.cpp`
exercises the header directly.

**Deviation from the WU-03 file list in `WORK-UNITS.md`:** that list named
only `tools/make_testpat.cpp` and `tests/test_testpat.cpp`. A third file,
`tools/testpat.hpp`, was added. Reason: `make_testpat` is its own
`add_executable`, not part of `scatter-core`, and `test_testpat` links only
`scatter-core` (via `scatter_test()`) — neither links the other's
translation unit, so the pattern-generation functions have to live
somewhere both can include directly. A header is that place; it is the same
interface/implementation split the project already uses for v210, just
header-only because there is no compiled library boundary between "tool"
and "test" here. Still well inside the WU sizing rule (2 source files + 1
test, under the 3-plus-test limit). Logged as ADR-019 below rather than
treated as a scope violation, since nothing settled was contradicted.

Ramp: column 0 and the last column of every plane land on `kCode10Min`
(4) and `kCode10Max` (1019) exactly, by construction of a rounded-division
formula, not by approaching them — true regardless of plane width. Verified
at width 720 and at width 8 (the narrow case).

Zone plate: luma only, a chirp (`cos(k * r^2)`, `k` scaled to 8 cycles
corner-to-centre) bounded to the nominal legal range
`[kCode10Black, kCode10WhiteNominal]` (64–940), not the full protocol range —
this is a resolution stimulus for WU-10, not an I2 excursion test. Chroma
flat at `kChromaZero`. `kZoneCycles = 8.0` is a starting point, explicitly
not tuned; WU-10 owns that.

Excursion: Y, Cb and Cr all cycle through six codes — 4, 20 (footroom), 64
(black), 940 (nominal white), 1000 (super-white), 1019 — column by column.
Requires plane width ≥ 6 for every code to appear at least once; true for
every width this project uses. Round-tripped through `v210::packImage` /
`unpackImage` in the test and confirmed byte-for-byte: none of these codes
triggers `pack`'s clamp, so I2 holds.

`writeV210` uses `v210::rowBytesMin(width)` as the file stride, per the scope
note. File size is `rowBytesMin(width) * height`, checked in the test.

### Measured, this session (sandbox, not the target machine)

- Manually ran `make_testpat all <dir> 12 4` and `make_testpat all <dir>`
  (default 720x576): produced `ramp.v210`, `zoneplate.v210`,
  `excursion.v210` at the expected sizes (128 bytes and 1,105,920 bytes
  respectively — `1920 * 576`, matching WU-02's own stride figure for 720).
- Cross-compiler: identical pass/fail result under Clang 18 and GCC 13.
- Four-configuration matrix (Release/Debug × tile 4/5): all green.

## Next work unit

**WU-04 — Chroma resampling**, per `WORK-UNITS.md`. Unstarted.

## Open questions

Unchanged: Q1 (tile size, WU-09), Q2 (4K Mini program outputs, WU-14), Q3
(macOS/Desktop Video version, WU-14).

## Blocked / red

Nothing, pending your `./tools/close.sh 03` run. If it comes back red,
overwrite this file's Tests section with the failure verbatim before the
next session, per `docs/workflow.md` section 3.

## Environment check still outstanding

Unchanged from session 2 — Desktop Video / UltraStudio 4K Mini smoke test,
independent of WU-04 and costs no session time.

## Append to DECISIONS.md

```
**ADR-019 — Test pattern generation logic is header-only
(tools/testpat.hpp), shared by the tool and its test.**
`tools/make_testpat.cpp` is its own `add_executable`, not part of
`scatter-core`, and `tests/test_testpat.cpp` links only `scatter-core` (via
`scatter_test()`). Neither links the other's translation unit, so
`makeRamp`, `makeZonePlate`, `makeExcursion` and `writeV210` live in a
header both include, rather than being duplicated or exposed only through
the executable. Same interface/implementation split the project already
uses for v210 (WU-02); header-only here specifically because there is no
compiled library boundary between "tool" and "test" for this unit.
```

## Append to CORRECTIONS.md

Nothing this session.
