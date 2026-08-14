# Handoff
Overwritten at the end of every session. This is the first thing to read.

---

**Session:** 10
**Tag:** none yet. WU-10 is `wip` in `WORK-UNITS.md`, not `green` — everything
below is verified in a Linux cloud sandbox only. `./tools/close.sh 10` still
needs to run at the real terminal on the M1 Max (AppleClang, Release, tile
2^5) before it can be tagged; this session could not run it (no AppleClang
reachable from the sandbox this session used).
**Phase:** 1 — portable core, file to file, 576p25, single-threaded

**Tests:** All ten green in the sandbox: the nine carried over unchanged from
WU-09 (`test_smoke`, `test_v210`, `test_chroma`, `test_ramp_roundtrip`,
`test_testpat`, `test_jacobian`, `test_ewa`, `test_binner`, `test_splat`,
none of their files touched) and `test_zoneplate`, new this session, checking
all three of WU-10's own accept criteria directly:
`test_i7_identity_full_pipeline()` (I7, full pipeline, file to file — flat,
ramp and excursion patterns, two source sizes), `test_zoneplate_4to1_matches_reference()`
and `test_zoneplate_32to1_matches_reference()` (the anti-aliasing accept
line), and `test_composite_partial_coverage_no_green_fringe()` plus
`test_pipeline_partial_coverage_no_fringe()` (I5).

Verified: Clang 18 and GCC 13, Release and Debug, `SCATTER_TILE_LOG2` 4 and
5 (eight configurations, all green, zero warnings under the project's exact
warning set — `-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion
-Werror` — checked directly in the build logs, not just a successful exit
code), plus GCC 13 with `-fsanitize=address,undefined
-fno-sanitize-recover=all` (Debug, both tile sizes): clean, no ASan or UBSan
report anywhere. Not yet run on the M1 Max with AppleClang.

**Build:** clean under `-Werror -Wconversion -Wsign-conversion` on Clang 18
and GCC 13. Not yet confirmed on AppleClang.

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
the device-bridge tools connecting to this machine. All implementation and
the full verification matrix above ran first in a disposable Linux cloud
sandbox, never on this machine directly — nothing was written here until it
was already green there. Files were then written to this machine via the
bridge, and `git add -A && git commit` ran through that same bridge; as in
prior sessions it still cannot clean up its own `index.lock`/`HEAD.lock`/
temp-object files afterward (unlink fails on this mount), so stale ones were
moved into `_to_delete/` rather than removed — safe to `rm -rf _to_delete/`
by hand. Git identity was already set locally on this mount from a prior
session (`Stephen Neal <stephenneal@Stephens-MacBook-Pro.local>`, confirmed
against `git log` before committing), so nothing needed reconfiguring.

## Next step — yours

Run `./tools/close.sh 10` at the real terminal. If it builds and tests green
on AppleClang, it tags `wu-10-green`; paste the output back (or just say it
passed) and the next session will flip `WORK-UNITS.md` to `green` and update
this file for WU-11, the same two-step pattern WU-06 through WU-09 used. If
it fails on AppleClang specifically (nothing in this session's own testing
suggests it would, but this session had no way to check), the failure output
is the most useful thing to bring back.

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

Nothing red. WU-10 is `wip`, verified in the sandbox, awaiting the M1 Max
close.

## Environment check still outstanding

Unchanged from session 2 — Desktop Video / UltraStudio 4K Mini smoke test,
independent of WU-10 and costs no session time.

## Append to DECISIONS.md

Nothing further this update — ADR-026 was appended in full earlier this
session; see `DECISIONS.md`.

## Append to CORRECTIONS.md

Nothing further this update — C-008, C-009 and C-010 were appended in full
earlier this session; see `CORRECTIONS.md`.
