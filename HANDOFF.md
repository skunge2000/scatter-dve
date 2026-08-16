# Handoff
Overwritten at the end of every session. This is the first thing to read.

---

**Session:** 32 (WU-22c — scoped and drafted, delivered via the device
bridge as reasoned-through-only; NOT built, run, committed, tagged, or
pushed by this session. WU-22b untouched, remains `green` at
`wu-22b-green`)
**Tag:** still `wu-22b-green` — no new tag this session. This session's
own assistant never runs `close.sh`, never tags, never commits, and never
pushes, per Steve's own standing instruction for every Metal/Cocoa- or
DeckLink-touching unit; that is unconditionally Steve's own action, at his
own real terminal, once he has built and verified WU-22c below.

**Before doing anything else in the next session:** run `git tag`, `git
log --oneline -10` and `git status --short` directly against
`~/src/scatter-dve` via the device bridge, the same as every session
before this one — do not trust this file's own account of tag/commit state
without checking it against the real repository first. **Specifically
check whether Steve's own WU-22c close-out (build, verify, commit, tag
`wu-22c-green`, and push — see "Steve's own next steps" below) actually
happened between this session ending and the next one starting.** If it
did, `WORK-UNITS.md`'s own WU-22c entry (currently `wip`) and this file
will both be stale in the same way Session 29-31's own WU-21i entry stayed
stale for three sessions (Flagged item 1 below) — fix the status line
directly rather than carrying the staleness forward again.

## Flagged now

**1. `WORK-UNITS.md`'s own WU-21i entry is still stale** (carried over
from Sessions 29, 30 and 31, not touched this session either — out of
scope again this time, WU-22c does not read or write that entry). It still
reads `wip` and "not yet built or run at Steve's own real terminal," but
the `wu-21i-green` tag exists and (per Session 30's own check) contains the
right files. **Still needs Steve's own by-eye acceptance detail (which
letter keys were tried, whether `Q` exits cleanly) before `WORK-UNITS.md`'s
status line can be honestly fixed** — this is now four sessions old. Worth
just doing directly next session rather than carrying it forward a fifth
time.

