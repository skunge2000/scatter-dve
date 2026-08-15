# Handoff
Overwritten at the end of every session. This is the first thing to read.

---

**Session:** 29
**Tag:** `wu-21b-green` confirmed present going into this session — verified
directly against the real repository's own `git tag`/`git log` (not trusted
from session 28's own handoff account). `HEAD` sat at `bb97fa1` ("WU-21b:
record real-hardware verification and wu-21b-green tag; note Weston 3-field
preference for future WU-23"), which already carries the WU-21b
real-hardware-verification update to `WORK-UNITS.md` — the commit session
28's own handoff flagged as the one outstanding loose end was in fact already
made before this session began. **WU-21c was scoped, built, delivered, and
then genuinely verified against real hardware, all within this same
session** — a first for this project: every prior DeckLink-touching unit's
own real-terminal confirmation has happened in a *later* session than the one
that reasoned it through and wrote it. `WORK-UNITS.md`'s own WU-21c line is
now `green`. `DECISIONS.md` runs through ADR-050 plus this same-session
verification addendum. `CORRECTIONS.md` still runs through C-016 — no new
entry (nothing documented was claimed and found wrong; see below).
**`wu-21c-green` itself has not been tagged yet — that is still Steve's own
action, exact command below.**
**Phase:** 5 (Live capture) continues. WU-21c (continuous SDI re-output —
`LiveFramePlayback`, `src/io/decklink_live_output.hpp`/`.cpp`) closes
`architecture.md` 10's own Phase 5 "done when" line ("live SDI in, warped,
SDI out") end to end for the first time, confirmed for real this session with
a live source patched directly into the Recorder 3G's own SDI input (not the
Monitor 3G → Recorder 3G self-loop WU-20b's/WU-21b's own tests use) and the
Monitor 3G's own HDMI-mirrored output watched live.

## This session in full

