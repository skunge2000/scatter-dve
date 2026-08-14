# Handoff
Overwritten at the end of every session. This is the first thing to read.

---

**Session:** 10
**Tag:** `wu-10-green` — confirmed. `./tools/close.sh 10` ran clean on the
M1 Max with AppleClang (Release, tile 2^5, the config `close.sh` builds) and
tagged it.
**Phase:** 1 — portable core, file to file, 576p25, single-threaded — **done**.
Both of Phase 1's own done-when tests (`docs/architecture.md` section 10)
are now green: the ramp round-trip (WU-05) and the zone plate (WU-10, this
session). Phase 2 — Shapes (WU-11 onward) starts next; see below.

**Tests:** All ten green on the M1 Max: the nine carried over unchanged from
WU-09 (`test_smoke`, `test_v210`, `test_chroma`, `test_ramp_roundtrip`,
`test_testpat`, `test_jacobian`, `test_ewa`, `test_binner`, `test_splat`,
none of their files touched) and `test_zoneplate`, new this session, checking
all three of WU-10's own accept criteria directly:
`test_i7_identity_full_pipeline()` (I7, full pipeline, file to file — flat,
ramp and excursion patterns, two source sizes), `test_zoneplate_4to1_matches_reference()`
and `test_zoneplate_32to1_matches_reference()` (the anti-aliasing accept
line), and `test_composite_partial_coverage_no_green_fringe()` plus
`test_pipeline_partial_coverage_no_fringe()` (I5).

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

WU-10 assembles the full pipeline end to end for the first time —
architecture.md section 3's whole signal path, v210 unpack through v210
pack, spliced together in `src/core/pipeline.cpp`'s `runFrame()` (pass 1 +
pass 2 over already-4:4:4 rasters) and `runFrameFile()` (adds the v210/chroma
stages either side). `src/core/resolve.hpp`/`.cpp` is 4.8's own half of pass
2 — `normaliseCell()` divides `AccumCell`'s premultiplied `Σ(w·colour)` by
`Σw` (int64, rounded, flagging `Σw <= 0` rather than dividing by zero); and
`composite()` calls it, then blends the result against a background using
`Σw` (clamped to `[0, kWeightUnity]`) as alpha — never the other order, so
I5's green-fringe bug (compositing the *premultiplied* fields directly
against zero) cannot happen by construction, only by regression, which is
what `test_composite_partial_coverage_no_green_fringe()` exists to catch.

**Design choices this session had to make that `docs/architecture.md` left
open — now ADR-026 in `DECISIONS.md`:** what "the background" 4.8 composites
against actually is (a caller-supplied flat colour, default black — not a
second raster, which is Phase 2's k-buffer question, WU-28, not this unit's
to answer); alpha's range (Σw clamped to `[0, kWeightUnity]`, since Σw is
coverage, not already a `[0, 1]` fraction — a single surface's weight
routinely exceeds unity under compression); and where `core/pipeline.cpp`'s
`runFrame()`/`runFrameFile()` are declared (`core/resolve.hpp`, not a new
`pipeline.hpp` — the thread pool/barriers section 8 associates with that
name are WU-16's, not built yet, and SESSION-PROTOCOL.md's 3-file cap is
already spent; same precedent ADR-021 already set for `file_source.cpp`/
`file_sink.cpp` in `video/raster.hpp`).

**Three corrections found this session, in `CORRECTIONS.md` as C-008,
C-009, C-010** — all found and fixed within WU-10's own files, none of them
defects in earlier, already-frozen units' own code:

