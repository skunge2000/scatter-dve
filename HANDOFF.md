# Handoff
Overwritten at the end of every session. This is the first thing to read.

---

**Session:** 9
**Tag:** `wu-09-green` — confirmed. `./tools/close.sh 09` ran clean on the
M1 Max with AppleClang (Release, tile 2^5, the config `close.sh` builds) and
tagged it.
**Phase:** 1 — portable core, file to file, 576p25, single-threaded

**Tests:** All nine green on the M1 Max: `test_smoke`, `test_v210`,
`test_chroma`, `test_ramp_roundtrip`, `test_testpat`, `test_jacobian`,
`test_ewa`, `test_binner` (eight unchanged from WU-08, none of their files
touched) and `test_splat`, new this session (6179 checks at tile 2^5, 1571
at tile 2^4 — the difference is the random-equivalence test's and
`test_clear_zeroes_all_banks`'s per-cell loops, both `O(kTilePixels)`, the
same reason WU-08's check count differed by tile size, not a bug).

Before that, this session verified in a Linux cloud sandbox (no AppleClang
there), on Clang 18 and GCC 13, under the project's exact warning set
(`-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror`), Release
and Debug, `SCATTER_TILE_LOG2` 4 and 5 (eight configurations, all green,
zero warnings — checked explicitly in the build logs, not just a successful
exit code), plus GCC 13 with `-fsanitize=address,undefined
-fno-sanitize-recover=all` (Debug) at both tile sizes: clean, no ASan or
UBSan report anywhere — same practice as prior sessions.

**Build:** clean under `-Werror -Wconversion -Wsign-conversion` on
AppleClang (M1 Max), Clang 18 and GCC 13.

## Where we are

WU-09 done and closed green. `src/core/splat.hpp`/`.cpp` are pass 2's
first half (architecture.md section 3, "PASS 2: tile resolve" -> "four-bank
splat"): `TileAccum` holds one `kTileSize` x `kTileSize` grid of
`AccumCell` per bank, four banks; `splatTile()` takes one tile's
`std::vector<Frag>` (exactly what WU-08's `TileBins::tile()` produces) and,
per fragment, bilinearly splats its weight across the (up to four) corners
of its 2x2 footprint that fall inside this tile, one read-modify-write per
bank per corner (architecture.md 4.5); `sumBanks()` is 4.5's own "resolve"
step — summing all four banks, identically addressed, into one final
`AccumCell` grid. `splatTileReference()` is the accept line's oracle:
identical fragment -> corner-contribution arithmetic (both paths call the
same internal helper, so they cannot drift apart by accident), but against
one unbanked accumulator — proven bit-identical to `splatTile()` +
`sumBanks()` by `tests/test_splat.cpp`'s randomised equivalence test,
satisfying WU-09's first accept criterion. `CMakeLists.txt`:
`src/core/splat.cpp` added to `scatter-core`'s sources (the same "needs its
own `.cpp` because it carries per-tile storage state" reasoning
`binner.cpp` already established); `test_splat` registered via
`scatter_test()`.

Note that WU-10's own `core/resolve.hpp`/`.cpp` (normalise, composite) is a
*different* "resolve" than `sumBanks()` here — architecture.md's signal
path (section 3) uses "resolve" for the whole of pass 2, but WORK-UNITS.md
splits it: WU-09 is the bank-summing step 4.5 itself calls "resolve";
WU-10's normalise/composite is 4.8. `sumBanks()` is named to keep the two
apart, deliberately not `resolve()`.

**Design choices this session had to make that `docs/architecture.md`
left open — now ADR-025 in `DECISIONS.md`:** the bilinear per-corner
weight formula (anchored to section 13's "fractional addresses supplying
splat weights", not invented — see ADR-025 for why this one had only one
geometrically sensible answer, unlike ADR-023's `maxK` or ADR-024's
supersampling thresholds); what happens when a splat corner falls outside
the tile being resolved (not an independent choice — the necessary
consequence of WU-08/ADR-024's already-frozen replication scheme, simply
skipped rather than clamped); and the signed decode of `SubPos` back to a
(base cell, fraction) pair, undoing ADR-024's one-pixel bias with a
widen-then-floor-shift technique applied to a signed range for the first
time in this codebase.

**A correction found this session, in `CORRECTIONS.md` as C-007:**
`AccumCell::w` (`WeightAccum`, int32 per I4) does not have the same
headroom at the literal "a million full-weight fragments" synthetic scale
that `AccumCell::Y`/`Cb`/`Cr` (`ColourAccum`, int64) do — routing a million
full-weight fragments through the real splat path onto one cell overflows
`AccumCell::w` (undefined behaviour) tens of thousands of fragments before
`AccumCell::Y` would be at any risk, since a fragment's weight contribution
is `w` alone while its colour contribution is `Y * w`. Not a defect in I4
(which only ever claimed int32 "may be" sufficient for weight, not that it
matches colour's synthetic ceiling) or in `splat.cpp`. `tests/test_splat.cpp`
splits its worst-case coverage accordingly:
`test_int64_headroom_full_pipeline()` drives 25000 full-weight fragments
through the real code path (safely under `AccumCell::w`'s own ~32768-count
ceiling, already far beyond a hypothetical 32-bit colour accumulator's
range), and `test_int64_headroom_million_fragment_arithmetic()` checks the
literal million-fragment claim directly against `ColourAccum`'s own
arithmetic, decoupled from the shared weight accumulator. See C-007 for the
full arithmetic.

**Delivery mechanics, not a design matter:** this session ran remotely, via
the device-bridge tools connecting to this machine, same as sessions 6
through 8. All implementation and the full verification matrix above ran
first in a disposable Linux cloud sandbox, never on this machine directly —
nothing was written here until it was already green there. Files were then
written to this machine via the bridge, and `git add -A && git commit` ran
through that same bridge; as in prior sessions it still cannot clean up its
own `index.lock`/`HEAD.lock`/temp-object files afterward (unlink fails on
this mount), so stale ones were moved into `_to_delete/` rather than
removed — safe to `rm -rf _to_delete/` by hand. Git identity was already
set locally on this mount from a prior session (`Stephen Neal
<stephenneal@Stephens-MacBook-Pro.local>`, confirmed against `git log`
before committing), so nothing needed reconfiguring. `./tools/close.sh 09`
was, as before, run by hand at the real terminal, in two steps this
session: an initial commit left WU-09 `wip` with everything above already
verified in the sandbox, then this closing update (`WORK-UNITS.md` to
`green`, this file) followed your pasted `close.sh` output confirming the
M1 Max/AppleClang build and tag.

## Next work unit

**WU-10 — Normalise, composite, first affine warp**, per `WORK-UNITS.md`.
**Files:** `src/core/resolve.hpp`, `src/core/resolve.cpp`,
`src/core/pipeline.cpp`, `tests/test_zoneplate.cpp`.
**Accept:** identity map still bit-exact (I7 holds through the full path);
zone plate through 4:1 and 32:1 compression shows no aliasing; no green
fringing on partial coverage (I5). Unstarted.

WU-10 is the consumer of WU-09's `sumBanks()` output — one `AccumCell` per
destination cell, per tile — dividing accumulated colour by accumulated
weight (I5, 4.8) and compositing. It is also the unit that finally
exercises `TileBins`/`generateFragments()` (WU-08) and `splatTile()`/
`sumBanks()` (WU-09) end to end against a real (if still affine-only) warp,
which is what its accept line's zone-plate and identity-map checks are for.

## Open questions

Unchanged: Q1 (tile size — still open; WU-09 implemented the mechanism at
both compile-time tile sizes as CMakeLists.txt's comment asked, but did not
benchmark them against each other. Benchmarking felt out of that session's
scope — WU-09's accept line was about correctness, not performance, and
nothing later than WU-10 needs Q1 resolved. Flagging it as worth doing once
there's a real warp to benchmark through, rather than an isolated splat
microbenchmark, which risks measuring something that doesn't match the
full pipeline's actual cache behaviour), Q2 (4K Mini program outputs,
WU-14), Q3 (macOS/Desktop Video version, WU-14).

## Blocked / red

Nothing. WU-09 closed green.

## Environment check still outstanding

Unchanged from session 2 — Desktop Video / UltraStudio 4K Mini smoke test,
independent of WU-10 and costs no session time.

## Append to DECISIONS.md

Nothing this update — ADR-025 was appended in full earlier this session; see
`DECISIONS.md`. Not reopened or amended now that the tag is confirmed.

## Append to CORRECTIONS.md

Nothing this update — C-007 was appended in full earlier this session; see
`CORRECTIONS.md`. Not reopened or amended now that the tag is confirmed.
