# Handoff
Overwritten at the end of every session. This is the first thing to read.

---

**Session:** 16
**Tag:** none yet — WU-15a is `wip`, not `green`. `wu-14-green` (last
session) remains the last confirmed tag.
**Phase:** 3 — SDI output, under way. WU-14 (device enumeration, `ComPtr`)
is `green`. WU-15a (scheduled playback of one looped, file-sourced, warped
frame) made real progress this session — one clean passing real-hardware
run — but is **not** `green`: a hardware incident partway through the
session (see below) means the remaining verification steps now target
different hardware than the run that passed.

**This session's own first job was research, not implementation**, per
`SESSION-PROTOCOL.md`'s required reading order and this project's own
established discipline since WU-14: `HANDOFF.md`, `INVARIANTS.md`,
`DECISIONS.md`, `CORRECTIONS.md`, `WORK-UNITS.md`, then `docs/architecture.md`
section 7 (Output) and section 10 (Phase 3), then the real Blackmagic
DeckLink SDK 16.0 headers and its own `FilePlayback`/`SignalGenerator`
samples — before scoping any code. That reading forced the WU-15a/WU-15b
split (`DECISIONS.md` ADR-032) and the design choices it records.
`decklink_output.hpp`/`.cpp` and `tests/test_decklink_output.cpp` were
written from that research and pushed to the real machine via the device
bridge, unbuilt, as usual for hardware-dependent work.

**First real-terminal run, first problem, first fix — all resolved
cleanly, in order:**

1. `cmake --build build` succeeded; `ctest -R test_decklink_output` failed
   at `LoopedFramePlayback::create()` returning null, no diagnostic beyond
   `FAIL test_decklink_output.cpp:138 bool(playback)`. Temporary per-step
   `stderr` diagnostics were added to `startWith()` (not in the final
   pushed version) and a second run pinpointed it: `DoesSupportVideoMode()`
   returned `S_OK` with `supported == false` for `bmdModePALp` +
   `bmdFormat10BitYUV` — the UltraStudio 4K Mini's driver does not offer
   that combination. `bmdModePALp` was ADR-032's own first choice,
   explicitly flagged unverified, with `bmdModePAL` named as the documented
   fallback — so this was the named risk, hit and resolved via its own
   pre-planned fallback, not a defect. Recorded as `DECISIONS.md` ADR-033.
2. Switching `tests/test_decklink_output.cpp`'s `kDisplayMode` to
   `bmdModePAL` and rebuilding: **`test_decklink_output` passed both
   checks, 5.34s, against the real UltraStudio 4K Mini** — the first actual
   signal this project has ever put out over SDI.
3. Steve then corrected my own stated *reasoning* for step 1: I had framed
   `bmdModePALp` ("576p25") failing as a 4K-Mini-specific driver gap;
   Steve's own domain knowledge is that 576p25 is not a real deployable
   HDMI/SDI/analogue broadcast signal at all, on any device. Verified via
   web research (BT.1358 defines 576p25 on paper for DVD-Video/file use;
   real deployed progressive SD broadcast was 576p50; standard SD
   transmission is interlaced, 576i25). Logged as `CORRECTIONS.md` C-013 —
   corrects the *stated reason*, not the decision itself (`bmdModePAL` was
   already right).

**Then the hardware incident.** Immediately after that passing run, Steve
ran the full suite (`ctest --test-dir build --output-on-failure`). Two
tests failed — `test_decklink_device` and `test_decklink_output`, both on
`CHECK(!devices.empty())` — because **the UltraStudio 4K Mini had gone
completely unresponsive**: not seen in Desktop Video Setup, not seen in
Media Express, not seen in Apple's own Thunderbolt System Report (not
"no signal" — absent from Thunderbolt enumeration entirely), and no longer
providing Thunderbolt/USB-PD power passthrough to the MacBook Pro.
Restarting the Blackmagic driver helper did not recover it. Steve swapped
in the spare UltraStudio Monitor 3G on the identical port and cable — it
worked immediately and normally — which isolates the fault to the 4K Mini
unit itself, not the Mac, the port, the cable, or the driver install.

