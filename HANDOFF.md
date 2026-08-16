# Handoff
Overwritten at the end of every session. This is the first thing to read.

---

**Session:** 30 (WU-22a only — WU-22b explicitly not started, see below)
**Tag:** `wu-21i-green` confirmed to exist and to contain all four files
(`git show wu-21i-green --stat`: `CORRECTIONS.md`, `DECISIONS.md`,
`WORK-UNITS.md`, `tests/test_decklink_live_sphere.cpp`) — the commit-gap
fix Session 29's own `HANDOFF.md` flagged was checked directly against the
real repository at the start of this session and found already resolved.
No new tag exists yet this session — WU-22a is delivered but **not yet
built, tested, or `close.sh`'d at Steve's own real terminal.**

**Before doing anything else in the next session:** run `git tag`, `git
log --oneline -10` and `git status --short` directly against
`~/src/scatter-dve` via the device bridge, the same as every session
before this one — do not trust this file's own account of tag state
without checking it against the real repository first.

## Flagged now

**1. `WORK-UNITS.md`'s own WU-21i entry is stale.** It still reads `wip`
and "not yet built or run at Steve's own real terminal," but the
`wu-21i-green` tag now exists and contains the right files (confirmed
above). This session did not rewrite that entry's status line, because it
only has the tag's existence to go on, not the actual by-eye acceptance
detail (which letter keys were tried, whether `Q` exits cleanly, etc.)
that an honest status line needs — that detail belongs to whoever was at
the terminal when it was tagged. **Next session (or Steve directly) should
fix WU-21i's status line in `WORK-UNITS.md` and this file's own tag
history once that detail is available.**

**2. `device_stage_files` (the device-bridge tool that re-reads files back
from `~/src/scatter-dve` for confirmation) failed all session with `HTTP
403 "untrusted_device"` — the device's sign-in is stale. `device_bash` and
`device_commit_files` (writing files *to* the device) both kept working
throughout, so nothing was blocked outright, but every file this session
wrote back was verified by `shasum -a 256` comparison via `device_bash`
instead of the normal re-stage-and-diff. **Steve: please sign in again in
the Claude desktop app on this device** — a sign-in banner should already
be showing there — so the next session's file staging works normally
again.

**3. A stale `.git/index.lock` exists in `~/src/scatter-dve`.** It showed
up after two consecutive `git status --short` calls via the device bridge
this session; `git status` itself still returns correct output despite it
(`M CMakeLists.txt`, `M src/core/pipeline.cpp`, `M src/core/resolve.hpp`,
`?? tests/test_coverage_capture.cpp`, plus whatever this session's
`DECISIONS.md`/`WORK-UNITS.md`/`HANDOFF.md` writes add), so it is not
currently blocking reads — but `device_bash` cannot delete files on a
mounted folder (a fixed limitation of this environment, not something to
retry), so it is still sitting there. **Run this before any `git add` /
`git commit` / `close.sh`:**

```
rm -f ~/src/scatter-dve/.git/index.lock
```

## This session in full

Session opened by requesting device-bridge access to `~/src/scatter-dve`
and `~/src/Blackmagic DeckLink SDK 16.0` (approved), reading all six
project docs in full from the staged copies, and then checking Session
29's own flagged commit gap directly against the real repository rather
than trusting `HANDOFF.md`'s account of it — found already resolved (see
Tag, above).

Asked which work unit was next; after two clarifying questions about what
"weight," "coverage," and "det J" actually mean, Steve chose: **weight
only to begin with, rendered in a window on the Mac** — i.e. WU-22
(diagnostic coverage view), split per this project's own established a/b
discipline into WU-22a (the weight-capture plumbing itself, portable, no
Mac dependency) and WU-22b (the Metal window that displays it, Mac-only,
not started).

**WU-22a** adds `PipelineParams::weightOut` (`src/core/resolve.hpp`): a
non-owning, caller-owned, opt-in pointer field, `nullptr` by default,
mirroring `PipelineParams::pool`'s own shape (WU-19a/ADR-044) rather than
threading a second output raster through every `runFrame()`/
`runFrameBytes()`/`runFrameFile()` signature. When set, it must point to
`destWidth * destHeight` tight-packed `WeightAccum` values; `core/
pipeline.cpp`'s `resolveOneTile()` writes each destination cell's raw
`AccumCell::w` (captured before `composite()`'s own clamp to
`[0, kWeightUnity]`) into it, immediately after that cell's ordinary
`dest.Y/Cb/Cr` writes. See `DECISIONS.md` ADR-056 for the full design
rationale.

A new test file, `tests/test_coverage_capture.cpp` (registered in
`CMakeLists.txt`), checks four things: capture is side-effect-free
(identical composited output whether `weightOut` is null or supplied);
weight reads exactly zero at genuinely uncovered cells and exactly
`kWeightUnity` at fully, evenly covered interior cells; captured values
match bit-for-bit against an independent recomputation via the public
`generateFragments()`/`splatTile()`/`sumBanks()` path (not
`resolveOneTile()` itself); and results are identical across thread
counts (I6). Writing test 2 surfaced a real, already-known codebase
property — `CORRECTIONS.md` C-008(a)'s edge-derivative damping (ADR-022's
edge-replication clamp) — showing up as doubled weight at the lattice's
own `u=0`/`v=0` edge; root-caused via direct `Lattice::jacobian()` calls
in a scratch program, then fixed by narrowing the "near-unity" check to
interior rows/columns and tightening it to exact equality. Judged, per
the ADR-048/C-006 precedent, as routine iteration rather than a new
lesson, so no new `CORRECTIONS.md` entry was logged for it.

