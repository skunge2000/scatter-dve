# Handoff
Overwritten at the end of every session. This is the first thing to read.

---

**Session:** 31 (WU-22b only — scoped and drafted, left `wip`, not
`green`; WU-22a untouched, remains `green` at `wu-22a-green`)
**Tag:** none new this session. `wu-22a-green` (Session 30) remains the
most recent tag on `main`; `git tag` this session confirmed all 28 tags
through it still present, `git log --oneline -10` confirmed `HEAD` is
still `ada6f07` from Session 30, and `git status --short` shows exactly
the working-tree changes this session made and nothing else:
`M CMakeLists.txt`, `M DECISIONS.md`, `M WORK-UNITS.md`, `?? src/diag/`,
`?? tools/coverage_view_demo.cpp`. Nothing was committed, nothing was
tagged, and `./tools/close.sh` was not run — WU-22b is explicitly
unverified (see below), so per Steve's own standing instruction only he
runs `close.sh`/tags a unit, and only once he has actually confirmed it
works at his own real terminal.

**Before doing anything else in the next session:** run `git tag`, `git
log --oneline -10` and `git status --short` directly against
`~/src/scatter-dve` via the device bridge, the same as every session
before this one — do not trust this file's own account of tag/working-tree
state without checking it against the real repository first. In
particular, confirm whether Steve has by then committed this session's
five changed/new files (`CMakeLists.txt`, `DECISIONS.md`, `WORK-UNITS.md`,
`src/diag/coverage_view.hpp`, `src/diag/coverage_view.mm`,
`tools/coverage_view_demo.cpp` — six files, the `git status --short`
output above undercounts `src/diag/` as one untracked directory) and
whether `coverage_view_demo` actually built and ran.

## Flagged now

**1. `WORK-UNITS.md`'s own WU-21i entry is still stale** (carried over
from Session 30, not touched this session — out of this session's own
scope, WU-22b only). It still reads `wip` and "not yet built or run at
Steve's own real terminal," but the `wu-21i-green` tag exists and (per
Session 30's own check) contains the right files. **Still needs Steve's
own by-eye acceptance detail (which letter keys were tried, whether `Q`
exits cleanly) before `WORK-UNITS.md`'s status line can be honestly
fixed** — this is now two sessions old and worth doing directly rather
than carrying forward a third time.