I gave Steve a reasoned, honestly-bounded assessment (now the permanent
record in `DECISIONS.md` ADR-034, not repeated in full here): this
project's code only ever calls the DeckLink SDK's public API surface
(enumeration, capability queries, `EnableVideoOutput`/`DisableVideoOutput`,
`CreateVideoFrame`/`GetBytes`, `ScheduleVideoFrame`,
`StartScheduledPlayback`/`StopScheduledPlayback`,
`SetScheduledFrameCompletionCallback`), none of which reaches firmware,
Thunderbolt bus enumeration, or power-delivery negotiation. This is a
different, more severe failure than the "reference-count leak locks the
device until reboot" risk architecture.md 12 already names (a driver-state
lockout with the device still hardware-visible) — what actually happened is
a hardware/firmware/power-level fault. Judged **unlikely** to be
code-caused, on the evidence (clean, fast, fully-passing single-test run;
death only surfaced during a later, broader run whose own non-hardware
tests passed normally; fault isolated to the one unit by direct swap), but
**not provable with certainty** from software evidence alone — a
pre-existing fault surfacing coincidentally on the unit's first real
signal-output use cannot be ruled out. No `CORRECTIONS.md` entry follows:
nothing this project claimed about the SDK or hardware has been shown wrong
by this incident; the 4K Mini simply stopped responding to *everything*,
including tools that share none of this project's own code.

