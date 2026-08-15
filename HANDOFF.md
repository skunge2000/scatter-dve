# Handoff
Overwritten at the end of every session. This is the first thing to read.

---

**Session:** 29
**Tag:** `wu-21b-green` confirmed to actually exist going into this session
— verified directly against the real repository's own `git tag`/`git log`
(not trusted from session 28's own handoff account). `HEAD` sat at
`bb97fa1` ("WU-21b: record real-hardware verification and wu-21b-green tag;
note Weston 3-field preference for future WU-23"), which already carries the
WU-21b real-hardware-verification update to `WORK-UNITS.md` — the commit
session 28's own handoff flagged as the one outstanding loose end (a short
`DECISIONS.md`/`WORK-UNITS.md`/`HANDOFF.md` commit) was in fact already made
before this session began; that uncertainty is resolved, not merely
repeated. **WU-21c was scoped and built this session — reasoned through
only, same shape as every DeckLink-touching unit before it (WU-14, WU-15a,
WU-20b, WU-21b): no Blackmagic SDK and no AppleClang/Xcode toolchain exist
in this session's own Linux cloud sandbox, so nothing here has been compiled
or run yet.**
**Phase:** 5 (Live capture) continues. WU-21c (continuous SDI re-output —
`LiveFramePlayback`, `src/io/decklink_live_output.hpp`/`.cpp`, scheduling
WU-21b's own `CaptureConsumer::copyLatestFrame()` onto `IDeckLinkOutput`
via a round-robin-refilled pool of frame buffers, replacing
`LoopedFramePlayback`'s own single-static-buffer design) was scoped this
session — real `Files:`/`Accept:` text, done only after the reading below —
built and delivered to the real repository. `WORK-UNITS.md`'s own WU-21c
line is `wip`. `DECISIONS.md` now runs through ADR-050. `CORRECTIONS.md`
still runs through C-016 — no new entry this session (nothing was claimed
and found wrong; see below).

## This session in full

**Reading, before anything else.** Per `SESSION-PROTOCOL.md`'s own reading
table, in full: `HANDOFF.md` (session 28's), `INVARIANTS.md`, `DECISIONS.md`
in full through ADR-049 including its own same-session real-hardware
verification addendum (not skimmed), `CORRECTIONS.md` through C-016,
`WORK-UNITS.md`.

**Folder access and tag/commit-state confirmation, before anything else.**
Requested and received device-bridge access to both `~/src/scatter-dve` (the
repo) and `~/src/Blackmagic DeckLink SDK 16.0` (the SDK). Confirmed this
session's own note about `device_bash`'s own different `$HOME` (mounted
paths reachable there as `~/mnt/scatter-dve`/`~/mnt/Blackmagic DeckLink SDK
16.0`, while `device_stage_files`/`device_commit_files` accept
`~/src/...` directly) — no rediscovery needed this time. Then, per the
explicit instruction not to trust `HANDOFF.md`'s own account: ran the real
repository's own `git tag`/`git log --oneline -8`/`git status --short`
directly via the device bridge. Result: `wu-21b-green` present, alongside
every earlier tag through it; `HEAD` at `bb97fa1`, which already carries the
WU-21b real-hardware-verification update — so the one outstanding commit
session 28's own handoff flagged was in fact already made before this
session began. Working tree otherwise clean.

**WU-21c's own first job: re-reading the SDK and this project's own related
code again.** Per this project's own established practice for DeckLink work
(ADR-031/032/046/047/048/049): re-read the real SDK's own `DeckLinkAPI.h`
directly (`IDeckLinkOutput`'s full member list, confirmed unchanged from
ADR-032/033's own account; `IDeckLinkVideoOutputCallback`'s exactly two
methods) — plus, for the first time this project has read it for its own
sake rather than only cited via ADR-032, the real SDK's own `FilePlayback`
sample (`DeckLinkPlaybackDevice.cpp`'s `ScheduledFrameCompleted()`/
`scheduleVideo()`), specifically to confirm how the SDK's own precedent
handles genuinely *changing* content across completions: confirmed directly
that `scheduleVideo()` obtains a fresh frame object from its own media
reader every call, never rescheduling a previously-completed pointer
unchanged — the real precedent this unit's own pool design extends.
Also reread `src/io/decklink_output.hpp`/`.cpp` (`LoopedFramePlayback`, the
preroll idiom and `fillFrameBuffer()` pattern this unit extends from one
buffer to a pool), `src/io/decklink_capture_consumer.hpp`/`.cpp` (WU-21b,
`copyLatestFrame()`, this unit's own upstream source of frames),
`src/io/decklink_input.hpp`/`.cpp` (WU-20b — confirmed this unit touches it
only indirectly, through `CaptureConsumer`), and `docs/architecture.md`
sections 3, 6, 7, 9, 12 plus ADR-010/032/037/039.

**Why this is a materially different mechanism from `LoopedFramePlayback`,
decided this session.** `LoopedFramePlayback` reschedules the same buffer
forever because its own content never changes. This unit's whole job is
that content changes every tick — a buffer currently queued for playback
cannot safely be overwritten, so more than one buffer is needed, cycled so
that only a buffer whose own completion has *just* fired is ever written
to. Full reasoning in `DECISIONS.md` ADR-050.

**Three design questions decided this session, all named in `DECISIONS.md`
ADR-050:** pool size is exactly `round(frameRate / 2)` — ADR-032's own
half-second preroll convention, reused for pool size because that number
*is* the count of buffers concurrently in flight in steady state, not a
coincidence; refill/reschedule is round-robin in lockstep with completion
order, relying on the SDK's own FIFO completion guarantee, needing no
separate in-flight/free bookkeeping; and — ADR-037's own second follow-up,
open since it was first named, and this unit's own concern for the first
time since a real capture clock and a real output clock now genuinely have
to interoperate — no explicit synchronisation of any kind: every refill
uses whatever `CaptureConsumer::copyLatestFrame()` returns right now, so a
capture/process rate mismatch shows as repeated frames
(`framesRepeated()`, this unit's own new counter) or silently superseded
ones, never a growing backlog. This is a narrow, concrete decision about how
two independently clocked producers/consumers meet at one shared buffer,
not a genlock implementation — ADR-010's free-running scope is unchanged.

**WU-21c itself.** `src/io/decklink_live_output.hpp`/`.cpp` (new):
`LiveFramePlayback`, implementing `IDeckLinkVideoOutputCallback` directly
(the same real `IUnknown` refcounting / `ComPtr::adopt()` idiom
`LoopedFramePlayback` already uses), constructed from an
`IDeckLinkOutput`/display mode/width/height and a `const CaptureConsumer&`
(caller-owned, referenced not owned, independent lifecycle — the same
"caller owns, both sides just reference" shape `CaptureSource`/
`CaptureConsumer` already established between themselves). `create()`
confirms `DoesSupportVideoMode`, enables output, and — enforced here
directly rather than left to a caller's own separate test, since a mismatch
would silently misalign every live frame rather than fail one static check
— confirms the SDK's own `RowBytesForPixelFormat()` agrees with
`video::v210::rowBytesMin()`; allocates the pool (`CreateVideoFrame()`
called exactly `poolSize` times, entirely at setup, never on the real-time
completion thread); prerolls by scheduling every pool buffer once via a
shared `refillAndSchedule()` helper (also used by every subsequent
completion); starts scheduled playback. `ScheduledFrameCompleted()` records
stats (reusing `io::PlaybackStats` directly, unmodified — ADR-029's own
"reuse a tested type" precedent, since `completed`/`displayedLate`/
`dropped`/`flushed` mean exactly the same thing here as for
`LoopedFramePlayback`), then cycles to the next pool index and calls
`refillAndSchedule()` again. `fillFrameBuffer()` is duplicated, not shared,
from `decklink_output.cpp`'s own copy — same "small amount of straight-line
SDK calls, duplicating is simpler and safer than a cross-unit refactor"
reasoning ADR-048 already used for `runFrameFile()`/`runFrameBytes()`.
`tests/test_decklink_live_output.cpp` (new): builds the full chain —
`CaptureSource` on the first format-detection-capable input,
`CaptureConsumer` against an identity lattice, `LiveFramePlayback` on the
first playback-capable output — over a bounded 5-second run, checking the
playback mechanics (`completed > 0`, `displayedLate == 0`, `dropped == 0`)
and the capture/consumer accounting invariants WU-21b's own test already
established, unchanged. `CMakeLists.txt`: `decklink_live_output.cpp` added
to `scatter-decklink`'s own source list (no new `target_link_libraries`
needed — `scatter-decklink` already privately links `scatter-core` as of
WU-21b, and `video::v210::rowBytesMin()` is this unit's only `scatter-core`
symbol); a new `test_decklink_live_output` executable registered, linking
both libraries, the same dual-link pattern `test_decklink_capture_consumer`
already uses.

**Verification.** WU-21c could not be built or run in this sandbox at all —
no Blackmagic SDK, no AppleClang/Xcode toolchain — the same shape every
DeckLink-touching unit has had going into its own real-terminal
confirmation since WU-14. Every file was written to the real repository via
the device bridge and re-read back from there to confirm each write landed
byte-for-byte, per `SESSION-PROTOCOL.md`'s own anti-drift rule 8 — the three
new source files were diffed byte-for-byte against this session's own
originals after the round trip (all three matched exactly); `CMakeLists.txt`,
`DECISIONS.md` and `WORK-UNITS.md` were re-staged and grepped for the new
content after their own writes, confirming each landed. Nothing in this
handoff is asserted from a write call returning without error alone.

**Corrections this session:** none. Nothing was claimed and later found
wrong; no gap was caught and silently fixed mid-session either (unlike
WU-21b's own `CMakeLists.txt` gap or WU-18's own most-vexing-parse mistake)
— this session's own single drafting pass for each file was written directly
against the real SDK headers/samples reread first, with no iteration needed.

## Where we are

**Phase 5 (Live capture) continues.** WU-20a/WU-20b/WU-21a/WU-21b remain
`green`. WU-21c is `wip` — reasoned through and delivered to the real
repository this session, not yet built, run, or tagged. `DECISIONS.md` runs
through ADR-050; `CORRECTIONS.md` unchanged through C-016.

**Delivery mechanics:** every file below was written to the real repository
via the device bridge this session and re-read back from there to confirm
the write landed correctly, per `SESSION-PROTOCOL.md`'s own anti-drift rule
8 — the three new source files (`src/io/decklink_live_output.hpp`,
`src/io/decklink_live_output.cpp`, `tests/test_decklink_live_output.cpp`)
byte-diffed clean against this session's own originals; `CMakeLists.txt`,
`DECISIONS.md` and `WORK-UNITS.md` re-staged and grepped to confirm their
own edits landed. Nothing in this handoff is asserted from a write call
returning without error alone.

## Next work unit

Steve's own real terminal, in order:

```
cd ~/src/scatter-dve
git status   # confirm clean before adding — no stray .git/index.lock this time,
             # but worth checking since a prior session found one left by a
             # device-bridge git status call
git add src/io/decklink_live_output.hpp src/io/decklink_live_output.cpp \
        tests/test_decklink_live_output.cpp CMakeLists.txt \
        DECISIONS.md WORK-UNITS.md HANDOFF.md
git commit -m "WU-21c: continuous SDI re-output -- LiveFramePlayback, ADR-050, reasoned through only, unbuilt in the sandbox"

cmake --build build
ctest --test-dir build --output-on-failure
# expect 26/27 -- the sole failure the already-accepted
# test_decklink_device/foundDuplexDevice exception (ADR-035); every other
# test, including the new test_decklink_live_output, expected to pass —
# UNVERIFIED, this sandbox cannot compile or run any of it

./build/test_decklink_live_output
# run directly, with the Monitor 3G -> Recorder 3G SDI loopback connected
# (the same cable WU-20b's/WU-21b's own tests already use -- no new
# physical setup). Report back: completed/displayedLate/dropped/flushed/
# framesRepeated, and the capture/consumer accounting line. Also worth a
# look by eye at the Monitor 3G's own SDI output while this runs -- with an
# identity-map lattice, whatever is patched into the Recorder 3G's input
# should reappear on the Monitor 3G's output, live (this closes a genuine
# physical feedback loop through the same cable -- see the test's own
# header comment).

git tag -a wu-21c-green -m "..."   # once green, or after accepting a known
                                    # exception the same way wu-15a-green/
                                    # wu-17-green/etc. already were
```

Once WU-21c is confirmed green, `DECISIONS.md`/`WORK-UNITS.md`/`HANDOFF.md`
need updating in place with the real result (same "record the real terminal
result, redeliver, re-read to confirm" pattern every prior unit has used) —
that update is next session's own first job unless done at the terminal
directly.

## Open questions

Unchanged in substance from session 28, plus WU-21c's own new ones.
`kCaptureRingCapacity`'s own chosen value of 8 (WU-20a/20b, ADR-046) still
has only the one `framesArrived`/`framesPushed` data point from WU-21b's own
real run, not conclusively diagnosed. Q3 (macOS/Desktop Video version), Q4
(lattice edge damping, C-008(a)) remain open. Q2 remains moot per ADR-037.
ADR-037's own genlock follow-up (#2, capture/output clock-domain drift) is
now *addressed* for WU-21c's own narrow case (no explicit sync, accept
repeat/skip — ADR-050) but not *measured*: `framesRepeated()`'s own real
value against real hardware is unknown until Steve's own terminal run.
Follow-up #1 (`test_decklink_device.cpp`'s full-duplex check) remains the
known, accepted ADR-035 exception. WU-20b's own `stopFromCallback()` safety
question (ADR-047) is unchanged. New this session: whether the "always
latest, no backlog" refill policy's frame-repeat/frame-skip behaviour
actually looks acceptable by eye in practice — not measured, not decided,
named as a possible future timestamp-alignment refinement in ADR-050 if it
does not.

## Blocked / red

Nothing red. WU-21c is delivered but genuinely unverified — reasoned
through only, same as WU-14/WU-15a/WU-20b/WU-21b before their own
real-terminal confirmations.

## Environment check

Unchanged from sessions 18-28 (ADR-037/039): **UltraStudio Monitor 3G**
remains the active, confirmed output target — now also this unit's own
live re-output target. **UltraStudio Recorder 3G** remains the confirmed
input target. **UltraStudio 4K Mini** remains on hold pending a PSU
replacement. No new physical setup for WU-21c beyond the existing Monitor
3G -> Recorder 3G SDI loopback cable, already in place since WU-20b/WU-21b —
running WU-21c's own test with that cable connected closes an actual live
feedback loop through it (see the test's own header comment).

## Append to DECISIONS.md

ADR-050 (the real `IDeckLinkOutput`/`IDeckLinkVideoOutputCallback` re-read;
the SDK's own `FilePlayback` sample reread specifically for its own
changing-content-per-completion precedent; why this is materially different
from `LoopedFramePlayback`; the pool-sizing, round-robin-refill and
no-explicit-sync genlock/clock-domain decisions; the row-bytes consistency
check upgraded to a hard runtime precondition; `fillFrameBuffer()`
duplicated rather than shared; `PlaybackStats` reused unmodified,
`framesRepeated()` kept separate; the "not decided here" list for WU-21d/
real measurement of `framesRepeated()`/possible timestamp alignment) was
appended in full this session; see `DECISIONS.md`. ADR-050 does not reopen
`docs/architecture.md`, ADR-010, ADR-024, ADR-026, ADR-029, ADR-031, ADR-032,
ADR-037, ADR-039, ADR-046, ADR-047, ADR-048 or ADR-049 — see its own closing
paragraph.

## Append to CORRECTIONS.md

None this session — nothing was claimed and found wrong.

---

## What to run at your terminal

See "Next work unit" above — the full commit/build/test/tag sequence,
including the `test_decklink_live_output` run against the real Monitor 3G ->
Recorder 3G loopback and a by-eye look at the Monitor 3G's own SDI output
while it runs. Nothing else outstanding from this session.
