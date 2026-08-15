# Handoff
Overwritten at the end of every session. This is the first thing to read.

---

**Session:** 17
**Tag:** `wu-15a-green`, unchanged this session — nothing new was built or
tagged.
**Phase:** 3 — SDI output. WU-15a (scheduled playback of one looped,
file-sourced, warped frame) is `green`. WU-15b (the one-hour unattended
endurance run) is still `todo` — this session did not run it, only scoped
exactly how it will run, per this session's own brief: resolve two real
open questions "before running anything for real, not to assume."

**This was a short, decision-only session — no hardware access, no code
touched beyond documentation.** Read `HANDOFF.md`/`INVARIANTS.md`/
`DECISIONS.md`/`CORRECTIONS.md`/`WORK-UNITS.md` in order per
`SESSION-PROTOCOL.md`, then re-read `DECISIONS.md` ADR-032 (the WU-15a/
WU-15b split and `LoopedFramePlayback`'s own design), ADR-034/035 (the 4K
Mini incident and the `close.sh` exception it left), ADR-036 (the
warp-visibility false alarm, read for its own diagnostic-rigor standard),
and ADR-037 (the Monitor 3G/Recorder 3G hardware split) — all as this
session's own brief asked. WU-15b's own `WORK-UNITS.md` line and ADR-032
both already fix *what* WU-15b is (WU-15a's own mechanism, run for an hour,
no new `Files:`/`Accept:` lines) but neither ever fixed *how* "run for an
hour" is actually invoked, or how an hour-long unattended run is actually
expected to survive being unattended — two real gaps, not assumptions to
paper over before Steve spends an hour of real hardware time on this.

**1. Duration-invocation mechanism, decided and frozen as `DECISIONS.md`
ADR-038.** `tests/test_decklink_output.cpp`'s own bounded-run test
hardcodes `std::this_thread::sleep_for(std::chrono::seconds(5))` (line
168) with no existing parameter for a caller to ask for longer. Two
options were weighed: a real CLI arg or environment variable (rejected —
new implementation by ADR-032's own already-frozen standard, needing a
name/type/default/validation decision none of `architecture.md` or any
existing ADR speaks to, and a real foot-gun if a stray default or leaked
env var ever made a future `close.sh` run hang for an hour on one test)
versus hand-editing the existing literal for exactly one manual run,
uncommitted, reverted immediately after (chosen — genuinely no new
implementation; `close.sh`'s own `ctest` run stays at its documented
~5-second cost forever regardless of what one dirty working tree
temporarily does). See ADR-038 for the full reasoning and the exact
mechanics: edit line 168 to `seconds(3600)`, `cmake --build build`, run
`./build/test_decklink_output` directly (not `close.sh` — its git-dirty
gate would correctly refuse, and there is nothing here for it to tag
anyway), then `git checkout -- tests/test_decklink_output.cpp` to revert
before touching the repository again.

**2. Unattended one-hour survival plan, decided and frozen below (not an
ADR — no code-design weight, the same category this file's own
"Environment check" section already uses for hands-on hardware
confirmations).** Closing a MacBook's lid triggers system sleep regardless
of any software-level sleep-prevention assertion unless the machine is in
clamshell mode with an external display already attached and powered — not
confirmed available here, so the frozen instruction is simply: don't close
the lid. On top of that, `caffeinate -s` prevents idle/timer system sleep
for as long as its wrapped command runs, while the Mac stays on AC power
(both conditions `caffeinate -s`'s own man page requires for the assertion
to hold at all). Run under `nohup` + background + `disown` rather than in a
foreground tab Steve has to leave open and untouched, so an accidental
Terminal.app quit doesn't `SIGHUP` the run. See "What to run at your
terminal," below, for the exact command line.

**No corrections this session.** Nothing earlier was found wrong — this
session only filled two gaps neither ADR-032 nor `WORK-UNITS.md`'s own
WU-15b line had ever actually closed, which is normal, expected
session-opening work per `SESSION-PROTOCOL.md`, not an error to log in
`CORRECTIONS.md`.

**Tests:** unchanged this session — no test was built or run (no
Blackmagic SDK, no AppleClang, no hardware access from the cloud sandbox,
same constraint as every hardware-dependent unit since WU-14).

**Build:** unchanged this session — no production or test code was
touched. `tests/test_decklink_output.cpp` and `src/io/decklink_output.hpp`/
`.cpp` are exactly as `wu-15a-green` left them.

## Where we are

`src/io/decklink_output.hpp`/`.cpp` — `LoopedFramePlayback`, unchanged
since `wu-15a-green`, confirmed working on real hardware (both the 4K Mini
and the Monitor 3G). `tests/test_decklink_output.cpp` — unchanged since
`wu-15a-green`; its own bounded-run literal (line 168, `seconds(5)`) is the
one Steve temporarily edits per ADR-038 for the WU-15b run itself, then
reverts. `DECISIONS.md` now runs through ADR-038. See `WORK-UNITS.md`'s
WU-15b entry for the short status note pointing at both.

**Corrections this session:** none.

