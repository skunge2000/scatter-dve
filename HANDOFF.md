# Handoff
Overwritten at the end of every session. This is the first thing to read.

---

**Session:** 27
**Tag:** `wu-20a-green` and `wu-20b-green` both confirmed to actually exist
this session — verified directly against the real repository's own `git
tag`/`git describe` (not trusted from session 26's own handoff account, which
described both as uncertain/unconfirmed going in). `WORK-UNITS.md`'s WU-20a
and WU-20b lines are corrected from `wip` to `green` accordingly. WU-21a
(`runFrameBytes()`, the in-memory sibling of `runFrame()`/`runFrameFile()`)
was built and genuinely verified in this session's own cloud sandbox, then
built, tested, and tagged at Steve's own real terminal within the same
session: `cmake --build build` clean, `ctest` green except the already-accepted
`test_decklink_device` exception (ADR-035), `./tools/close.sh 21a` correctly
refused to auto-tag over that exception, `git tag -a wu-21a-green ...` run
manually per the standing ADR-035 convention, confirmed present on `git tag`.
`WORK-UNITS.md`'s WU-21a line is now `green` (`wu-21a-green`) — the first
unit this project has had a tag confirmed within the very session it was
built, rather than at the start of the next one.
**Phase:** 5 (Live capture) continues. WU-21 ("full loop through at 576i25")
was scoped this session — real `Files:`/`Accept:` text, done only after the
reading below, same discipline as every prior unit — and split into three
pieces (WU-21a/b/c), the same "portable piece now, DeckLink-specific piece(s)
next" shape ADR-046 already used for WU-20a/WU-20b. Only WU-21a (portable
byte-conversion) was built this session. WU-21b (DeckLink capture-side pixel
read, draining `CaptureFrameRing`) is sketched in `WORK-UNITS.md` but not
built. WU-21c (continuous SDI re-output scheduling, closing the actual "full
loop through" loop end to end) is named but not yet sketched in detail.
`DECISIONS.md` now runs through ADR-048; `CORRECTIONS.md` still runs through
C-016 — no new entry this session (see below for why).

## This session in full

**Reading, before anything else.** Per `SESSION-PROTOCOL.md`'s own reading
table, in full: `HANDOFF.md` (session 26's), `INVARIANTS.md`, `DECISIONS.md`
in full (through ADR-047 going in, including C-016's own lesson and the
resulting Session-close/anti-drift rule 8 — explicitly treated as binding on
this session too), `CORRECTIONS.md` (through C-016), `WORK-UNITS.md`.

**Folder access and tag confirmation, before anything else.** Requested and
received device-bridge access to both `~/src/scatter-dve` (the repo) and
`~/src/Blackmagic DeckLink SDK 16.0` (the SDK); the SDK path resolved
directly, no fallback needed. Then, per the explicit instruction not to trust
`HANDOFF.md`'s own account of tag state: ran the real repository's own `git
status`/`git tag`/`git log` via the device bridge (`tools/context.sh` was not
used directly — `git` itself was queried instead, equivalent information).
Result: working tree clean, `HEAD` at `524e48a`, both `wu-20a-green` and
`wu-20b-green` present. `WORK-UNITS.md`'s own WU-20a/WU-20b lines corrected
from `wip` to `green` to match.

**WU-21's own first job: re-reading the SDK again.** Per this project's own
established practice for DeckLink work (ADR-031/032/046/047): re-read the
real SDK's own `DeckLinkAPI.h` (`IDeckLinkVideoInputFrame`,
`IDeckLinkVideoBuffer`, `StartAccess`/`EndAccess`, `GetBytes()`,
`GetRowBytes()`, `BMDBufferAccessFlags`) and the three real capture samples
(`CaptureStills`, `InputLoopThrough`, `CapturePreview`) again — not just
ADR-047's own summary. Confirmed: `IDeckLinkVideoBuffer` is the same
interface obtained via `QueryInterface` for both the read and write
directions (the one `LoopedFramePlayback::fillFrameBuffer()`, WU-15a, ADR-032,
already uses on the output side); the accessor is `GetRowBytes()`, not
`GetBytesPerRow()`; and — the key finding — none of the three existing
capture samples reads pixel bytes out of a retained frame via
`IDeckLinkVideoBuffer` at all. Reading real pixel bytes out of a retained
`IDeckLinkVideoInputFrame` is genuinely new ground for this project, matching
the original task's own framing. Also reread `src/io/decklink_output.hpp`/
`.cpp` (the write-direction pattern this unit's own eventual WU-21b half
extends to input), `src/io/decklink_input.hpp`/`.cpp` (WU-20b, the ring this
work drains), `src/core/ring_buffer.hpp` (WU-20a), and `docs/architecture.md`
sections 3, 6, 7, 9, 12 plus ADR-037/039 (real target hardware unchanged:
UltraStudio Recorder 3G for input, UltraStudio Monitor 3G for output).

**The split decision.** WU-21's full ambition ("full loop through at
576i25") does not fit one session's own 3-file/~400-line cap, and — more
importantly — does not fit this sandbox's own verification ability in one
piece: draining `CaptureFrameRing` and reading bytes out of a retained
`IDeckLinkVideoInputFrame` is DeckLink-only surface this sandbox cannot
compile or run, exactly the same shape that has forced every other
DeckLink-touching split in this project (WU-12a/b, WU-15a/b, WU-16a/b,
WU-19a/b, WU-20a/b). Split into three: WU-21a (portable byte-conversion,
`runFrameBytes()` — no DeckLink dependency at all, buildable and genuinely
testable here), WU-21b (DeckLink capture-side pixel read/ring-drain,
DeckLink-only, next session's own job), WU-21c (continuous SDI re-output
scheduling, closing the loop end to end, later still). Only WU-21a was built
this session. See `DECISIONS.md` ADR-048 for the full reasoning.

**WU-21a itself.** `src/core/resolve.hpp`/`src/core/pipeline.cpp` (both
extended): new `runFrameBytes()`, the in-memory sibling of `runFrame()`
(raster-to-raster) and `runFrameFile()` (file-to-file) — takes packed v210
bytes plus row strides in memory, unpacks, upsamples chroma, calls the
existing `runFrame()`, downsamples chroma, packs back to v210 bytes in
memory. This exists because a captured frame's own pixel bytes live inside a
DeckLink-owned buffer for the duration of one `StartAccess`/`EndAccess`
bracket, never as a file on disk — so `runFrameFile()`'s own file-to-file
path cannot be the route from a captured frame into this project's own warp
pipeline; WU-21b will need exactly this entry point. Deliberately does
**not** refactor `runFrameFile()` to call `runFrameBytes()` internally (the
two remain independently written) — per this project's own "never
rename/refactor across module boundaries mid-unit" discipline, and because
`runFrameFile()` is untouched, already-verified WU-14/WU-16 code with no
reason to risk it for a refactor this unit's own `Accept:` does not need.
`tests/test_pipeline_bytes.cpp` (new): two checks, both run for real —
`runFrameBytes()` matches `runFrameFile()`'s own output exactly for a
genuine off-centre affine warp (0.7x compression over a zone plate, not the
identity map's degenerate case); `runFrameBytes()` itself satisfies I7
(identity map round-trips bit-exactly) against flat-chroma content, the same
foundational property `test_zoneplate.cpp`'s own `testI7Pattern()` already
established for `runFrameFile()`. `CMakeLists.txt`: `test_pipeline_bytes`
registered via the existing `scatter_test()` function.

**One test bug caught and fixed within this same session, before any claim
was made** (not logged in `CORRECTIONS.md` — see below): the I7 check first
used `testpat::makeRamp()` and failed (`CHECK(dstBytes == srcBytes)`), because
`CORRECTIONS.md` C-006 already establishes that a ramp's own non-flat chroma
does not survive `chroma::upsampleImage()`/`downsampleImage()` unchanged,
regardless of the lattice — true for the identity map too. Fixed by switching
to `testpat::makeZonePlate()` (flat chroma), matching `test_zoneplate.cpp`'s
own `chromaExpectedExact=true` convention. Also fixed one genuine compile
error during the same editing pass: `video::v210::unpackImage`/`packImage`
(wrong — `pipeline.cpp` is in `namespace scatter` directly, and `v210` is a
sibling of `video`, not nested under it) corrected to unqualified
`v210::unpackImage`/`v210::packImage`, matching the file's own existing
pattern for `video::readV210File`/`chroma::upsampleImage`.

**Verification.** GCC 13.3.0 and Clang 18.1.3, Release and Debug, at
`SCATTER_TILE_LOG2` 4 and 5 (8 configurations total) — all 19 project tests
green in every configuration. Plus GCC 13 and Clang 18 both with
`-fsanitize=address,undefined -fno-sanitize-recover=all` at both tile sizes —
clean, no ASan or UBSan report. TSan not run — this unit introduces no new
concurrency, unlike WU-20a. This unit touches no DeckLink or Apple-only
surface at all — unlike WU-14/WU-15a/WU-20b, there is no piece of it this
sandbox could not already fully verify for real.

**Corrections this session:** none. The chroma-test and namespace-
qualification bugs above were both caught and fixed within the same editing
pass, before any claim of "done" was made to anyone — matching the C-015/
ADR-043 precedent that routine self-caught iteration during genuine
build/test execution is not a `CORRECTIONS.md`-worthy event; a correction
entry is for something that was claimed fixed/delivered and then found
wrong. Nothing here was ever claimed working before it actually was.

## Where we are

**Phase 5 (Live capture) continues.** WU-20a and WU-20b both confirmed
`green` and tagged (verified directly against the real repository this
session). WU-21a (`runFrameBytes()`) is implemented, genuinely verified in
this session's own cloud sandbox (full 8-configuration matrix plus
ASan/UBSan, all clean), and — within this same session — also built,
tested, and tagged at Steve's own real terminal: `cmake --build build`
clean, `ctest` green (`test_pipeline_bytes` passing, only the already-accepted
`test_decklink_device`/ADR-035 exception failing), `./tools/close.sh 21a`
correctly declined to auto-tag over that exception, `git tag -a
wu-21a-green ...` run manually per the standing ADR-035 convention, and
confirmed present on `git tag` alongside `wu-20a-green`/`wu-20b-green`.
`WORK-UNITS.md`'s WU-21a line is now `green` (`wu-21a-green`). WU-21b
(DeckLink capture-side pixel read) is sketched in `WORK-UNITS.md`, not
built. `DECISIONS.md` now runs through ADR-048; `CORRECTIONS.md` unchanged
through C-016.

**Delivery mechanics:** all files below were written to the real repository
via the device bridge this session and re-read back from there to confirm
the write landed correctly, per `SESSION-PROTOCOL.md`'s own anti-drift rule
8 (C-016's own lesson, explicitly applied here) — nothing in this handoff is
asserted from the write call returning without error alone. Steve committed,
built, tested, closed, and tagged at his own terminal within this same
session (one stale `.git/index.lock` from an unrelated earlier process was
hit and cleared before the commit — not a device-bridge write this time,
since git commands are never run via the bridge). This `WORK-UNITS.md`'s
own WU-21a status-line update (to `green`) is itself being written back to
the real repository and re-read to confirm before this session closes — see
the end of this file.

## Next work unit

**WU-21b — DeckLink capture-side pixel read.** `todo`, sketched in
`WORK-UNITS.md` (matching the style ADR-046 used to sketch WU-20b before it
was built). Its own first job: drain WU-20b's own `CaptureFrameRing`
(`src/io/decklink_input.hpp`) on a consumer thread, obtain
`IDeckLinkVideoBuffer` via `QueryInterface` on a retained
`IDeckLinkVideoInputFrame`, bracket `GetBytes()` with `StartAccess`/
`EndAccess`, and feed the resulting bytes/`GetRowBytes()` into WU-21a's own
new `runFrameBytes()`. `DECISIONS.md` ADR-048 names the open questions this
sketch deliberately leaves undecided: consumer-thread ownership/lifetime
against `CaptureSource::stop()`; whether extracted bytes get copied into a
pool buffer or handed to `runFrameBytes()` while still inside the
`StartAccess`/`EndAccess` bracket; genlock/clock-domain drift (still open
since ADR-037/047); whether `kCaptureRingCapacity=8` matches real observed
buffering depth once there is a real consumer to measure against. Real
`Files:`/`Accept:` scoping is that session's own first job, same as every
unit in this project. WU-21c (continuous SDI re-output scheduling) follows
WU-21b, later still — not yet sketched in detail.

Once WU-21 (all of a/b/c) closes, Phase 5's own remaining unit is WU-22
(diagnostic coverage view), `todo`, unscoped.

## Open questions

Unchanged from session 26: Q3 (macOS/Desktop Video version), Q4 (lattice edge
damping, C-008(a)) remain open. Q2 remains moot per ADR-037. ADR-037's own
follow-up #2 (genlock) remains open — WU-21a's own code does not touch it (a
portable byte-conversion function has no clock domain); still squarely
WU-21b/c's own concern once a real capture/output pair actually runs
concurrently. Follow-up #1 (`test_decklink_device.cpp`'s full-duplex check)
remains the known, accepted ADR-035 exception, unrelated to Phase 5's own new
work. WU-20b's own `stopFromCallback()` safety question (named in ADR-047,
still open) is unchanged this session — WU-21a's own code does not touch it
either.

## Blocked / red

Nothing red. WU-21a is fully closed — genuinely verified in this session's
own cloud sandbox (8-configuration matrix + ASan/UBSan, all green) and,
within the same session, built, tested, closed, and tagged (`wu-21a-green`)
at Steve's own real terminal.

## Environment check

Unchanged from sessions 18-26 (ADR-037/039): **UltraStudio Monitor 3G**
remains the active, confirmed output target. **UltraStudio Recorder 3G**
remains the confirmed input target, exercised for real by WU-20b. **UltraStudio
4K Mini** remains on hold pending a PSU replacement. WU-21a's own `Accept:`
needs nothing physical at all — it is pure portable code; the physical Monitor
3G → Recorder 3G SDI loopback cable from WU-20b's own `Accept:` will be
needed again once WU-21b is built and its own hardware-gated test runs.

## Append to DECISIONS.md

ADR-048 (tag-state confirmation; the SDK re-read findings, including that no
existing capture sample reads pixel bytes via `IDeckLinkVideoBuffer`; the
WU-21a/WU-21b/WU-21c split rationale; `runFrameBytes()`'s own design and why
`runFrameFile()` was deliberately not refactored to call it; the test file
and its own two checks; the full verification matrix results; the two bugs
caught and fixed within-session; the "not decided here" list for WU-21b) was
appended in full this session; see `DECISIONS.md`. ADR-048 does not reopen
`docs/architecture.md`, ADR-010, ADR-021, ADR-026, ADR-031, ADR-032, ADR-038,
ADR-040, ADR-041, ADR-046 or ADR-047 — see its own closing paragraph.

## Append to CORRECTIONS.md

None this session. The two bugs found while building WU-21a (a chroma-plane
test-pattern choice and a namespace-qualification compile error) were both
caught and fixed within the same editing pass, before either was ever
claimed working — see `DECISIONS.md` ADR-048's own account, and the C-015/
ADR-043 precedent this follows.

---

## What to run at your terminal

Already done this session, in full, at your own terminal:

```
cd ~/src/scatter-dve
rm -f .git/index.lock   # stale lock from an unrelated earlier process, confirmed no real git process was running first
git add src/core/resolve.hpp src/core/pipeline.cpp \
        tests/test_pipeline_bytes.cpp CMakeLists.txt \
        DECISIONS.md WORK-UNITS.md HANDOFF.md
