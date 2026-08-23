# Handoff
Overwritten at the end of every session. This is the first thing to read.

---

**Session:** 41 (WU-28c close-out — confirming and tagging Session 39's
own already-built, already-sandbox-tested code at Steve's real terminal; no
new design work, no source file changed this session).

**Tag:** `wu-28c-green`, tagged and pushed at Steve's own real terminal this
session, on top of `wu-32-green`.

## Before doing anything else in the next session

Run `git tag`, `git log --oneline -10`, `git status --short` and `git status
-sb` directly against `~/src/scatter-dve` via the device bridge, the same as
every session before this one — do not trust this file's own account of
tag/commit state without checking it against the real repository first.

## This session in full

Opened by verifying real repository state directly via the device bridge,
per standing discipline, rather than trusting the prior session's own
account: `git tag` listed `wu-32-green` (in addition to every earlier tag);
`git log --oneline -5` showed `HEAD` at `3798a5f` ("WU-32: import
WU-SM-01/WU-SM-02 historical findings"); `git status -sb` read `## main...
origin/main` with no ahead/behind marker; `git status --short` showed
exactly three modified files — `src/core/binner.cpp`, `src/core/binner.hpp`,
`tests/test_binner.cpp` — matching Session 39's own account of what WU-28c
left uncommitted, nothing else. (`.git/index.lock` present at 0 bytes,
read-only git commands unaffected — the known non-blocking artifact
`SESSION-PROTOCOL.md` already documents.)

Steve had already run `cmake --build build` and `ctest --test-dir build
--output-on-failure` at his own real terminal (AppleClang, ARM64, the
Blackmagic SDK configured — a fuller build than this project's own cloud
sandbox ever compiles) before this session started: 30 of 31 tests passing,
the sole failure `test_decklink_device`'s own `foundDuplexDevice` check —
ADR-035's already-accepted exception (the Monitor 3G is playback-only),
unrelated to WU-28c, and expected on every one of Steve's own real-terminal
runs. `test_binner` itself reported `Passed`, which — since
`tests/harness.hpp`'s own `summary()` returns nonzero on any failed
`CHECK`/`CHECK_ONCE` — certifies every one of its checks passed, including
the new `test_self_fold_front_and_back_get_different_tags()`.

**Independently re-verified this session in the cloud sandbox, not merely
accepted from the real-terminal `Passed` alone:** confirmed the three
modified files' own content directly against `DECISIONS.md` ADR-065 and
`WORK-UNITS.md`'s own WU-28c `Files:`/`Accept:` before trusting either —
`generateFragmentsTagByFacing()`, `generateFragmentsRowRangeTagByFacing()`
and the shared `generateFragmentsRowRangeImpl()` refactor are all present in
`src/core/binner.hpp`/`.cpp` exactly as described;
`test_self_fold_front_and_back_get_different_tags()` is present in
`tests/test_binner.cpp`. Fresh clone of `origin/main`, confirmed at
`wu-32-green`/`3798a5f` before any file was touched, with these same three
files overlaid on top (git working-tree changes, not yet committed to
`origin` at clone time — expected, since committing them is this session's
own job). Full 8-configuration matrix: GCC 13.3.0 and Clang 18.1.3, Release
and Debug, `SCATTER_TILE_LOG2` 4 and 5, plus GCC+ASan/UBSan — all clean,
zero warnings under `-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion
-Werror`, no sanitizer report. Full portable `ctest` suite 23/23 passing in
every configuration (22 pre-existing plus WU-32's own
`test_scan_order_invariance`), no regressions anywhere outside `test_binner`
itself. `test_binner` alone: 38401 checks passing on every GCC/Clang
Release/tile-2^5 configuration (matching Session 39's own cloud-sandbox
figure exactly), 10177 on Debug/tile-2^4 — a real and expected count
difference by tile size (fewer tiles, fewer boundary-replica checks), not a
discrepancy to chase.

This session's own job was confirmation and close-out, not design or code —
no `DECISIONS.md`/`INVARIANTS.md`/`CORRECTIONS.md` entry needed. Updated
`WORK-UNITS.md`'s own WU-28c entry: header tag `wip` → `green`, `*Status:*`
line rewritten to record both Session 39's cloud-sandbox evidence and this
session's real-terminal-plus-independent-re-verification evidence, per the
same doc-only stale-status-line correction pattern Session 35/37 already
used for WU-28a/WU-28b. Also corrected one small stale parenthetical in
WU-26's own entry ("the one consumer scoped so far (WU-28c, not yet
built)" → "now `green`") while already in this file for the same unit —
not a new finding, just accuracy while the file was open.

Delivered `WORK-UNITS.md` and this file to the real repository via
`SendUserFile` + `device_commit_files`, then re-staged both from the device
and diffed against this session's own edited copies before writing this
sentence — `SESSION-PROTOCOL.md`'s own rule 8.

## Where we are

Phase 7: WU-26 `wip` (unchanged in substance, one stale parenthetical
fixed), WU-27 `todo` (unchanged since WU-32), WU-28a/WU-28b/**WU-28c**
`green`, WU-28d `todo` — genuinely unblocked now that WU-28c's own real API
exists and is confirmed at Steve's own real terminal, not just the cloud
sandbox. WU-29/WU-33/WU-34/WU-35 unchanged since WU-32. `DECISIONS.md`
still runs through ADR-074, `INVARIANTS.md` through I11, `CORRECTIONS.md`
through C-023 — none touched this session.

## Next work unit

**WU-28d** (wire self-fold occlusion into the live sphere demo,
`tests/test_decklink_live_sphere.cpp`) is the natural next pick — WU-28c's
own real API now exists and is confirmed real-terminal green, closing the
last dependency `WORK-UNITS.md`'s own WU-28d entry names. DeckLink-linked,
so expect reasoned-through-only delivery from the cloud sandbox, the same
as every DeckLink-touching unit before it — Steve's own real-hardware
by-eye check (does the folding sphere's back half now visibly disappear
behind its front half) remains this one's actual accept criterion, per its
own `WORK-UNITS.md` entry. WU-27, WU-33, WU-34 and WU-35 (blocked in
substance on Task A1, not procedurally) all remain independently pickable,
unchanged from WU-32's own handoff.

## Open questions

Unchanged from WU-32's own handoff: `kCaptureRingCapacity` = 8, Q3
(macOS/Desktop Video version), Q4 (lattice edge damping, C-008(a)), Task A1
(UK 2,158,671 in full, gates WU-35), Task D6 (re-derive the 1080p50 budget,
C-023), and ADR-070's coarse-grid-facet-normal-vs-WU-26-normal open
question. None touched this session.

## Blocked / red

Nothing red, nothing newly blocked.

## Environment check

Unchanged from prior sessions. Steve's own real terminal confirms the
Blackmagic SDK is configured and `test_decklink_output`/`test_decklink_
input`/`test_decklink_capture_consumer`/`test_decklink_live_output`/
`test_decklink_live_sphere` all built and passed this session (only
`test_decklink_device`'s own `foundDuplexDevice` check fails, the standing
ADR-035 exception) — nothing in this session's own scope (WU-28c is
core-only) exercised DeckLink hardware behaviour, but the passing run is
worth recording as current evidence the hardware path itself is healthy.

## Append to DECISIONS.md

Nothing this session — WU-28c's own design is fully recorded in ADR-065
already (Session 39); this session only confirmed and tagged it.

## Append to CORRECTIONS.md

Nothing this session — no error found that rose to a logged correction.

## Closed out this session

**WU-28c — self-fold facing tag: per-fragment tag from surface normals.**
Tagged `wu-28c-green` at Steve's own real terminal, pushed to `origin`.

## Steve's own next steps

**1. Rebuild and test WU-28c's close-out at your own real terminal.**

```
rm -f ~/src/scatter-dve/.git/index.lock
cd ~/src/scatter-dve
git status --short
```

Expected: `src/core/binner.cpp`, `src/core/binner.hpp`,
`tests/test_binner.cpp` modified (already built and tested — you already
ran this build), plus `WORK-UNITS.md` and `HANDOFF.md` modified by this
session's own doc-only close-out. Nothing else.

**2. Commit, tag and push.** Same manual-tag fallback every DeckLink-adjacent
close-out has used since ADR-035 — `git add`/`git commit` before `git tag`,
so the tag lands on the commit that actually contains these changes:

```
cd ~/src/scatter-dve
git add src/core/binner.hpp src/core/binner.cpp tests/test_binner.cpp WORK-UNITS.md HANDOFF.md
git commit -m "WU-28c: self-fold facing tag from surface normals (ADR-065)"
git tag -a wu-28c-green -m "WU-28c: self-fold facing tag from surface normals green (test_decklink_device/foundDuplexDevice is ADR-035's known exception)"
git push origin main
git push origin --tags
```

**3. Verify it landed correctly:**

```
cd ~/src/scatter-dve
git log --oneline -3
git tag | tail -5
git status -sb
```

`git log --oneline -3` should show your own new commit at `HEAD`, carrying
`wu-28c-green`; `git status -sb` should read `## main...origin/main` with no
ahead marker and no modified files listed at all.
