# Handoff
Overwritten at the end of every session. This is the first thing to read.

---

**Session:** 14
**Tag:** still `wu-12b-green` — no new tag yet. WU-13 is implemented and
fully verified in a Linux cloud sandbox (see "Tests"/"Build" below) but
`./tools/close.sh 13` has not been run on the M1 Max with AppleClang yet —
that needs a human at the real terminal (cloud sessions cannot reach
AppleClang). `WORK-UNITS.md`'s WU-13 line is marked `wip`, not `green`,
until that happens.
**Phase:** 2 — Shapes, done as of WU-12 (`wu-12b-green`). WU-13 —
keyframed lattices, temporal interpolation (morph) — is Phase 2's own last
listed unit in `WORK-UNITS.md` and is implemented this session. Phase 3
(SDI output, WU-14/WU-15) is next once WU-13 closes green.

**This session's own first job was design, not implementation** — WU-13
went into the session as a bare `### WU-13 ... `todo`` line with no
**Files:**/**Accept:**, unlike every unit since WU-12a. Re-read
`docs/architecture.md` 4.1 in full (the only place the morph mechanism is
named at all) and `DECISIONS.md` ADR-028's own boundary note (a page
turn's `turnProgress` is not architecture.md 4.1's `t`; WU-13's own morph
is temporal interpolation *between two whole, independently authored
lattices*), then scoped and recorded `WORK-UNITS.md`'s WU-13
**Files:**/**Accept:** lines and wrote `DECISIONS.md` ADR-030 (the design
in full — keyframe count, blend formula and its bit-exactness rationale,
linear vs. Catmull-Rom, function name/signature/home, and why no
`core/shapes/*`, `core/binner.cpp`, `core/splat.cpp`, `core/resolve.*` or
`core/pipeline.cpp` change is needed) *before* writing any implementation
code, the same discipline ADR-028 itself applied to the WU-12a/WU-12b
split. The unit fit within SESSION-PROTOCOL.md's sizing cap as scoped —
two source files touched (`core/lattice.hpp`, `core/lattice.cpp`), one new
test file — so no WU-13a/WU-13b split was needed.

