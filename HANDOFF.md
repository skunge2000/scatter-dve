# Handoff
Overwritten at the end of every session. This is the first thing to read.

---

**Session:** 31 (WU-22b — scoped, drafted, confirmed working at Steve's
own real terminal, and tagged, all within this session; WU-22a untouched,
remains `green` at `wu-22a-green`)
**Tag:** `wu-22b-green` — confirmed directly against the real repository
this session: `git tag` lists it, `git log --oneline -5` shows `HEAD` at
`4db0517` with `wu-22b-green` pointing at it, and `git show wu-22b-green
--stat` lists exactly the seven files this unit's own commit touched
(`CMakeLists.txt`, `DECISIONS.md`, `HANDOFF.md`, `WORK-UNITS.md`,
`src/diag/coverage_view.hpp`, `src/diag/coverage_view.mm`,
`tools/coverage_view_demo.cpp`). `git status --short` is clean and the
stale `.git/index.lock` flagged twice earlier this session is gone.
`./tools/close.sh 22b` refused to auto-tag on the same already-accepted
`test_decklink_device`/ADR-035 duplex-check failure `wu-22a-green` also
hit (27/28, unrelated to this unit); the manual `git tag -a wu-22b-green
...` fallback every DeckLink-adjacent unit since ADR-035 has used was run
instead. No `origin` remote configured — commit and tag are local-only,
same as every earlier unit.

**Before doing anything else in the next session:** run `git tag`, `git
log --oneline -10` and `git status --short` directly against
`~/src/scatter-dve` via the device bridge, the same as every session
before this one — do not trust this file's own account of tag state
without checking it against the real repository first.

## Flagged now

**1. `WORK-UNITS.md`'s own WU-21i entry is still stale** (carried over
from Sessions 29 and 30, not touched this session either — out of scope
both times). It still reads `wip` and "not yet built or run at Steve's
own real terminal," but the `wu-21i-green` tag exists and (per Session
30's own check) contains the right files. **Still needs Steve's own
by-eye acceptance detail (which letter keys were tried, whether `Q` exits
cleanly) before `WORK-UNITS.md`'s status line can be honestly fixed** —
this is now three sessions old. Worth just doing directly next session
rather than carrying it forward a fourth time.

