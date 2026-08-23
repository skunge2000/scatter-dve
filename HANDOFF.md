# Handoff
Overwritten at the end of every session. This is the first thing to read.

---

**Session:** 45 (WU-23a2b build — field mode's own runFrame()-level
driver; no DeckLink, no hardware).

**Tag:** none yet — `wu-23a2b-green` is Steve's own next action; see
"Steve's own next steps" below.

## Before doing anything else in the next session

Run `git tag --sort=creatordate`, `git log --oneline -10`, `git status
--short` and `git status -sb` directly against `~/src/scatter-dve` via
the device bridge, the same as every session before this one — do not
trust this file's own account of tag/commit state without checking it
against the real repository first.

## This session in full

Opened with a continuation prompt whose own job was two-fold: confirm
real repository state, then give a real scoping proposal for WU-23a2b —
the driver's exact signature, which height `fieldRowCount()` is evaluated
against, the threading-scope call, and the new test file's own name and
Accept: criteria — worked out from the real code, *before* writing any
code, and only build once that scoping was actually confirmed with
Steve.

**Repository state, confirmed three ways before reading anything else:**
`git tag --sort=creatordate` (newest: `wu-23a2a-green`), `git log
--oneline -10` (`HEAD` = `c07f38b`, "WU-23a2a: field-parity row
visitation in core/binner.hpp/.cpp (ADR-076)"), `git status -sb` (`##
main...origin/main`, clean, no ahead/behind) — all run directly against
`~/src/scatter-dve` via the device bridge. Matches `HANDOFF.md`'s own
prior account exactly; nothing to correct.

**Scoping.** Read `SESSION-PROTOCOL.md`, `INVARIANTS.md`, `DECISIONS.md`
ADR-034/ADR-035/ADR-037/ADR-041/ADR-075/ADR-076, `CORRECTIONS.md` C-024,
`WORK-UNITS.md`'s own WU-23a/WU-23a2/WU-23a2a/WU-23a2b entries,
`docs/architecture.md` section 5's own "Interlace" note, and the real
code directly: `core/resolve.hpp`, `core/pipeline.cpp` (`runFrame()`'s
own `threads<=1` branch and its private `resolveOneTile()`),
`core/binner.hpp` (`generateFragmentsFieldRows()`'s own signature and doc
comment), `video/interlace.hpp`/`.cpp` (unmodified this session).
Proposed, then confirmed with Steve directly before any code:

1. **Signature: one `Lattice`, not two.** `generateFragmentsFieldRows()`
   already takes a single lattice plus `rowOffset`; the driver calls it
   twice, varying only `rowOffset`.
2. **`fieldRowCount()` evaluated against `params.destHeight`, not
   `src.height`** — each parity's own resolve is already a full
   destination-sized raster, so `extractField()` decimates *that*, keyed
   to the destination frame's own height.
3. **Declared in `core/resolve.hpp`, implemented in
   `core/pipeline.cpp`** — ADR-021/ADR-026's own precedent for a new
   orchestration entry point with no state of its own.
4. **Single-threaded only, this unit** — `params.threads`/`params.pool`
   not consulted; a threaded field-mode path deferred, not scheduled.
5. **`tests/test_field_pipeline.cpp`, new**, registered in
   `CMakeLists.txt`.

All five confirmed as the recommended option, matching this session's own
written proposal exactly.

**WU-23a2b build.** `runFrameField()` added to `core/resolve.hpp`
(declaration) / `core/pipeline.cpp` (definition): calls the existing
private `resolveOneTile()` directly, in the exact shape `runFrame()`'s
own `threads<=1` branch already uses, once per parity
(`generateFragmentsFieldRows()` standing in for `generateFragments()` as
PASS 1's own entry point), into two temporary full-resolution rasters;
`video::extractField()` decimates each to its own parity rows of the
destination frame; `video::interleaveFields()` recombines them into
`dest`. Two restraint decisions surfaced while implementing, not
anticipated in ADR-076's own scoping text: `resolveOneTile()` also
drives `PipelineParams::kBufferMode`/`weightOut` when a caller sets them,
and wiring either through unexamined would be silently wrong for field
mode specifically (a null-pointer crash for `kBufferMode`; a silent
single-parity clobber for `weightOut`, since both per-parity resolves
cover the same destination index space) — resolved by requiring both
left at their defaults, documented as unchecked preconditions, the same
restraint ADR-026/ADR-029 already used elsewhere rather than inventing an
answer nobody has asked for. See `DECISIONS.md` ADR-077 for the full
account.

Built and tested in this session's own Linux cloud sandbox (Ubuntu
24.04): full 8-configuration matrix (GCC 13.3.0 / Clang 18.1.3,
Release/Debug, `SCATTER_TILE_LOG2` 4 and 5), all green, zero warnings
under this project's full `-Wall -Wextra -Wpedantic -Wconversion
-Wsign-conversion -Werror` set, plus GCC 13 `-fsanitize=address,undefined
-fno-sanitize-recover=all` at both tile sizes, clean. `ctest`: 25 of 25
in every configuration (24 carried over unchanged, plus
`test_field_pipeline`, new). `test_field_pipeline` alone: 27654 checks
passing.

Delivered `core/resolve.hpp`, `core/pipeline.cpp`,
`tests/test_field_pipeline.cpp`, `CMakeLists.txt`, `WORK-UNITS.md`,
`DECISIONS.md` (ADR-077) and this file to the real repository via
`SendUserFile` + `device_commit_files`, then re-staged all of them from
the device and diffed against this session's own edited copies before
writing this sentence — `SESSION-PROTOCOL.md`'s own rule 8.

## Where we are

**Phase 6's own field-mode thread is now complete: WU-23a, WU-23a2a and
WU-23a2b all built and cloud-sandbox-green**, pending Steve's own
real-terminal confirmation and tag for this unit (`wu-23a2a-green` is
already tagged from the session before this one). `WU-23b` (de-interlace
to frame, Weston 3-field) is separate again and still not started —
gated on its own Weston-3-field research, not on this unit.
`WU-24`/`WU-25` untouched. `DECISIONS.md` now runs through ADR-077;
`INVARIANTS.md` unchanged through I11; `CORRECTIONS.md` unchanged through
C-024 (nothing this session rose to a correction — see "Append to
CORRECTIONS.md" below).

## Next work unit

**WU-23b** (de-interlace to frame, Weston 3-field, then re-interlace) is
the natural next pick if continuing straight down Phase 6's own interlace
thread — but it is gated on working out the real Weston 3-field algorithm
first (its own research/design step, likely its own ADR, before any
`Files:`/`Accept:` scoping is possible), genuinely new ground, not an
extension of WU-23a/WU-23a2's own field-split machinery (which this
unit's own re-interlace half reuses only for the trivial
decimate-on-output direction). Everything named in Session 43's own "Next
work unit" section (WU-28d, WU-27, WU-33, WU-35, WU-37) is unchanged and
still pickable — this session did not touch Phase 7 at all.

## Open questions

Unchanged from Session 42/43's own list (`kCaptureRingCapacity`, Q3, Q4,
Task A1, Task D6, ADR-070's open question, WU-35's `compositeLayered()`
question, the real Weston 3-field algorithm for WU-23b) — this session
did not touch any of them. One new item, named but not resolved by this
session (ADR-077): what weight-capture (`PipelineParams::weightOut`) or
k-buffer resolve (`PipelineParams::kBufferMode`) should even mean for
field mode's own two independently-resolved parities — `runFrameField()`
currently requires both left at their defaults; a future unit that needs
either has a real design question to answer first.

## Blocked / red

Nothing red. WU-23b remains named-but-blocked (Session 43's own note,
unchanged), not red.

## Environment check

This session's own build/test verification is the Linux cloud sandbox
matrix described above, not a real-terminal run.

**Standing condition, unchanged from Session 43/44, C-024: `tools/close.sh`
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
origin --tags`), the same as Session 43/44's own close-out. This unit has
no DeckLink dependency of its own (it never links `scatter-decklink`, and
this cloud sandbox has no Blackmagic SDK configured at all — the check
does not even run here), but the check still runs on Steve's own real
terminal, where the SDK *is* configured, on every close-out regardless of
which unit is being closed — C-024's own point, still true.

