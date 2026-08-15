# Handoff
Overwritten at the end of every session. This is the first thing to read.

---

**Session:** 16
**Tag:** none yet — WU-15a is `wip`, not `green`. `wu-14-green` (last
session) remains the last confirmed tag.
**Phase:** 3 — SDI output, under way. WU-14 (device enumeration, `ComPtr`)
is `green`. WU-15a (scheduled playback of one looped, file-sourced, warped
frame) is drafted this session but **unbuilt and unverified** — see below.

**This session's own first job was research, not implementation**, per its
own brief and this project's own established discipline since WU-14: read
`HANDOFF.md`, `INVARIANTS.md`, `DECISIONS.md`, `CORRECTIONS.md` and
`WORK-UNITS.md` in full (`SESSION-PROTOCOL.md`'s required order), re-read
`docs/architecture.md` section 7's Output subsection and section 10's Phase
3 description, then read the real Blackmagic DeckLink SDK 16.0 headers
(`DeckLinkAPI.h`, `DeckLinkAPIModes.h`, `DeckLinkAPITypes.h`) and several of
the SDK's own C++ samples (`FilePlayback`'s `DeckLinkPlaybackDevice.h/.cpp`,
`SignalGenerator`'s `SyncController.mm`/`DeckLinkOutputDevice.mm`) for the
actual scheduled-playback idiom — preroll depth, the completion-callback
refill pattern, what `CreateVideoFrame()`/writing a frame's own bytes
actually requires — before scoping `WORK-UNITS.md`'s WU-15 line or writing
any project code. What that reading found, the choices it forced, and the
WU-15a/WU-15b split it led to are in `DECISIONS.md` ADR-032 — summary below.

**Design choices this session had to make — now ADR-032 in
`DECISIONS.md`:** architecture.md 7's "`CreateVideoFrame()` and write into
`GetBytes()`" is not literally true — `GetBytes()` lives on a separate
`IDeckLinkVideoBuffer` interface, obtained via `QueryInterface` from the
frame and bracketed by `StartAccess()`/`EndAccess()`, confirmed directly
against the SDK's own `SignalGenerator` sample rather than assumed.
`src/io/decklink_output.cpp` asks the SDK for `bmdFormat10BitYUV`'s own row
bytes (`RowBytesForPixelFormat`) rather than trusting this project's own
`v210::rowBytesMin()` — the same "never compute row stride yourself" rule
architecture.md 7 already states for the input side, applied symmetrically
to output; `tests/test_decklink_output.cpp` checks the two actually agree,
once, directly against the real hardware. The frame source is raw packed
`.v210` bytes read straight from disk (`<fstream>`), not
`video::readV210File()`'s unpack path — `bmdFormat10BitYUV` *is* v210, so
unpacking to `Sample` planes and repacking would be pure waste for this
unit's own job. Preroll is half a second of frames, computed from the
negotiated display mode's own frame rate — not architecture.md 7's
illustrative "3-frame" figure, which neither real sample actually uses;
refill is exactly one replacement frame per `ScheduledFrameCompleted()`
call, matching `FilePlayback`'s own idiom exactly. Display mode is
`bmdModePALp` (ADR-007's "576p25" half, chosen over "576i25" because this
project has no de-interlace/field-split machinery yet) — **unverified
against the real UltraStudio 4K Mini this session**; if unsupported,
`DoesSupportVideoMode()` fails cleanly (a null result, not a crash) and
`bmdModePAL` is the documented fallback to try by hand.

**The WU-15 split, decided after the SDK reading, not before it (same order
ADR-028 used for WU-12a/WU-12b):** `WORK-UNITS.md`'s WU-15 line, going into
this session, asked for "one hour on a broadcast monitor, no dropped
frames" as a single accept criterion — exactly what last session's own
`HANDOFF.md` flagged as likely not fitting "one session, one work unit."
Split into **WU-15a** (this session — get one genuinely warped frame,
written to a real `.v210` file by this project's own pipeline, looping on
the real hardware for a bounded few-second run with zero dropped/late
frames, confirmed once by eye on a broadcast monitor) and **WU-15b** (not
this session, not new implementation — the same mechanism run unattended
for a full hour, reported back by hand, the literal remaining half of
architecture.md 10 Phase 3's own "done when" line). See `DECISIONS.md`
ADR-032 for the full reasoning.

**Tests:** none run this session — see "Delivery mechanics" below.
`tests/test_decklink_output.cpp` (new, drafted, unbuilt) has two checks:
`test_v210_rowbytes_matches_project_own_computation()` (the SDK's own
`RowBytesForPixelFormat` against this project's `v210::rowBytesMin()`, both
against the real UltraStudio 4K Mini) and
`test_looped_playback_runs_with_no_dropped_or_late_frames()` (builds a
cylinder-over-zone-plate warped frame via `runFrameFile()`, writes it to
`/tmp/scatter_wu15a_frame.v210`, plays it via `LoopedFramePlayback` for five
seconds, asserts zero dropped/late completions and a clean stop). Fourteen
tests carried over from WU-01 through WU-13 plus `test_decklink_device`
(WU-14) are unaffected — no file either of them depends on changed.

Unlike every unit since WU-06, and the same as WU-14, this session's own
implementation was **not** first run through the Linux cloud sandbox's
Clang 18/GCC 13/ASan/UBSan matrix — no Blackmagic SDK and no
AppleClang/Xcode toolchain exist there, and `CMakeLists.txt`'s
`BLACKMAGIC_SDK_DIR` guard means that matrix never sees these files at all.
`decklink_output.hpp`/`.cpp` and `tests/test_decklink_output.cpp` were
reasoned through against the real SDK headers and samples and written
straight to this machine via the device bridge, unbuilt.

**Build:** not attempted this session. Needs `cmake --build build` (the
existing `build/` directory's cached `BLACKMAGIC_SDK_DIR` should carry
through unchanged, per WU-14's own precedent — no reconfigure expected to be
necessary, but worth confirming) then `ctest --test-dir build -R
test_decklink_output` (or the full suite) at the real terminal.

## Where we are

`src/io/decklink_output.hpp`/`.cpp` — `LoopedFramePlayback`, a looped
single-frame scheduled-playback wrapper around `IDeckLinkOutput`
(`create()`, `stop()`, `stats()`), implementing `IDeckLinkVideoOutputCallback`
directly. `tests/test_decklink_output.cpp` — the two checks above, run
against real hardware. `CMakeLists.txt` — `decklink_output.cpp` added to the
existing `scatter-decklink` target; `test_decklink_output` added alongside
`test_decklink_device`, linking both `scatter-decklink` and `scatter-core`
(the first `scatter-decklink`-target test to also need `scatter-core`, for
`runFrameFile()`/`buildCylinderLattice()`/the `testpat` helpers/
`v210::rowBytesMin()` — it builds its own warped source frame rather than
shipping a checked-in fixture file). See `DECISIONS.md` ADR-032 for the full
design and `WORK-UNITS.md`'s WU-15a/WU-15b entries for accept criteria and
scope.

**Corrections this session:** none. Nothing found while reading the real
SDK contradicted an earlier claim in `DECISIONS.md`, `INVARIANTS.md` or
`CORRECTIONS.md` — architecture.md 7's Output subsection was
underspecified/imprecise in the ways ADR-032 documents (the `GetBytes()`
indirection, the illustrative preroll figure), not wrong in a way that
breaks anything already built; every earlier unit's own files are
untouched.

**Delivery mechanics, not a design matter:** this session ran remotely, via
the device-bridge tools connecting to this machine, same as every session
since 6. As with WU-14 (session 15), this session's own implementation was
not verified in the Linux cloud sandbox first (see "Tests" above for why).
Files were written to this machine via the bridge; `git add` and `git
commit` ran through that same bridge and, as in every prior session, could
not clean up its own `index.lock`/`HEAD.lock`/temp-object files afterward
(unlink fails on this mount) — stale ones were moved into `_to_delete/`
rather than removed, on top of whatever was already accumulated there from
earlier sessions; safe to `rm -rf _to_delete/` by hand whenever convenient.

## Next work unit

Build and run WU-15a at the real terminal (see "Build" above and "What to
run" below). If it goes green: tag `wu-15a-green`, update `WORK-UNITS.md`'s
status line, and either start WU-15b (the one-hour unattended run — no new
code, see `DECISIONS.md` ADR-032 and `WORK-UNITS.md`'s own WU-15b entry) or
move on to WU-16 (thread pool, QoS, per-worker bin arenas — Phase 4) if
WU-15b is deferred to run in the background of a later session rather than
blocking the next one. If it goes red: this file's own "Blocked / red"
section is where the failing test and its exact output belong, verbatim,
for the next session to start from — don't reconstruct the fix from
memory.

## Open questions

Unchanged from session 15: Q1 (tile size), Q2 (4K Mini program outputs), Q3
(macOS/Desktop Video version), Q4 (lattice edge damping, C-008(a)) — all
still open, none blocking, nothing this session touched any of them.

New this session: whether `bmdModePALp` is actually supported by the
UltraStudio 4K Mini with `bmdFormat10BitYUV` — unverified (see ADR-032);
`LoopedFramePlayback::create()` is written to fail cleanly rather than crash
if not, so the first real-terminal run itself answers this. Whether
`IDeckLinkOutput::RowBytesForPixelFormat(bmdFormat10BitYUV, 720, ...)`
actually equals `v210::rowBytesMin(720)` on the real hardware — expected
(`bmdFormat10BitYUV` is FourCC `'v210'`) but not yet confirmed;
`test_v210_rowbytes_matches_project_own_computation()` is the first real
check of this.

## Blocked / red

Nothing red — WU-15a has simply not been built yet. Not the same as
blocked: no dependency is missing, only the real-terminal build/run step
itself, per this unit's own "cannot be built or tested in the cloud
sandbox" constraint (same as WU-14).

## Environment check

Unchanged from session 15: the UltraStudio 4K Mini enumerates and is
confirmed full duplex (WU-14). **Still not separately confirmed:** Desktop
Video Setup showing both input and output active, or a capture/playback
round trip in Media Express (architecture.md 10's own Phase 0 checklist).
WU-15a's own accept criterion does not strictly require this (it opens the
output side only, via `EnableVideoOutput`/`DoesSupportVideoMode`, and does
not touch input at all) — but if `DoesSupportVideoMode(bmdModePALp,
bmdFormat10BitYUV)` fails at the real terminal, checking Desktop Video
Setup for the negotiated output format/mode list is the first place to
look, before treating it as a code defect.

## Append to DECISIONS.md

ADR-032 was appended in full earlier this session; see `DECISIONS.md`. Not
reopened or amended now.

## Append to CORRECTIONS.md

Nothing this update — see "Corrections this session" above.

---

## What to run at your terminal

```
cd ~/src/scatter-dve
cmake --build build
ctest --test-dir build -R test_decklink_output --output-on-failure
```

If that configure/build step complains about a stale cache, re-run the
explicit configure WU-14 used:

```
cmake -B build -DBLACKMAGIC_SDK_DIR="/Users/stephenneal/src/Blackmagic DeckLink SDK 16.0"
```

While `test_looped_playback_runs_with_no_dropped_or_late_frames` runs (about
five seconds), point a broadcast monitor — or Media Express's own input
preview — at the UltraStudio 4K Mini's SDI output and confirm by eye that a
warped (cylinder-curved) test pattern actually appears. That visual
confirmation is not something the test itself can assert.

If everything passes and the frame is visible: run the full suite and
`./tools/close.sh 15a` as usual, and let me know the result — I'll update
`WORK-UNITS.md`'s status line and tag `wu-15a-green` from what you report
back, per this project's own "the assistant does not run `close.sh`" rule.

If `bmdModePALp` turns out unsupported (`DoesSupportVideoMode` fails, so the
test's very first `CHECK(bool(playback))` fails cleanly rather than
crashing), tell me and I'll swap in `bmdModePAL` or another progressive
mode as a follow-up, per ADR-032's own documented fallback.
