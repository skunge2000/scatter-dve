# Handoff
Overwritten at the end of every session. This is the first thing to read.

---

**Session:** 48 (WU-23b2 scoping — Weston 3-field de-interlace: live-capture
wiring. Scoping only, no code, per this session's own continuation
prompt's explicit brief).

**Tag:** `wu-23b1-green` is still the newest real tag on the repository
(confirmed this session). This session wrote no code, so there is
nothing new to tag — `DECISIONS.md` (ADR-080), `CORRECTIONS.md` (C-027)
and `WORK-UNITS.md` (WU-23b2 split into WU-23b2a/WU-23b2b) were committed
doc-only, no build/test implications. See "Steve's own next steps" below.

## Before doing anything else in the next session

Run `git tag --sort=creatordate`, `git log --oneline -10`, `git rev-parse
HEAD origin/main` and `git status --short` directly against
`~/src/scatter-dve` (via the device bridge, or at a real terminal) — do
not trust this file's own account without checking it against the real
repository first. **In particular, confirm this session's own commits
actually reached `origin/main`** — see "Steve's own next steps" below;
this session's own device-bridge sandbox has no network egress to
GitHub, so the commits made here are local-only on the Mac until Steve
pushes them himself.

## This session in full

Opened with a continuation prompt whose job was scoping WU-23b2
(live-capture wiring plus the output-side re-interlace decimate) — named
but deliberately left unscoped by the immediately preceding session's own
WU-23b1 build, per ADR-078's "scope the wiring unit after the component
it wires exists" sequencing.

**Repository state, confirmed directly before reading anything else:**
`git tag --sort=creatordate` (newest: `wu-23b1-green`), `git log
--oneline -10` (`HEAD` = `6de0818`, "WU-23b1: Weston 3-field de-interlace
filter core..."), `git rev-parse HEAD origin/main` (both
`6de0818672202330b5d9add212d32394b2f07aef`) — all matching the
continuation prompt's own expected state exactly.

**Handled the one loose end the continuation prompt flagged:**
`git status --short` showed exactly `M CORRECTIONS.md` and `M HANDOFF.md`
(the immediately preceding session's own close-out fix, written to the
real repository after `wu-23b1-green` was already tagged/pushed). Hit
the device bridge's own known `.git/index.lock`/`HEAD.lock` stray-file
problem twice while committing this and while re-checking status
afterward (the bridge's sandbox cannot delete files it creates during its
own git invocations) — moved each stray lock into `_to_delete/` rather
than fighting it, per this project's own standing convention, then
retried. Committed as `982e3e2` ("Session 47 close-out: fix wrong
git-add path in HANDOFF.md, log C-026"). **`git push origin main` failed:
this session's own device-bridge sandbox has no network egress to
GitHub** (`403 from proxy after CONNECT`, confirmed on a second attempt,
not transient) — a new, previously-unencountered limitation of this
particular sandbox, distinct from the already-known index-lock problem.
`982e3e2` is a real commit on the Mac's own `main` branch but is not yet
on `origin`. See "Steve's own next steps" below — this needs a manual
push before this session's own new commit (below) goes on top of it.

**Read directly, not from memory or paraphrase, before proposing any
design** (continuation prompt's own five-item list): `io/decklink_capture_consumer.hpp`/`.cpp`,
`video/deinterlace.hpp`, `core/resolve.hpp`, `core/pipeline.cpp`
(specifically `runFrameBytes()`, lines ~630-680, and `runFrameField()` as
its own precedent for a new orchestration entry point), `video/interlace.hpp`/`.cpp`,
and `docs/architecture.md` sections 3 and 5.

**Found a real gap in WU-23b2's own prior (pre-split) scoping stub,
logged as `CORRECTIONS.md` C-027:** `CaptureConsumer::processOne()`
cannot call `Deinterlacer::push()` "ahead of the existing warp" as that
stub assumed — `processOne()` calls `scatter::runFrameBytes()` exactly
once, and that function's own chroma-upsampled weave `Raster444` (the
exact shape `push()` needs) is a local variable, never exposed to any
caller. The real trigger for needing a new `core/resolve.hpp`/
`core/pipeline.cpp` orchestration entry point is this, not (as the stub
guessed) the output-side decimate's own complexity — which turned out to
be a provable no-op (below).

**Settled all three of ADR-078's own open design questions, `DECISIONS.md`
ADR-080:**

1. **A new orchestration entry point is required, and does not fit
   inside `io/decklink_capture_consumer.cpp` alone.** New sibling
   function `bool runFrameBytesDeinterlaced(video::Deinterlacer&,
   const Lattice&, const std::uint8_t* srcBytes, std::ptrdiff_t
   srcRowBytes, int srcWidth, int srcHeight, const PipelineParams&,
   std::uint8_t* dstBytes, std::ptrdiff_t dstRowBytes)`, declared in
   `core/resolve.hpp`, defined in `core/pipeline.cpp` — the same
   "declare a new `.cpp`'s public entry point in an existing, related
   header" shape `runFrameField()` already used. `Deinterlacer` is a
   reference parameter, not folded into `PipelineParams` (which every
   non-interlaced caller in this codebase also uses).
2. **Output-side "[re-interlace]" is a provable no-op, not new code.**
   `extractField()`+`interleaveFields()`, re-derived directly from
   `video/interlace.cpp`'s own row-index arithmetic, reproduce their
   input exactly when applied to the same source frame in sequence — an
   algebraic identity, not an approximation — because
   `docs/architecture.md` section 5 frames de-interlace-to-frame and
   field mode as *alternative* processing paths, never combined, so this
   project's own frame-rate-only mode never has two independently-warped
   fields to recombine on the output side. `runFrameBytesDeinterlaced()`
   sends `runFrame()`'s own `warped` output straight to chroma
   downsample, with a comment citing ADR-080 for why this is correct,
   not a corner cut.
3. **One `Deinterlacer` instance, not two.** A single instance already
   produces one complete progressive frame per push (anchor parity
   verbatim, the other reconstructed); a second, opposite-anchor
   instance would only matter for field-rate output, already ruled out
   by ADR-078. `CaptureConsumer` gets exactly one member, anchored
   `FieldParity::Top` (this project's own existing convention;
   `bmdModePAL`'s real field dominance has never been independently
   confirmed against hardware, but which parity is "anchor" is a
   labelling choice, not a correctness one).
4. **Stream-start behaviour.** Traced `Deinterlacer::push()`'s own state
   machine precisely: only the very first push of a given instance's own
   lifetime ever returns `false` — a one-time, once-per-`CaptureConsumer`
   event, not a recurring cost. Decision: leave `m_latestFrame` untouched
   on that one frame (extending, not inventing, `copyLatestFrame()`'s
   own existing "nothing produced yet" semantics) and skip the
   warp/splat/resolve pipeline for it entirely, which
   `runFrameBytesDeinterlaced()`'s own `false`-return-before-touching-
   `runFrame()` behaviour already guarantees. Counted by a new, fourth
   `CaptureConsumerStats` counter (name left for the build session), not
   `framesFailed` — this is not an error.

**Gave WU-23b2 real `Files:`/`Accept:` lines, splitting it** (confirmed
necessary: the full wiring touches four source files, one over
`SESSION-PROTOCOL.md`'s 3-file cap) into:

- **WU-23b2a** — `runFrameBytesDeinterlaced()`: `core/resolve.hpp`,
  `core/pipeline.cpp` (2 files), plus `tests/test_pipeline_bytes.cpp`
  (edited, exempt from the cap). No DeckLink dependency — portable,
  buildable next.
- **WU-23b2b** — `CaptureConsumer` wiring: `io/decklink_capture_consumer.hpp`/`.cpp`
  (2 files), plus `tests/test_decklink_capture_consumer.cpp` (edited,
  exempt). Gated on WU-23b2a's own actual interface once built;
  real-hardware-gated for its own `Accept:` line.

Both units' own `Accept:` lines target 576i25/625i50 explicitly, not
1080i — Steve's own stay-in-SD-domain scope decision, reiterated once
more here per that decision's own standing "don't let it carry forward
silently" request.

**Wrote `DECISIONS.md` (ADR-080), `CORRECTIONS.md` (C-027) and
`WORK-UNITS.md` (WU-23b2 split into WU-23b2a/WU-23b2b, and the Phase-6
summary paragraph updated) to the real repository via the device
bridge, then re-staged all three from the device and diffed against this
session's own edited copies before writing this sentence —
`SESSION-PROTOCOL.md`'s own rule 8.** This `HANDOFF.md` is written the
same way. All four files are staged as local, uncommitted changes on the
Mac as of this sentence — see "Steve's own next steps" below for the
commit this session's own close-out block hands him, on top of the
still-unpushed `982e3e2` from the loose-end paragraph above.

## Where we are

**WU-23b2 is scoped, not built** — `DECISIONS.md` now runs through
ADR-080; `WORK-UNITS.md`'s WU-23b2 entry is replaced by WU-23b2a (`todo`)
and WU-23b2b (`todo`, blocked on WU-23b2a); `INVARIANTS.md` unchanged
through I11; `CORRECTIONS.md` now runs through C-027. No code exists yet
for either new unit.

## Next work unit

**WU-23b2a** (`runFrameBytesDeinterlaced()`, `core/resolve.hpp`/
`core/pipeline.cpp`) is the natural next pick — fully scoped this
session (`DECISIONS.md` ADR-080, `WORK-UNITS.md`), no DeckLink
dependency, genuinely depends on nothing but WU-23b1's own already-built
`video::Deinterlacer`. WU-23b2b (`CaptureConsumer` wiring) is scoped but
blocked on WU-23b2a's own actual interface once built — do not start it
first. Everything named in Session 43's own "Next work unit" section
(WU-28d, WU-27, WU-33, WU-35, WU-37) is unchanged and still pickable if
the interlace thread is set aside instead.

**Steve's own explicit stay-in-SD-domain scope decision (WU-24/WU-25
skipped until he says otherwise) is unchanged and still in force** —
carried forward unmodified this session, reiterated here per that
decision's own standing request not to let it be lost silently.

## Open questions

Unchanged from Session 42/43's own list (`kCaptureRingCapacity`, Q3, Q4,
Task A1, Task D6, ADR-070's open question, WU-35's `compositeLayered()`
question) — this session did not touch any of them. WU-23b2's own
stream-start question (ADR-078) is now resolved (ADR-080, above), not
open any longer. One new item: `DeinterlaceCoefficients` (Simple vs
Complex) for `CaptureConsumer`'s own new constructor parameter is
deliberately left undecided (ADR-080) — WU-23b2b's own build session
should raise it with Steve rather than default it silently.

## Blocked / red

Nothing red. WU-23b2b is blocked on WU-23b2a by design (not a problem —
the same sequencing WU-23a2b was blocked on WU-23a2a). **New this
session: this session's own commit (`982e3e2`, the prior session's
loose-end fix) is on the Mac's own local `main` but not yet on
`origin`** — the device-bridge sandbox this session ran in has no
network egress to GitHub. Not a repository problem, a sandbox
limitation; Steve's own real terminal has ordinary network access and
just needs to push. See "Steve's own next steps" below.

