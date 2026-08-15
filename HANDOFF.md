# Handoff
Overwritten at the end of every session. This is the first thing to read.

---

**Session:** 28
**Tag:** `wu-21a-green` confirmed to actually exist this session — verified
directly against the real repository's own `git tag`/`git log` (not trusted
from session 27's own handoff account). `HEAD` sits at `0d2b004` ("WU-21a:
mark green, tagged wu-21a-green (session 27 close)"), and that commit
already includes `WORK-UNITS.md` and `HANDOFF.md` — the pair session 27's
own `HANDOFF.md` flagged as possibly still uncommitted going into this
session was in fact already committed; that uncertainty is resolved, not
merely repeated. No new tag this session — the assistant does not tag;
that remains Steve's own action at his own terminal, once WU-21b is built
and tested there.
**Phase:** 5 (Live capture) continues. WU-21b (DeckLink capture-side pixel
read — `CaptureConsumer` draining WU-20b's own `CaptureFrameRing`,
`StartAccess`/`GetBytes`/`EndAccess` against a retained
`IDeckLinkVideoInputFrame`, feeding the mapped bytes into WU-21a's own
`runFrameBytes()`) was scoped this session — real `Files:`/`Accept:` text,
done only after the reading below — and built: `src/io/
decklink_capture_consumer.hpp`/`.cpp` (new), `tests/
test_decklink_capture_consumer.cpp` (new), `CMakeLists.txt` (extended).
Reasoned through only, same shape as WU-14/WU-15a/WU-20b/WU-21a's own
DeckLink-touching predecessors before it — this sandbox has neither the
Blackmagic SDK nor an AppleClang/Xcode toolchain, so none of it is built or
run here; UNVERIFIED until Steve's own real terminal builds, tests, and runs
it against the Monitor 3G → Recorder 3G loopback. `WORK-UNITS.md`'s own
WU-21b line moves from `todo` to `wip` (not `green` — the assistant does not
run `close.sh` or tag). `DECISIONS.md` now runs through ADR-049;
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
(`status`, `log`, `tag`, `show`) all still worked fine with it present, but
it should be removed before your own next `git commit` or `close.sh`, in
case those are less tolerant of it — see "What to run at your terminal"
below.

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
this handoff was written.

**Verification: none possible in this sandbox, by design.** Unlike WU-21a
(portable, fully built and tested here), WU-21b touches only DeckLink-only
surface — this Linux cloud sandbox has neither the Blackmagic SDK nor an
AppleClang/Xcode toolchain, so nothing here has been compiled or run. Every
file was, however, written to the real repository via the device bridge and
re-read back from there — twice, for `CMakeLists.txt`, once the gap above
was found — to confirm each write landed byte-for-byte, per
`SESSION-PROTOCOL.md`'s own anti-drift rule 8 (C-016's own lesson, applied
here exactly as it was meant to be): nothing in this handoff is asserted
from a write call returning without error alone.

**Corrections this session:** none. The `CMakeLists.txt` gap above was
caught and fixed within the same session, before any claim of "delivered"
was made to Steve — matching the C-015/ADR-043 precedent that self-caught
iteration during the anti-drift re-read step is not a `CORRECTIONS.md`-worthy
event; a correction entry is for something claimed working and then found
wrong. Nothing here was ever claimed working before it was actually
delivered and confirmed.

## Where we are

**Phase 5 (Live capture) continues.** WU-21a remains `green`
(`wu-21a-green`), confirmed directly against the real repository this
session. WU-21b (`CaptureConsumer`) is implemented and delivered to the real
repository, re-read to confirm, but entirely UNVERIFIED — reasoned through
only, no build or run anywhere yet. `WORK-UNITS.md`'s WU-21b line is now
`wip`. `DECISIONS.md` runs through ADR-049; `CORRECTIONS.md` unchanged
through C-016.

**Delivery mechanics:** all four changed/new files (`src/io/
decklink_capture_consumer.hpp`, `src/io/decklink_capture_consumer.cpp`,
`tests/test_decklink_capture_consumer.cpp`, `CMakeLists.txt`) were written
to the real repository via the device bridge this session and re-read back
from there to confirm the write landed correctly, per `SESSION-PROTOCOL.md`'s
own anti-drift rule 8 — `CMakeLists.txt` twice, since the first delivery was
found (on re-read) to be missing the new test's own build-target block, and
was corrected and redelivered before this handoff was written. `DECISIONS.md`
and `WORK-UNITS.md` themselves are being written back to the real repository
the same way as this session closes — see the end of this file. Nothing in
this handoff is asserted from a write call returning without error alone.

## Next work unit

**Immediate: nothing code-side — this is a build/test/tag session at your
own terminal.** WU-21b needs `cmake --build build`, `ctest --test-dir build
--output-on-failure` (expect `test_decklink_capture_consumer` to appear
alongside the existing suite; `test_decklink_device`'s own `foundDuplexDevice`
failure remains the already-accepted ADR-035 exception, unrelated), and —
with the Monitor 3G → Recorder 3G SDI loopback connected — a run of
`test_decklink_capture_consumer` itself to see whether `framesProcessed` is
actually nonzero and, if so, whether `copyLatestFrame()`'s returned size
matches expectation. Remove the stale `.git/index.lock` first (see below).
If it builds and passes clean, `./tools/close.sh 21b` (or the manual
`git tag -a wu-21b-green ...` step, if `close.sh` declines for any
ADR-035-shaped reason) is yours to run — the assistant does not do this.

