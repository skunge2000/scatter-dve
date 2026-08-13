# Handoff
Overwritten at the end of every session. This is the first thing to read.

---

**Session:** 8
**Tag:** `wu-08-green` — confirmed. `./tools/close.sh 08` ran clean on the
M1 Max with AppleClang (Release, tile 2^5, the config `close.sh` builds) and
tagged it.
**Phase:** 1 — portable core, file to file, 576p25, single-threaded

**Tests:** All eight green on the M1 Max: `test_smoke`, `test_v210`,
`test_chroma`, `test_ramp_roundtrip`, `test_testpat`, `test_jacobian`,
`test_ewa` (seven unchanged from WU-07, none of their files touched) and
`test_binner`, new this session (38348 checks at tile 2^5, 10124 at tile
2^4 — its boundary-replication check is O(width × height × tilesX ×
tilesY), hence the difference, not a bug).

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

WU-08 done and closed green. `src/core/binner.hpp`/`.cpp` add pass 1 of
the pipeline (architecture.md section 3): for every source sample,
evaluate the lattice and its Jacobian (WU-06), derive K
(`densityCompensation()`, WU-07), decide how many fragments that sample
becomes under magnification (architecture.md 4.6), build a `Frag` for
each, and bin it into every destination tile its four-bank splat
footprint (4.5) touches (4.4's boundary replication). `TileBins` holds
the per-tile `std::vector<Frag>` bins; `generateFragments()` returns
`BinStats` (source samples, primary fragments, replica fragments,
off-raster drops) so `tests/test_binner.cpp` checks WORK-UNITS.md's
accept line directly rather than re-deriving it from bin contents.
`CMakeLists.txt`: `src/core/binner.cpp` added to `scatter-core`'s
sources (the first `.cpp` `core/` has needed since `jacobian.hpp`, WU-07,
stayed header-only); `test_binner` registered via `scatter_test()`.

**Design choices this session had to make that `docs/architecture.md`
left open — now ADR-024 in `DECISIONS.md`:** source-pixel-index ->
lattice-parameter mapping, linear across the lattice's full
`[0, kLatticeMax]` range regardless of source resolution; K and the
supersampling decision use the Jacobian with respect to *source pixels*,
not the raw lattice-parameter-space Jacobian 4.2's formula names
directly (the two differ by a resolution-dependent constant scale factor
— using the wrong one would make K depend on source resolution instead
of the warp; this surfaced while getting `test_binner.cpp`'s
supersampling-threshold checks to agree with a hand-computed
expectation, not from an up-front read of the gap); tile-local
coordinate encoding for a fragment replicated into a neighbouring tile
(`SubPos` biased by one pixel, uniformly for home and replica fragments,
since it's unsigned but a replica's position relative to its new tile
can be up to one pixel negative); supersampling thresholds
(`threshold2x2 = 1.0`, anchored to architecture.md 4.6's own literal
"det J > 1"; `threshold4x4 = 4.0`) and the hard cap; off-raster samples
dropped rather than clamped into a fabricated position. Not an ADR,
deliberately: `Frag::z`'s quantisation from `Vec3::z`'s double is not
yet fixed by anything — nothing reads it before WU-28's k-buffer and
WU-08's accept criteria do not exercise it. Neither reopens
architecture.md, which left these gaps open on purpose — same
relationship ADR-020/022/023 have to it.

A correctness trap worth flagging for future sessions writing similar
synthetic-lattice tests: Catmull-Rom's edge handling (ADR-022 —
control-vertex lookups outside `[0, kLatticeMax]` replicate the nearest
edge vertex) means `jacobian()` does *not* reproduce an affine field's
true constant slope exactly at the lattice's own domain boundary (`u` or
`v` equal to `0` or `kLatticeMax`) — only in the interior, where all
four stencil neighbours are real, unclamped control vertices. Correct,
frozen WU-06 behaviour (its own `test_jacobian.cpp` checks `jacobian()`
against numeric differentiation of `eval()` itself, which stays
self-consistent regardless), not something WU-08 needed to or should
fix — but any source raster's own border pixels necessarily land
exactly on `u`/`v` `0` or `kLatticeMax` (the lattice covers the whole
source raster), so a synthetic affine-field test lattice sees a
distorted Jacobian there specifically. `tests/test_binner.cpp`'s
supersampling-threshold test isolates a single fully-interior pixel for
exactly this reason.

**Delivery mechanics, not a design matter:** this session ran remotely, via
the device-bridge tools connecting to this machine, same as sessions 6 and
7. All implementation and the full verification matrix above ran first in a
disposable Linux cloud sandbox, never on this machine directly — nothing
was written here until it was already green there. Files were then written
to this machine via the bridge, and `git add -A && git commit` ran through
that same bridge; as in prior sessions it still cannot clean up its own
`index.lock`/`HEAD.lock`/temp-object files afterward (unlink fails on this
mount), so stale ones were moved into `_to_delete/` rather than removed —
safe to `rm -rf _to_delete/` by hand. Git identity was already set locally
on this mount from a prior session (`Stephen Neal
<stephenneal@Stephens-MacBook-Pro.local>`, confirmed against `git log`
before committing), so nothing needed reconfiguring. `./tools/close.sh 08`
was, as before, run by hand at the real terminal, in two steps this
session: an initial commit left WU-08 `wip` with everything above already
verified in the sandbox, then this closing update (`WORK-UNITS.md` to
`green`, this file) followed your pasted `close.sh` output confirming the
M1 Max/AppleClang build and tag.

## Next work unit

**WU-09 — Four-bank splat**, per `WORK-UNITS.md`.
**Files:** `src/core/splat.hpp`, `src/core/splat.cpp`, `tests/test_splat.cpp`.
**Accept:** four-bank result identical to a single-accumulator reference
implementation; int64 headroom verified at synthetic worst case. Unstarted.

WU-09 is the consumer of WU-08's `Frag::x`/`y` bias convention
(ADR-024) — its splat needs to subtract `kSubPixelOne` before taking the
integer base cell for the four addresses architecture.md 4.5 describes
(base, base+1, base+stride, base+stride+1).

## Open questions

Unchanged: Q1 (tile size, WU-09), Q2 (4K Mini program outputs, WU-14), Q3
(macOS/Desktop Video version, WU-14).

## Blocked / red

Nothing. WU-08 closed green.

## Environment check still outstanding

Unchanged from session 2 — Desktop Video / UltraStudio 4K Mini smoke test,
independent of WU-09 and costs no session time.

## Append to DECISIONS.md

Nothing this update — ADR-024 was appended in full earlier this session; see
`DECISIONS.md`. Not reopened or amended now that the tag is confirmed.

## Append to CORRECTIONS.md

Nothing this session.