**2. (Resolved this session, root cause now known and documented — see
`DECISIONS.md`/this file's own history for detail if it recurs.)**
`device_stage_files` failed with `HTTP 403 "untrusted_device"` early this
session, the same failure Session 30 also hit. Steve identified the actual
cause partway through this session: his Mac going to sleep mid-session
invalidates the device's trusted-sign-in state — not a one-off account
glitch. He re-enabled access and staging worked normally for the rest of
the session (used repeatedly afterward with no further failures). **For
any future session:** if `device_stage_files` starts failing with
`untrusted_device`, it almost certainly means the Mac slept since the
session began — ask Steve to re-enable access in the Claude desktop app
(no fresh sign-in needed), then retry. `device_bash`/`device_commit_files`
keep working through this condition regardless.

**3. `.git/index.lock` is back again — pattern now clear across three
sessions running, worth stating plainly rather than re-"resolving" it each
time.** It appeared and disappeared more than once within this single
session alone: present early on, absent by the time Steve committed and
tagged (his own real `git commit`/`git tag`/`close.sh` all ran cleanly
through it), then present again after this session's own later
`device_bash` calls (`git status --short`, `git diff --stat`) run purely
to confirm the final doc edits below. The pattern, now observed
repeatedly: `device_bash` git commands that only read (`status`, `log`,
`diff`, `show`) still succeed and print correct output even with a stale
lock present (confirmed again this session — the warning is noise, not a
failure), but `device_bash` itself can never remove the lock file
(documented environment limitation, not a bug), so it silently
accumulates across a session's own read-only `device_bash` git calls.
**Steve: before your own next `git add`/`git commit`/`git tag`, run
`rm -f ~/src/scatter-dve/.git/index.lock` once more** — this is now a
routine first step for every session, not a one-off fix, and does not
need investigating further each time it recurs.

## This session in full

Session opened by requesting device-bridge access to `~/src/scatter-dve`
and `~/src/Blackmagic DeckLink SDK 16.0` (approved), reading
`SESSION-PROTOCOL.md`, `HANDOFF.md`, `WORK-UNITS.md`, `DECISIONS.md`,
`CORRECTIONS.md` and `INVARIANTS.md` in full, and verifying Session 30's
own account of tag/commit state directly against the real repository
(`git tag`, `git log --oneline -10`, `git status --short`, `git show
wu-22a-green --stat`) before trusting any of it — checked out exactly as
claimed.

WU-22b was scoped directly with Steve, per his own explicit instruction
not to assume a design, over two rounds of clarifying questions (launch
mechanism, offline-vs-live-wired split, colour mapping, window sizing,
refresh rate, threading needs across a display-window thread and a
live-pipeline thread). Full reasoning and every decision recorded in
`DECISIONS.md` ADR-057: in-process launch design intent (a future flag on
the live-sphere demo, chosen on Steve's own "lowest overhead in
processing terms" criterion, not built this session), an offline/
static-data-driven first cut for this unit specifically (matching this
project's own portable-piece-now/platform-piece-next discipline), a
grayscale colour mapping (black at 0, white at `kWeightUnity`, clipped
above), a fixed non-resizable window, redraw-on-new-data, and a
double-buffer/`dispatch_async`-to-main-thread threading design recorded as
intent for a future WU-22c (added to `WORK-UNITS.md` this session as
`todo`, unscoped) rather than built this session.

Delivered four files — `src/diag/coverage_view.hpp` (the platform-
independent public interface, pimpl'd, no Apple framework `#include`),
`src/diag/coverage_view.mm` (this project's first Objective-C++
translation unit — `NSWindow`, `MTKView`/`MTKViewDelegate`, an inline MSL
shader compiled at runtime, a keypress-quit/window-close mechanism),
`tools/coverage_view_demo.cpp` (a hand-run tool, not a test, the same
`add_executable`-only shape `make_testpat.cpp` established at WU-03,
building one sphere-warped frame with `weightOut` capture enabled), and a
new `if(APPLE)` block in `CMakeLists.txt` — entirely reasoned through, not
built or run anywhere, since this sandbox has no Xcode/AppleClang/Metal/
Cocoa toolchain at all. Written locally, then pushed to the real
repository via the device bridge's `device_bash` (heredocs against the
mounted repository — `device_stage_files` failed with `untrusted_device`
at this point, see Flagged item 2), and confirmed landed correctly via
`device_bash`'s own `wc -l`/`grep`/`sha256sum`/`git status`.
`DECISIONS.md` ADR-057 was drafted and appended with the full design
rationale and five explicitly flagged known risk points (shader UV/
vertical-flip convention, ARC correctness, inline runtime shader
compilation, the `[NSApp stop:]`-plus-dummy-event quit mechanism, the
hand-mirrored `kWeightUnityLocal` constant). `WORK-UNITS.md`'s stale
WU-22b stub was replaced with a real, scoped entry, and WU-22c was added.

Steve then re-enabled device access mid-session (see Flagged item 2),
built and ran `coverage_view_demo` at his own real terminal, and reported
back exactly what happened rather than a vague "it worked": `cmake
--build build` clean with zero warnings; the window opened showing a
512x512 grayscale partial-dome shape (the demo's own 1.2-radian
`angleSpanH`/`angleSpanV`, not a full wrap) with edges visibly brighter
than the centre — a specific, falsifiable match to `CORRECTIONS.md`
C-011's own prediction (the sphere's front-facing point is the
sparsest-covered, not the densest), not merely "some non-uniform
pattern"; both quit paths (`Q` with the window focused, and the window's
own close control) closed the window and returned the shell promptly,
with no hang. None of ADR-057's five known risk points turned out to be
real defects. This confirmation was folded back into `DECISIONS.md`
ADR-057 (a verification addendum, added before this ADR's own first
commit — not a reopening of an already-committed decision) and into
`WORK-UNITS.md`'s WU-22b entry, both updated from "reasoned through only"
to the full real-terminal result before Steve committed.

Steve committed all seven files (`4db0517`,
"WU-22b: diagnostic coverage view, Metal/Cocoa window (CoverageWindow,
coverage_view_demo)"), then ran `./tools/close.sh 22b`, which refused to
auto-tag on the same `test_decklink_device`/ADR-035 duplex-check failure
`wu-22a-green` also hit — so the manual `git tag -a wu-22b-green ...`
step every DeckLink-adjacent unit since ADR-035 has used was run instead,
and verified directly against the real repository (`git show
wu-22b-green --stat` listing exactly the seven files above). **WU-22b is
genuinely `green`.**

## Where we are

Phase 5 (Live capture) is unchanged from Session 30's own account. Phase
6 (Scale up) now reads: WU-22a `green` (`wu-22a-green`), WU-22b `green`
(`wu-22b-green`, new this session), WU-22c `todo` (unscoped, new this
session — wiring `CoverageWindow` into the live capture/output pipeline).
`DECISIONS.md` runs through ADR-057. `CORRECTIONS.md` is unchanged this
session (still through C-018) — nothing this session surfaced was a new
codebase-logic lesson rather than confirmation that the design reasoning
in ADR-057 was correct.

## Next work unit

Not decided here, deliberately — Steve's own call at the start of the
next session. WU-22c (wiring `CoverageWindow` into
`tests/test_decklink_live_sphere.cpp`'s own live pipeline, behind a
`--show-coverage`-style flag) is the natural continuation now that
`CoverageWindow` itself is confirmed working, but it needs its own real
scoping session first, per this project's own established practice, and
it needs Steve's own real terminal throughout — this sandbox cannot
reason through or verify anything touching Metal, Cocoa, or the
DeckLink-only live capture/output path at once. WU-21d (cold-start
black-fill fix), starting to scope WU-28 (k-buffer), and fixing WU-21i's
stale status line (Flagged item 1, now three sessions old) remain open
candidates too, if Steve would rather pick up one of those instead.

## Open questions

Unchanged from Session 30: `kCaptureRingCapacity`'s value of 8
(WU-20a/20b, ADR-046), the cold-start green-frame artifact (WU-21d), and
front/back occlusion/transparency (WU-28) all remain open. Q3 (macOS/
Desktop Video version) remains open. Q4 (lattice edge damping, C-008(a))
remains open, not touched this session (WU-22b never reaches
`core/jacobian.hpp` or the splat/resolve path — it only reads an
already-captured `weightOut` buffer).

## Blocked / red

Nothing red. WU-22b is genuinely `green`, confirmed at Steve's own real
terminal and tagged. WU-22a remains genuinely `green`, untouched this
session.

## Environment check

Unchanged: **UltraStudio Monitor 3G** (output, HDMI-mirrored) and
**UltraStudio Recorder 3G** (input) both last confirmed working in Session
29's own real-hardware runs (not touched this or the last two sessions —
none of WU-22a/22b has a DeckLink dependency). **UltraStudio 4K Mini**
remains on hold pending a PSU replacement. This session's own new
Metal/Cocoa dependency IS now confirmed against Steve's real Mac
hardware/GPU/OS version — `coverage_view_demo` built and ran cleanly,
first attempt, no toolchain or driver issues encountered.

## Append to DECISIONS.md

ADR-057 (WU-22b design: the full scoping conversation and every design
decision from it, the WU-22b/WU-22c split, the five known risk points, and
— folded into the same entry once Steve confirmed it, before this ADR's
own first commit — the full real-terminal verification result) —
appended in full this session; see `DECISIONS.md`. Does not reopen
ADR-031, ADR-040, ADR-044, ADR-054, ADR-055 or ADR-056.

## Append to CORRECTIONS.md

None this session — nothing surfaced that was a new lesson rather than
confirmation that ADR-057's own design reasoning (particularly the C-011-
based prediction about where coverage should be brightest/darkest) was
correct as reasoned through.

## Closed out this session

WU-22b's full loop — scoping conversation, sandbox drafting, delivery via
the device bridge, Steve's own real-terminal build/run/verification,
commit, the ADR-035-exception manual tag, and `git show wu-22b-green
--stat` confirmation — all completed within this same session. Nothing
outstanding for WU-22b itself; WU-22c is the deliberately-deferred
follow-on, not an unfinished piece of this unit.
