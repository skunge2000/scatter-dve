# Handoff
Overwritten at the end of every session. This is the first thing to read.

---

**Session:** 13
**Tag:** `wu-12b-green` — confirmed. `./tools/close.sh 12b` ran clean on
the M1 Max with AppleClang (Release, tile 2^5, the config `close.sh`
builds) on the first attempt — all thirteen tests passed, no
cloud/AppleClang divergence this time (unlike WU-11's own C-012).
**Phase:** 2 — Shapes, **done**. WU-12b — priority-tag opaque mode — is
implemented (`core/resolve.hpp`/`.cpp` only) and green. Combined with
WU-12a (transparent mode, `wu-12a-green`), both of US 4,563,703 FIG. 5's
compositing modes are now reproduced. The WU-12a/WU-12b split in
`WORK-UNITS.md` is not reconciled back into a single WU-12 entry —
permanent record of how the work actually split, the same precedent
WU-04's session set correcting WU-03's stale status line in place rather
than erasing it. Phase 3 (SDI output, WU-14/WU-15) is next per
`WORK-UNITS.md`'s own ordering, though see "Next" below for a note on
WU-13's own place in that ordering.

**Tests:** All thirteen green on the M1 Max: the twelve carried over
unchanged from WU-01 through WU-12a plus `test_layered_composite`, new
this session (28 checks: four direct `AccumCell` unit-test cases against
`compositeLayered()` alone — tag-mismatch sum path, and the opaque path's
full-alpha/zero-alpha/partial-alpha edges, each checked by exact equality,
not tolerance — plus two pipeline-level checks reusing WU-12a's own
page-turn-flap-over-page-behind construction, duplicated locally per
SESSION-PROTOCOL.md rule 2, checked exactly against an independent local
re-derivation of the read-replace-write formula). 336976 checks total
across all thirteen executables (Clang 18, Release, tile 2^5, in the cloud
sandbox; `close.sh`'s own run reports pass/fail per executable, not a
check count).

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

WU-12b adds `compositeLayered()` to the existing `core/resolve.hpp`/`.cpp`
— no new source file for `scatter-core` itself, only its own new test
executable, `tests/test_layered_composite.cpp`. Given two already-splatted
`AccumCell` layers (`lower`, `upper`), the upper layer's own tag, a
caller-configured opaque tag, and a `Background`, it returns a
`CompositedCell`: where the upper tag matches the opaque tag, `lower` is
composited against `bg` first (the "read"), then `upper`'s own resolved
colour is composited over *that* result using `upper`'s own alpha (the
"write", replacing what was read) — architecture.md 4.7 phase 2's own
"read-replace-write" for exactly two caller-ordered layers, not WU-28's
general k-buffer (ADR-009 unchanged). Where the tags don't match, the two
`AccumCell`s are summed component-wise first (WU-12a's own
accumulation-sums identity, exact per I6) and composited once — WU-12a's
own default, reused not reimplemented.