**Tests:** All fourteen green in the Linux cloud sandbox (the thirteen
carried over unchanged from WU-01 through WU-12b plus `test_morph.cpp`,
new this session): boundary reductions at `t == 0`/`t == 1` checked exact
(`==`, every one of 16641 control vertices, both keyframes — the blend
formula is specifically chosen to be rounding-free there, see ADR-030), an
interior `t == 0.35` checked against an independently-computed reference
blend at tight relative tolerance (`1e-12`, C-012), and `Lattice::
jacobian()`'s analytic derivatives checked against central differences
(WU-06's own method, reused) at 20 representative `(u, v)` points across 5
different morph fractions, on a lattice morphed from two distinct,
genuinely curved keyframes (a page-turn mid-curl, which has its own
flat/curl seam, blended with a cylinder at a different radius/span/centre)
— 150189 checks in `test_morph.cpp` alone (Clang 18, Release, tile 2^5).
No `runFrame()`-level check — ADR-030 records why this unit sits at WU-06's
own layer (pure lattice mathematics) rather than the shape layer.

Verified in the Linux cloud sandbox (no AppleClang there) on Clang 18 and
GCC 13, under the project's exact warning set (`-Wall -Wextra -Wpedantic
-Wconversion -Wsign-conversion -Werror`), Release and Debug,
`SCATTER_TILE_LOG2` 4 and 5 (eight configurations, all fourteen tests
green, zero warnings — checked explicitly in the build logs, not just exit
codes), plus GCC 13 with `-fsanitize=address,undefined
-fno-sanitize-recover=all` (Debug) at both tile sizes: clean, no ASan or
UBSan report anywhere — same practice as every session since WU-06.

**Not yet verified:** `./tools/close.sh 13` on the M1 Max with AppleClang
(Release, tile 2^5, the config `close.sh` builds). Please run it — see
"Next work unit" below.

**Build:** clean under `-Werror -Wconversion -Wsign-conversion` on Clang 18
and GCC 13 in the cloud sandbox. AppleClang not yet checked this session.

## Where we are

WU-13 adds `morphLattice(const Lattice& from, const Lattice& to, double t)`
to the existing `core/lattice.hpp`/`.cpp` — no new source file for
`scatter-core` itself, only its own new test executable,
`tests/test_morph.cpp` (same "extend an existing file" shape WU-12b used
for `compositeLayered()`). Given two already-built keyframe `Lattice`s and
a blend fraction, it returns a new `Lattice` whose every control vertex is
`from*(1-t) + to*t`, componentwise across `(x, y, z)` — architecture.md
4.1's own "temporal interpolation between shape lattices... Mirage's
morph", for exactly two keyframes (not an ordered sequence — ADR-028's own
wording already committed to this before this session started; see
ADR-030 for why that reading is recorded, not re-decided).

**Design choices this session had to make — now ADR-030 in
`DECISIONS.md`:** what a keyframe is operationally (nothing more than an
already-built `Lattice`; no new struct pairs one with a time or frame
number, and selecting which two keyframes bracket a given frame plus
reducing that to `t` is left to a future orchestration layer, not
scheduled — the same kind of scope cut ADR-026 made for the k-buffer's
background and ADR-029 made for the opaque tag as a `PipelineParams`
field); the interpolation method (linear, checked against ADR-022's
Catmull-Rom basis and rejected for it — that basis needs a 4-point spatial
neighbourhood a 2-keyframe temporal blend does not have, and nothing in
architecture.md asks for tangent continuity across a morph); the exact
blend formula (`from*(1-t) + to*t`, not the algebraically equivalent
`from + t*(to-from)` — chosen specifically because the first form is
bit-exact at both `t == 0` and `t == 1`, per CORRECTIONS.md C-012's
"multiplying by an exact 0.0/1.0 introduces no rounding" lesson, giving
the morph the same "reduces exactly to the boundary case" property
ADR-027/ADR-028 already established for other shapes' own limiting
parameters); and the function's home (`core/lattice.hpp`/`.cpp`, not a new
`core/keyframe.hpp`/`.cpp` and not `core/shapes/shapes.hpp` — weighed
against ADR-021/ADR-026's "existing header when the new scope doesn't need
one" precedent, and against `shapes.hpp`'s own convention of *populating*
a lattice from a parametric surface, which `morphLattice()` does not do —
it *consumes* two already-populated lattices instead). See ADR-030 for the
full reasoning, including the unclamped-`t` convention and why no
`core/pipeline.cpp` change or `runFrame()`-level test is needed.

**Corrections this session:** none. No implementation choice made while
writing `core/lattice.cpp` or `tests/test_morph.cpp` turned out to
contradict an earlier claim in `DECISIONS.md`, `INVARIANTS.md` or
`CORRECTIONS.md`. The blend formula's bit-exactness at `t == 0`/`t == 1`
was checked directly (exact-equality tests in `tests/test_morph.cpp`),
not merely assumed from the C-012 reasoning alone, and held on both Clang
18 and GCC 13.

**Delivery mechanics, not a design matter:** this session ran remotely,
via the device-bridge tools connecting to this machine, same as sessions 6
through 13. Implementation and the full verification matrix above ran
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
reconfiguring.

## Next work unit

**First: close WU-13.** Run `./tools/close.sh 13` at the real terminal on
the M1 Max. If it comes back green, tag `wu-13-green` and update
`WORK-UNITS.md`'s WU-13 status line from `wip` to `green` (this session
left it `wip` deliberately — see SESSION-PROTOCOL.md, "do not run
close.sh yourself" is a cloud-session limitation, not a sign anything is
suspect). If it comes back red, the failure is most likely a
cross-compiler floating-point comparison the cloud sandbox's Clang
18/GCC 13 combination did not surface — the same class of issue C-012
found at WU-11 — since `test_morph.cpp`'s own exact-equality checks
(`t == 0`/`t == 1`) are the most C-012-sensitive code this session wrote;
if that class of failure shows up, loosen the specific failing check to a
tight relative tolerance within `tests/test_morph.cpp` alone (no
production code change expected), the same fix WU-11's own session made.

**Then:** `WORK-UNITS.md`'s strict ordering ("Units are ordered; do not
skip") names WU-14 — DeckLink device enumeration and `ComPtr` — next,
starting Phase 3 (SDI output). Still `todo`, no **Files:**/**Accept:**
filled in. Unlike WU-13, WU-14 is genuinely new ground for this project —
the first unit to link the Blackmagic DeckLink SDK and the first to touch
`src/io/` beyond the SDK-free `file_source.cpp`/`file_sink.cpp` ADR-021
already carved out — so expect a next session to need real research into
the SDK's `ComPtr`/`IDeckLinkIterator` shape (architecture.md 7) before
scoping, not just a re-read of an existing architecture.md section the way
WU-13 (4.1) and WU-11/12 (4.7) could. It also cannot be built or tested in
a Linux cloud sandbox at all — no Blackmagic SDK there — so expect that
session's own implementation and verification to need to happen directly
on the M1 Max, a different delivery shape than every session since WU-06.

## Open questions

Unchanged from session 13: Q1 (tile size), Q2 (4K Mini program outputs),
Q3 (macOS/Desktop Video version) — all still open, none blocking. Q4
(`core/lattice.cpp`'s `jacobian()` edge damping, C-008(a)) — still open,
still not urgent; this session's own work adds no new evidence either
way — `morphLattice()` blends control-vertex data only, it does not call
`jacobian()` itself, and `test_morph.cpp`'s own Jacobian check exercises
the same `jacobian()` code path C-008(a) already describes, at the same
edges, with no new symptom.

No new open question from this session beyond what ADR-030 already
resolved — see "Design choices" above.

## Blocked / red

Nothing red. WU-13 is `wip`: implemented and fully verified in the cloud
sandbox, waiting only on `./tools/close.sh 13` at the real terminal (see
"Next work unit").

## Environment check still outstanding

Unchanged from session 2 — Desktop Video / UltraStudio 4K Mini smoke test,
independent of WU-13 and costs no session time. Worth doing before WU-14
starts, since WU-14 is the first unit that actually needs the device to
enumerate.

## Append to DECISIONS.md

ADR-030 — see `DECISIONS.md`, appended in full earlier this session.

## Append to CORRECTIONS.md

Nothing this update — see "Corrections this session" above.
