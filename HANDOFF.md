# Handoff
Overwritten at the end of every session. This is the first thing to read.

---

**Session:** 4
**Tag:** `wu-04-green` (pending — see "Still to do" below)
**Phase:** 1 — portable core, file to file, 576p25, single-threaded

**Tests:** Not yet run on the M1 Max with AppleClang. Verified instead on
Clang 18 and GCC 13, both under the project's exact warning set (`-Wall
-Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror`), Release and
Debug, `SCATTER_TILE_LOG2` 4 and 5 (four configurations × two compilers, all
green). `test_chroma` passes 21342 checks; `test_smoke` and `test_v210` are
unchanged at 2076 and 635 (WU-04 touched neither).

New this session: also built and ran the full suite under GCC 13 with
`-fsanitize=address,undefined -fno-sanitize-recover=all` (Debug). Clean —
no ASan or UBSan report anywhere in `scatter-core` or the three test
binaries. This is the first time any session has actually run the suite
under a sanitizer; every prior HANDOFF listed it as outstanding because it
isn't wired into `CMakeLists.txt` and nobody had run it by hand. Still
outstanding: the equivalent run with AppleClang's sanitizers on the M1 Max
itself, and a permanent `-DSCATTER_SANITIZE=ON` CMake option so this stops
depending on someone remembering the flags — worth a future work unit if it
keeps being useful.

**Run `./tools/close.sh 04` to get the authoritative result and tag.**

**Build:** clean under `-Werror -Wconversion -Wsign-conversion` on Clang 18
and GCC 13. Not yet built with AppleClang.

## Where we are

WU-04 done, pending your local confirmation. `src/video/chroma.hpp` and
`src/video/chroma.cpp` (new) hold horizontal-only 4:2:2↔4:4:4 resampling:
`upsampleRow`/`upsampleImage` (4-tap, co-sited-exact) and
`downsampleRow`/`downsampleImage` (11-tap half-band low-pass, 7 nonzero
taps). `tests/test_chroma.cpp` checks flat-field exactness, two hand-worked
impulse-response vectors (the real proof the coefficients and their signs
are right, not just their sum), hand-worked step-edge ringing magnitudes,
and image-wrapper stride/row-independence — see ADR-020 below for the exact
filter numbers and the reasoning behind them.

**Deviation from the WU-04 file list in `WORK-UNITS.md`:** that list named
only `src/video/chroma.hpp`, `src/video/chroma.cpp`, `tests/test_chroma.cpp`.
`CMakeLists.txt` was also touched, to add `chroma.cpp` to the `scatter-core`
sources and register `test_chroma` via the existing `scatter_test()`
function — two one-line additions, no new build-graph shape. Same situation
WU-02 handled by listing `CMakeLists.txt` explicitly and WU-03 handled
without listing it; not logged as a new ADR, since it's the same mechanical
registration both those units already normalised.

Design choices fixed this session, and why, are in the new ADR-020 below —
the short version: the architecture doc left the exact tap count and
coefficients open ("4- or 6-tap", "9- or 11-tap"), and WU-18's NEON path
needs concrete numbers to diff against, so this session picked them.

While reading the current state I noticed `WORK-UNITS.md` still marked
WU-03 `todo` even though `wu-03-green` exists as a git tag — a previous
session's doc update evidently got missed. Corrected in this session's
`WORK-UNITS.md`, alongside marking WU-04 `wip` (not `green`: that still
needs your local `close.sh` run).

## Next work unit

**WU-05 — File I/O and identity passthrough**, per `WORK-UNITS.md`. This is
the I7 milestone. Unstarted.

## Open questions

Unchanged: Q1 (tile size, WU-09), Q2 (4K Mini program outputs, WU-14), Q3
(macOS/Desktop Video version, WU-14).

## Blocked / red

Nothing, pending your `./tools/close.sh 04` run. If it comes back red,
overwrite this file's Tests section with the failure verbatim before the
next session, per `docs/workflow.md` section 3.

**One local cleanup needed before your `git commit`:** this session wrote
the new files and the `CMakeLists.txt` change straight to your working tree
and ran `git add -A` successfully, but every attempt to run `git commit`
through the remote bridge left a stale `.git/index.lock` behind (the bridge
can create files but can't unlink them, and git's own lock cleanup needs
to). Each stale lock was renamed out of the way — `.git/index.lock`
doesn't exist right now — but you'll find
`.git/index.lock.leftover-please-delete`,
`.git/index.lock.leftover-please-delete2` and `...delete3` sitting in
`.git/`. Delete those three, confirm `git status` shows the four files
staged (`CMakeLists.txt` modified; `chroma.hpp`, `chroma.cpp`,
`test_chroma.cpp` new), then commit normally:

```
rm .git/index.lock.leftover-please-delete*
git status
git commit -m "WU-04: chroma resampling, 4:2:2 <-> 4:4:4"
./tools/close.sh 04
```

## Environment check still outstanding

Unchanged from session 2 — Desktop Video / UltraStudio 4K Mini smoke test,
independent of WU-05 and costs no session time.

## Append to DECISIONS.md

```
**ADR-020 — Chroma resampling filter coefficients, fixed at WU-04.**
`docs/architecture.md` section 5 specified filter *shape* only — "4- or
6-tap" for the 422->444 upsampler, "9- or 11-tap" half-band for the 444->422
downsampler — and left the concrete taps open. WU-18's NEON path needs
something byte-exact to diff against (the same relationship v210.hpp/.cpp
already has to WU-17), so this session picked and froze them:

