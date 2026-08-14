# Handoff
Overwritten at the end of every session. This is the first thing to read.

---

**Session:** 12
**Tag:** none yet. WU-12a is implemented and verified in a Linux cloud
sandbox only — `./tools/close.sh 12a` has not run on the M1 Max with
AppleClang. **That is the immediate next action, at the real terminal, not
from this session.**
**Phase:** 2 — Shapes. WU-12 split into WU-12a (this session, page-turn
shape + transparent mode — implemented, cloud-verified, not yet
`close.sh`'d) and WU-12b (priority-tag opaque mode — designed, not
implemented). See `DECISIONS.md` ADR-028 for why it split and the design
sketch WU-12b starts from.

**Tests:** All twelve green in the cloud sandbox: the ten carried over
unchanged from WU-10/WU-11 plus `test_shapes` (WU-11's own, untouched this
session) and `test_pageturn`, new this session — 126512 checks (Clang 18,
Release, tile 2^5), checking WU-12a's own accept criteria directly: every
`buildPageTurnLattice()` control vertex is exactly flat or exactly on the
configured curl cylinder per the `turnProgress` split
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

Verified in a Linux cloud sandbox (no AppleClang there — same limitation
every session since WU-06 has had) on Clang 18 and GCC 13, under the
project's exact warning set (`-Wall -Wextra -Wpedantic -Wconversion
-Wsign-conversion -Werror`), Release and Debug, `SCATTER_TILE_LOG2` 4 and
5 (eight configurations, all green, zero warnings — checked explicitly in
the build logs, not just exit codes), plus GCC 13 with
`-fsanitize=address,undefined -fno-sanitize-recover=all` (Debug) at both
tile sizes: clean, no ASan or UBSan report anywhere — same practice as
every session since WU-06.

**Build:** clean under `-Werror -Wconversion -Wsign-conversion` on Clang 18
and GCC 13 in the cloud sandbox. Not yet built with AppleClang.

## Where we are

WU-12a adds `src/core/shapes/pageturn.cpp` and extends
`src/core/shapes/shapes.hpp` with `PageTurnParams`/`buildPageTurnLattice()`
— a page-turn surface, flat near a fixed spine and rolling into a partial
cylinder (arc-length parametrised, so position and tangent match exactly
at the flat/curl seam for any `turnProgress`) for the portion that has
turned. No change to `Lattice`, `core/binner.cpp`, `core/splat.cpp` or
`core/resolve.cpp` — transparent-mode compositing of two surfaces (this
unit's flap plus a "page behind") works by calling
`generateFragments()` twice into one `TileBins` and splatting once, which
`tests/test_pageturn.cpp` proves sums exactly right, not merely assumed
from WU-11's "shape-agnostic" note.

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
needing a tolerance-based colour comparison of the kind C-012 warns about;
see `tests/test_pageturn.cpp`'s own header comment for why it's checked
that way. Two ordinary compile-time fixes during this session's own
sandbox iteration (a `-Wvexing-parse` false-function-declaration on
`std::vector<AccumCell> tileCells(std::size_t(kTilePixels))`, fixed by
brace-initialising instead; an unused-`kPi`-constant warning after that
constant turned out not to be needed in this file) were caught and fixed
before the first successful build — not shipped, not a design error,
nothing to log here.

## Next work unit

**WU-12b — Page turn, priority-tag opaque.** `WORK-UNITS.md` now has its
own **Files:**/**Accept:** lines, and `DECISIONS.md` ADR-028 already
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
- Write `compositeLayered()`'s own test (new file — do not extend
  `tests/test_pageturn.cpp`, which is WU-12a's) reusing WU-12a's own
  two-layer construction (a page-turn flap over a full-canvas page behind)
  from `tests/test_pageturn.cpp`, duplicated locally per
  `SESSION-PROTOCOL.md` rule 2, but this time checking the *opaque* tag
  path specifically: at the flap's own well-covered pixels, the composited
  result should resolve close to the flap's own colour (the page behind is
  hidden there, not blended in) while at pixels where only the page behind
  has coverage it is unaffected — the literal "opaque with priority tag
  set" contrast against WU-12a's own already-proven transparent default.
- No `core/shapes/*`, `core/binner.cpp` or `core/splat.cpp` change is
  expected — if one turns out to be needed, treat that as a reason to stop
  and reconsider the design before writing more code, not push through it,
  per `SESSION-PROTOCOL.md`'s anti-drift rule 3 ("never reopen an ADR...
  propose a superseding ADR instead").
- Once WU-12b is itself green (cloud-verified, then `close.sh 12b`'d),
  WU-12 as a whole (the parent entry `HANDOFF.md`'s session-11 note and
  the original task both referred to) is done: both of US 4,563,703 FIG.
  5's modes reproduced. `WORK-UNITS.md`'s WU-12a/WU-12b split does not need
  reconciling back into a single WU-12 entry — the split itself is
  permanent record, matching how WU-03's original stale status line was
  corrected in place rather than erased (WU-04's own session).

**Before that, at the real terminal:** run `./tools/close.sh 12a`. Tags
`wu-12a-green` on success (the tag format `close.sh` produces from
whatever string is passed — `12a` here, not just a bare number — is
untested by any earlier session but nothing in `tools/close.sh` assumes
`NN` is numeric; it only uses it to build the tag name and the commit
message). If it comes back red, the same discipline every session since
WU-06 uses: isolate the failure, fix within the smallest possible file
scope, re-verify the full cloud-sandbox matrix, ship the fix as its own
commit, ask for `close.sh 12a` again — don't guess at a fix without seeing
the actual failure output first.

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

Nothing red. WU-12a is cloud-green, awaiting `close.sh 12a` on real
hardware before it can be tagged.

## Environment check still outstanding

Unchanged from session 2 — Desktop Video / UltraStudio 4K Mini smoke test,
independent of WU-12a/WU-12b and costs no session time.

## Append to DECISIONS.md

Nothing this update — ADR-028 was appended in full earlier this session;
see `DECISIONS.md`. Not reopened or amended now.

## Append to CORRECTIONS.md

Nothing this update — see "Corrections this session" above.