**Reading, before anything else.** Per `SESSION-PROTOCOL.md`'s own reading
table, in full: `HANDOFF.md` (session 28's), `INVARIANTS.md`, `DECISIONS.md`
in full through ADR-049 including its own same-session real-hardware
verification addendum, `CORRECTIONS.md` through C-016, `WORK-UNITS.md`.

**Folder access and tag/commit-state confirmation, before anything else.**
Requested and received device-bridge access to both `~/src/scatter-dve` and
`~/src/Blackmagic DeckLink SDK 16.0`. Ran the real repository's own `git
tag`/`git log --oneline -8`/`git status --short` directly via the device
bridge rather than trusting `HANDOFF.md`'s own account: `wu-21b-green`
present, `HEAD` at `bb97fa1`, working tree otherwise clean — the one
outstanding commit session 28's own handoff flagged was in fact already made.

**WU-21c's own first job: re-reading the SDK and this project's own related
code again.** Re-read the real SDK's own `DeckLinkAPI.h` directly
(`IDeckLinkOutput`'s full member list confirmed unchanged from ADR-032/033's
own account; `IDeckLinkVideoOutputCallback`'s exactly two methods) and the
real SDK's own `FilePlayback` sample (`DeckLinkPlaybackDevice.cpp`'s
`scheduleVideo()`) specifically for how it handles genuinely *changing*
content across completions — confirmed it obtains a fresh frame object every
call, never rescheduling a previously-completed pointer unchanged, the real
precedent this unit's own pool design extends. Also reread
`src/io/decklink_output.hpp`/`.cpp` (`LoopedFramePlayback`),
`src/io/decklink_capture_consumer.hpp`/`.cpp` (WU-21b),
`src/io/decklink_input.hpp`/`.cpp` (WU-20b), and `docs/architecture.md`
sections 3, 6, 7, 9, 12 plus ADR-010/032/037/039.

**Why this is a materially different mechanism from `LoopedFramePlayback`,
and the three design questions decided this session** — pool size exactly
`round(frameRate / 2)`, round-robin refill relying on the SDK's own FIFO
completion guarantee, and no-explicit-sync between the capture and output
clock domains (ADR-037's own long-open follow-up, addressed here for the
first time) — full reasoning in `DECISIONS.md` ADR-050, unchanged from this
session's own earlier drafting.

**WU-21c itself.** `src/io/decklink_live_output.hpp`/`.cpp` (new):
`LiveFramePlayback`, a fixed pool of `IDeckLinkMutableVideoFrame` buffers,
round-robin refilled from `CaptureConsumer::copyLatestFrame()` and
rescheduled exactly once per completion. `tests/test_decklink_live_output.cpp`
(new): builds the full chain — `CaptureSource`, `CaptureConsumer` against an
identity lattice, `LiveFramePlayback` — over a bounded 5-second run.
`CMakeLists.txt`: `decklink_live_output.cpp` added to `scatter-decklink`'s
source list, new `test_decklink_live_output` executable. Full detail
unchanged from this session's own earlier drafting; see `DECISIONS.md`
ADR-050.

**Real-hardware verification, same session, run at Steve's own real
terminal after this unit was first written and delivered.** `cmake --build
build` clean. `ctest --test-dir build --output-on-failure`: 25 of 26 tests
passing, the sole failure the already-accepted
`test_decklink_device`/`foundDuplexDevice` exception (ADR-035), unrelated;
the new `test_decklink_live_output` itself passing, all 10 automated checks
green. `./build/test_decklink_live_output` run directly, with a live source
patched into the Recorder 3G's own SDI input and the Monitor 3G's own
HDMI-mirrored output watched live, over its own bounded 5-second window:

```
completed=124 displayedLate=0 dropped=0 flushed=0 framesRepeated=18
framesArrived=124 framesPushed=97 | framesPopped=89 framesProcessed=89 framesFailed=0
```

`framesProcessed(89) + framesFailed(0) == framesPopped(89)` and
`framesPopped(89) <= framesPushed(97)` both hold exactly, as this unit's own
`Accept:` requires; `completed(124) > 0`, `displayedLate == 0`,
`dropped == 0`, all as required — every formal `Accept:` criterion in
`WORK-UNITS.md` passed.

`framesPushed(97) - framesPopped(89) == 8`, exactly `kCaptureRingCapacity` —
the same exact relationship WU-21b's own real run showed
(`89 - 81 == 8`), a second real data point consistent with WU-21b's own
reading (ADR-049's addendum): the gap reads as "up to one ring's worth left
unconsumed when the bounded run stops," not a growing backlog. The
`framesArrived`/`framesPushed` gap itself (124 vs. 97, ~22%) is close in
magnitude to WU-21b's own ~28% — reinforcing, not newly resolving,
`kCaptureRingCapacity=8`'s own still-open question.

`framesRepeated=18` of `completed=124` (~15%) is this unit's own first real
data point for the no-explicit-sync genlock/clock-domain decision: roughly
one output tick in seven found nothing fresher than what was already
scheduled and repeated it rather than stall — visible by eye as a mild
jerk/stutter on the Monitor 3G's own HDMI-mirrored output during this run,
consistent with `CaptureConsumer`'s own measured throughput (89 processed
frames over ~5 seconds, roughly 18fps) running below the output's own fixed
25fps schedule. Exactly the accepted consequence ADR-050's own no-
explicit-sync paragraph named in advance, now measured rather than merely
anticipated.

**A second, previously unanticipated finding, caught by eye on this same run
and confirmed by re-reading this unit's own `refillAndSchedule()`:** the
re-output picture on the Monitor 3G's own output showed a few seconds of
solid green before genuine captured content appeared, at the very start of
playback. Not a defect in the loop itself — `startWith()`'s own preroll loop
schedules every pool buffer once, via `refillAndSchedule()`, before
`StartScheduledPlayback` runs; at that instant `CaptureConsumer` has not yet
produced its own first output, so `copyLatestFrame()` returns false for each
of those initial calls, and per this unit's own "nothing fresher — leave the
buffer's existing content unchanged" policy, each pool buffer's content is
left exactly as `CreateVideoFrame()` first allocated it — effectively
zero-filled `v210`, which decodes as a strongly saturated green once
converted for display, a well-known artifact of displaying unwritten YUV.
Green clears once enough completions have cycled through to refill every
pool slot with real content, gated on `CaptureSource`/`CaptureConsumer`
producing their own first output — matching the "few seconds" observed.
ADR-050's own genlock paragraph reasoned about the *steady-state* "nothing
fresher since last refill" case; it did not anticipate the *cold-start*
"never filled at all" case, which is the one actually hit here. Named, not
fixed this session — a candidate fix (fill each pool buffer with black
immediately after `CreateVideoFrame()`, before the preroll loop schedules
any of them) is left for whoever next touches this file. See `DECISIONS.md`
ADR-050's own addendum for the full writeup.

**Verification.** Every file was written to the real repository via the
device bridge and re-read back from there to confirm each write landed
byte-for-byte (three new source files diffed byte-for-byte; `CMakeLists.txt`,
`DECISIONS.md`, `WORK-UNITS.md` re-staged and grepped), per
`SESSION-PROTOCOL.md`'s own anti-drift rule 8 — then, unlike every prior
DeckLink-touching unit, genuinely built, tested, and run at Steve's own real
terminal within this same session rather than left for the next one.

**Corrections this session:** none. Nothing documented was claimed and later
found wrong. (The cold-start-green finding above is new evidence added to
ADR-050 by addendum, in the same way WU-21b's own ring-capacity gap was
added to ADR-049 by addendum — not a correction of anything previously
claimed, since ADR-050's original text never claimed the cold-start case had
been considered.)

## Where we are

**Phase 5 (Live capture) continues.** WU-20a/WU-20b/WU-21a/WU-21b/WU-21c all
`green` in substance — WU-21c's own `wu-21c-green` git tag itself is the one
remaining step, Steve's own action, command below. `DECISIONS.md` runs
through ADR-050 plus its own same-session verification addendum;
`CORRECTIONS.md` unchanged through C-016.

## Next work unit

Steve's own real terminal — only the commit and tag remain; build/test/run
are already done:

```
cd ~/src/scatter-dve
git status
git add src/io/decklink_live_output.hpp src/io/decklink_live_output.cpp \
        tests/test_decklink_live_output.cpp CMakeLists.txt \
        DECISIONS.md WORK-UNITS.md HANDOFF.md
git commit -m "WU-21c: continuous SDI re-output -- LiveFramePlayback, ADR-050, real-hardware verified same session"

git tag -a wu-21c-green -m "completed=124 displayedLate=0 dropped=0 framesRepeated=18 | framesArrived=124 framesPushed=97 framesPopped=89 framesProcessed=89 framesFailed=0"
```

After that, WU-21d (candidates named in ADR-050 and above: the cold-start
black-fill fix; the literal one-hour endurance run; real measurement/possible
timestamp alignment if the ~15% repeat rate proves visually unacceptable
over a longer run) is not yet scoped — next session's own first job if
picked up.

## Open questions

`kCaptureRingCapacity`'s own chosen value of 8 (WU-20a/20b, ADR-046) now has
two consistent real-hardware data points (WU-21b: 123 arrived/89 pushed;
WU-21c: 124 arrived/97 pushed, both showing `pushed - popped == 8` exactly)
— still not conclusively diagnosed, reinforced rather than resolved. Q3
(macOS/Desktop Video version), Q4 (lattice edge damping, C-008(a)) remain
open. Q2 remains moot per ADR-037. ADR-037's own genlock follow-up (#2) is
now both *addressed* (ADR-050's no-explicit-sync decision) and *measured*
(`framesRepeated=18` of `completed=124`, ~15%, over this one 5-second run) —
whether that rate is acceptable over a longer, real-use run is still open.
Follow-up #1 (`test_decklink_device.cpp`'s full-duplex check) remains the
known, accepted ADR-035 exception. WU-20b's own `stopFromCallback()` safety
question (ADR-047) is unchanged. **New this session:** the cold-start green
artifact (see above) — named, not fixed; candidate fix identified
(initialize pool buffers to black before preroll) but not implemented.

## Blocked / red

Nothing red. WU-21c is verified against real hardware this session — every
formal `Accept:` criterion passed. Only the `git tag` itself remains,
Steve's own action.

## Environment check

Unchanged from sessions 18-28 (ADR-037/039): **UltraStudio Monitor 3G**
remains the active output target — this session additionally confirmed its
SDI and HDMI outputs carry the same mirrored signal, which is how the by-eye
verification above was actually done (independent live source into Recorder
3G's SDI-in; Monitor 3G's HDMI-out to a separate display — not the SDI
self-loop WU-20b's/WU-21b's own tests use). **UltraStudio Recorder 3G**
remains the confirmed input target. **UltraStudio 4K Mini** remains on hold
pending a PSU replacement.

## Append to DECISIONS.md

ADR-050 (full design, delivered earlier this session) plus a same-session
real-hardware verification addendum recording: the `ctest`/
`test_decklink_live_output` results and numbers above; the second
`kCaptureRingCapacity` data point; `framesRepeated`'s own first measured
value; and the cold-start green finding with its candidate fix. The
addendum does not reopen anything named in ADR-050's own "Does not reopen"
paragraph — it records a real-terminal result and one new finding, nothing
more.

## Append to CORRECTIONS.md

None this session — nothing documented was claimed and found wrong.