## Environment check

This session did no building or testing — scoping only, reading state
files and source directly via the device bridge. The standing condition
from prior sessions (C-024: `tools/close.sh` cannot currently succeed
for any unit, on Steve's real terminal, because of the PSU/two-device-
architecture mismatch — `DECISIONS.md` ADR-034/035/037, `CORRECTIONS.md`
C-024) is unchanged and unaffected — nothing this session did touches it
either way. **New this session, worth carrying forward explicitly: the
device-bridge sandbox has no network egress to GitHub** (confirmed
directly, not assumed — a real `git push` attempt, twice, both failing
identically with a proxy 403) — any future session that commits
doc-only or code changes via this bridge should expect the same and plan
to hand Steve an explicit `git push` step, not assume the bridge can
push on its own the way it can commit.

## Append to DECISIONS.md

**ADR-080** — already appended in full this session (WU-23b2 scoping:
split into WU-23b2a/WU-23b2b; `runFrameBytesDeinterlaced()` design;
stream-start, single-instance and output re-interlace-is-a-no-op
questions all settled). See `DECISIONS.md`.

## Append to CORRECTIONS.md

**C-027** — already appended in full this session (WU-23b2's own prior
scoping stub wrongly assumed `processOne()` could call `Deinterlacer::push()`
directly and that a third file's necessity hinged on the output decimate's
own complexity; corrected, full account in `DECISIONS.md` ADR-080). See
`CORRECTIONS.md`.