- **C-008** — two distinct floating-point issues in `core/lattice.cpp`,
  both invisible until WU-10 finally ran an affine map through the whole
  chain. *(a)* ADR-022's edge-replication clamp, correct for `eval()`, damps
  `jacobian()`'s derivative by up to 50% for any source pixel within one
  lattice cell of an edge — i.e. always including a raster's own first/last
  row and column, any raster size. Genuine, already-frozen (ADR-022), not
  fixable from this unit's files; worked around in `tests/test_zoneplate.cpp`
  by keeping the I7 check's rasters within `kLatticeSize` and by using a
  coverage/hull check (robust to the weight distortion) rather than an exact
  value for the I5 full-pipeline check's own edge-adjacent columns, which
  cannot avoid the raster's real edge by construction. Flagged below as Q4
  for a future `core/lattice.cpp` fix. *(b)* an identity map's *computed*
  pixel-space determinant lands on either side of exactly `1.0` from
  ordinary floating-point rounding (measured: ~47% of one test frame's
  interior pixels, noise below `1e-10`), spuriously triggering
  `chooseSupersample()`'s 2x2 path for roughly half of them. Fixed with a
  `1e-6` margin on `PipelineParams`' default supersample thresholds
  (`core/resolve.hpp`'s `defaultPipelineSupersample()`) — far above the
  measured noise, far below any real determinant difference. `core/binner.cpp`
  itself is unchanged.
- **C-009** — the zone-plate test's first draft compared `runFrame()`'s
  output against an independent *box-average* reference. Wrong reference:
  architecture.md 4.5's splat always spreads a source sample across exactly
  a 2x2 destination neighbourhood regardless of compression ratio, so the
  algorithm's actual implicit reconstruction filter is triangular ("tent"),
  not box — the two agree on energy but not shape, and diverged by up to
  236 code values at a single pixel on an otherwise-correct, non-aliased
  4:1 frame. `referenceCode()` now computes the triangular-kernel reference
  instead (independent of `core/binner.cpp`/`core/splat.cpp`, using only
  `Lattice::eval()` and a hand-written hat-weight function), matching the
  real pipeline to within 0.54 code (4:1) and 6.71 code (32:1) — both
  explained by ordinary fixed-point rounding. No production code changed.
- **C-010** — the I5 full-pipeline test's first draft assumed a flat
  source's fully-covered destination columns resolve to *exactly* the
  source colour. Not quite true in fixed point: ADR-025 already documents
  that the four-bank splat truncates each of a fragment's corner
  contributions separately rather than conserving weight exactly across
  them, and summed across many overlapping fragments this can leave a
  code value or two of drift even at full coverage (measured: a consistent
  +1 on Y and Cr). Fixed with a small (`kRoundingMargin = 4`), well-bounded
  tolerance in place of exact equality for composited columns; pure-
  background columns (no accumulation math at all) stay exact.

**Delivery mechanics, not a design matter:** this session ran remotely, via
the device-bridge tools connecting to this machine, same as sessions 6
through 9. All implementation and the full verification matrix above ran
first in a disposable Linux cloud sandbox, never on this machine directly —
nothing was written here until it was already green there. Files were then
written to this machine via the bridge, and `git add -A && git commit` ran
through that same bridge; as in prior sessions it still cannot clean up its
own `index.lock`/`HEAD.lock`/temp-object files afterward (unlink fails on
this mount), so stale ones were moved into `_to_delete/` rather than
removed — safe to `rm -rf _to_delete/` by hand. Git identity was already
set locally on this mount from a prior session (`Stephen Neal
<stephenneal@Stephens-MacBook-Pro.local>`, confirmed against `git log`
before committing), so nothing needed reconfiguring. `./tools/close.sh 10`
was, as before, run by hand at the real terminal, in two steps this
session: an initial commit left WU-10 `wip` with everything above already
verified in the sandbox, then this closing update (`WORK-UNITS.md` to
`green`, this file) followed your pasted `close.sh` output confirming the
M1 Max/AppleClang build and tag.

## Next work unit

**WU-11 — Cylinder and sphere**, per `WORK-UNITS.md`'s Phase 2 — Shapes.
**Files/Accept:** not yet scoped — `WORK-UNITS.md`'s WU-11 entry is
currently a bare heading, unlike WU-02 through WU-10's, which all had
**Files:**/**Accept:** filled in before their session started. Per
SESSION-PROTOCOL.md ("`WORK-UNITS.md`, edited as scope firms up"), the next
session should read `docs/architecture.md` section 8's module layout
(`src/core/shapes/plane.cpp cylinder.cpp sphere.cpp pageturn.cpp
explode.cpp` — cylinder and sphere are two of five, `plane.cpp` likely
already covered implicitly by the affine-map path WU-01 through WU-10 built
and tested, `pageturn.cpp`/`explode.cpp` later WU-12/Phase-7 units) and
whatever earlier section defines each shape's lattice-generation formula,
fill in WU-11's own **Files:**/**Accept:** lines, and only then start
implementing — the same "read every exact signature before designing"
discipline this session's own brief asked for going into WU-10.

WU-11 is Phase 2's first unit: the first non-affine lattice this project
builds. Everything WU-06 through WU-10 built — `Lattice::eval()`/
`jacobian()`, the fragment generator, the four-bank splat, resolve/composite,
`runFrame()`/`runFrameFile()` — is shape-agnostic; it consumes whatever
`Lattice` a caller hands it, built by `makeAffineLattice()`-equivalent
helpers so far (test-local, never shipped in `core/`). WU-11 is the first
unit that populates a `Lattice`'s control vertices from a genuinely curved
surface instead of a plane, exercising that whole already-built chain
against a non-planar warp for the first time — nothing about `runFrame()`'s
own contract should need to change for it, since it never assumed the
lattice was affine.

## Open questions

Q1 (tile size, WU-14 originally, still open): WU-10 now has a real warp to
benchmark both tile sizes through, but WU-10's own accept line is about
correctness, not performance, and benchmarking felt like a second task
riding along on this one rather than part of it — left undone again,
deliberately, not an oversight. Still worth doing once there's a
motivating reason to look at performance at all (WU-16 or WU-19 are the
more natural point). Q2 (4K Mini program outputs, WU-14), Q3 (macOS/Desktop
Video version, WU-14) — both unchanged. New this session:

**Q4 — `core/lattice.cpp`'s `jacobian()` damps the analytic derivative near
the lattice's own edges (CORRECTIONS.md C-008(a)), a real limitation for any
production-scale raster, not just this session's small test cases.** Every
source raster's own first/last row and column falls in the damaged region,
regardless of raster size — at 720x576 or 1920x1080 this is a one-pixel-wide
border, not a large fraction of the frame, but it is a real, measurable
(up to 50%) density-compensation error right at the frame edge, and nothing
in WU-10's own file scope can fix it (it lives in `core/lattice.cpp`, frozen
at WU-06/ADR-022). A real fix likely means computing `jacobian()`'s edge
case from a true extrapolated tangent instead of ADR-022's duplicated-point
stencil, without changing `eval()`'s own edge behaviour (which is correct
as is). Not urgent — nothing has hit this in practice, no accept line has
failed because of it, and this session's own two full-pipeline tests both
worked around it without needing the fix — but worth a small, focused
future work unit rather than being rediscovered from scratch. Whoever picks
this up should re-read C-008(a) in full first, including the exact
`basisDeriv(0)`/`basisDeriv(1)` mechanism, before touching `core/lattice.cpp`.

## Blocked / red

Nothing. WU-10 closed green.

## Environment check still outstanding

Unchanged from session 2 — Desktop Video / UltraStudio 4K Mini smoke test,
independent of WU-11 and costs no session time.

## Append to DECISIONS.md

Nothing this update — ADR-026 was appended in full earlier this session; see
`DECISIONS.md`. Not reopened or amended now that the tag is confirmed.

## Append to CORRECTIONS.md

Nothing this update — C-008, C-009 and C-010 were appended in full earlier
this session; see `CORRECTIONS.md`. Not reopened or amended now that the tag
is confirmed.