**2. (Resolved this session, and root cause now known — record this so
it stops being re-diagnosed from scratch each time.) `device_stage_files`
again failed with `HTTP 403 "untrusted_device"` early this session (three
files: `binner.hpp`, `raster.hpp`, `pipeline.cpp`, requested together),
the same failure Session 30 also hit, worked around the same way
(`device_bash`'s own `grep`/`wc -l`/`sha256sum`/`git status` against the
mounted files directly). Steve identified the actual cause later the same
session: his Mac going to sleep mid-session invalidates the device's
trusted-sign-in state, which is what `untrusted_device` actually reports
— not a one-off account glitch, and not something a session-start
re-request of folder access fixes on its own. He re-enabled access this
session and a follow-up `device_stage_files` call succeeded immediately
(`HANDOFF.md` itself staged clean, 16453 bytes). **For any future
session:** if `device_stage_files` starts failing with `untrusted_device`
partway through, don't spend time re-diagnosing — it almost certainly
means the Mac slept since the session began; ask Steve to re-enable
access in the Claude desktop app (no fresh sign-in needed, per this
session's own experience), then retry. `device_bash`/`device_commit_files`
reliably keep working through this condition regardless, so the
write-then-confirm fallback above remains available even before Steve
re-enables access.

**3. A stale `.git/index.lock` was found again this session** (Session 30
flagged the same thing as "resolved" after Steve removed it manually at
his own terminal — it has since reappeared, most likely simply from
`git status`/`git log` being run again this session via the device
bridge, the same way it appeared last time). `device_bash` cannot delete
files on a mounted folder, so it needs Steve's own
`rm -f ~/src/scatter-dve/.git/index.lock` at his real terminal again
before his own `git status`/`git add`/`git commit` will work cleanly.
Exact command:
```
rm -f ~/src/scatter-dve/.git/index.lock
```

**4. WU-22b is entirely unverified — this is not a gap to close casually,
it is this unit's own defining constraint this session.** See "This
session in full" and `DECISIONS.md` ADR-057 for the full reasoning. In
short: this sandbox has no Xcode/AppleClang/Cocoa/Metal toolchain of any
kind, so unlike every earlier Apple-only or DeckLink-only unit (where the
sandbox could at least build and test a portable majority), none of
WU-22b's own four changed/new files have been compiled or run anywhere.
See "What Steve needs to do next" below for the exact commands to change
that.

## This session in full

Session opened, per Steve's own explicit instruction, by requesting
device-bridge access to `~/src/scatter-dve` and `~/src/Blackmagic DeckLink
SDK 16.0` (approved), then reading `SESSION-PROTOCOL.md`, `HANDOFF.md`,
`WORK-UNITS.md`, `DECISIONS.md`, `CORRECTIONS.md` and `INVARIANTS.md` in
full from the staged copies — not skimmed, not assumed from memory.

Per Steve's own explicit instruction not to trust `HANDOFF.md`'s account
of tag/commit state without checking it directly, ran `git tag`, `git log
--oneline -10` and `git status --short` against the real repository via
the device bridge before treating anything in `HANDOFF.md` as fact.
Confirmed `wu-22a-green` exists and `git show wu-22a-green --stat` lists
exactly the seven files `HANDOFF.md` claimed (`CMakeLists.txt`,
`DECISIONS.md`, `HANDOFF.md`, `WORK-UNITS.md`, `src/core/pipeline.cpp`,
`src/core/resolve.hpp`, `tests/test_coverage_capture.cpp`) — Session 30's
own account checked out exactly.

Per Steve's own explicit instruction not to assume a design for WU-22b,
scoped it directly with him before writing any code, over two rounds of
`AskUserQuestion` (the first round re-asked in clearer, self-contained
form at his own request): launch mechanism, whether it runs concurrently
with live capture/output or starts offline, target refresh rate, the
weight-to-colour mapping, window size/resizability, and
`PipelineParams::weightOut`'s own buffer lifetime/threading needs. Steve's
own answers, and the reasoning connecting them, are recorded in full in
`DECISIONS.md` ADR-057 — in short: in-process launch (a future flag on the
live-sphere demo, chosen on Steve's own explicit "lowest overhead in
processing terms" criterion), an offline/static-data-driven first cut for
*this* unit specifically (matching this project's own established
portable-piece-now/platform-piece-next discipline, not a contradiction of
the in-process choice), grayscale colour mapping (black at 0, white at
`kWeightUnity`, clipped above), a fixed, non-resizable window matching
`destWidth`/`destHeight` exactly, redraw-on-new-data rather than
timer-driven, and a double-buffer/`dispatch_async`-to-main-thread
threading design recorded as intent for a future WU-22c rather than built
this session (there is no second thread in this session's own deliverable
to exercise it against).

**Delivered four files, all new or changed, none of it built or run
anywhere — see Flagged item 4 above.** `src/diag/coverage_view.hpp` (new
— the platform-independent public interface, `CoverageWindowConfig`/
`CoverageWindow`, pimpl'd, no Apple framework `#include` anywhere in it,
the same "keep the platform dependency behind the .cpp/.mm boundary"
shape `com_ptr.hpp`/`decklink_device.hpp` already use for the Blackmagic
SDK); `src/diag/coverage_view.mm` (new — this project's first
Objective-C++ translation unit: `NSWindow`, `MTKView`/`MTKViewDelegate`,
Metal device/texture/pipeline state, an inline MSL shader compiled at
runtime via `newLibraryWithSource:options:error:`, and a keypress-quit/
window-close mechanism matching the existing terminal convention);
`tools/coverage_view_demo.cpp` (new — a hand-run tool, not a test, the
same `add_executable`-only shape `tools/make_testpat.cpp` established at
WU-03, since there is no automatable pass/fail criterion for "does a GUI
window look right"; builds one sphere-warped frame with
`PipelineParams::weightOut` capture enabled and opens a `CoverageWindow`
on it, choosing a sphere because `CORRECTIONS.md` C-011 already
establishes its front-facing point is usually the sparsest-covered point,
not the densest, making the displayed image a livelier check of genuine
capture data); and a new `if(APPLE)` block appended to `CMakeLists.txt`
(`enable_language(OBJCXX)`, the `scatter-diag` static library linking
`Cocoa`/`Metal`/`MetalKit`/`QuartzCore`, and the `coverage_view_demo`
executable).

Written locally first, then delivered to the real repository via the
device bridge's `device_bash` (heredocs run directly against the mounted
repository — `device_stage_files` failed immediately with the same
`untrusted_device` error Session 30 hit, see Flagged item 2), and
confirmed landed correctly via `device_bash`'s own `wc -l`/`grep`/
`sha256sum`/`git status --short` against the mounted files directly
(exact line counts, shader-boundary markers, and title-string content all
checked by hand against what was intended) — not by compiling, since
nothing here can compile Objective-C++/Metal/Cocoa code at all.

`DECISIONS.md` ADR-057 was drafted and appended in full (5413 to 5641
lines, confirmed by `wc -l` before and after): the full scoping
conversation, every design decision and its reasoning, the WU-22b/WU-22c
split, five explicitly flagged known risk points for whoever builds this
first (the shader's UV/vertical-flip convention, ARC correctness, whether
inline runtime shader compilation actually works on Steve's own Metal/OS
version, the `[NSApp stop:]`-plus-dummy-event quit mechanism, and the
hand-mirrored `kWeightUnityLocal` constant possibly drifting from
`core/types.hpp`'s real `kWeightUnity`), and an explicit accept criterion
that is Steve's own by-eye judgement, not a programmatic check.
`WORK-UNITS.md`'s stale WU-22b stub (still reading "not scoped or started"
from Session 30) was replaced with a real, scoped entry marked `wip`, and
a new WU-22c entry was added, `todo`, unscoped beyond ADR-057's own
design-intent notes — wiring `CoverageWindow` into the live capture/output
pipeline, deferred to its own future scoping session.

## Where we are

Phase 5 (Live capture) is otherwise unchanged from Session 30's own
account. Phase 6 (Scale up) now reads: WU-22a `green` (`wu-22a-green`,
unchanged), WU-22b `wip` (scoped, drafted, delivered to the real
repository, entirely unbuilt/unrun/untagged), WU-22c `todo` (unscoped,
new this session). `DECISIONS.md` runs through ADR-057. `CORRECTIONS.md`
is unchanged this session (still through C-018) — nothing this session
surfaced rose to the level of a new correction.

## What Steve needs to do next

Exact commands, in order, at your own real terminal:

**1. Clear the stale lock, then check what's changed:**
```
rm -f ~/src/scatter-dve/.git/index.lock
cd ~/src/scatter-dve
git status --short
git diff --stat
```

**2. Configure and build**, from the existing build directory (Session
30's own configuration should still be valid — this session only added
files and appended a new `if(APPLE)` block to `CMakeLists.txt`, it did not
change any existing target):
```
cmake --build build
```
Watch for two things specifically: that `scatter-diag` and
`coverage_view_demo` actually get configured at all (CMake's own
configure-step output should include the line `scatter-dve: APPLE --
scatter-diag, coverage_view_demo configured (WU-22b, UNVERIFIED)` — if
CMake was not re-run since the `CMakeLists.txt` change, force a
reconfigure first with `cmake -S . -B build` before the build command
above), and the actual compiler output for `coverage_view.mm` — this is
the first Objective-C++ file in the project, so any errors here are
genuinely new territory, not a regression of something that worked before.

**3. Run the demo tool by hand:**
```
./build/coverage_view_demo
```
Expected: a terminal line `coverage_view_demo: frame resolved, opening
coverage window (512x512) -- press Q or close the window to quit`,
followed by a 512x512 window titled "scatter-dve — coverage view demo
(WU-22b)" showing a grayscale image. It should NOT be a flat/uniform
image — the sphere warp should show visibly brighter and darker regions
(if it looks flipped top-to-bottom, that's known risk point (1) in
`DECISIONS.md` ADR-057 — see there for the one-line fix). Press `Q` or
close the window; the terminal should then print `coverage_view_demo:
window closed, exiting` and return to the shell promptly (if it hangs
instead, that's known risk point (4) in ADR-057).

**4. Report back exactly what happened** — build errors verbatim if any,
whether the window opened, what the image looked like, and whether quit
worked cleanly. That report is what turns WU-22b from `wip` into
something a future session can call `green` and tag.

**5. Only once WU-22b is confirmed working**, tag it yourself (this
project's own convention, ADR-035's own exception pattern used for every
DeckLink-touching unit does not apply here since this unit is Apple/
Metal-only, not DeckLink — `./tools/close.sh 22b` should work normally,
but confirm it does before relying on it, and fall back to
`git tag -a wu-22b-green -m "..."` by hand if it does not).

## Next work unit

Not decided here, deliberately. If WU-22b confirms working, WU-22c (wiring
`CoverageWindow` into the live pipeline) is the natural next scoping
conversation. If WU-22b turns up build errors or the wrong visual result,
fixing those is the obvious next session's job instead — this file's own
"What Steve needs to do next" section above has the exact commands and
what to report back. WU-21d (cold-start black-fill fix) and starting to
scope WU-28 (k-buffer) remain open candidates too, exactly as Sessions 29
and 30 left them, if Steve would rather pick up one of those instead.

## Open questions

Unchanged from Session 30: `kCaptureRingCapacity`'s value of 8
(WU-20a/20b, ADR-046), the cold-start green-frame artifact (WU-21d), and
front/back occlusion/transparency (WU-28) all remain open. Q3 (macOS/
Desktop Video version) remains open. Q4 (lattice edge damping, C-008(a))
remains open but not touched this session (WU-22b never reaches
`core/jacobian.hpp` or the splat/resolve path at all — it only reads an
already-captured `weightOut` buffer).

## Blocked / red

Nothing red. Nothing was built or tested this session at all (WU-22b is
entirely Apple/Metal/Cocoa-dependent, this sandbox has none of that
toolchain), so nothing here has failed a real build or test either —
WU-22b is honestly `wip`/unverified, not `red`. WU-22a remains genuinely
`green`, untouched this session.

## Environment check

Unchanged: **UltraStudio Monitor 3G** (output, HDMI-mirrored) and
**UltraStudio Recorder 3G** (input) both last confirmed working in Session
29's own real-hardware runs (not touched this or last session — neither
WU-22a nor WU-22b has a DeckLink dependency). **UltraStudio 4K Mini**
remains on hold pending a PSU replacement. Nothing about WU-22b's own new
Metal/Cocoa dependency has been confirmed against Steve's actual Mac
hardware/GPU/OS version yet — that confirmation is exactly what "What
Steve needs to do next" above asks for.

## Append to DECISIONS.md

ADR-057 (WU-22b design: the full scoping conversation and every design
decision from it, the WU-22b/WU-22c split, the five known risk points,
the explicit non-programmatic accept criterion, and the "unverified in
full" status) — appended in full this session (5413 to 5641 lines,
confirmed by `wc -l`); see `DECISIONS.md`. Does not reopen ADR-031,
ADR-040, ADR-044, ADR-054, ADR-055 or ADR-056.

## Append to CORRECTIONS.md

None this session — nothing this session surfaced was a new lesson rather
than routine design-and-draft work; see `DECISIONS.md` ADR-057's own
"known risk points" section for what to watch for at Steve's own next
real build, some of which may yet turn into a `CORRECTIONS.md` entry once
he reports back what actually broke, if anything did.

## Closed out this session

Nothing closed to `green` this session — that is deliberate, not an
oversight: WU-22b's own defining constraint (see Flagged item 4) is that
none of it could be verified here at all. What *is* closed out: the
scoping conversation itself (no longer an open question — ADR-057 has the
full, final design), and the delivery of all four drafted files plus the
`DECISIONS.md`/`WORK-UNITS.md` documentation to the real repository,
confirmed landed correctly by content, not merely "sent."