Verified across the project's full matrix — Clang 18 and GCC 13, Release
and Debug, tile `2^4` and `2^5` (8 configs), plus GCC ASan/UBSan — all
green, 14221/14221 checks passing in `test_coverage_capture` itself and
no regressions elsewhere. Delivered to the real repository (`src/core/
resolve.hpp`, `src/core/pipeline.cpp`, `tests/test_coverage_capture.cpp`,
`CMakeLists.txt`) via the device bridge and confirmed written correctly
by sha256 comparison (see Flagged item 2 above for why sha256 rather than
the normal re-stage-and-diff). `DECISIONS.md` ADR-056, the `WORK-UNITS.md`
WU-22a/WU-22b entries, and this file are delivered the same way.

**WU-22b (the Metal window itself) was deliberately not started.** It is
this project's first Metal/Cocoa dependency of any kind — nothing else in
`src/` touches Apple's UI frameworks — and deserves its own real scoping
session rather than being drafted sight-unseen in a Linux sandbox that
cannot compile, let alone run, anything Metal-related.

## Where we are

Phase 5 (Live capture) is otherwise unchanged from Session 29's own
account. Phase 6 (Scale up) now has its first real entry: WU-22a `wip`
(delivered, not yet built/tested/tagged at Steve's own terminal), WU-22b
`todo` (unscoped). `DECISIONS.md` runs through ADR-056. `CORRECTIONS.md`
is unchanged this session (still through C-018) — see above for why the
C-008(a) finding did not get a new entry.

## Next work unit

**Build, test, and `close.sh` WU-22a first**, at Steve's own real
terminal — exact commands below. It cannot go green until that happens;
nothing else should be treated as "next" until it does. After that,
WU-22b (the Metal window) is the natural continuation if Steve wants to
see the weight capture actually rendered — but it needs a real scoping
conversation first (target frame rate for the display refresh, whether it
shares the Mac's main event loop with anything else, a colour ramp for
the weight visualisation, window size/resizability), not an assumed
design. WU-21d (cold-start black-fill fix) and starting to scope WU-28
(k-buffer) remain open candidates too, exactly as Session 29 left them,
if Steve would rather pick up one of those instead.

## Open questions

Unchanged from Session 29: `kCaptureRingCapacity`'s value of 8
(WU-20a/20b, ADR-046), the cold-start green-frame artifact (WU-21d), and
front/back occlusion/transparency (WU-28) all remain open. Q3 (macOS/
Desktop Video version), Q4 (lattice edge damping, C-008(a)) remain open
from earlier sessions — Q4 in particular is now directly exercised (not
just theorised about) by WU-22a's own test 2, see above.

## Blocked / red

Nothing red. WU-22a is fully green in the cloud sandbox; it simply has
not yet been run at Steve's own real terminal, which is the only place
that can happen for a final "genuinely done" verdict per this project's
own rules.

## Environment check

Unchanged: **UltraStudio Monitor 3G** (output, HDMI-mirrored) and
**UltraStudio Recorder 3G** (input) both last confirmed working in
Session 29's own real-hardware runs (not touched this session — WU-22a is
portable, no DeckLink dependency). **UltraStudio 4K Mini** remains on hold
pending a PSU replacement.

## Append to DECISIONS.md

ADR-056 (WU-22a design: the WU-22a/WU-22b split, `PipelineParams::
weightOut`'s own rationale, the C-008(a) finding, the full verification
matrix, the `untrusted_device` staging failure and its sha256 workaround,
the stale `.git/index.lock`) — appended in full this session; see
`DECISIONS.md`. Does not reopen any earlier ADR.

## Append to CORRECTIONS.md

None this session — see "This session in full" above for why the
C-008(a) finding during WU-22a test-writing did not warrant a new entry.

## Exact commands for Steve's own real terminal

Run these in order. Do not skip the lock-file removal or the `git status`
check — both matter given what this session found.

```
rm -f ~/src/scatter-dve/.git/index.lock
cd ~/src/scatter-dve
git status --short
```

Expect to see modified `CMakeLists.txt`, `src/core/pipeline.cpp`,
`src/core/resolve.hpp`, `DECISIONS.md`, `WORK-UNITS.md`, `HANDOFF.md`, and
a new untracked `tests/test_coverage_capture.cpp`. If anything else shows
up, stop and check it before continuing — it means something this
session's account does not cover.

```
git add CMakeLists.txt src/core/pipeline.cpp src/core/resolve.hpp tests/test_coverage_capture.cpp DECISIONS.md WORK-UNITS.md HANDOFF.md
git commit -m "WU-22a: opt-in weight-capture plumbing (PipelineParams::weightOut)"
```

Then build and test it yourself before trusting the sandbox's own "all
green" claim:

```
cd ~/src/scatter-dve
cmake -B build -G Ninja -DBLACKMAGIC_SDK_DIR="/Users/stephenneal/src/Blackmagic DeckLink SDK 16.0"
cmake --build build
ctest --test-dir build --output-on-failure -R coverage_capture
```

If that passes, run the full suite and close the unit (this tags
`wu-22a-green` and pushes if `origin` is configured — `close.sh` refuses
on a dirty tree or an existing tag, so the commit above must land first):

```
./tools/close.sh 22a
git show wu-22a-green --stat
```

Confirm `git show wu-22a-green --stat` lists all seven files above before
treating WU-22a as genuinely closed — the same check Session 29's own
flagged gap should have caught earlier for WU-21i.
