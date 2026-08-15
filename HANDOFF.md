# Handoff
Overwritten at the end of every session. This is the first thing to read.

---

**Session:** 26
**Tag:** `wu-20a-green` should already stand from session 25's own close, still
not re-confirmed by anything run this session (`test_ring_buffer` passed in
Steve's own full-suite run below, but the `close.sh 20a`/tag step itself was
not re-verified). WU-20b was built this session, reasoned through only at
first, then genuinely verified at Steve's own real terminal: build clean,
`test_decklink_input` passing with the Monitor 3G → Recorder 3G loopback
connected. `WORK-UNITS.md`'s WU-20b line is still `wip`, not `green` —
`./tools/close.sh 20b` and the tag are the only things left, and tagging is
Steve's own step, not the assistant's.
**Phase:** 5 (Live capture) continues. WU-20a (ring buffer) should already be
closed from last session. WU-20b (DeckLink capture object:
format-detection-aware `EnableVideoInput`, `IDeckLinkInputCallback`
implementation, retained frames pushed into a WU-20a `RingBuffer`) was built
this session — real `Files:`/`Accept:` scoping done first, then the code —
and is now confirmed at Steve's real terminal. One genuine first-compile
defect this sandbox could not catch: `-Wsign-conversion` on
`videoFrame->GetFlags() & bmdFrameHasNoInputSource` (`_BMDFrameFlags` to
`BMDFrameFlags`), fixed with an explicit `static_cast<BMDFrameFlags>(...)`.
`DECISIONS.md` now runs through ADR-047; `CORRECTIONS.md` now runs through
C-016 — one entry appended this session, and it is not about the SDK: the
fix above was first delivered only to the assistant's own sandbox copy, not
written to `~/src/scatter-dve` on the Mac, so Steve's first rebuild hit the
identical error. Caught because Steve pasted the second build log rather
than trusting "fixed." `SESSION-PROTOCOL.md` was updated this session (new
Session-close paragraph, anti-drift rule 8) so no future session tells Steve
to rebuild before a device-bridge write-back is confirmed.

## This session in full

**Reading, before anything else.** Per `SESSION-PROTOCOL.md`'s own reading
table, in full: `HANDOFF.md`, `INVARIANTS.md`, `DECISIONS.md` in full (through
ADR-046 going in), `CORRECTIONS.md` (through C-015), `WORK-UNITS.md`. Then,
per this project's own established practice for DeckLink work (ADR-031/032/
046): re-read the real Blackmagic DeckLink SDK's own `DeckLinkAPI.h`
(`IDeckLinkInput`, `IDeckLinkInputCallback`, `VideoInputFrameArrived`,
`VideoInputFormatChanged`, `EnableVideoInput`, `StartStreams`/`StopStreams`,
`bmdVideoInputEnableFormatDetection`, `IDeckLinkVideoFrame`/
`IDeckLinkVideoInputFrame`/`IDeckLinkVideoBuffer`, `IDeckLinkDisplayMode`) and
the three real capture samples (`CaptureStills`, `InputLoopThrough`,
`CapturePreview`) again — not just ADR-046's own summary of last session's
reading — confirming frame retention (`AddRef`) and `VideoInputFormatChanged`
handling exactly as ADR-046 already recorded, nothing corrected. Also reread
this project's own `src/io/decklink_device.hpp`/`.cpp`, `src/io/com_ptr.hpp`
(ADR-031 in full), `src/io/decklink_output.hpp`/`.cpp` (WU-15a) for
established idioms, `src/core/ring_buffer.hpp` (WU-20a/ADR-046), and
`docs/architecture.md` sections 3, 6, 7, 9, 12 plus ADR-037/039 (real target:
**UltraStudio Recorder 3G** for input, **UltraStudio Monitor 3G** for output,
not the paused 4K Mini).