**2. (New this session, process not codebase — recorded here and in
`SESSION-PROTOCOL.md` itself, not just this file.)** After the
`setKeyHandler()` follow-up above, the assistant told Steve to rebuild and
retest in narrated prose ("just re-run `cmake --build build` first, then
try the controls with the coverage window focused") instead of exact
command lines — a direct violation of a standing instruction Steve had
given verbally at the start of every DeckLink/Metal-touching session so
far, but which had never actually been written into `SESSION-PROTOCOL.md`
itself. Steve corrected this and asked explicitly that it be recorded so
it stops needing to be repeated every session. Fixed two ways: (a)
`SESSION-PROTOCOL.md`'s own anti-drift rules gained a new rule 9, below,
spelling this out as a standing, binding rule for every future session,
not just this one; (b) the assistant then re-sent the actual rebuild/test
instructions as exact command blocks in the same turn. **For any future
session: every instruction to Steve for his own real terminal is exact,
copy-pasteable commands in a fenced block — always, including small
mid-session follow-ups, not only full close-outs.**

**3. WU-22c itself (this session's own work) is delivered but
`UNVERIFIED IN FULL` — nothing in it has been built, run, committed,
tagged, or pushed.** This is the load-bearing flag for next session, not a
carried-over one: nine files were written and delivered via the device
bridge this session (`tests/test_decklink_live_sphere.cpp`,
`src/diag/coverage_view.hpp`/`.mm`,
`src/io/decklink_capture_consumer.hpp`/`.cpp`, `CMakeLists.txt`,
`DECISIONS.md`, `WORK-UNITS.md`, this file), all delivered and confirmed
landed correctly (see "This session in full" below), but this session's
own sandbox has no Blackmagic SDK, no Cocoa, no Metal, no AppleClang/Xcode
toolchain — none of it has been compiled or run anywhere yet. **Steve's own
next steps** (exact commands below, under "Steve's own next steps") are
the actual completion of this unit: build, run both with and without
`--show-coverage`, commit, tag (`wu-22c-green` if green, expecting the
same ADR-035 duplex-check exception every DeckLink-adjacent unit since
ADR-035 has hit — see below), and push (`SESSION-PROTOCOL.md`'s own
Session-31-updated close-out rule, spelled out explicitly since the
manual-tag fallback does not auto-push).

**4. `.git/index.lock` pattern — still the same known, non-blocking
behavior documented across Sessions 29-31, not re-investigated this
session since this session made no local commits of its own (nothing to
trigger it).** `device_bash` git commands that only read (`status`, `log`,
`diff`, `show`) succeed and print correct output even with a stale lock
present; `device_bash` itself can never remove the lock file (documented
environment limitation, not a bug). **Steve: before your own next
`git add`/`git commit`/`git tag` for WU-22c, run `rm -f
~/src/scatter-dve/.git/index.lock` first** — routine, not a one-off fix.

**5. `device_stage_files` HTTP 403 `untrusted_device` — still the same
known behavior documented in Session 31, not hit this session.** If it
recurs: it means the Mac slept mid-session and invalidated the device's
trusted-sign-in state, not an account problem — ask Steve to re-enable
access in the Claude desktop app (no fresh sign-in needed), then retry.
`device_bash`/`device_commit_files` keep working through this condition
regardless. Not encountered this session — noted only so a future session
does not have to re-diagnose it from scratch a third time.

## This session in full

Session opened by requesting device-bridge access to `~/src/scatter-dve`
and `~/src/Blackmagic DeckLink SDK 16.0` (approved), reading
`SESSION-PROTOCOL.md` in full — including its own "Session close" section,
updated Session 31 to require an explicit `git push` in every future
close-out, read carefully per Steve's own instruction since it changes
this session's own final command block — plus `HANDOFF.md`, `WORK-UNITS.md`,
`DECISIONS.md`, `CORRECTIONS.md` and `INVARIANTS.md` in full, and verifying
Session 31's own account of tag/commit state directly against the real
repository (`git tag`, `git log --oneline -10`, `git status --short`)
before trusting any of it — `wu-22b-green` confirmed present, `git show
wu-22b-green --stat` confirmed the exact seven files claimed, `origin`
confirmed configured at `https://github.com/skunge2000/scatter-dve.git`,
and `main` confirmed in sync with `origin/main` — checked out exactly as
claimed, no discrepancies found.

WU-22c was scoped directly with Steve, per his own explicit instruction
not to assume a design beyond ADR-057's own WU-22c paragraph, via direct
questions covering: the flag's exact name/behavior, how the terminal
keypress loop and Cocoa's own required main-thread run loop coexist (named
by Steve as "a real, unresolved architecture question, not a detail to
assume"), which thread produces frames and whether ADR-057's own
double-buffer/`dispatch_async` sketch still fit once the real threading
structure was read in full (`decklink_capture_consumer.hpp`/`.cpp`,
`decklink_live_output.hpp`/`.cpp`, `decklink_input.hpp` all read in full
before answering), what pressing `Q` in the coverage window should quit,
and redraw cadence. Full reasoning and every decision recorded in
`DECISIONS.md` ADR-058: `--show-coverage` (Steve's own choice to keep
ADR-057's own working name); one unified main-thread loop via GCD dispatch
sources (`DISPATCH_SOURCE_TYPE_READ` on `STDIN_FILENO`, queued onto
`dispatch_get_main_queue()`, alongside `CoverageWindow::run()`'s own
`[NSApp run]`), function-pointer GCD APIs only (no Blocks, keeping the test
a plain `.cpp`); a new non-blocking `IncrementalKeyParser` for the flag-on
path only, since the existing blocking `readKey()` would freeze the whole
Cocoa run loop on a stray ESC keypress if reused there; a new opt-in
`CaptureConsumer::CoverageCallback` hook (default `nullptr`, zero cost when
absent, mirroring `PipelineParams::pool`/`weightOut`'s own convention); a
per-frame heap-allocated `dispatch_async_f` hand-off (not a literal
reusable double-buffer — see ADR-058 for why this still satisfies ADR-057's
own design intent); a new additive `CoverageWindow::requestQuit()` method
so one `Q`, from either channel, quits the whole session (falls out of the
existing post-loop cleanup, not new shutdown code); and why the coverage
window never opens in a non-interactive run (no quit signal would ever
reach it, would hang an unattended `ctest` run forever).

Delivered nine files, all written locally in the sandbox first, then
staged to the real repository via `device_stage_files`... `SendUserFile` +
`device_commit_files` (chosen over `device_bash` heredocs specifically to
avoid shell-escaping risk against the C++ code's own many quotes/
backslashes/printf format strings), and confirmed landed correctly via
`device_bash`'s own `wc -l`/`diff`/`sha256sum`/`git status --short`/
`git diff --stat` (see the delivery-confirmation block below for the exact
counts): `tests/test_decklink_live_sphere.cpp` (rewritten — argv parsing,
the `coverageActive` gate, `IncrementalKeyParser`, `CoverageInputContext`/
`handleCoverageStdinReadable()`, `CoverageDispatchContext`/
`applyCoverageOnMainThread()`, the flag-on unified loop alongside the
byte-for-byte-unchanged flag-off blocking loop), `src/diag/coverage_view.hpp`/
`.mm` (additive only — the new `requestQuit()` method, WU-22b's own
existing surface untouched), `src/io/decklink_capture_consumer.hpp`/`.cpp`
(additive only — the new optional `CoverageCallback` constructor parameter
and its wiring inside `processOne()`, WU-21b's own existing behavior
unchanged when absent), `CMakeLists.txt` (the `scatter-diag` block moved
ahead of the `scatter-decklink` block — a CMake ordering requirement, not a
behavior change, explained in a new comment at the moved block's own site —
plus `scatter-diag` added to `test_decklink_live_sphere`'s own link line),
`DECISIONS.md` (ADR-058 appended in full), `WORK-UNITS.md` (WU-22c's own
entry replaced from unscoped `todo` to a fully scoped `wip` entry, `Files`/
`Accept` sections matching this project's own established convention), and
this file. All of it entirely reasoned through, not built or run anywhere
— this sandbox has no Blackmagic SDK, no Cocoa, no Metal, no AppleClang/
Xcode toolchain at all, and this unit touches both the DeckLink live
capture/output path and Metal/Cocoa at once, for the first time in one
unit.

**Delivery confirmation (this session's own device_bash checks after
writing every file):** `wc -l` on each of the nine files matched this
session's own sandbox copies exactly; `git status --short` at
`~/src/scatter-dve` after delivery showed exactly the nine files listed
above as modified, nothing else; `git diff --stat` confirmed each file's
own diff was the intended change (additive-only diffs for
`coverage_view.hpp`/`.mm` and `decklink_capture_consumer.hpp`/`.cpp`; a
full-file replacement diff for `tests/test_decklink_live_sphere.cpp`; a
reordering-plus-one-line diff for `CMakeLists.txt`; append-only diffs for
`DECISIONS.md`/`WORK-UNITS.md`/`HANDOFF.md`). No `.git/index.lock` issue
encountered this session (Flagged item 4 — this session made no commits of
its own, nothing to trigger it).

**Same-session follow-up, after Steve built and ran the delivery above at
his own real terminal:** he reported back precisely — the coverage window
opened and updated, but none of the sphere controls worked while it had
keyboard focus, only the terminal drove them. Correct behavior and not a
build failure: macOS keyboard focus is per-window, so the stdin channel
this unit built only ever sees a keystroke typed while the *terminal* has
focus, and `ScatterCoverageMTKView`'s own `-keyDown:` (WU-22b) never
recognized anything but `Q`. This was a genuine open question this ADR's
own scoping conversation never asked ("should the coverage window also
drive the full control scheme, or stay display-plus-quit-only") — asked
directly, Steve chose the former. Fix delivered as an amendment to this
same session's own not-yet-committed work (folded into `DECISIONS.md`
ADR-058 as an addendum and into `WORK-UNITS.md`'s WU-22c entry, the same
"revise before first commit, don't reopen" pattern Session 31 already used
for ADR-057's own verification addendum): `CoverageWindow` gains
`SpecialKey`/`setKeyHandler()` (`coverage_view.hpp`/`.mm`), a second,
generic, opt-in hook parallel to `requestQuit()` — `ScatterCoverageMTKView`'s
own `-keyDown:` still checks `Q` first, unchanged, then classifies arrow
keys via Cocoa's own documented `NSUpArrowFunctionKey`-style constants and
forwards everything else (arrow identity or raw character) to the handler
if one is set; `CoverageWindow` itself still carries no sphere-specific
vocabulary of its own. `tests/test_decklink_live_sphere.cpp` adds
`mapCoverageWindowKey()` (a third small duplicate of the letter/arrow
mapping, alongside `readKey()` and `IncrementalKeyParser::mapLetter` — three
different-shaped call sites) and wires `coverageWindow->setKeyHandler()`
to run the same `applyKey()`/`consumer.setLattice()` logic the stdin
channel already uses. Both input channels now run on the main thread only
(GCD's main queue and Cocoa's own event dispatch are the same thread), so
no new synchronisation was needed for the shared `yaw`/`pitch`/`centerX`/
`centerY`/`radius` state. Re-delivered and re-confirmed landed (same
`device_bash` `wc -l`/`sha256sum`/`git status --short` checks) for the
three files this follow-up touched: `src/diag/coverage_view.hpp`,
`src/diag/coverage_view.mm`, `tests/test_decklink_live_sphere.cpp` — plus
`DECISIONS.md` and `WORK-UNITS.md` for the addendum/entry update, and this
file. **This follow-up is itself just as `UNVERIFIED IN FULL` as the rest
of WU-22c — it has not yet been rebuilt or reverified at Steve's own real
terminal.**

## Where we are

Phase 5 (Live capture) is unchanged from Session 31's own account. Phase 6
(Scale up) now reads: WU-22a `green` (`wu-22a-green`), WU-22b `green`
(`wu-22b-green`, unchanged this session), WU-22c `wip` (scoped and drafted
this session, `UNVERIFIED IN FULL` — not yet built, run, committed, or
tagged). `DECISIONS.md` runs through ADR-058. `CORRECTIONS.md` is
unchanged this session (still through C-018) — nothing this session
surfaced was a new codebase-logic lesson; every design question this
session hit was resolved by reading this project's own existing code and
ADRs (the threading structure, the existing keypress-quit mechanism), not
by discovering a defect in them.

## Next work unit

Not decided here, deliberately — but the overwhelmingly natural next step
is **Steve's own build/run/verify/commit/tag/push of WU-22c**, not a new
unit — see "Steve's own next steps" below for the exact commands. Once
that is done and confirmed `green`, candidates for the session after that:
WU-21d (cold-start black-fill fix), starting to scope WU-28 (k-buffer), and
fixing WU-21i's own stale status line (Flagged item 1, now four sessions
old) all remain open, unchanged from Session 31's own account.

## Open questions

Unchanged from Session 31: `kCaptureRingCapacity`'s value of 8 (WU-20a/20b,
ADR-046), the cold-start green-frame artifact (WU-21d), and front/back
occlusion/transparency (WU-28) all remain open. Q3 (macOS/Desktop Video
version) remains open. Q4 (lattice edge damping, C-008(a)) remains open,
not touched this session (WU-22c never reaches `core/jacobian.hpp` or the
splat/resolve path — it only adds an optional read of an already-captured
`weightOut` buffer and a display path for it).

**New open question from this session, named in `DECISIONS.md` ADR-058's
own closing paragraph:** whether GCD's own main-queue draining actually
interleaves promptly with Cocoa's own event processing during `[NSApp
run]`, in the specific way this unit's design depends on. Reasoned from
Apple's own published documentation, not confirmed — the single most
likely first problem if the coverage-enabled interactive build behaves
oddly (sluggish or dropped keypresses, specifically in the
`--show-coverage` build, not the flag-off one).

## Blocked / red

Nothing red. WU-22b remains genuinely `green`, untouched this session.
WU-22c is not red — it is `wip`, fully scoped and drafted, delivered and
confirmed landed in the real repository, waiting on Steve's own real
terminal for its first build.

## Environment check

Unchanged from Session 31: **UltraStudio Monitor 3G** (output,
HDMI-mirrored) and **UltraStudio Recorder 3G** (input) both last confirmed
working in Session 29's own real-hardware runs (not touched this session —
WU-22c has not been built yet, so nothing new has run against either
device). **UltraStudio 4K Mini** remains on hold pending a PSU replacement.
Metal/Cocoa itself remains confirmed working on Steve's real Mac
hardware/GPU/OS version, per Session 31's own `coverage_view_demo` run —
this session's own new Cocoa/GCD surface (the unified main-thread loop,
dispatch sources) is new territory on top of that confirmed base, not yet
itself confirmed. `origin` (`https://github.com/skunge2000/scatter-dve.git`)
remains configured, unchanged this session — no push happened this session
since nothing was committed.

## Append to DECISIONS.md

ADR-058 (WU-22c design: the full scoping conversation, the unified
main-thread-loop/GCD-dispatch-source design, `IncrementalKeyParser`, the
`CoverageCallback`/`dispatch_async_f` hand-off design and why it satisfies
ADR-057's own "double-buffer" intent without a literal fixed buffer pair,
the new `CoverageWindow::requestQuit()` method, the non-interactive-run
downgrade, and the full `UNVERIFIED IN FULL` disclaimer) — appended in
full this session; see `DECISIONS.md`. Does not reopen `docs/architecture.md`,
ADR-031, ADR-040, ADR-044, ADR-046, ADR-047, ADR-048, ADR-049, ADR-050,
ADR-053, ADR-054, ADR-055, ADR-056 or ADR-057 — see ADR-058's own closing
"does not reopen" paragraph for the detail on each.

## Append to CORRECTIONS.md

None this session — nothing surfaced that was a new lesson rather than a
design question resolved by reading this project's own existing code
(threading structure, existing keypress-quit mechanism) more carefully.

## Closed out this session

Nothing closed out — WU-22c is deliberately left `wip`, not `green`: this
session's own explicit, standing constraint is that it cannot compile or
run anything touching Metal, Cocoa, or the Blackmagic DeckLink SDK, and
WU-22c touches both at once, in the same unit, for the first time. Every
file is delivered and confirmed landed in the real repository, and every
design decision is recorded in `DECISIONS.md` ADR-058, but the actual
build/run/commit/tag/push loop is entirely Steve's own next action — see
below.

## Steve's own next steps

Run these at your own real terminal (not via the device bridge) to build
and verify WU-22c, then close it out. `.git/index.lock` may need clearing
first (Flagged item 4):

```
rm -f ~/src/scatter-dve/.git/index.lock
cd ~/src/scatter-dve
cmake --build build
```

If that succeeds cleanly, run the test both ways — first unchanged
(flag-off), then with the new coverage window:

```
./build/test_decklink_live_sphere
./build/test_decklink_live_sphere --show-coverage
```

For the flag-off run, confirm it behaves exactly as WU-21i's own already-
`green` build already does (no change expected or acceptable). For the
`--show-coverage` run, confirm: a coverage window opens alongside live SDI
playback; it updates roughly once per live frame; **all ten controls
(cursor keys for yaw/pitch, X/x/Y/y for position, Z/z for radius) work
twice over — once with the terminal focused, and separately with the
coverage window itself clicked into and focused** (this second half is the
same-session follow-up fix, prompted by your own first build showing only
the terminal driving controls — worth deliberately testing both, not just
one); and `Q`, pressed either at the terminal or with the coverage window
focused (or closing the window via its own close control), cleanly ends
both the window and the whole session — capture/consumer/playback all
stop, stats print to `stderr`, the process exits, with no hang either way.

If both runs behave as expected, commit and close out. `./tools/close.sh
22c` will very likely refuse to auto-tag on the same already-accepted
`test_decklink_device`/ADR-035 duplex-check exception every DeckLink-
adjacent unit since ADR-035 has hit (the real hardware — UltraStudio
Monitor 3G — is playback-only, not full-duplex) — if so, use the manual
tag fallback below. Either way, **push is required as an explicit final
step** per `SESSION-PROTOCOL.md`'s own Session-31-updated "Session close"
section — `close.sh` pushes automatically only on a successful *auto*-tag,
which this unit is expected not to get:

```
git add tests/test_decklink_live_sphere.cpp src/diag/coverage_view.hpp src/diag/coverage_view.mm src/io/decklink_capture_consumer.hpp src/io/decklink_capture_consumer.cpp CMakeLists.txt DECISIONS.md WORK-UNITS.md HANDOFF.md
git commit -m "WU-22c: wire CoverageWindow into the live capture/output pipeline (--show-coverage)"
./tools/close.sh 22c
```

If `close.sh` refuses to auto-tag (the expected ADR-035 duplex-check
outcome), run the manual fallback and the required explicit push
yourself:

```
git tag -a wu-22c-green -m "WU-22c: wire CoverageWindow into the live capture/output pipeline (--show-coverage)"
git push origin main
git push origin --tags
```

If `close.sh` *does* auto-tag successfully, it already runs `git push
origin HEAD --tags` for you — no further push command needed in that case.

Once done, a `git show wu-22c-green --stat` should list exactly the nine
files this session delivered (the same nine listed in "This session in
full" above), and `git status -sb` should read `## main...origin/main`
with no `[ahead]`/`[behind]` marker.
