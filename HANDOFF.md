# Handoff
Overwritten at the end of every session. This is the first thing to read.

---

**Session:** 43 (WU-23 scoping + WU-23a build — Phase 6 opens; field mode's
own field-split/interleave half; no DeckLink, no hardware).

**Tag:** none yet — `wu-23a-green` is Steve's own next action; see "Steve's
own next steps" below.

## Before doing anything else in the next session

Run `git tag`, `git log --oneline -10`, `git status --short` and `git status
-sb` directly against `~/src/scatter-dve` via the device bridge, the same as
every session before this one — do not trust this file's own account of
tag/commit state without checking it against the real repository first.

## This session in full

Opened by verifying real repository state three ways, not just one: `git
tag --sort=creatordate`, `git log --oneline -10` and `git status -sb`
against a fresh GitHub clone; then diffing that clone byte-for-byte against
the same files staged live off `~/src/scatter-dve` via the device bridge.
Identical. `HEAD` was `0dc2247` ("WORK-UNITS.md: correct six stale status
lines..."), one commit past the `wu-36-green` tag (`d32131b`); tree clean,
`## main...origin/main`, no ahead/behind. Worth flagging, not fixing:
`HANDOFF.md`'s own Session-42 account never mentioned `0dc2247` — a small,
harmless second instance of the exact "status word lagging the real
repository" problem the continuation prompt that opened this session warned
about, this time in `HANDOFF.md` itself rather than `WORK-UNITS.md`. Nothing
to correct (no claim was wrong, `0dc2247` is documentation-only and the tree
was clean) — noted for whoever next feels surprised by it.

This was a continuation prompt's own two-part job: real scoping for WU-23
("Interlace and field mode," Phase 6's own opening bare line, untouched
since it was written) before any code, then build and verify whatever the
first split piece turned out to be.

**Scoping.** Read `docs/architecture.md` section 5's own "Interlace" note
and section 10's Phase 6 "done when" line (which, read directly, does not
actually mention interlace — flagged, not fixed, see ADR-075), the current
`core/pipeline.cpp`/`resolve.hpp`/`binner.hpp`/`.cpp`/`lattice.hpp`/
`types.hpp` and `video/raster.hpp` directly (not assumed from the
architecture doc's own diagram), and grepped `src/`/`tests/`/`DECISIONS.md`
for any existing interlace/field scaffolding — none, confirming WU-36's own
audit. Confirmed de-interlace-to-frame and field mode are genuinely
different code paths (one a real temporal reconstruction filter, Steve's
own Weston-3-field preference; the other no reconstruction at all, just an
independent per-field warp) and split them: **WU-23a**/**WU-23a2** (field
mode's own two halves) and **WU-23b** (de-interlace-to-frame, gated on
working out Weston 3-field itself first). Full reasoning in `DECISIONS.md`
ADR-075.

**A further split, found while building, not before.** The scoping
session's own first-cut plan for field mode — extract each field as its own
half-height `SourceRaster`, run today's unchanged `generateFragments()` on
each independently — turned out to silently discard the half-line vertical
phase between the two fields, because `core/binner.hpp`'s v-parameter
normalises across *whatever height is passed in*, not the true frame
height. Fixing that honestly needs a new `core/binner.hpp`/`.cpp` sibling
entry point, which together with the two new `video/interlace.*` files
would exceed `SESSION-PROTOCOL.md`'s 3-source-file cap — so field mode
itself split again: WU-23a is the pure data-layout half (field split and
interleave, no lattice/warp involvement at all), WU-23a2 is the
lattice-aware half, not started. See ADR-075 and `video/interlace.hpp`'s
own file comment for the complete reasoning.

**WU-23a build.** New `src/video/interlace.hpp`/`.cpp`: `FieldParity`
(`Top`/`Bottom`), `fieldRowCount()` (a `constexpr` sizing helper, same
spirit as `core/binner.hpp`'s own `tileCount()`), `extractField()` (frame
→ one field's own rows, a pure row copy), `interleaveFields()` (two fields
→ one frame, its own exact inverse). New `tests/test_interlace.cpp`:
`fieldRowCount()` accounts for every row of a frame exactly once between
the two parities (checked directly); `extractField()` reproduces each
field's own source rows bit-exactly, both parities, even and odd frame
height; `interleaveFields()` round-trips a marked test frame bit-for-bit,
every plane, every row, for even and odd frame height and for this
project's own two real geometries (720×576, 1920×1080) — this unit's own
first Accept: criterion. `CMakeLists.txt`: `src/video/interlace.cpp` added
to `scatter-core`'s source list, `scatter_test(test_interlace)` added.
+297/-0 across the four files.

Built and tested in this session's own Linux cloud sandbox (Ubuntu 24.04):
full 8-configuration matrix (GCC 13.3.0 / Clang 18.1.3 × Release/Debug ×
`SCATTER_TILE_LOG2` 4/5), all green, zero warnings under this project's
full `-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror` set,
plus GCC 13 `-fsanitize=address,undefined -fno-sanitize-recover=all` at
both tile sizes, clean. No AArch64 cross-compile run — this unit touches no
platform-specific surface at all (no NEON, no Apple-only API), the same
scope WU-16a/WU-19a/WU-26/WU-28c's own portable-only units already used.
`ctest`: 24 of 24 in every configuration (23 pre-existing, unaffected, plus
`test_interlace`). `test_interlace` alone: 302 checks passing.

Delivered `src/video/interlace.hpp`, `src/video/interlace.cpp`,
`tests/test_interlace.cpp`, `CMakeLists.txt`, `WORK-UNITS.md`,
`DECISIONS.md` (ADR-075) and this file to the real repository via
`SendUserFile` + `device_commit_files`, then re-staged all of them from the
device and diffed against this session's own edited copies before writing
this sentence — `SESSION-PROTOCOL.md`'s own rule 8.

## Where we are

`WU-23` (bare `todo`) is now three real units: **WU-23a** built and
cloud-sandbox-green this session, pending Steve's own real-terminal
confirmation and tag; **WU-23a2** (lattice-aware per-field fragment
generation) and **WU-23b** (de-interlace-to-frame, Weston 3-field) both
scoped-in-name-only, `todo`, not started. `WU-24`/`WU-25` untouched.
`DECISIONS.md` now runs through ADR-075; `INVARIANTS.md` unchanged through
I11; `CORRECTIONS.md` unchanged through C-023 (nothing this session rose to
a correction — see "Append to CORRECTIONS.md" below).

## Next work unit

**WU-23a2** is the natural next pick if continuing straight down Phase 6's
own field-mode thread: real `Files:`/`Accept:` scoping for the
`core/binner.hpp`/`.cpp` sibling entry point ADR-075 already names (field-
parity-aware fragment generation, v-parameter denominator kept at full
frame height), then wiring it together with this session's own
`video/interlace.*` into an actual `runFrame()`-level field-mode entry
point. **WU-23b** (de-interlace-to-frame) is not a real pick yet — it needs
its own research/design pass on the actual Weston 3-field algorithm before
any `Files:`/`Accept:` scoping is possible, let alone code. Everything
named in Session 42's own "Next work unit" section (WU-28d, WU-27, WU-33,
WU-35, WU-37) is unchanged and still pickable — this session did not touch
Phase 7 at all.

## Open questions

Unchanged from Session 42's own list (`kCaptureRingCapacity`, Q3, Q4, Task
A1, Task D6, ADR-070's open question, WU-35's `compositeLayered()`
question), plus one addition: the real Weston 3-field algorithm itself is
not yet worked out anywhere in this repository — WU-23b is blocked on that
research, not on any code decision.

## Blocked / red

Nothing red. WU-23b is named-but-blocked (see above), not red — no code has
been attempted for it to fail.

## Environment check

This session's own build/test verification is the Linux cloud sandbox
matrix described above, not a real-terminal run. `docs/wu-audit-2026-08.md`
and everything else from Session 42 is unaffected — this session added
files, it did not touch anything Session 42 touched.

## Append to DECISIONS.md

**ADR-075** — already appended in full this session (WU-23 scoping split;
field mode's own v-parameter finding; WU-23a build). See `DECISIONS.md`.

## Append to CORRECTIONS.md

Nothing this session. The `HANDOFF.md`-lags-`HEAD`-by-one-commit
observation and the architecture.md-Phase-6-doesn't-mention-interlace
observation are both documentation gaps noticed, not claims that were made
and later found wrong — recorded in this file and in ADR-075 respectively,
the same "worth a look, not a correction" framing WU-36's own audit used
for its own two similar cosmetic findings.

## Closed out this session

**WU-23 scoping** (three-way split, ADR-075) **and WU-23a build** (field
split and interleave). Cloud-sandbox green, full matrix, zero warnings,
clean sanitizers. Ready for Steve's own real-terminal build, commit, tag
and push.

## Steve's own next steps

**1. Confirm the tree at your own real terminal.**

```
cd ~/src/scatter-dve
git status --short
```

Expected: `src/video/interlace.hpp` and `src/video/interlace.cpp`
untracked (new files); `tests/test_interlace.cpp` untracked (new file);
`CMakeLists.txt`, `WORK-UNITS.md`, `DECISIONS.md` and `HANDOFF.md`
modified. Nothing else.

**2. Build and test.**

```
cd ~/src/scatter-dve
cmake --build build
ctest --test-dir build --output-on-failure
```

Expected: every previously-passing test still passes, plus a new
`test_interlace` (302 checks, per this session's own cloud-sandbox run).

**3. Commit, then close (build + test + tag + push, in one step).**

```
cd ~/src/scatter-dve
git add CMakeLists.txt src/video/interlace.hpp src/video/interlace.cpp \
        tests/test_interlace.cpp WORK-UNITS.md DECISIONS.md HANDOFF.md
git commit -m "WU-23a: field split and interleave (ADR-075)"
./tools/close.sh 23a
```

`close.sh` builds, runs `ctest`, and — only if both succeed — tags
`wu-23a-green` and pushes the commit and tag to `origin` automatically. If
it refuses or fails, it will say why; do not tag by hand unless it names the
already-accepted `test_decklink_device`/ADR-035 exception specifically (this
unit has no DeckLink-linked test at all, so that exception should not come
up).

**4. Verify it landed correctly.**

```
cd ~/src/scatter-dve
git log --oneline -3
git tag | tail -5
git status -sb
```

`git log --oneline -3` should show your own new commit at `HEAD`, carrying
`wu-23a-green`; `git tag | tail -5` should include `wu-23a-green`;
`git status -sb` should read `## main...origin/main` with no ahead marker
and no modified files listed at all.