## Append to DECISIONS.md

**ADR-077** — already appended in full this session (WU-23a2b build:
`runFrameField()`, field mode's own `runFrame()`-level driver). See
`DECISIONS.md`.

## Append to CORRECTIONS.md

Nothing this session.

## Closed out this session

**WU-23a2b build** (field mode's own `runFrame()`-level driver,
`runFrameField()`). Cloud-sandbox green, full matrix, zero warnings,
clean sanitizers. Ready for Steve's own real-terminal build, commit, tag
and push. **Phase 6's own field-mode thread (WU-23a/WU-23a2a/WU-23a2b) is
now complete.**

## Steve's own next steps

**1. Confirm the tree at your own real terminal.**

```
cd ~/src/scatter-dve
git status --short
```

Expected: `src/core/resolve.hpp`, `src/core/pipeline.cpp` and
`CMakeLists.txt` modified; `tests/test_field_pipeline.cpp` new
(untracked); `WORK-UNITS.md`, `DECISIONS.md` and `HANDOFF.md` modified.
Nothing else.

**2. Build and test — manually, not via `tools/close.sh` (see
"Environment check" above).**

```
cd ~/src/scatter-dve
cmake --build build
ctest --test-dir build --output-on-failure
```

Expected: every previously-passing test still passes, plus the new
`test_field_pipeline` (27654 checks in this session's own cloud-sandbox
run) — **except** `test_decklink_device`'s own
`test_at_least_one_device_is_full_duplex` check, which is expected to
keep failing until either the hardware or the check itself changes (see
"Environment check" above and C-024 in `CORRECTIONS.md`). That one named
failure is not a regression from this unit; anything else failing is.

**3. Commit, then tag and push by hand — `tools/close.sh` cannot succeed
right now (see above), so do not run it.**

```
cd ~/src/scatter-dve
git add src/core/resolve.hpp src/core/pipeline.cpp tests/test_field_pipeline.cpp \
        CMakeLists.txt WORK-UNITS.md DECISIONS.md HANDOFF.md
git commit -m "WU-23a2b: field mode's own runFrame()-level driver, runFrameField() (ADR-077)"
git tag -a wu-23a2b-green -m "WU-23a2b: field mode's own runFrame()-level driver, green except the already-accepted ADR-035/ADR-034 duplex-check exception"
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
carrying `wu-23a2b-green`; `git tag | tail -5` should include
`wu-23a2b-green`; `git status -sb` should read `## main...origin/main`
with no ahead marker and no modified files listed at all.