git commit -m "WU-21a: runFrameBytes(), the in-memory sibling of runFrame()/runFrameFile() -- ADR-048; WU-20a/WU-20b confirmed green"
# -> 502dd8c, 7 files changed, 850 insertions(+), 238 deletions(-)

cmake --build build
ctest --test-dir build --output-on-failure
# -> 96% tests passed, 1 failed (test_decklink_device / foundDuplexDevice,
#    ADR-035's own already-accepted exception); test_pipeline_bytes passing

./tools/close.sh 21a
# -> TESTS FAILED (the same accepted exception) -- close.sh correctly
#    declined to auto-tag, per its own no-exceptions design

git tag -a wu-21a-green -m "WU-21a: runFrameBytes(), verified byte-identical to runFrameFile() for a genuine warp and I7-exact for flat chroma"
git tag
# -> wu-21a-green present, alongside wu-20a-green and wu-20b-green
```

Nothing outstanding from this session. `WORK-UNITS.md`'s WU-21a status line
has been updated to `green` (`wu-21a-green`) to match and is being written
back to the real repository as this handoff closes — you do not need to
commit that update by hand; it will show as an uncommitted change to
`WORK-UNITS.md` (and this `HANDOFF.md`) next time you look, same as every
other session's own closing pair of files. A short commit for those two
alone next session (or now, your call) is the only loose end:

```
git add WORK-UNITS.md HANDOFF.md
git commit -m "WU-21a: mark green, tagged wu-21a-green (session 27 close)"
```
