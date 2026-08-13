# Handoff
Overwritten at the end of every session. This is the first thing to read.

---

**Session:** 4
**Tag:** `wu-04-green` — confirmed. `./tools/close.sh 04` ran clean on the
M1 Max with AppleClang (Release, tile 2^5, the config `close.sh` builds) and
tagged it.
**Phase:** 1 — portable core, file to file, 576p25, single-threaded

**Tests:** All four green on the M1 Max: `test_smoke`, `test_v210`,
`test_chroma`, `test_testpat`. `test_chroma` is new this session (21342
checks); `test_smoke` and `test_v210` are unchanged (WU-04 touched neither).

Before that, this session verified on Clang 18 and GCC 13 in a Linux
sandbox (no AppleClang there), both under the project's exact warning set
(`-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror`), Release
and Debug, `SCATTER_TILE_LOG2` 4 and 5 (four configurations × two compilers,
all green) — those extra configurations (Debug, tile 2^4) are still only
sandbox-verified, not run on the M1 Max, same as prior sessions' practice.
Also ran the full suite under GCC 13 with
`-fsanitize=address,undefined -fno-sanitize-recover=all` (Debug): clean, no
ASan or UBSan report anywhere. First time any session has actually run a
sanitizer rather than just listing it as outstanding. Still outstanding: the
equivalent sanitizer run with AppleClang on the M1 Max itself, and a
permanent `-DSCATTER_SANITIZE=ON` CMake option so this stops depending on
someone remembering the flags — worth a future work unit if it keeps being
useful.

**Build:** clean under `-Werror -Wconversion -Wsign-conversion` on AppleClang
(M1 Max), Clang 18 and GCC 13.

## Where we are

WU-04 done and closed green. `src/video/chroma.hpp` and `src/video/chroma.cpp`
hold horizontal-only 4:2:2↔4:4:4 resampling: `upsampleRow`/`upsampleImage`
(4-tap, co-sited-exact) and `downsampleRow`/`downsampleImage` (11-tap
half-band low-pass, 7 nonzero taps). `tests/test_chroma.cpp` checks
flat-field exactness, two hand-worked impulse-response vectors (the real
proof the coefficients and their signs are right, not just their sum),
hand-worked step-edge ringing magnitudes, and image-wrapper stride/row-
independence. Filter numbers and the reasoning behind them are ADR-020 in
`DECISIONS.md` (already appended, not just proposed here).

**Deviation from the WU-04 file list in `WORK-UNITS.md`:** that list named
only `src/video/chroma.hpp`, `src/video/chroma.cpp`, `tests/test_chroma.cpp`.
`CMakeLists.txt` was also touched, to add `chroma.cpp` to the `scatter-core`
sources and register `test_chroma` via the existing `scatter_test()`
function — two one-line additions, no new build-graph shape. Same situation
WU-02 handled by listing `CMakeLists.txt` explicitly and WU-03 handled
without listing it; not logged as a new ADR, since it's the same mechanical
registration both those units already normalised.

While reading the current state at session start, `WORK-UNITS.md` still
marked WU-03 `todo` even though `wu-03-green` already existed as a git tag —
a previous session's doc update evidently got missed. Corrected this
session, alongside marking WU-04 `green`.

**Delivery mechanics note, not a design matter:** getting these changes
from the assistant onto this machine went through a remote file bridge, and
`git commit` run *through that bridge* reliably left a stale
`.git/index.lock` it couldn't clean up — a bridge-side unlink restriction,
not anything wrong with git or this repo. Committing directly at this
terminal, as you just did, doesn't have that problem. Noted here only so a
future session doesn't mistake it for a repository issue.

## Next work unit

**WU-05 — File I/O and identity passthrough**, per `WORK-UNITS.md`. This is
the I7 milestone. Unstarted.

## Open questions

Unchanged: Q1 (tile size, WU-09), Q2 (4K Mini program outputs, WU-14), Q3
(macOS/Desktop Video version, WU-14).

## Blocked / red

Nothing. WU-04 closed green.

## Environment check still outstanding

Unchanged from session 2 — Desktop Video / UltraStudio 4K Mini smoke test,
independent of WU-05 and costs no session time.

## Append to DECISIONS.md

Nothing further — ADR-020 was appended in full during this session; see
`DECISIONS.md`.

## Append to CORRECTIONS.md

Nothing this session — the `WORK-UNITS.md` status drift noted above was a
doc-sync slip, not a design claim that was wrong, so it's fixed in place
rather than logged here.
