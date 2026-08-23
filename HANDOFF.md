# Handoff
Overwritten at the end of every session. This is the first thing to read.

---

**Session:** 46 (WU-23b scoping — Weston 3-field de-interlace: source
confirmed, multi-frame-history question resolved, unit split into
WU-23b1/WU-23b2; no code this session).

**Tag:** `wu-23a2b-green` is real, confirmed on the real repository this
session (see below) — Steve's own tag/push from Session 45 already
landed. Nothing to tag this session; this was scoping only.

## Before doing anything else in the next session

Run `git tag --sort=creatordate`, `git log --oneline -10`, `git status
--short` and `git status -sb` directly against `~/src/scatter-dve` via
the device bridge, the same as every session before this one — do not
trust this file's own account of tag/commit state without checking it
against the real repository first.

## This session in full

Opened with a continuation prompt whose own job was scoping only, per
`SESSION-PROTOCOL.md`'s "one session, one work unit" sizing: confirm the
real Weston 3-field source, resolve the multi-frame-history question
Session 45's own handoff and `DECISIONS.md` ADR-075 left open, and give
WU-23b real `Files:`/`Accept:` lines (splitting further if needed). No
code written this session, as instructed.

**Repository state, confirmed directly before reading anything else:**
`git tag --sort=creatordate` (newest: `wu-23a2b-green`), `git log
--oneline -10` (`HEAD` = `6855c38`, "WU-23a2b: field mode's own
runFrame()-level driver, runFrameField() (ADR-077)"), `git status -sb`
(`## main...origin/main`, clean, no ahead/behind) — all run directly
against `~/src/scatter-dve` via the device bridge. Matches the
continuation prompt's own expected state exactly; **`HANDOFF.md`'s own
prior "Tag: none yet" line was stale** (Steve's own real-terminal tag
and push from Session 45 had already landed by the time this session
opened) — not a `CORRECTIONS.md`-worthy error, just this file's own
overwrite-each-session nature catching up.

**Source confirmed.** Fetched and read `libavfilter/vf_w3fdif.c`
directly (not assumed from the continuation prompt's own summary):
Weston 3-field, BBC R&D's algorithm (Jim Easterbrook's implementation of
Martin Weston's process), FFmpeg filter by Mark Himsley — confirmed, not
`vf_bwdif.c`. Every element of the continuation prompt's own algorithm
summary checked out against the real source: both coefficient sets
(simple 2+3 taps, complex 4+5 taps, scaled 2^15), the reflect-based edge
handling, uniform plane treatment, frame-rate vs field-rate output
modes, and the explicit `prev`/`cur`/`next` sliding-window driving
model. See `DECISIONS.md` ADR-078 for the full account.

**Multi-frame-history question resolved.** Read
`src/io/decklink_capture_consumer.hpp`/`.cpp` and `core/resolve.hpp`
directly: no persistent cross-frame state exists anywhere in this
project today. `CaptureConsumer::processOne()` is stateless per call
except for the current lattice and the most recently produced *output*
frame — no source-side history. Every `core/resolve.hpp` entry point
(`runFrame()`/`runFrameBytes()`/`runFrameFile()`/`runFrameField()`) is a
stateless free function. `core/ring_buffer.hpp`'s `RingBuffer` (WU-20a)
is the nearest precedent — a generic, platform-independent, reusable
component a DeckLink-specific caller later owns as a member — but it is
a cross-thread handle queue, not a filter's own sliding-window state, so
it is a precedent for *how* to build the new component, not something
WU-23b reuses directly. **Decision: a new standalone stateful
component, `video::Deinterlacer` (`video/deinterlace.hpp`/`.cpp`, new,
not yet built), owning its own three-field history internally**, placed
in `video/` (alongside `interlace.hpp`, `chroma.hpp`) rather than
`core/`, since it is video-format-level processing with no lattice/warp
involvement — reusable identically by the live-capture path (as a new
owned member of `CaptureConsumer`) and any future file-sequence driver.
See ADR-078 for the full reasoning, including the integer-arithmetic/
edge-convention recommendation (signed 64-bit accumulation, this
project's own round-half-up descale idiom, w3fdif's own reflect-edge
convention adopted as-is and scoped narrowly to this one filter) and the
open stream-start question left for WU-23b2.

**Unit split, confirmed necessary against the 3-source-file cap, not
assumed going in.** The filter's own core math and history buffer
(`video/deinterlace.hpp`/`.cpp`, 2 files, plus its own test) is
self-contained and independently testable, no DeckLink dependency.
Wiring it into the live-capture path needs at minimum
`io/decklink_capture_consumer.hpp`/`.cpp`, likely also
`core/resolve.hpp`/`core/pipeline.cpp` depending on how the output-side
re-interlace decimate ends up shaped — a real design question left for
that unit's own scoping session, not guessed here. Split into
**WU-23b1** (filter core, given real `Files:`/`Accept:` this session,
ready to build next) and **WU-23b2** (live-capture wiring plus the
output-side re-interlace decimate, not yet scoped — genuinely depends on
WU-23b1's own actual interface once built).

Wrote `DECISIONS.md` ADR-078 (full design/scoping account),
`WORK-UNITS.md` (replaced the single WU-23b stub with real WU-23b1
`Files:`/`Accept:` lines and a WU-23b2 placeholder), and this file to
the real repository via the device bridge, then re-staged all three from
the device and diffed against this session's own edited copies before
writing this sentence — `SESSION-PROTOCOL.md`'s own rule 8.

## Where we are

**Phase 6's own field-mode thread (WU-23a/WU-23a2a/WU-23a2b) is
complete and tagged** (`wu-23a2b-green`, confirmed real this session).
**WU-23b is now scoped, not built:** WU-23b1 (`video::Deinterlacer`
filter core) has real `Files:`/`Accept:` lines and is the natural next
pick; WU-23b2 (live-capture wiring) is named but deliberately not
scoped yet. `WU-24`/`WU-25` untouched. `DECISIONS.md` now runs through
ADR-078; `INVARIANTS.md` unchanged through I11; `CORRECTIONS.md`
unchanged through C-024 (nothing this session rose to a correction).

## Next work unit

**WU-23b1** (`video::Deinterlacer` — Weston 3-field filter core,
`video/deinterlace.hpp`/`.cpp`, `tests/test_deinterlace.cpp`) is the
natural next pick: fully scoped this session (`DECISIONS.md` ADR-078,
`WORK-UNITS.md`), no DeckLink dependency, buildable and testable
entirely in the cloud sandbox the same way WU-23a was. Re-verify the
coefficient sets and line-offset structure directly against the real
`libavfilter/vf_w3fdif.c` source again before writing code — this
session's own confirmation is not a substitute for that at build time,
the same "do not rely on recall" discipline (`SESSION-PROTOCOL.md` rule
6) every prior session has applied to its own prior findings.
Everything named in Session 43's own "Next work unit" section (WU-28d,
WU-27, WU-33, WU-35, WU-37) is unchanged and still pickable if the
interlace thread is set aside instead.

## Open questions

Unchanged from Session 42/43's own list (`kCaptureRingCapacity`, Q3, Q4,
Task A1, Task D6, ADR-070's open question, WU-35's `compositeLayered()`
question) — this session did not touch any of them. **One new item,
named but not resolved by this session (ADR-078):** WU-23b2's own
stream-start question — what `video::Deinterlacer` should do (or what
`CaptureConsumer` should do with its output) for the first field of a
capture run, before two frames of history exist to reconstruct from. Not
answered here; a real design question for WU-23b2's own scoping.

## Blocked / red

Nothing red. Nothing newly blocked.

## Environment check

This session did no build/test work at all — scoping and documentation
only, confirmed via `git`/file reads against the real repository through
the device bridge. The standing condition from Session 43/44/45 (C-024:
`tools/close.sh` cannot currently succeed for any unit, on Steve's real
terminal, because of the PSU/two-device-architecture mismatch — see
`DECISIONS.md` ADR-034/035/037 and `CORRECTIONS.md` C-024) is unchanged
and not relevant this session, since nothing was built or tagged.

## Append to DECISIONS.md

**ADR-078** — already appended in full this session (WU-23b scoping:
Weston 3-field source confirmed; multi-frame-history resolved to
`video::Deinterlacer`; split into WU-23b1/WU-23b2). See `DECISIONS.md`.

## Append to CORRECTIONS.md

Nothing this session. (The stale "Tag: none yet" line in the prior
`HANDOFF.md` was caught and corrected by this file's own overwrite, per
`SESSION-PROTOCOL.md`'s own design for that file — not a
`CORRECTIONS.md`-worthy error, since nothing this project *claimed* as a
technical fact was shown wrong.)

## Closed out this session

**WU-23b scoping.** No code. Real `Files:`/`Accept:` lines for WU-23b1
written into `WORK-UNITS.md`; the full design account in `DECISIONS.md`
ADR-078; WU-23b2 named but deliberately left unscoped. This is a
complete, legitimate session outcome per `SESSION-PROTOCOL.md`'s own
"one session, one work unit" sizing, applied here to a unit whose real
work this session was confirming the source, resolving the architectural
question, and designing — not a formality rushed past to reach code.

## Steve's own next steps

**Nothing to build, test, tag or push this session — this was scoping
only.** Confirm the tree reflects only documentation changes:

```
cd ~/src/scatter-dve
git status --short
```

Expected: `DECISIONS.md`, `WORK-UNITS.md` and `HANDOFF.md` modified.
Nothing else — no source or test files touched.

If you're happy with the WU-23b1/WU-23b2 split and the
`video::Deinterlacer` design in `DECISIONS.md` ADR-078, commit this
scoping work (no tag — nothing was built or tested, so there is nothing
for a `wu-NN-green` tag to certify):

```
cd ~/src/scatter-dve
git add DECISIONS.md WORK-UNITS.md HANDOFF.md
git commit -m "WU-23b scoping: Weston 3-field de-interlace source confirmed, video::Deinterlacer design, split into WU-23b1/WU-23b2 (ADR-078)"
git push origin main
```

No `git tag` step this time — that's for a session that actually builds
and verifies code, next time.