**Decision (Steve's, recorded as `DECISIONS.md` ADR-034):** pause work
against the 4K Mini — Blackmagic support and/or a hardware check are
Steve's own next steps outside this project, not mine. Continued WU-15a
verification, and WU-15b when it starts, **targets the UltraStudio Monitor
3G** instead, until the 4K Mini's status is resolved. This does **not**
supersede ADR-006/ADR-013 (the 4K Mini remains the project's intended
primary hardware once it's working) or ADR-011 (the Monitor 3G's originally
scoped role, a diagnostic coverage view) — it's a hardware-availability
pivot, not a redesign; see ADR-034 for the precise relationship. **No code
change was needed for the pivot itself**: `decklink_device.cpp`'s
enumeration (WU-14) and `firstPlaybackCapableOutput()` in
`tests/test_decklink_output.cpp` (WU-15a) already select by
`supportsPlayback`/`QueryInterface(IID_IDeckLinkOutput)`, not by model name
or any 4K-Mini-specific assumption, so the Monitor 3G is picked up
unchanged once it's the device the SDK's own iterator returns. **What is
genuinely still open:** whether `DoesSupportVideoMode(bmdModePAL,
bmdFormat10BitYUV)` succeeds on the Monitor 3G the way it did on the 4K
Mini — untested; the Monitor 3G is a simpler, output-only device (no
capture inputs) and its supported-mode list isn't assumed to match.

**Tests:** `test_decklink_output` passed once, in full, against the 4K
Mini (5.34s, both checks) before the incident. Not yet run against the
Monitor 3G. The full suite has not gone green this session — the two
hardware-dependent tests failed on empty device enumeration once the 4K
Mini went unresponsive; all fourteen non-hardware-dependent tests from
WU-01 through WU-13 were unaffected by anything this session touched.

Unlike every unit since WU-06, this session's own implementation was not
first run through the Linux cloud sandbox's Clang/GCC/ASan/UBSan matrix —
no Blackmagic SDK and no AppleClang/Xcode toolchain exist there, and
`CMakeLists.txt`'s `BLACKMAGIC_SDK_DIR` guard means that matrix never sees
these files. Written straight to the real machine via the device bridge,
same as WU-14.

**Build:** succeeded this session (`cmake --build build`, existing cache,
no reconfigure needed). Next real-terminal step is a fresh `ctest` run with
the Monitor 3G attached in place of (or alongside — Thunderbolt permitting)
the 4K Mini.

## Where we are

`src/io/decklink_output.hpp`/`.cpp` — `LoopedFramePlayback`, a looped
single-frame scheduled-playback wrapper around `IDeckLinkOutput`
(`create()`, `stop()`, `stats()`), implementing `IDeckLinkVideoOutputCallback`
directly. Confirmed working against real hardware (the 4K Mini) once,
before the incident; unchanged since — no code fix is implicated by the
incident, so none was made. `tests/test_decklink_output.cpp` — the two
checks, `kDisplayMode = bmdModePAL` (ADR-033), builds its own warped source
frame via `runFrameFile()`/`buildCylinderLattice()`. `CMakeLists.txt` —
`decklink_output.cpp` in the `scatter-decklink` target; `test_decklink_output`
alongside `test_decklink_device`, linking both `scatter-decklink` and
`scatter-core`. See `DECISIONS.md` ADR-032/ADR-033/ADR-034 and
`WORK-UNITS.md`'s WU-15a/WU-15b entries.

**Corrections this session:** C-013 (`CORRECTIONS.md`) — corrected my own
stated reasoning for why `bmdModePALp` failed (576p25 isn't a real
broadcast signal at all, not a 4K-Mini-specific gap); does not change the
`bmdModePAL` decision itself. No correction logged for the hardware
incident (see ADR-034 — nothing this project claimed was shown wrong).

**Delivery mechanics, not a design matter:** this session ran remotely, via
the device-bridge tools. `git add`/`git commit` for `CMakeLists.txt`,
`DECISIONS.md` (ADR-032), `HANDOFF.md`, `WORK-UNITS.md`,
`src/io/decklink_output.hpp`/`.cpp`, `tests/test_decklink_output.cpp` ran
mid-session as commit `c5605f4`. **Not yet committed:** the `bmdModePAL`
fix in `tests/test_decklink_output.cpp`, ADR-033, ADR-034, C-013, and this
file plus `WORK-UNITS.md`'s status updates — all pushed to the Mac this
session but sitting as uncommitted working-tree changes; `git status
--short` shows `CORRECTIONS.md`, `DECISIONS.md`, `WORK-UNITS.md`,
`tests/test_decklink_output.cpp` modified. Commit these before starting the
Monitor 3G run, so a build/test failure against the new hardware target
doesn't get tangled up with this session's own already-settled writing.
Stale `index.lock`/`HEAD.lock`/temp-object files from the bridge's own
`unlink`-can't-work-on-this-mount limitation continue to accumulate in
`_to_delete/`; safe to `rm -rf _to_delete/` by hand whenever convenient.

## Next work unit

**Immediate, still within WU-15a, not a new unit:** commit the pending
changes listed above, then physically switch to the UltraStudio Monitor 3G
(4K Mini disconnected or left unresponsive, per Steve's own hardware
troubleshooting) and re-run at the real terminal:

```
cd ~/src/scatter-dve
git add CORRECTIONS.md DECISIONS.md WORK-UNITS.md HANDOFF.md tests/test_decklink_output.cpp
git commit -m "WU-15a: bmdModePAL fix, ADR-033/034, C-013 -- 4K Mini hardware incident, pivot to Monitor 3G"
ctest --test-dir build --output-on-failure
```

If `test_decklink_device`/`test_decklink_output` now enumerate the Monitor
3G and `DoesSupportVideoMode(bmdModePAL, bmdFormat10BitYUV)` succeeds on
it: point a broadcast monitor at the Monitor 3G's SDI/HDMI output, confirm
by eye that the warped cylinder frame appears, then run the full suite and
`./tools/close.sh 15a` and report back — I'll update `WORK-UNITS.md` and
tag `wu-15a-green`. If `DoesSupportVideoMode` fails on the Monitor 3G for
`bmdModePAL`: tell me the exact `hr`/`supported` output (same as the
`bmdModePALp` diagnostic this session) and I'll research the Monitor 3G's
own supported-mode list before picking a next mode to try — don't guess one
by hand without that.

If the 4K Mini recovers on its own or via Blackmagic support before the
Monitor 3G run happens, that's fine too — either device satisfies WU-15a's
own `Accept:` criterion (a broadcast monitor, not a specific one), though
`WORK-UNITS.md`'s `Accept:` line was written assuming the 4K Mini and is
worth a one-line note either way once this closes, for the historical
record.

## Open questions

Unchanged from session 15: Q1 (tile size), Q2 (4K Mini program outputs —
now also entangled with whether the 4K Mini itself is even usable again),
Q3 (macOS/Desktop Video version), Q4 (lattice edge damping, C-008(a)).

Resolved this session: whether `bmdModePALp` is supported on the 4K Mini —
no (ADR-033); whether `RowBytesForPixelFormat(bmdFormat10BitYUV, ...)`
matches `v210::rowBytesMin()` — yes, confirmed on the 4K Mini before the
incident.

New this session: is the 4K Mini recoverable at all, and if not, what does
that mean for architecture.md's own hardware assumptions (composite/
component analogue inputs, 12G headroom, full duplex) longer-term — not
this session's call, Steve's own hardware diagnosis to run first. Does
`DoesSupportVideoMode(bmdModePAL, bmdFormat10BitYUV)` succeed on the
Monitor 3G — unverified, first job of the next real-terminal run.

