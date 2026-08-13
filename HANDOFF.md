# Handoff
Overwritten at the end of every session. This is the first thing to read.

---

**Session:** 8
**Tag:** none yet. WU-08 is `wip` in `WORK-UNITS.md`, not closed — this
session's verification ran entirely in the cloud sandbox; `./tools/close.sh
08` still needs to run on the M1 Max with AppleClang before tagging
`wu-08-green`.
**Phase:** 1 — portable core, file to file, 576p25, single-threaded

**Tests:** `test_binner`, new this session — 38348 checks at tile 2^5,
10124 at tile 2^4 (the difference is expected: its boundary-replication
check is O(width × height × tilesX × tilesY), not a bug). The other seven
suites are unchanged from WU-07's files and rebuild/pass alongside
`test_binner` in every configuration below, but were not themselves
re-verified for content this session.

This session verified in a Linux cloud sandbox (no AppleClang there), on
Clang 18 and GCC 13, under the project's exact warning set (`-Wall
-Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror`), Release and
Debug, `SCATTER_TILE_LOG2` 4 and 5 (eight configurations, all green, zero
warnings — checked explicitly in the build logs, not just a successful
exit code), plus GCC 13 with `-fsanitize=address,undefined
-fno-sanitize-recover=all` (Debug) at both tile sizes: clean, no ASan or
UBSan report anywhere — same practice as WU-06 and WU-07.

**Not yet verified:** AppleClang on the M1 Max. `./tools/close.sh 08`
needs to run there before this unit can tag green.

## Where we are

WU-08 implemented, not yet closed. `src/core/binner.hpp`/`.cpp` add pass 1
of the pipeline (architecture.md section 3): for every source sample,
evaluate the lattice and its Jacobian (WU-06), derive K
(`densityCompensation()`, WU-07), decide how many fragments that sample
becomes under magnification (architecture.md 4.6), build a `Frag` for
each, and bin it into every destination tile its four-bank splat
footprint (4.5) touches (4.4's boundary replication). `TileBins` holds
the per-tile `std::vector<Frag>` bins; `generateFragments()` returns
`BinStats` (source samples, primary fragments, replica fragments,
off-raster drops) so `tests/test_binner.cpp` can check WORK-UNITS.md's
accept line directly rather than re-deriving it from bin contents.
`CMakeLists.txt`: `src/core/binner.cpp` added to `scatter-core`'s
sources (the first `.cpp` `core/` has needed since `jacobian.hpp`, WU-07,
stayed header-only — `TileBins` carries real per-tile storage, not just
pure functions of their arguments); `test_binner` registered via
`scatter_test()`.

architecture.md 4.1, 4.4 and 4.6 leave several concrete choices open here
— the same kind of gap ADR-020/022/023 filled for earlier units. This
session's choices are ADR-024 in `DECISIONS.md`:

- Source-pixel-index -> lattice-parameter mapping, linear across the
  lattice's full `[0, kLatticeMax]` range regardless of source
  resolution.
- K and the supersampling decision use the Jacobian with respect to
  *source pixels*, not the raw lattice-parameter-space Jacobian 4.2's
  formula names directly — the two differ by a resolution-dependent
  constant scale factor (the chain rule, via the same constant the first
  bullet's mapping uses), and using the wrong one would make K depend on
  source resolution instead of on the warp. This was **not** an
  intentional gap noticed up front; it surfaced while getting
  `test_binner.cpp`'s supersampling-threshold checks to agree with a
  hand-computed expectation, and is worth flagging precisely because it
  would have been easy to miss (`densityCompensation()`'s own WU-07
  tests never exercised it, since they hand-built `Jacobian` values
  directly rather than going through a `Lattice` and a pixel raster).
- Tile-local coordinate encoding for a fragment replicated into a
  neighbouring tile: `SubPos` is unsigned, but a replica's position
  relative to its new tile is up to one pixel negative (as far as the
  four-bank splat's footprint reaches outside a fragment's home tile).
  Every stored coordinate is biased by one pixel (`kSubPixelOne`)
  uniformly, home or replica, so one decode rule applies to a tile's
  whole bin.
- Supersampling thresholds (`threshold2x2 = 1.0`, anchored to
  architecture.md 4.6's own literal "det J > 1"; `threshold4x4 = 4.0`)
  and the hard cap, all `SupersampleConfig` fields a caller can override.
- Off-raster source samples are dropped (counted in
  `BinStats::droppedOffRaster`), not clamped into a fabricated position.

Not an ADR, deliberately: `Frag::z`'s quantisation from `Vec3::z`'s
double is not yet fixed by anything — nothing reads it before WU-28's
k-buffer and WU-08's accept criteria do not exercise it.
`core/binner.cpp` rounds and saturates to `uint16_t` as a placeholder.

A real correctness trap this session worked around, worth a note for
whoever writes similar tests later: Catmull-Rom's edge handling
(ADR-022 — control-vertex lookups outside `[0, kLatticeMax]` replicate
the nearest edge vertex) means `jacobian()` does *not* reproduce an
affine field's true constant slope exactly at the lattice's own domain
boundary (`u` or `v` equal to `0` or `kLatticeMax`) — only in the
interior, where all four stencil neighbours are real, unclamped control
vertices. This is correct, frozen WU-06 behaviour (its own
`test_jacobian.cpp` checks `jacobian()` against numeric differentiation
of `eval()` itself, which stays self-consistent regardless), not
something WU-08 needed to or should fix — but a naive synthetic test
lattice that evaluates every source pixel including the raster's own
border pixels (which necessarily land exactly on `u`/`v` `0` or
`kLatticeMax`, since the lattice covers the whole source raster) will
see a distorted Jacobian at those border pixels specifically.
`tests/test_binner.cpp`'s supersampling-threshold test isolates a single
fully-interior pixel for exactly this reason; its other tests either
don't depend on the Jacobian's exact value (the compression/count test)
or don't depend on it at all (the boundary-replication test, which only
needs `eval()`'s position, exact everywhere including the boundary,
unlike the derivative).

## Delivery mechanics, not a design matter

This session ran remotely, via the device-bridge tools connecting to
this machine, same as sessions 6 and 7. All implementation and the full
verification matrix above ran first in a disposable Linux cloud sandbox,
never on this machine directly — nothing was written here until it was
already green there. Files were then written to this machine via the
bridge, and `git add -A && git commit` ran through that same bridge; as
in prior sessions it still cannot clean up its own
`index.lock`/`HEAD.lock`/temp-object files afterward (unlink fails on
this mount), so stale ones were moved into `_to_delete/` rather than
removed — safe to `rm -rf _to_delete/` by hand. Git identity was already
set locally on this mount from a prior session (confirmed against `git
log` before committing), so nothing needed reconfiguring.

Unlike WU-06 and WU-07, this session stops here rather than running
`close.sh` itself — that step needs AppleClang on the M1 Max, which the
cloud sandbox cannot reach. **Run `./tools/close.sh 08` at the real
terminal**, then update `WORK-UNITS.md`'s WU-08 line to `green` and this
file, the same closing step WU-06 and WU-07 took as their session's own
second half.

## Next work unit

**WU-09 — Four-bank splat**, per `WORK-UNITS.md`, once WU-08 is closed
green.
**Files:** `src/core/splat.hpp`, `src/core/splat.cpp`, `tests/test_splat.cpp`.
**Accept:** four-bank result identical to a single-accumulator reference
implementation; int64 headroom verified at synthetic worst case.

WU-09 is the consumer of this session's `Frag::x`/`y` bias convention
(ADR-024) — its splat needs to subtract `kSubPixelOne` before taking the
integer base cell for the four addresses architecture.md 4.5 describes
(base, base+1, base+stride, base+stride+1). WU-08 does not implement or
test that consumption; only that the encoding itself is well-formed and
that `tests/test_binner.cpp` can decode a `Frag`'s position correctly
where it needs to (it currently only decodes colour, not position, in
its own checks).

## Open questions

Unchanged: Q1 (tile size, WU-09), Q2 (4K Mini program outputs, WU-14), Q3
(macOS/Desktop Video version, WU-14).

## Blocked / red

Nothing red. WU-08 is `wip`, awaiting `./tools/close.sh 08` on the M1 Max.

## Environment check still outstanding

Unchanged from session 2 — Desktop Video / UltraStudio 4K Mini smoke test,
independent of WU-08/WU-09 and costs no session time.

## Append to DECISIONS.md

ADR-024, appended in full earlier this session; see `DECISIONS.md`.

## Append to CORRECTIONS.md

Nothing this session — the Jacobian chain-rule issue above was caught
before being implemented wrong, not corrected after, so it belongs in
ADR-024 rather than `CORRECTIONS.md`.