**The split decision.** `SESSION-PROTOCOL.md`'s sizing cap ("touch at most 3
source files plus its test... if a unit cannot meet this, split it before
starting") was checked explicitly, after the reading above, not before it or
by guessing — the same order every prior split in this project used. WU-20b
fits one session: ADR-046's own sketch already named exactly two new source
files (`decklink_input.hpp`/`.cpp`) plus one test, comfortably under the cap.
The two candidate further splits considered and rejected: "EnableVideoInput +
format detection" vs. "callback/retention/ring-push wiring" (rejected —
neither half is independently useful without the other, mirroring
`LoopedFramePlayback`'s own single-class shape on the output side); a
real-hardware endurance split in the WU-15a/WU-15b shape (rejected — nothing
in this unit's own `Accept:` needs an unattended run longer than one sitting,
unlike WU-15b's genuine one-hour requirement). See `DECISIONS.md` ADR-047 for
the full reasoning.

**WU-20b itself.** `src/io/decklink_input.hpp`/`.cpp` (new): `CaptureSource`,
mirroring `io/decklink_output.hpp`'s own `LoopedFramePlayback` shape
(ADR-032) applied to input for the first time — implements
`IDeckLinkInputCallback` directly, real `IUnknown` refcounting,
`ComPtr::adopt()` on `create()`. `create()` confirms
`BMDDeckLinkSupportsInputFormatDetection` (a hard requirement, not a soft
fallback — WU-20's own "format-detection-aware" premise), confirms
`DoesSupportVideoMode()`, then `SetCallback`/`EnableVideoInput`/
`StartStreams` (`CaptureStills`'/`CapturePreview`'s own call order). Three
design questions ADR-046 left open, decided this session (see ADR-047 for
the full reasoning): ring capacity fixed at 8 (`kCaptureRingCapacity`, double
architecture.md 7's own named "3-4 frames" high end); pixel format never
changes on a format-change restart — always `bmdFormat10BitYUV`, since
ADR-005 already fixes v210 as this project's only supported I/O format and
nothing downstream could consume anything else; a `bmdFrameHasNoInputSource`
frame is filtered before ever attempting a ring push, and `CaptureStills`'
own signal-recovery restart (`StopStreams`/`FlushStreams`/`StartStreams` on
the first valid frame after an invalid one, that recovery frame itself not
pushed either) is reused directly from the real SDK sample, not reinvented —
unlike WU-20a's ring buffer, nothing in architecture.md contradicts
`CaptureStills`' own answer here, so there was no reason to design a new one.
A failed format-change or signal-recovery restart both call a shared
`stopFromCallback()`, reasoned through but **not verified safe** — calling
`SetCallback(nullptr)`/`DisableVideoInput()` from inside a callback the SDK
itself is currently invoking is a genuinely new category of unverified
behaviour this project has not flagged before; see ADR-047.
`tests/test_decklink_input.cpp` (new): hardware-only, gated on
`BLACKMAGIC_SDK_DIR` exactly as `test_decklink_device.cpp`/
`test_decklink_output.cpp` already are. Its own header comment documents the
concrete real-hardware precondition its `Accept:` needs — the UltraStudio
Monitor 3G's SDI output physically patched into the Recorder 3G's SDI input,
a genuine loopback signal, since (unlike WU-15a's own test) this sandbox
cannot synthesize an SDI signal for a capture unit to receive. Checks a
bounded ~5-second run: `create()` succeeds, `stop()` returns cleanly, and the
unconditional accounting invariant `framesPushed + ring.droppedCount() <=
framesArrived` holds whether or not real signal is actually present — the
one thing checkable without assuming anything about the physical setup.
`CMakeLists.txt`: `decklink_input.cpp` added to the `scatter-decklink`
target, `test_decklink_input` added alongside `test_decklink_device`/
`test_decklink_output`.

**Explicitly stated, per this project's own discipline (ADR-031/032/046):**
this sandbox has no Blackmagic SDK and no AppleClang/Xcode toolchain at all.
Anything landing in the `scatter-decklink` target (gated on
`BLACKMAGIC_SDK_DIR`) — which is all of WU-20b — cannot be compiled here,
only reasoned through against the real SDK headers and samples, exactly as
WU-14/WU-15a/WU-20b's own DeckLink-dependent half already were. See
`DECISIONS.md` ADR-047's own closing sections for the full statement,
including the one genuinely new unverified-behaviour flag
(`stopFromCallback()`, above).

**Corrections this session:** one, C-016 — not about the SDK. This session's
own re-read of the real SDK headers and the three capture samples confirmed
ADR-046's own prior account of them exactly, nothing there was found wrong.
The correction is about delivery: a real `-Wsign-conversion` defect Steve's
build caught was fixed in the assistant's own sandbox copy but not, at
first, written back to the real repository — see C-016 and the top of this
file.

## Where we are

**Phase 5 (Live capture) continues.** WU-20a (ring buffer) should already be
`green` from session 25's own close, still not re-confirmed by a `close.sh
20a`/tag check this session (`test_ring_buffer` did pass in Steve's own full
run). WU-20b (DeckLink capture object) is implemented and now genuinely
verified at Steve's real terminal — build clean, `test_decklink_input`
passing with the physical loopback connected — `wip` in `WORK-UNITS.md`
only because `./tools/close.sh 20b` and the tag are still outstanding.
`DECISIONS.md` now runs through ADR-047; `CORRECTIONS.md` through C-016.

**Delivery mechanics:** WU-20b's implementation was done entirely reasoned
through against the real SDK headers and samples in this session's own cloud
sandbox — no compile, no run, the same shape every DeckLink-touching unit has
had. All files (`decklink_input.hpp`, `decklink_input.cpp`,
`test_decklink_input.cpp`, `CMakeLists.txt`, `DECISIONS.md`, `WORK-UNITS.md`,
this `HANDOFF.md`) were written to the real repository via the device
bridge — except `decklink_input.cpp`'s own post-build fix, which initially
was not (C-016); it has since been written back and re-read to confirm.
`SESSION-PROTOCOL.md` itself was also changed this session (Session close,
anti-drift rule 8) and is now written to the real repository too. Steve
builds, tests, and tags at his own terminal — the standing operational note
(device-bridge commits on this machine leave stale `.git/index.lock`/
`HEAD.lock` files) still applies; git commands are not run via the bridge
this session either.

## Next work unit

**WU-21 — Full loop through at 576i25.** `todo`, unscoped. Its own first job,
per this project's established discipline: drain WU-20b's own
`CaptureFrameRing` on a consumer thread, and read pixel bytes out of a
retained `IDeckLinkVideoInputFrame` for the first time anywhere in this
project — the `IDeckLinkVideoBuffer`/`StartAccess`/`EndAccess` pattern
ADR-032 already established for output extends to input the same way
ADR-046 already extended the `GetBytes()`-is-not-on-the-frame finding.
`DECISIONS.md` ADR-047 names three things left open for WU-21 to pick up:
the consumer side of the ring itself; genlock/clock-domain drift between the
Recorder 3G's own capture clock and the Monitor 3G's own output clock
(ADR-037's second follow-up, still open); and whether
`kCaptureRingCapacity`'s chosen value of 8 actually matches real observed
buffering depth once there is a real consumer to measure against. Real
`Files:`/`Accept:` scoping is that session's own first job, same as every
unit in this project.

Once WU-21 closes, Phase 5's own remaining unit is WU-22 (diagnostic
coverage view), `todo`, unscoped.

## Open questions

Unchanged from session 25: Q3 (macOS/Desktop Video version), Q4 (lattice
edge damping, C-008(a)) remain open. Q2 remains moot per ADR-037. ADR-037's
own follow-up #2 (genlock) remains open — WU-20b's own code does not touch
it (naming a device does not require reasoning about its clock domain,
ADR-039); still squarely WU-21's own concern once a real capture/output pair
actually runs concurrently. Follow-up #1 (`test_decklink_device.cpp`'s
full-duplex check) is the known, accepted ADR-035 exception, unrelated to
Phase 5's own new work.

**New this session, named in ADR-047, not resolved:** whether
`CaptureSource::stopFromCallback()` — calling `SetCallback(nullptr)`/
`DisableVideoInput()` from inside a callback the SDK itself is currently
invoking — is actually safe. Reasoned through against the real SDK's own
documented behaviour and against `CaptureStills`' own comparable
call-from-within-the-callback pattern on its signal-recovery path, but not
confirmed by execution anywhere. Worth Steve's own attention at the real
terminal, especially if a format-change or signal-recovery restart ever
actually fails during testing (a mismatched-format loopback source would be
one way to exercise this deliberately).

## Blocked / red

Nothing red. WU-20b has now been built and tested at Steve's real terminal —
clean build (after the C-016 fix), `test_decklink_input` passing with the
physical loopback connected. Only the close/tag step remains, which is
Steve's own action, not a block.

## Environment check

Unchanged from sessions 18-25 (ADR-037/039): **UltraStudio Monitor 3G**
remains the active, confirmed output target. **UltraStudio Recorder 3G** is
in hand, named (ADR-039) as Phase 5's own input target — WU-20b's own job to
actually exercise it, next, at the real terminal. **UltraStudio 4K Mini**
remains on hold pending a PSU replacement. **New this session:** WU-20b's own
`Accept:` needs a physical SDI cable connecting the Monitor 3G's own output
to the Recorder 3G's own input for a genuine loopback signal — both devices
are already in hand, so this is a cabling step, not a purchase.

## Append to DECISIONS.md

ADR-047 (WU-20b built as one unit, not split further, with the reasoning;
the ring-capacity/pixel-format/no-input-source-frame design questions
ADR-046 left open, all decided; `CaptureStills`' own signal-recovery restart
reused directly; the `stopFromCallback()` unverified-behaviour flag; the
real-hardware loopback test design; and why this sandbox cannot compile or
run any of it) was appended in full this session; see `DECISIONS.md`.
ADR-047 does not reopen `docs/architecture.md`, ADR-005, ADR-024, ADR-031,
ADR-032, ADR-037, ADR-039 or ADR-046 — see its own closing paragraph.

## Append to CORRECTIONS.md

C-016 appended this session — the `-Wsign-conversion` fix to
`decklink_input.cpp` was initially delivered only to the assistant's own
sandbox copy, not to `~/src/scatter-dve`, so Steve's first rebuild hit the
identical error a second time. Not an SDK or design-decision correction —
see `DECISIONS.md` ADR-047's own account of the SDK re-read, which found
nothing wrong there. `SESSION-PROTOCOL.md`'s Session close section and a new
anti-drift rule 8 now require a device-bridge write-back plus re-read
confirmation before any file is called "delivered."

---

## What to run at your terminal

Nothing has been committed yet — same standing operational note as every
prior session: committing through the device bridge on this repo reliably
leaves stale `.git/index.lock`/`HEAD.lock` files behind, so this was left
for you to run directly:

```
cd ~/src/scatter-dve
git add src/io/decklink_input.hpp src/io/decklink_input.cpp \
        tests/test_decklink_input.cpp CMakeLists.txt \
        DECISIONS.md WORK-UNITS.md HANDOFF.md CORRECTIONS.md \
        SESSION-PROTOCOL.md
git commit -m "WU-20b: DeckLink capture object (format detection, IDeckLinkInputCallback, ring push) -- ADR-047, verified at the real terminal; C-016; SESSION-PROTOCOL.md device-bridge delivery rule"
```

Build, test, and loopback cable are already done — this session's own
real-terminal run already showed a clean `cmake --build build` and
`test_decklink_input` passing (`test_decklink_device`'s `foundDuplexDevice`
failure is ADR-035's own already-accepted exception, unrelated). Straight to
close:

```
./tools/close.sh 20b
git tag -a wu-20b-green -m "WU-20b: DeckLink capture object, verified against real UltraStudio Recorder 3G + Monitor 3G loopback"
```

If WU-20a's own tag from last session was never confirmed run, it's still
outstanding too — `./tools/close.sh 20a` and `git tag -a wu-20a-green ...`,
per session 25's own handoff, unrelated to this session's own work but not
re-verified here either. `close.sh` will likely report the `test_decklink_device`
exception as a suite failure and refuse to tag on its own (it has no
knowledge of ADR-035); that is expected — the manual `git tag -a` commands
above are how every unit since ADR-035 has actually been closed, per
`DECISIONS.md`'s own account of `wu-15a-green`.