## Closed out this session

**WU-23b2 scoping.** `DECISIONS.md` ADR-080, `CORRECTIONS.md` C-027,
`WORK-UNITS.md`'s WU-23b2 entry replaced by WU-23b2a/WU-23b2b with real
`Files:`/`Accept:` lines, this `HANDOFF.md`. No source code changed —
scoping only, per this session's own explicit brief. Also closed out the
immediately preceding session's own loose end: `CORRECTIONS.md`/`HANDOFF.md`
committed as `982e3e2` (not yet pushed — see below).

## Steve's own next steps

No build or test implications this session — nothing to build, nothing
to run. Two things need doing at your own real terminal, in order: push
this session's own commit (doc-only, no tag), then push the still-local
`982e3e2` from the immediately preceding session's own loose end (the
same commit, both travel together since neither has reached `origin`
yet):

```
cd ~/src/scatter-dve
git status --short
```

If that shows exactly `M CORRECTIONS.md`, `M DECISIONS.md`,
`M HANDOFF.md` and `M WORK-UNITS.md` and nothing else (this session's own
four state-file edits, on top of the already-committed `982e3e2`):

```
cd ~/src/scatter-dve
git add CORRECTIONS.md DECISIONS.md HANDOFF.md WORK-UNITS.md
git commit -m "WU-23b2 scoping: split into WU-23b2a/WU-23b2b (ADR-080), corrects prior wiring assumption (C-027)"
git push origin main
```

(No `git tag` — this is a doc-only scoping session, nothing built or
tested changed, `SESSION-PROTOCOL.md`'s own tagging discipline applies
only to units that end green.) The `git push origin main` above carries
both this session's own new commit and the still-unpushed `982e3e2`
ahead of it in one push, since neither has reached `origin` yet.

**File paths above match this session's own real `git status --short`,
re-diffed immediately before writing this block, not merely recalled
from earlier in the session** — `CORRECTIONS.md` C-026's own general
lesson, applied here.