## Blocked / red

Not blocked on code — WU-15a's own mechanism already ran clean once on real
hardware. Blocked on **hardware availability**: the UltraStudio 4K Mini is
unresponsive (see the incident write-up above and `DECISIONS.md` ADR-034
for full detail), and the next verification step needs the UltraStudio
Monitor 3G physically in place before it can run. Nothing to fix in the
repository before that swap happens.

## Environment check

**Changed this session.** The UltraStudio 4K Mini, previously confirmed
enumerating and full-duplex (WU-14), is now unresponsive — absent from
Desktop Video Setup, Media Express, and Apple's Thunderbolt System Report;
no Thunderbolt power passthrough. Isolated to the unit itself via a
cable/port swap against the UltraStudio Monitor 3G, which is confirmed
working on that same port and cable. Desktop Video Setup input/output-active
confirmation and a Media Express capture/playback round trip (architecture.md
10's own Phase 0 checklist) were never separately confirmed for the 4K Mini
before it failed, and are now moot for it until/unless it recovers; the
same checklist, run against the Monitor 3G, is worth doing before relying
on it for anything beyond WU-15a's own playback-only need (it has no
capture input at all, so the "capture" half of that checklist doesn't apply
to it regardless).

## Append to DECISIONS.md

ADR-033 (display mode confirmed: `bmdModePAL`) and ADR-034 (4K Mini
hardware incident; Monitor 3G becomes the active verification target) were
both appended in full earlier this session; see `DECISIONS.md`. Neither
reopens ADR-032, and ADR-034 does not reopen ADR-006/ADR-013/ADR-011 either
— see ADR-034's own closing section for the precise relationships.

## Append to CORRECTIONS.md

C-013 was appended in full earlier this session; see `CORRECTIONS.md`. No
further entry for the hardware incident — see "Corrections this session"
above for why.

---

## What to run at your terminal

```
cd ~/src/scatter-dve
git add CORRECTIONS.md DECISIONS.md WORK-UNITS.md HANDOFF.md tests/test_decklink_output.cpp
git commit -m "WU-15a: bmdModePAL fix, ADR-033/034, C-013 -- 4K Mini hardware incident, pivot to Monitor 3G"
```

Then, with the UltraStudio Monitor 3G connected in place of the 4K Mini:

```
ctest --test-dir build --output-on-failure
```

While `test_looped_playback_runs_with_no_dropped_or_late_frames` runs
(about five seconds), point a broadcast monitor at the Monitor 3G's SDI or
HDMI output and confirm by eye that a warped (cylinder-curved) test pattern
appears — the automated checks confirm the DeckLink-side mechanics, not
what's actually on the wire.

If everything passes and the frame is visible: run the full suite and
`./tools/close.sh 15a`, and let me know the result — I'll update
`WORK-UNITS.md` and tag `wu-15a-green` from what you report back, per this
project's own "the assistant does not run `close.sh`" rule.

If `DoesSupportVideoMode(bmdModePAL, bmdFormat10BitYUV)` fails on the
Monitor 3G, paste the exact `hr`/`supported`/`displayMode` values (same
diagnostic shape as the `bmdModePALp` one this session) and I'll research
the Monitor 3G's own supported-mode list before proposing a next mode —
don't want to guess one by hand without that.

Separately, and not blocking any of the above: whenever you've made
progress with Blackmagic support or your own hardware checks on the 4K
Mini, let me know what you find — I'll fold the outcome into `DECISIONS.md`
(either "recovered, ADR-034's pivot is temporary and closed" or something
that needs its own new decision if it turns out not to be).