**After WU-21b closes: WU-21c — continuous SDI re-output**, closing the
actual "full loop through at 576i25" loop end to end (scheduling a
live-produced frame stream onto `IDeckLinkOutput`, a materially different
mechanism from `LoopedFramePlayback`'s own "loop one static frame forever"
design). Not yet sketched in detail — that scoping is that session's own
first job, same discipline as every unit in this project.

Once WU-21 (all of a/b/c) closes, Phase 5's own remaining unit is WU-22
(diagnostic coverage view), `todo`, unscoped.

## Open questions

Unchanged in substance from session 27, plus one new item this session named
explicitly (not resolved): `kCaptureRingCapacity`'s own chosen value of 8
(WU-20a/20b, ADR-046) is still not measured against real observed buffering
depth — WU-21b's own test reports the counters needed to make that
comparison, but this session does not run the test, so does not make it.
Q3 (macOS/Desktop Video version), Q4 (lattice edge damping, C-008(a)) remain
open. Q2 remains moot per ADR-037. ADR-037's own genlock follow-up (#2,
capture/output clock-domain drift) remains open — `CaptureConsumer`
processes one already-arrived frame's worth of bytes at a time and has no
clock-domain behaviour of its own; squarely WU-21c's own concern once a real
capture/output pair actually runs concurrently. Follow-up #1
(`test_decklink_device.cpp`'s full-duplex check) remains the known, accepted
ADR-035 exception. WU-20b's own `stopFromCallback()` safety question
(ADR-047) is unchanged this session.

## Blocked / red

Nothing red. WU-21b is `wip`, reasoned through and delivered, awaiting
Steve's own real-terminal build/test/tag — expected, not a blocker, the same
state WU-14/WU-15a/WU-20b were each in at this same point in their own
sessions.

## Environment check

Unchanged from sessions 18-27 (ADR-037/039): **UltraStudio Monitor 3G**
remains the active, confirmed output target. **UltraStudio Recorder 3G**
remains the confirmed input target, exercised for real by WU-20b.
**UltraStudio 4K Mini** remains on hold pending a PSU replacement. WU-21b's
own `Accept:` needs the same Monitor 3G → Recorder 3G SDI loopback cable
WU-20b's own `Accept:` already used — no new physical setup.

## Append to DECISIONS.md

ADR-049 (tag/commit-state confirmation, including that the WU-21a
`WORK-UNITS.md`/`HANDOFF.md` pair was already committed going in; the
`.git/index.lock` finding; the SDK/code re-read findings; the four ADR-048
open questions resolved for WU-21b; `CaptureConsumer`'s full design; the
test file; the `CMakeLists.txt` changes including the one gap caught and
fixed within-session; the "not decided here" list for WU-21c/genlock/ring
capacity) was appended in full this session; see `DECISIONS.md`. ADR-049
does not reopen `docs/architecture.md`, ADR-026, ADR-031, ADR-032, ADR-037,
ADR-039, ADR-046, ADR-047 or ADR-048 — see its own closing paragraph.

## Append to CORRECTIONS.md

None this session. The one bug found while building WU-21b (a `CMakeLists.txt`
edit missing the new test's own build-target block) was caught and fixed
within the same session, before it was ever claimed delivered — see
`DECISIONS.md` ADR-049's own account, and the C-015/ADR-043 precedent this
follows.

---

## What to run at your terminal

**First, clear the stale lock** (confirm no real git process is running
first):

```
cd ~/src/scatter-dve
rm -f .git/index.lock
```

**Then commit this session's own new/changed files:**

```
git add src/io/decklink_capture_consumer.hpp src/io/decklink_capture_consumer.cpp \
        tests/test_decklink_capture_consumer.cpp CMakeLists.txt \
        DECISIONS.md WORK-UNITS.md HANDOFF.md
git commit -m "WU-21b: CaptureConsumer, DeckLink capture-side pixel read -- ADR-049, reasoned through only, unbuilt in the sandbox"
```

**Then build and test:**

```
cmake --build build
ctest --test-dir build --output-on-failure
# expect: test_decklink_capture_consumer present and run; only the
# already-accepted test_decklink_device/foundDuplexDevice exception (ADR-035)
# should fail, same as every session since it was accepted
```

**With the Monitor 3G -> Recorder 3G SDI loopback connected**, rerun (or
just read the same `ctest` output above) and check
`test_decklink_capture_consumer`'s own stderr line
(`framesArrived=... framesPushed=... framesPopped=... framesProcessed=...
framesFailed=...`) — with the loopback connected, `framesProcessed` should
be nonzero; without it, the NOTE line explains that zero is expected, not a
defect.

**If everything above is clean:**

```
./tools/close.sh 21b
# -> if it tags automatically, note the tag name here for next session's
#    own "before anything else" tag check
```

If `close.sh` declines for any reason (an unexpected failing test, an
ADR-035-shaped exception it doesn't know how to except), the manual
`git tag -a wu-21b-green -m "..."` step every unit since ADR-035 has used is
yours to run instead — same as WU-21a's own close last session.

Nothing else outstanding from this session. `WORK-UNITS.md`'s WU-21b status
line is `wip`, not `green` — this handoff does not claim it built or ran
anywhere; that confirmation is entirely yours to produce at your own
terminal, and this file will need a further update once you have.