- Upsample (co-sited-exact, one filtered tap per pair): `out[2i] = in[i]`
  exactly; `out[2i+1] = (-in[i-1] + 9in[i] + 9in[i+1] - in[i+2] + 8) >> 4`.
  Coefficients (-1, 9, 9, -1)/16 — the standard 4-tap half-sample
  interpolator, symmetric, unity DC gain, negative outer lobes.
- Downsample: `out[i] = (3in[2i-5] - 25in[2i-3] + 150in[2i-1] + 256in[2i] +
  150in[2i+1] - 25in[2i+3] + 3in[2i+5] + 256) >> 9`. Coefficients (3, -25,
  150, 256, 150, -25, 3)/512 — a standard truncated half-band low-pass; the
  half-band property means every tap at a nonzero even offset from centre
  is exactly zero, so nominally "11-tap" is 7 nonzero multiplies.
- Edge handling: index clamped to the plane (replicate the boundary
  sample), same choice both directions.
- Rounding: round-half-up via "add half the divisor, then arithmetic
  shift" — the same convention `core/types.hpp`'s `toCode10` uses. C++20's
  guaranteed-arithmetic signed right shift makes this round correctly for
  the negative partial sums the outer lobes produce, not just positive
  ones.
- No clamp on the numeric result beyond what a 16-bit unsigned `Sample` can
  hold. I2 forbids clamping to any legal-range value, and a
  representational-range wrap (well-defined modulo-65536 narrowing, not
  UB) is not that — it is not pulling anything toward a legal range, the
  container is just finite. Worst-case overshoot on a step is 1/16 of the
  step (upsample) or 22/512 (downsample), both worked by direct
  computation in `tests/test_chroma.cpp`; this does not reach the
  representable boundary for chroma content with any reasonable headroom
  around `kChromaZero`, and has not been exercised at the v210 protocol
  limits themselves.

Both filters sum to an exact power of two, so a flat field survives with
zero rounding error in either direction — the property `tests/test_chroma.cpp`
checks first, before the coefficients are checked individually.
```

## Append to CORRECTIONS.md

Nothing this session — the `WORK-UNITS.md` status drift noted above was a
doc-sync slip, not a design claim that was wrong, so it's fixed in place
rather than logged here.