**Design choices this session had to make that ADR-028's own sketch left
open — now ADR-029 in `DECISIONS.md`:** the function's actual name (kept
`compositeLayered`, ADR-028's own placeholder — nothing surfaced while
implementing it that argued for a different name) and full signature
(`CompositedCell compositeLayered(const AccumCell& lower, const AccumCell&
upper, std::uint8_t upperTag, std::uint8_t opaqueTag, const Background& bg
= kDefaultBackground) noexcept`, parameter order and the defaulted `bg`
mirroring `composite()`'s own shape); the implementation approach — two
calls to `composite()` (the "read" step's `CompositedCell` reinterpreted as
a `Background` for the "write" step, `asBackground()`) rather than a
hand-rolled blend, which gets partial upper coverage correct for free at
both edges (zero alpha -> lower alone unaffected; alpha at or above
`kWeightUnity` -> upper's own colour outright) instead of by extra
casework; and confirming (not deciding fresh — ADR-028's own sketch already
described it this way) that the opaque tag is a function parameter, not a
`PipelineParams` field, since nothing in WU-12b's own scope adds an
orchestration entry point that would need one. See ADR-029 for the full
reasoning, including why only the upper layer's tag is read and not the
lower's.

**Corrections this session:** none. No implementation choice made while
writing `core/resolve.cpp` or `tests/test_layered_composite.cpp` turned out
to contradict an earlier claim in `DECISIONS.md`, `INVARIANTS.md` or
`CORRECTIONS.md` — the "reuse `composite()` twice" approach was checked
directly (the exact-equality tests in `tests/test_layered_composite.cpp`'s
own Part A), not merely assumed correct. `close.sh 12b` itself also came
back green on the first attempt — no platform-specific floating-point
divergence this session, unlike WU-11's own experience with C-012 (this
unit's own arithmetic is entirely fixed-point, per I6, so there was no
transcendental-function or FMA-contraction surface for that class of issue
to appear on in the first place).

**Delivery mechanics, not a design matter:** this session ran remotely, via
the device-bridge tools connecting to this machine, same as sessions 6
through 12. Implementation and the full verification matrix above ran
first in a disposable Linux cloud sandbox, never on this machine directly.
Files were then written to this machine via the bridge, and `git add -A &&
git commit` ran through that same bridge; as in prior sessions it still
cannot clean up its own `index.lock`/`HEAD.lock`/temp-object files
afterward (unlink fails on this mount), so stale ones were moved into
`_to_delete/` rather than removed — safe to `rm -rf _to_delete/` by hand;
it now holds further accumulated debris from this session on top of prior
ones. Git identity was already set locally on this mount from a prior
session (`Stephen Neal <stephenneal@Stephens-MacBook-Pro.local>`, confirmed
against `git log`/`git config` before committing), so nothing needed
reconfiguring. `./tools/close.sh 12b` was, as before, run by hand at the
real terminal.

## Next work unit

`WORK-UNITS.md`'s strict ordering ("Units are ordered; do not skip") names
WU-13 — keyframed lattices, temporal interpolation (morph) — next, still
`todo` with no **Files:**/**Accept:** filled in yet (unlike WU-12b, which
had both waiting from WU-12a's own session). architecture.md 4.1 names the
mechanism ("Optionally keyframed, with temporal interpolation between
shape lattices. This is Mirage's morph") but, like every unit since WU-04,
leaves the concrete parametrisation for the unit that builds it to work
out and record — expect a next session to start by re-reading
architecture.md 4.1 in full, scoping WU-13's own **Files:**/**Accept:**
lines in `WORK-UNITS.md` before writing code (the same sizing-cap
discipline ADR-028 applied to WU-12a/WU-12b), and likely an ADR of its own
for whatever it leaves open. ADR-028's own note (recorded at WU-12a) is
still the relevant boundary to keep in mind: a page turn's `turnProgress`
is *not* an instance of this `t` — WU-13's own morph is lattice-to-lattice
interpolation, a different and still-unbuilt mechanism, orthogonal to any
one shape's own parameters.

## Open questions

Unchanged from WU-10/WU-11/WU-12a: Q1 (tile size), Q2 (4K Mini program
outputs), Q3 (macOS/Desktop Video version) — all still open, none blocking.
Q4 (`core/lattice.cpp`'s `jacobian()` edge damping, C-008(a)) — still open,
still not urgent; this session's own work is entirely within
`core/resolve.*` and adds no new evidence either way.

No new open question from this session beyond what ADR-029 already
resolved — see "Design choices" above.

## Blocked / red

Nothing. WU-12b closed green; WU-12 as a whole done.

## Environment check still outstanding

Unchanged from session 2 — Desktop Video / UltraStudio 4K Mini smoke test,
independent of WU-13 and costs no session time.

## Append to DECISIONS.md

Nothing this update — ADR-029 was appended in full earlier this session;
see `DECISIONS.md`. Not reopened or amended now that the tag is confirmed.

## Append to CORRECTIONS.md

Nothing this update — see "Corrections this session" above; nothing to
log, and the tag is confirmed clean, not reopened or amended now.
