# Handoff
Overwritten at the end of every session. This is the first thing to read.

---

**Session:** 12
**Tag:** `wu-12a-green` — confirmed. `./tools/close.sh 12a` ran clean on
the M1 Max with AppleClang (Release, tile 2^5, the config `close.sh`
builds) on the first attempt — no cloud/AppleClang divergence this time,
unlike WU-11's own C-012.
**Phase:** 2 — Shapes. WU-12 split into WU-12a (this session, page-turn
shape + transparent mode — done) and WU-12b (priority-tag opaque mode —
designed, not yet implemented; starts next). See `DECISIONS.md` ADR-028
for why it split and the design sketch WU-12b starts from.

**Tests:** All twelve green on the M1 Max: the ten carried over unchanged
from WU-10/WU-11 plus `test_shapes` (WU-11's own, untouched this session)
and `test_pageturn`, new this session — 126512 checks (Clang 18, Release,
tile 2^5, in the cloud sandbox; `close.sh`'s own run reports pass/fail per
executable, not a check count), checking WU-12a's own accept criteria
directly: every `buildPageTurnLattice()` control vertex is exactly flat or
exactly on the configured curl cylinder per the `turnProgress` split
(`test_pageturn_flat_or_on_curl_cylinder`); the spine never moves, any
`turnProgress` (`test_pageturn_spine_never_moves`); `turnProgress == 0`
reduces exactly to the flat/affine case
(`test_pageturn_flat_at_zero_progress`); `Lattice::jacobian()` matches
central differences on a populated, genuinely curling page-turn lattice,
including at and near the flat/curl seam
(`test_pageturn_jacobian_matches_central_difference`); and a page-turn
flap plus a full-canvas "page behind", splatted into shared tile bins,
sum exactly (bit-for-bit) to the two layers' own separately-splatted
`AccumCell`s added component-wise, at every destination cell, with the
flap's own best-covered pixel showing strictly more accumulated weight and
a composited colour that differs from page-behind-alone by more than
rounding
(`test_pipeline_pageturn_transparent_accumulates_over_page_behind`) — the
first direct proof, not just an assumption carried over from WU-11's own
note, that the pipeline built by WU-06 through WU-11 is shape-agnostic
*and* handles two independently generated surfaces sharing one frame
correctly, not just one shape at a time.

Before that, this session verified in a Linux cloud sandbox (no AppleClang
there) on Clang 18 and GCC 13, under the project's exact warning set
(`-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror`),
Release and Debug, `SCATTER_TILE_LOG2` 4 and 5 (eight configurations, all
green, zero warnings — checked explicitly in the build logs, not just exit
codes), plus GCC 13 with `-fsanitize=address,undefined
-fno-sanitize-recover=all` (Debug) at both tile sizes: clean, no ASan or
UBSan report anywhere — same practice as every session since WU-06.

**Build:** clean under `-Werror -Wconversion -Wsign-conversion` on
AppleClang (M1 Max), Clang 18 and GCC 13.

## Where we are

WU-12a adds `src/core/shapes/pageturn.cpp` and extends
`src/core/shapes/shapes.hpp` with `PageTurnParams`/`buildPageTurnLattice()`
— a page-turn surface, flat near a fixed spine and rolling into a partial
cylinder (arc-length parametrised, so position and tangent match exactly
at the flat/curl seam for any `turnProgress`) for the portion that has
turned. No change to `Lattice`, `core/binner.cpp`, `core/splat.cpp` or
`core/resolve.cpp` — transparent-mode compositing of two surfaces (this
unit's flap plus a "page behind") works by calling `generateFragments()`
twice into one `TileBins` and splatting once, which `tests/
test_pageturn.cpp` proves sums exactly right, not merely assumed from
WU-11's "shape-agnostic" note.

**Design choices this session had to make that `docs/architecture.md` and
last session's own open question left open — now ADR-028 in
`DECISIONS.md`:** the shape-function `t` question (`turnProgress` is this
shape's own parameter, the same kind of thing `CylinderParams::angleSpan`
already is — *not* `architecture.md` 4.1's `t`, which names WU-13's own
keyframe/lattice-interpolation mechanism, a different and still-unbuilt
thing); the flat/curl parametrisation itself (arc-length continuity
derivation, the spine-never-moves and reduces-to-flat-at-zero-progress
properties, both checked directly, not just asserted); and the scope split
with WU-12b, including a design sketch (not yet implemented) for a
narrower-than-k-buffer "opaque with priority tag set" mechanism — two
already-splatted layers, caller-ordered, "read-replace-write" only for the
layer whose tag matches a configured opaque tag, summed otherwise. See
ADR-028 for the full reasoning on all of these, including why WU-12b
cannot fit in this same unit under `SESSION-PROTOCOL.md`'s sizing cap
(`core/shapes/*` for the shape, `core/resolve.*` for opacity — four source
files together, one over the cap, and different enough concerns that
combining them would violate the cap's spirit even if the count somehow
fit).

**Corrections this session:** none. No design assumption made while
writing `tests/test_pageturn.cpp` turned out wrong the way C-011/C-012 did
at WU-11 — the accumulation-sums identity check in particular was designed
to be exact (bit-for-bit, via I6) from the start, specifically to avoid
needing a tolerance-based colour comparison of the kind C-012 warns about.
`close.sh 12a` itself also came back green on the first attempt — no
platform-specific floating-point divergence this session, unlike WU-11's
own experience with C-012.

**Delivery mechanics, not a design matter:** this session ran remotely, via
the device-bridge tools connecting to this machine, same as sessions 6
through 11. All implementation and the full verification matrix above ran
first in a disposable Linux cloud sandbox, never on this machine directly.
Files were then written to this machine via the bridge, and `git add -A &&
git commit` ran through that same bridge; as in prior sessions it still
cannot clean up its own `index.lock`/`HEAD.lock`/temp-object files
afterward (unlink fails on this mount), so stale ones (13
`tmp_obj_*` files, `HEAD.lock`, `index.lock` recreated a second time by
`git status` itself, and a stray `objects/maintenance.lock`) were moved
into `_to_delete/` rather than removed — safe to `rm -rf _to_delete/` by
hand; it now holds accumulated debris from several sessions, not just this
one. Git identity was already set locally on this mount from a prior
session (`Stephen Neal <stephenneal@Stephens-MacBook-Pro.local>`, confirmed
against `git log`/`git config` before committing), so nothing needed
reconfiguring. `./tools/close.sh 12a` was, as before, run by hand at the
real terminal.

## Next work unit

**WU-12b — Page turn, priority-tag opaque.** `WORK-UNITS.md` already has
its own **Files:**/**Accept:** lines, and `DECISIONS.md` ADR-028 already
sketches the mechanism (read the whole of ADR-028's own "priority-tag
opacity" section before starting, not just the `WORK-UNITS.md` summary).
Concretely, next session should:

- Design and freeze `compositeLayered()`'s actual name and signature in
  `core/resolve.hpp` (ADR-028's sketch: two `AccumCell`s — lower, upper —
  the upper layer's own tag, a caller-configured opaque tag, and a
  `Background` — returning a `CompositedCell`), then implement it in
  `core/resolve.cpp`. Two branches: tag matches the opaque tag ->
  `composite()` the lower layer against `bg` first, then blend the upper
  layer's own resolved colour over *that* result using the upper layer's
  own alpha (`cell.w` clamped to `[0, kWeightUnity]`, same convention
  `composite()` already uses); tag does not match -> sum the two
  `AccumCell`s component-wise first (WU-12a's own accumulation-sums
  identity, exact per I6), then `composite()` once — WU-12a's own default,
  reused rather than reimplemented.
- Write `compositeLayered()`'s own test (new file — do not extend `tests/
  test_pageturn.cpp`, which is WU-12a's) reusing WU-12a's own two-layer
  construction (a page-turn flap over a full-canvas page behind) from
  `tests/test_pageturn.cpp`, duplicated locally per `SESSION-PROTOCOL.md`
  rule 2, but this time checking the *opaque* tag path specifically: at
  the flap's own well-covered pixels, the composited result should
  resolve close to the flap's own colour (the page behind is hidden
  there, not blended in) while at pixels where only the page behind has
  coverage it is unaffected — the literal "opaque with priority tag set"
  contrast against WU-12a's own already-proven transparent default.
- No `core/shapes/*`, `core/binner.cpp` or `core/splat.cpp` change is
  expected — if one turns out to be needed, treat that as a reason to stop
  and reconsider the design before writing more code, not push through it,
  per `SESSION-PROTOCOL.md`'s anti-drift rule 3 ("never reopen an ADR...
  propose a superseding ADR instead").
- Once WU-12b is itself green (cloud-verified, then `close.sh 12b`'d), WU-12
  as a whole is done: both of US 4,563,703 FIG. 5's modes reproduced.
  `WORK-UNITS.md`'s WU-12a/WU-12b split does not need reconciling back into
  a single WU-12 entry — the split itself is permanent record, matching
  how WU-03's original stale status line was corrected in place rather
  than erased (WU-04's own session).

## Open questions

Unchanged from WU-10/WU-11: Q1 (tile size), Q2 (4K Mini program outputs),
Q3 (macOS/Desktop Video version) — all still open, none blocking. Q4
(`core/lattice.cpp`'s `jacobian()` edge damping, C-008(a)) — still open,
still not urgent; `tests/test_pageturn.cpp`'s own Jacobian check includes
lattice-edge points (reusing WU-06/WU-11's own point list) and passed at
the tolerance those tests already use, so this session adds no new
evidence either way.

No new open question from this session beyond WU-12b's own design sketch,
already captured above and in ADR-028 — not an "open question" in the Q1-4
sense (those are environment/tuning unknowns), a scoped-but-not-yet-built
next unit instead.

## Blocked / red

Nothing. WU-12a closed green.

## Environment check still outstanding

Unchanged from session 2 — Desktop Video / UltraStudio 4K Mini smoke test,
independent of WU-12b and costs no session time.

## Append to DECISIONS.md

Nothing this update — ADR-028 was appended in full earlier this session;
see `DECISIONS.md`. Not reopened or amended now that the tag is confirmed.

## Append to CORRECTIONS.md

Nothing this update — see "Corrections this session" above; nothing to
log, and the tag is confirmed clean, not reopened or amended now.
