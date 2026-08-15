# Handoff
Overwritten at the end of every session. This is the first thing to read.

---

**Session:** 28
**Tag:** `wu-21a-green` confirmed to actually exist going into this session
— verified directly against the real repository's own `git tag`/`git log`
(not trusted from session 27's own handoff account). `HEAD` sat at
`0d2b004` ("WU-21a: mark green, tagged wu-21a-green (session 27 close)"),
which already included `WORK-UNITS.md` and `HANDOFF.md` — the pair session
27's own `HANDOFF.md` flagged as possibly still uncommitted going into this
session was in fact already committed; that uncertainty is resolved, not
merely repeated. **WU-21b was built this session, then genuinely built,
tested, run against the real Monitor 3G → Recorder 3G SDI loopback, and
tagged, all at Steve's own real terminal within this same session:
`wu-21b-green` confirmed present on `git tag`.**
**Phase:** 5 (Live capture) continues. WU-21b (DeckLink capture-side pixel
read — `CaptureConsumer` draining WU-20b's own `CaptureFrameRing`,
`StartAccess`/`GetBytes`/`EndAccess` against a retained
`IDeckLinkVideoInputFrame`, feeding the mapped bytes into WU-21a's own
`runFrameBytes()`) was scoped this session — real `Files:`/`Accept:` text,
done only after the reading below — built, delivered to the real repository,
and then, within this same session, built (`cmake --build build`, clean),
tested (`ctest`, 24/25 passing, only the already-accepted
`test_decklink_device` exception failing), run directly against the real
loopback (`framesArrived=123 framesPushed=89 | framesPopped=81
framesProcessed=81 framesFailed=0`, all 7 checks passing), and tagged
(`wu-21b-green`) by Steve at his own terminal. `WORK-UNITS.md`'s own WU-21b
line is now `green` (`wu-21b-green`). `DECISIONS.md` now runs through
ADR-049 plus a same-session real-hardware verification addendum;
`CORRECTIONS.md` still runs through C-016 — no new entry this session (see
below for why). WU-21c (continuous SDI re-output) remains named but not yet
sketched in detail.

## This session in full

**Reading, before anything else.** Per `SESSION-PROTOCOL.md`'s own reading
table, in full: `HANDOFF.md` (session 27's), `INVARIANTS.md`, `DECISIONS.md`
in full through ADR-048 (not skimmed), `CORRECTIONS.md` through C-016,
`WORK-UNITS.md` — including C-016's own lesson and the resulting
Session-close/anti-drift rule 8, explicitly treated as binding on this
session too.

**Folder access and tag/commit-state confirmation, before anything else.**
Requested and received device-bridge access to both `~/src/scatter-dve` (the
repo) and `~/src/Blackmagic DeckLink SDK 16.0` (the SDK); the SDK path
resolved directly, no fallback needed. Note for next session: the mounted
paths as actually reachable from this session's own tools are
`~/mnt/scatter-dve` and `~/mnt/Blackmagic DeckLink SDK 16.0` when using
`device_bash` specifically (that tool's own `$HOME` differs from the
`device_stage_files`/`device_commit_files` tools, which accept
`~/src/scatter-dve` directly) — cost some early back-and-forth this session,
worth remembering next time rather than rediscovering. Then, per the
explicit instruction not to trust `HANDOFF.md`'s own account of tag/commit
state: ran the real repository's own `git tag`/`git log --oneline -8`/`git
status --short` directly via the device bridge. Result: `wu-21a-green`
present, alongside every earlier tag through it; `HEAD` at `0d2b004`, which
already carries `WORK-UNITS.md`/`HANDOFF.md`'s own WU-21a-green update — so
the "possibly still uncommitted pair" session 27's own handoff flagged was
in fact already committed before this session began. Working tree otherwise
clean at that point (before this session's own new files were written).
Also found: a stale, empty `.git/index.lock` sitting in the working tree,
left over from an earlier full-form `git status` run via the device bridge
earlier in this same session — `device_bash` cannot unlink files on a
mounted folder, so git's own attempted write-then-remove of an
index-refresh lock left the empty file behind. Read-only git commands
(`status`, `log`, `tag`, `show`) all still worked fine with it present; it
was removed at Steve's own terminal before his own commit (see below), and
did not in fact block anything.

**WU-21b's own first job: re-reading the SDK and this project's own related
code again.** Per this project's own established practice for DeckLink work
(ADR-031/032/046/047/048): re-read the real SDK's own `DeckLinkAPI.h`
directly (`IDeckLinkVideoBuffer`, `IDeckLinkVideoFrame`,
`IDeckLinkVideoInputFrame`, `StartAccess`/`EndAccess`, `GetBytes()`,
`GetRowBytes()`) — confirmed unchanged from ADR-032/048's own account, the
same interface obtained via `QueryInterface` both directions. Also reread
`src/io/decklink_output.hpp`/`.cpp` (`LoopedFramePlayback::
fillFrameBuffer()`, the write-direction pattern this unit mirrors),
`src/io/decklink_input.hpp`/`.cpp` (WU-20b, the `CaptureSource`/
`CaptureFrameRing` this unit drains), `src/core/ring_buffer.hpp` (WU-20a,
confirming `tryPop()` has no blocking form), `src/core/resolve.hpp`'s
`runFrameBytes()` declaration and `PipelineParams` (WU-21a), and
`docs/architecture.md` sections 3, 6, 7, 9, 12 plus ADR-037/039. The three
real capture samples (`CaptureStills`, `InputLoopThrough`, `CapturePreview`)
were re-consulted via the extensive prior cross-referencing already recorded
in ADR-046/047/048 rather than reread line by line a second time this
session — no new finding beyond what those entries already recorded from
them.

**The four questions ADR-048 left open, decided.** Consumer-thread
ownership/lifetime is fully independent of `CaptureSource`'s own (the two
objects share only the ring). Extracted bytes are handed to
`runFrameBytes()` synchronously, still inside the `StartAccess`/`EndAccess`
bracket — no pool buffer. The ring's own drain policy while empty is a short
(1ms) poll sleep, since `RingBuffer` has no blocking pop and this consumer
thread, unlike the driver's own callback thread, is free to wait. A
captured frame's own reported `GetWidth()`/`GetHeight()`/`GetRowBytes()` are
trusted directly, per frame, never checked against the display mode
`CaptureSource::create()` was given; destination geometry is instead fixed
once, at construction, via `PipelineParams::destWidth`/`destHeight`. Full
reasoning in `DECISIONS.md` ADR-049 and, duplicated in substance, in
`src/io/decklink_capture_consumer.hpp`'s own header comment.

**WU-21b itself.** `src/io/decklink_capture_consumer.hpp`/`.cpp` (new):
`CaptureConsumerStats` (`framesPopped`/`framesProcessed`/`framesFailed`,
atomic) and `CaptureConsumer` — constructed from a caller-owned
`CaptureFrameRing&`, a `Lattice` and a `PipelineParams` (both copied in);
`start()`/`stop()` mirror `CaptureSource`/`LoopedFramePlayback`'s own
`compare_exchange_strong` idiom; `run()` polls the ring on a private thread;
`processOne()` checks the pixel format defensively, reads width/height/row
bytes off the frame with explicit casts (SDK returns `long`, this project's
`-Wconversion -Wsign-conversion -Werror` needs the cast), obtains
`IDeckLinkVideoBuffer` via `QueryInterface`, brackets `GetBytes()` with
`StartAccess`/`EndAccess`, calls `scatter::runFrameBytes()` inside that
bracket, and stores the result into `m_latestFrame` under a mutex only on
full success; `copyLatestFrame()` retrieves it. `tests/
test_decklink_capture_consumer.cpp` (new): mirrors `test_decklink_input.cpp`'s
own style — device enumeration, `CaptureSource::create()` at `bmdModePAL`, a
`CaptureConsumer` built against a locally-duplicated identity lattice, a
bounded 5-second run, then two accounting-invariant checks that hold
unconditionally, and a NOTE-not-fail path if zero frames were processed
(nothing physically connected). `CMakeLists.txt`: `decklink_capture_consumer
.cpp` added to `scatter-decklink`'s own source list; new
`target_link_libraries(scatter-decklink PRIVATE scatter-core)` — the first
time that library has needed one, since this is the first
`scatter-decklink` file calling into `scatter-core`; a new
`test_decklink_capture_consumer` executable registered, linking both
libraries, the same dual-link pattern `test_decklink_output` already uses.

**One gap caught and fixed within this same session, before any claim was
made** (not logged in `CORRECTIONS.md` — see below): this session's own
first `CMakeLists.txt` edit added the new library source and the new
`target_link_libraries` line, but omitted the new test's own
`add_executable`/`target_link_libraries`/`target_include_directories`/
`add_test` block entirely. Found by rereading the file straight back from
the real repository after the first delivery — the anti-drift rule 8
re-read step this project already requires — and comparing it against the
neighbouring `test_decklink_input` block; not by any build, since this
sandbox cannot build DeckLink code at all. Fixed and re-delivered before
this handoff was first written, well before Steve's own build confirmed it
compiled correctly.

**Real-hardware verification, later in this same session, at Steve's own
real terminal.** After this handoff's own first draft described WU-21b as
"reasoned through only... UNVERIFIED," Steve committed, built, tested, ran,
and tagged it, all in the same session: `git commit` (`165da8a`, 7 files, 999
insertions), `cmake --build build` clean (SDK found, `scatter-decklink` and
every DeckLink-gated target configured), `ctest --test-dir build
--output-on-failure` — 24 of 25 tests passing, the sole failure the
already-accepted `test_decklink_device`/`foundDuplexDevice` exception
(ADR-035). `./build/test_decklink_capture_consumer` run directly, with the
Monitor 3G → Recorder 3G SDI loopback connected: `framesArrived=123
framesPushed=89 | framesPopped=81 framesProcessed=81 framesFailed=0` over
its own bounded 5-second window, all 7 automated checks passing including
the `copyLatestFrame()` size check — the first genuine, real-signal
confirmation anywhere in this project that a captured frame's own pixel
bytes can be read through `IDeckLinkVideoBuffer` and fed into
`runFrameBytes()` end to end. `wu-21b-green` tagged and confirmed present on
`git tag`. This handoff, `DECISIONS.md`, and `WORK-UNITS.md` were then
updated in place to record the real result (see `DECISIONS.md` ADR-049's
own addendum) and redelivered/re-read to confirm, same as every other file
this session.

One real, not-yet-fully-diagnosed data point surfaced by these numbers:
`framesArrived(123)` vs. `framesPushed(89)` — roughly 28% of arriving
callbacks never reached the ring. `framesPushed(89) - framesPopped(81) ==
8`, exactly `kCaptureRingCapacity`, which reads as the ordinary "up to one
ring's worth left over when the bounded run stops," not a growing backlog —
so the gap looks more like ring-full drops concentrated somewhere during the
run (plausibly a startup burst, before the consumer thread had spun up) than
sustained backpressure, but this session has no per-frame timing
instrumentation to actually distinguish the two. First real measurement this
project has had for the `kCaptureRingCapacity=8` question named since
ADR-046 — not resolved, named for whoever next revisits it.

**Verification.** WU-21a remains genuinely verified in the Linux cloud
sandbox (unchanged from last session). WU-21b could not be built or run in
this sandbox at all — no Blackmagic SDK, no AppleClang/Xcode toolchain — but
unlike WU-14/WU-15a/WU-20b's own sessions, this session did not end there:
Steve's own real terminal genuinely built, tested, and ran it against real
hardware within the same session, and the numbers above are real, not
reasoned through. Every file was written to the real repository via the
device bridge and re-read back from there to confirm each write landed
byte-for-byte, per `SESSION-PROTOCOL.md`'s own anti-drift rule 8 — twice for
`CMakeLists.txt` (the build-target gap), and a second round for
`HANDOFF.md`/`WORK-UNITS.md`/`DECISIONS.md` themselves once the real-terminal
results came in. Nothing in this handoff is asserted from a write call
returning without error alone.

**Corrections this session:** none. The `CMakeLists.txt` gap above was
caught and fixed within the same session, before any claim of "delivered"
was made to Steve — matching the C-015/ADR-043 precedent that self-caught
iteration during the anti-drift re-read step is not a `CORRECTIONS.md`-worthy
event; a correction entry is for something claimed working and then found
wrong. This session's own first close-out did tell Steve WU-21b was
"entirely unverified" shortly before he in fact verified it minutes later —
not a wrong claim at the time it was made (it was accurate then), so not a
`CORRECTIONS.md` event either; the record was updated in place as soon as
the real result was known, per the same rule 8 discipline.

## Where we are

**Phase 5 (Live capture) continues.** WU-21a remains `green`
(`wu-21a-green`). WU-21b is now also `green` (`wu-21b-green`), genuinely
built, tested, and run against real hardware at Steve's own real terminal
within this session, confirmed on `git tag`. `WORK-UNITS.md`'s WU-21b line
reflects this. `DECISIONS.md` runs through ADR-049 plus its own real-hardware
verification addendum; `CORRECTIONS.md` unchanged through C-016.

**Delivery mechanics:** all files below were written to the real repository
via the device bridge this session and re-read back from there to confirm
the write landed correctly, per `SESSION-PROTOCOL.md`'s own anti-drift rule
8 — `CMakeLists.txt` twice (the build-target gap), and `HANDOFF.md`/
`WORK-UNITS.md`/`DECISIONS.md` twice each (once as the original session
close, once again once Steve's own real-terminal results came in). Nothing
in this handoff is asserted from a write call returning without error alone.

## Next work unit

**WU-21c — continuous SDI re-output**, closing the actual "full loop through
at 576i25" loop end to end (scheduling a live-produced frame stream onto
`IDeckLinkOutput`, a materially different mechanism from
`LoopedFramePlayback`'s own "loop one static frame forever" design). Not yet
sketched in detail — that scoping is that session's own first job, same
discipline as every unit in this project. `CaptureConsumer::copyLatestFrame()`
(WU-21b, this session) is the mechanism it will pull produced bytes from.

Once WU-21 (all of a/b/c) closes, Phase 5's own remaining unit is WU-22
(diagnostic coverage view), `todo`, unscoped.

## Open questions

Unchanged in substance from session 27, plus one new item this session named
and, unusually, partially measured: `kCaptureRingCapacity`'s own chosen
value of 8 (WU-20a/20b, ADR-046) now has a first real data point
(`framesArrived=123` vs. `framesPushed=89`, see above) but is not
conclusively diagnosed — still open for whoever next revisits it. Q3
(macOS/Desktop Video version), Q4 (lattice edge damping, C-008(a)) remain
open. Q2 remains moot per ADR-037. ADR-037's own genlock follow-up (#2,
capture/output clock-domain drift) remains open — `CaptureConsumer`
processes one already-arrived frame's worth of bytes at a time and has no
clock-domain behaviour of its own; squarely WU-21c's own concern once a real
capture/output pair actually runs concurrently. Follow-up #1
(`test_decklink_device.cpp`'s full-duplex check) remains the known, accepted
ADR-035 exception. WU-20b's own `stopFromCallback()` safety question
(ADR-047) is unchanged this session.

## Blocked / red

Nothing red. WU-21b is fully closed — genuinely built, tested, and run
against real hardware at Steve's own real terminal within this session, and
tagged `wu-21b-green`.

## Environment check

Unchanged from sessions 18-27 (ADR-037/039): **UltraStudio Monitor 3G**
remains the active, confirmed output target. **UltraStudio Recorder 3G**
remains the confirmed input target, exercised for real by WU-20b and now
WU-21b. **UltraStudio 4K Mini** remains on hold pending a PSU replacement.
The Monitor 3G → Recorder 3G SDI loopback cable was connected and exercised
for real this session (WU-21b's own `Accept:`); no new physical setup for
WU-21c beyond what's already in place.

## Append to DECISIONS.md

ADR-049 (tag/commit-state confirmation, including that the WU-21a
`WORK-UNITS.md`/`HANDOFF.md` pair was already committed going in; the
`.git/index.lock` finding; the SDK/code re-read findings; the four ADR-048
open questions resolved for WU-21b; `CaptureConsumer`'s full design; the
test file; the `CMakeLists.txt` changes including the one gap caught and
fixed within-session; the "not decided here" list for WU-21c/genlock/ring
capacity) plus a same-session real-hardware verification addendum (build,
test, and real-loopback run results; the `wu-21b-green` tag; the
`framesArrived`/`framesPushed` gap as a new, unresolved data point for
`kCaptureRingCapacity`) were both appended in full this session; see
`DECISIONS.md`. ADR-049 does not reopen `docs/architecture.md`, ADR-026,
ADR-031, ADR-032, ADR-037, ADR-039, ADR-046, ADR-047 or ADR-048 — see its own
closing paragraph; the addendum reopens nothing further.

## Append to CORRECTIONS.md

None this session. The one bug found while building WU-21b (a `CMakeLists.txt`
edit missing the new test's own build-target block) was caught and fixed
within the same session, before it was ever claimed delivered — see
`DECISIONS.md` ADR-049's own account, and the C-015/ADR-043 precedent this
follows. This session's own first close-out described WU-21b as
"entirely unverified," accurately, moments before Steve verified it for
real — an accurate claim overtaken by events, not a wrong one, so not a
`CORRECTIONS.md` event either.

---

## What to run at your terminal

Already done this session, in full, at your own terminal:

```
cd ~/src/scatter-dve
rm -f .git/index.lock   # stale lock from an earlier device-bridge git status call this session
git add src/io/decklink_capture_consumer.hpp src/io/decklink_capture_consumer.cpp \
        tests/test_decklink_capture_consumer.cpp CMakeLists.txt \
        DECISIONS.md WORK-UNITS.md HANDOFF.md
git commit -m "WU-21b: CaptureConsumer, DeckLink capture-side pixel read -- ADR-049, reasoned through only, unbuilt in the sandbox"
# -> 165da8a, 7 files changed, 999 insertions(+), 255 deletions(-)

cmake --build build
ctest --test-dir build --output-on-failure
# -> 24/25 passed; test_decklink_device/foundDuplexDevice the only failure
#    (ADR-035's own already-accepted exception); test_decklink_capture_consumer
#    passing

./build/test_decklink_capture_consumer
# -> framesArrived=123 framesPushed=89 | framesPopped=81 framesProcessed=81
#    framesFailed=0 -- PASS (7 checks), genuine signal through the Monitor 3G
#    -> Recorder 3G loopback

git tag
# -> wu-21b-green present, alongside every earlier tag through wu-21a-green
```

Nothing outstanding from this session. `DECISIONS.md`, `WORK-UNITS.md`, and
this `HANDOFF.md` were updated a second time, after the real-terminal results
above came in, to replace the "reasoned through only, unverified" account
this session's own first draft gave with the real one — written back to the
real repository and re-read to confirm before this session closes. A short
commit for that second update is the only loose end:

```
git add DECISIONS.md WORK-UNITS.md HANDOFF.md
git commit -m "WU-21b: record real-hardware verification and wu-21b-green tag (session 28 close)"
```
