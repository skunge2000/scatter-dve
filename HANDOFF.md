# Handoff
Overwritten at the end of every session. This is the first thing to read.

---

**Session:** 44 (WU-23a2 scoping + WU-23a2a build — field mode's own
lattice-aware per-field fragment generation, split further; no DeckLink,
no hardware).

**Tag:** none yet — `wu-23a2a-green` is Steve's own next action; see
"Steve's own next steps" below.

## Before doing anything else in the next session

Run `git tag --sort=creatordate`, `git log --oneline -10`, `git status
--short` and `git status -sb` directly against `~/src/scatter-dve` via the
device bridge, the same as every session before this one — do not trust
this file's own account of tag/commit state without checking it against
the real repository first.

## This session in full

Opened with a continuation prompt (`CORRECTIONS.md` C-024, from the
session before this one) whose own job was two-fold: confirm real
repository state, then give a real scoping proposal for WU-23a2 —
including the prompt's own open question (is `extractField()` needed by
the honest fix?) worked out from the real code — *before* writing any
code, and only build once that scoping was actually confirmed with Steve.

**Repository state, confirmed three ways before reading anything else:**
`git tag --sort=creatordate` (newest: `wu-23a-green`), `git log --oneline
-10` (`HEAD` = `46e9240`, "WU-23a: field split and interleave
(ADR-075)"), `git status -sb` (`## main...origin/main`, clean, no
ahead/behind) — all run directly against `~/src/scatter-dve` via the
device bridge. Matches `HANDOFF.md`'s own prior account exactly; nothing
to correct.

**Scoping.** Read `SESSION-PROTOCOL.md`, `INVARIANTS.md`, `DECISIONS.md`
ADR-075/ADR-034/ADR-035/ADR-037, `CORRECTIONS.md` C-024, `WORK-UNITS.md`'s
own WU-23a2 entry, and the real code directly: `core/binner.hpp`/`.cpp`
(`generateFragmentsRowRangeImpl()`'s own row-range/full-height-denominator
split, ADR-041), `core/resolve.hpp`/`core/pipeline.cpp` (ADR-026's own
precedent for where new orchestration entry points get declared), and
`video/interlace.hpp`/`.cpp` (WU-23a, unmodified this session). Two real
findings, both in `DECISIONS.md` ADR-076:

1. **The `extractField()`-usage question, settled.** Not needed on the
   input side — a new binner sibling entry point strides the *full*
   `SourceRaster` directly (offset 0 or 1, step 2), keeping the
   v-parameter's denominator at `src.height` the whole time, the same
   mechanism WU-16b's own row-range entry point already established,
   generalised from a contiguous range to a strided one. Needed on the
   output side, though — correcting the continuation prompt's own
   imprecise phrasing there: `interleaveFields()` cannot do "even/odd row
   selection" out of two full-resolution per-field warp outputs by
   itself (its own precondition requires two *already field-sized*
   rasters); the real sequence is `extractField()` down to each field's
   own parity rows first, *then* `interleaveFields()` to recombine.