**Delivery mechanics:** this session ran remotely via the device-bridge
tools throughout, same as every hardware-adjacent session since WU-14 —
but touched only `DECISIONS.md`, `WORK-UNITS.md` and this file; no `src/`,
`tests/` or `CMakeLists.txt` change. One commit this session — see `git
log` for its actual hash, made after this file was written. Working tree
is clean as of this handoff.

## Next work unit

WU-15b itself — Steve's own hands-on step, not a session's own job to
assert green from a terminal (ADR-032, unchanged). Both open questions
this session was asked to resolve are now frozen (ADR-038; the survival
plan below), so there is nothing left to decide before running it — only
to run it and report back `stats().dropped`/`stats().displayedLate`/
`stats().completed` (the log line `test_decklink_output.cpp`'s own test
already prints at `stop()`) plus a by-eye confirmation the warped cylinder
stayed visible throughout, watching specifically for ADR-036's own
already-documented 4:3-on-16:9 false-alarm mode.

After WU-15b closes, next is WU-16 (thread pool, QoS, per-worker bin
arenas — Phase 4) — or, per `DECISIONS.md` ADR-037's own three named
follow-ups (not resolved yet, not this session's job either): retiring or
rescoping `test_decklink_device.cpp`'s full-duplex check, revisiting
genlock for two independent-clock devices, or naming the Recorder 3G
explicitly in future capture-side scoping. Worth a look before picking
whichever of WU-16 or one of those three comes next.

## Open questions

Unchanged from session 16: Q1 (tile size), Q3 (macOS/Desktop Video
version), Q4 (lattice edge damping, C-008(a)). Q2 (4K Mini program
outputs) remains moot per ADR-037.

Resolved this session: WU-15b's own duration-invocation mechanism
(ADR-038) and its unattended-survival plan (below) — both were genuinely
open, not previously assumed either way, per this session's own brief.

## Blocked / red

Nothing red, nothing blocked. WU-15b is `todo`, waiting on Steve's own
hour at the real terminal — not blocked, just not yet run.

## Environment check

Unchanged from session 16 (ADR-037): **UltraStudio Monitor 3G** is the
active output target, confirmed working (`bmdModePAL` +
`bmdFormat10BitYUV`); **UltraStudio Recorder 3G** is in hand for input, not
yet touched by any code; **UltraStudio 4K Mini** remains on hold pending a
PSU replacement, not part of the active plan. WU-15b runs against the
Monitor 3G, same as WU-15a's own verification did.

## Append to DECISIONS.md

ADR-038 was appended in full this session; see `DECISIONS.md`. Does not
reopen ADR-032 — completes a gap that entry left implicit (WU-15b's own
*scope* versus its *invocation mechanism*).

## Append to CORRECTIONS.md

Nothing this session — see "No corrections this session" above.

---

## What to run at your terminal

**WU-15b — the one-hour endurance run.** Two things are now frozen
(ADR-038 for the first, this section for the second) so this is a
mechanical recipe, not a judgement call:

1. Confirm a clean tree: `git status` (should show nothing to commit — if
   it doesn't, stop and figure out why before proceeding).
2. Edit `tests/test_decklink_output.cpp` line 168:
   ```cpp
   std::this_thread::sleep_for(std::chrono::seconds(5));
   ```
   becomes
   ```cpp
   std::this_thread::sleep_for(std::chrono::seconds(3600));
   ```
   This is the file's only bounded-run-length literal — nothing else
   needs to change.
3. Rebuild: `cmake --build build`.
4. Keep the Mac on AC power and **do not close the lid** for the whole
   hour — clamshell sleep overrides any software sleep-prevention unless
   an external display is already attached and powered, which isn't
   confirmed here.
5. Run it detached, so an accidental Terminal.app close doesn't kill it:
   ```sh
   nohup caffeinate -s ./build/test_decklink_output > /tmp/wu15b_run.log 2>&1 &
   disown
   ```
   Point a broadcast monitor at the Monitor 3G's SDI output for the hour,
   watching for the warped cylinder — remember ADR-036's own false-alarm
   mode: a 720x576, 4:3-ish frame on a 16:9 monitor can look deceptively
   close to un-warped at a glance. Look for the letterboxing/oval shape
   specifically, not just "does it look round."
6. After an hour, check `/tmp/wu15b_run.log` for the
   `completed=... displayedLate=... dropped=... flushed=...` line the test
   itself prints at `stop()`. `Accept:` is `displayedLate == 0` and
   `dropped == 0` across the whole run.
7. Revert the edit: `git checkout -- tests/test_decklink_output.cpp`, then
   `git status` to confirm the tree is clean again before doing anything
   else in the repository — including any future `close.sh` run for a
   later work unit, whose own git-dirty gate would otherwise (correctly)
   refuse on what would look like an unrelated dirty tree.
8. Report back: the four counts, and whether the by-eye check held for
   the whole hour. That's WU-15b's own `Accept:` criteria, in full —
   nothing else to check, and nothing to tag (ADR-032: WU-15b was never
   scoped with `Files:`/`Accept:` source lines a `close.sh` run could gate
   on).

If anything about the Monitor 3G/Recorder 3G split changes, or the 4K
Mini's PSU gets replaced, let me know whenever it happens — no rush,
doesn't block WU-15b.