2. **File budget.** The binner sibling (2 files) plus the driver (2 more,
   per ADR-026's precedent — `core/resolve.hpp`/`core/pipeline.cpp`)
   together exceed `SESSION-PROTOCOL.md`'s 3-file cap for one unit, the
   same shape that already split WU-23a from WU-23a2. Split into
   **WU-23a2a** (binner sibling alone) and **WU-23a2b** (the driver, not
   started).

Confirmed directly with Steve, before any code: the WU-23a2a/WU-23a2b
split; `rowOffset` as a plain `int` rather than `video::FieldParity` (so
`core/binner.hpp` keeps its existing zero dependency on `video/`); and
building WU-23a2a alone this session, WU-23a2b left for next time. All
three the recommended option.

**WU-23a2a build.** `core/binner.hpp`/`.cpp`: new
`generateFragmentsFieldRows()`; the shared private
`generateFragmentsRowRangeImpl()` gains a `rowStep` parameter (all three
existing call sites pass 1 explicitly, unchanged behaviour byte for
byte). `tests/test_binner.cpp`: two new test functions —
`test_field_rows_match_row_range_ground_truth()` (per-row
`generateFragmentsRowRange()` calls, accumulated, equal one
`generateFragmentsFieldRows()` call byte-for-byte, not merely as a
multiset) and
`test_field_rows_reject_naive_half_height_extraction_bug()` (the naive
`extractField()`-then-`generateFragments()` plan ADR-075 already rejected
is shown, directly, to collapse Bottom field's own row 0 onto Top field's
own row 0's destination, where the honest fix correctly keeps them one
pixel apart) — plus three small helpers (`sameFrag()`, `samePosition()`,
`findFrags()`). No `CMakeLists.txt` change — `test_binner` was already
registered.

Built and tested in this session's own Linux cloud sandbox (Ubuntu
24.04): full 8-configuration matrix (GCC 13.3.0 / Clang 18.1.3,
Release/Debug, `SCATTER_TILE_LOG2` 4/5), all green, zero warnings under
this project's full `-Wall -Wextra -Wpedantic -Wconversion
-Wsign-conversion -Werror` set, plus GCC 13
`-fsanitize=address,undefined -fno-sanitize-recover=all` at both tile
sizes, clean. No AArch64 cross-compile — this unit touches no
platform-specific surface. `ctest`: 24 of 24 in every configuration
(unchanged count — `test_binner` is extended, not new). `test_binner`
alone: 38 727 checks passing.

Delivered `core/binner.hpp`, `core/binner.cpp`, `tests/test_binner.cpp`,
`WORK-UNITS.md`, `DECISIONS.md` (ADR-076) and this file to the real
repository via `SendUserFile` + `device_commit_files`, then re-staged all
of them from the device and diffed against this session's own edited
copies before writing this sentence — `SESSION-PROTOCOL.md`'s own rule 8.

## Where we are

`WU-23a2` is now two real units: **WU-23a2a** built and
cloud-sandbox-green this session, pending Steve's own real-terminal
confirmation and tag; **WU-23a2b** (the `runFrame()`-level driver) scoped
in outline (see `DECISIONS.md` ADR-076 and `WORK-UNITS.md`'s own entry)
but not yet given real `Files:`/`Accept:` lines — that is next session's
first job if it picks this thread up. `WU-23b`/`WU-24`/`WU-25` untouched.
`DECISIONS.md` now runs through ADR-076; `INVARIANTS.md` unchanged
through I11; `CORRECTIONS.md` unchanged through C-024 (nothing this
session rose to a correction — see "Append to CORRECTIONS.md" below).

## Next work unit

**WU-23a2b** is the natural next pick if continuing straight down Phase
6's own field-mode thread: real `Files:`/`Accept:` scoping for the
`core/resolve.hpp`/`core/pipeline.cpp` driver `DECISIONS.md` ADR-076
already outlines (call `generateFragmentsFieldRows()` twice, once per
parity, into two full-resolution rasters; `extractField()` each down to
its own parity rows; `interleaveFields()` to recombine), then building
it — including the identity-lattice round-trip Accept: criterion WU-23a's
own first Accept: criterion deliberately deferred (ADR-075, `HANDOFF.md`'s
Session-43 entry). No shared identity-lattice test helper exists yet to
reuse — `tests/test_binner.cpp` and `tests/test_zoneplate.cpp` each
duplicate their own locally; WU-23a2b's own test does the same.
Everything named in Session 43's own "Next work unit" section (WU-28d,
WU-27, WU-33, WU-35, WU-37) is unchanged and still pickable — this
session did not touch Phase 7 at all.

## Open questions

Unchanged from Session 42/43's own list (`kCaptureRingCapacity`, Q3, Q4,
Task A1, Task D6, ADR-070's open question, WU-35's `compositeLayered()`
question, the real Weston 3-field algorithm for WU-23b) — this session
did not touch any of them.

## Blocked / red

Nothing red. WU-23b remains named-but-blocked (Session 43's own note,
unchanged), not red.

## Environment check

This session's own build/test verification is the Linux cloud sandbox
matrix described above, not a real-terminal run.

**Standing condition, unchanged from Session 43, C-024: `tools/close.sh`
cannot currently succeed at all, for any unit, on Steve's real
terminal.** The 4K Mini's PSU is still out; the going-forward hardware is
a Monitor 3G (output-only) and a Recorder 3G (capture-only), and per
ADR-034/ADR-035, `test_decklink_device`'s own
`test_at_least_one_device_is_full_duplex` check will keep failing even
once the PSU is fixed, because the real architecture is two devices now,
not one full-duplex one. `close.sh` treats any `ctest` failure as
blocking and refuses to tag — **this session's own close-out is manual
build, manual test, confirm no failure other than that one named check,
then tag by hand and push explicitly** (`git push origin main`; `git push
origin --tags`), the same as Session 43's own close-out. This unit has no
DeckLink dependency of its own (it never links `scatter-decklink`, and
this cloud sandbox has no Blackmagic SDK configured at all — the check
does not even run here), but the check still runs on Steve's own real
terminal, where the SDK *is* configured, on every close-out regardless of
which unit is being closed — C-024's own point, still true.

## Append to DECISIONS.md

**ADR-076** — already appended in full this session (WU-23a2
scoping/split; the `extractField()`-usage question settled; WU-23a2a
build). See `DECISIONS.md`.

## Append to CORRECTIONS.md

Nothing this session. The continuation prompt's own imprecise phrasing
about what `interleaveFields()` does (see ADR-076) is a gap in the
prompt, not a claim this project's own state files made and later found
wrong — recorded in ADR-076's own text, the same "worth a look, not a
correction" framing prior sessions have used for similar cosmetic
findings outside this project's own prior claims.

## Closed out this session

**WU-23a2 scoping** (two-way split, ADR-076) **and WU-23a2a build**
(field-parity row visitation in `core/binner.hpp`/`.cpp`). Cloud-sandbox
green, full matrix, zero warnings, clean sanitizers. Ready for Steve's
own real-terminal build, commit, tag and push.

## Steve's own next steps

**1. Confirm the tree at your own real terminal.**

```
cd ~/src/scatter-dve
git status --short
```

Expected: `core/binner.hpp`, `core/binner.cpp`, `tests/test_binner.cpp`,
`WORK-UNITS.md`, `DECISIONS.md` and `HANDOFF.md` modified. Nothing else —
no new (untracked) files this session, unlike WU-23a.

**2. Build and test — manually, not via `tools/close.sh` (see above).**

```
cd ~/src/scatter-dve
cmake --build build
ctest --test-dir build --output-on-failure
```

Expected: every previously-passing test still passes, `test_binner`'s own
check count is up (38 727 in this session's own cloud-sandbox run,
against the prior count before WU-23a2a) — **except**
`test_decklink_device`'s own `test_at_least_one_device_is_full_duplex`
check, which is expected to keep failing until either the hardware or the
check itself changes (see "Environment check" above and C-024 in
`CORRECTIONS.md`). That one named failure is not a regression from this
unit; anything else failing is.

**3. Commit, then tag and push by hand — `tools/close.sh` cannot succeed
right now (see above), so do not run it.**

```
cd ~/src/scatter-dve
git add core/binner.hpp core/binner.cpp tests/test_binner.cpp \
        WORK-UNITS.md DECISIONS.md HANDOFF.md
git commit -m "WU-23a2a: field-parity row visitation in core/binner.hpp/.cpp (ADR-076)"
git tag -a wu-23a2a-green -m "WU-23a2a: field-parity row visitation, green except the already-accepted ADR-035/ADR-034 duplex-check exception"
git push origin main
git push origin --tags
```

**4. Verify it landed correctly.**

```
cd ~/src/scatter-dve
git log --oneline -3
git tag | tail -5
git status -sb
```

`git log --oneline -3` should show your own new commit at `HEAD`,
carrying `wu-23a2a-green`; `git tag | tail -5` should include
`wu-23a2a-green`; `git status -sb` should read `## main...origin/main`
with no ahead marker and no modified files listed at all.
