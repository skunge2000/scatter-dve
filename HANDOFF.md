# Handoff
Overwritten at the end of every session. This is the first thing to read.

---

**Session:** 50 (WU-23b2b build — Weston 3-field de-interlace:
`CaptureConsumer` wiring).

**Tag:** `wu-23b2a-green` is still the newest real tag on the repository
as of this session's own start (confirmed directly). This session's own
new code was written with no compiler anywhere in this session's own
reach able to see it (see below) — Steve's own real-terminal build has
since run twice: the first `cmake --build build` failed (C-028, fixed),
the first `ctest` after that failed two tests on a stale accounting
invariant this session had not updated (C-029, fixed). **Neither fix has
itself been rebuilt/retested yet as of this file being written — that is
still Steve's own next step, below.** Nothing is tagged this session.

## Before doing anything else in the next session

Run `git tag --sort=creatordate`, `git log --oneline -10`, `git rev-parse
HEAD origin/main` and `git status --short` directly against
`~/src/scatter-dve` — do not trust this file's own account without
checking it against the real repository first.

## This unit is DeckLink-linked — there was no cloud-sandbox build for it, at all

**This session itself could not compile or test any of its own code** —
`io/decklink_capture_consumer.hpp`/`.cpp` link the Blackmagic DeckLink
SDK, which exists only on the Mac. Every line was written by reading the
real, current file contents directly, then reasoning against
`core/resolve.hpp`'s own `runFrameBytesDeinterlaced()` (WU-23b2a, built
and tagged) and `video/deinterlace.hpp`'s own `Deinterlacer` class
exactly as frozen — not by compiling and iterating. **Updated mid-session
as Steve's own real-terminal runs came back:** `cmake --build build` now
compiles clean (after C-028's fix); `ctest` has not yet been confirmed
green (C-029's fix addresses the two failures seen so far, but is itself
unverified — see "Steve's own next steps"). Do not claim this code is
fully green/verified until Steve confirms the re-run below.

## This session in full

Opened with a continuation prompt whose own job was building WU-23b2b.
Confirmed repository state directly before reading anything else: `git
tag --sort=creatordate` (newest `wu-23b2a-green`, matching the
continuation prompt's own expected state exactly), `git log --oneline
-10` (`HEAD` = `347ffd6`, "WU-23b2a: runFrameBytesDeinterlaced()
orchestration entry point (ADR-080)"), `git rev-parse HEAD origin/main`
(same hash twice, `347ffd6ab3ad5955de21708ef63120af680ef73f`), `git
status --short` (clean). No drift from the continuation prompt's own
snapshot — nothing to reconcile.

**Read directly, not from memory or paraphrase, before writing anything:**
`SESSION-PROTOCOL.md`, `HANDOFF.md`, `INVARIANTS.md`, `DECISIONS.md`
(ADR-080 in full), `CORRECTIONS.md` (C-026/C-027 in full),
`WORK-UNITS.md` (WU-23b2b's own entry in full),
`io/decklink_capture_consumer.hpp`, `io/decklink_capture_consumer.cpp`,
`core/resolve.hpp` (`runFrameBytesDeinterlaced()`'s own declaration and
its full comment block), `core/pipeline.cpp` (its definition, to confirm
exactly when `dstBytes`/`weightOut` are and are not touched on a `false`
return), `video/deinterlace.hpp` (`Deinterlacer`'s own constructor and
`push()` signature), `video/interlace.hpp` (`FieldParity`), and
`tests/test_decklink_capture_consumer.cpp`'s own current accounting
invariant.

**Two open design points ADR-080 left for this session, both settled and
logged as `DECISIONS.md` ADR-081 (one entry, covering both — the two are
small build-time details of the same unit, not separate architectural
questions):**

1. **`DeinterlaceCoefficients`: raised with Steve directly, per ADR-080's
   own instruction not to default it silently. Steve's answer: `Complex`**
   (4 low-pass + 5 high-pass taps, not `Simple`'s 2+3). `CaptureConsumer`'s
   constructor takes it as a required parameter, no default.
2. **`processOne()`'s three now-possible outcomes reach `run()` via a new
   private `ProcessResult` enum class** (`Processed`/`Failed`/
   `StreamStart`), replacing the old plain `bool` return — picked over a
   second `bool` (rejected: an unreachable fourth combination to reason
   about at every call site) or an out-parameter (rejected: the three
   outcomes are mutually exclusive alternatives, not a primary result plus
   optional detail). The new `CaptureConsumerStats` counter is named
   `framesStreamStart`. Full reasoning in `DECISIONS.md` ADR-081.

**Built (written) WU-23b2b exactly per ADR-080, plus the two ADR-081
decisions above — no other redesign:**

- `io/decklink_capture_consumer.hpp`: new `#include
  "video/deinterlace.hpp"`; `CaptureConsumerStats` gains
  `framesStreamStart`; new private `ProcessResult` enum class;
  `processOne()`'s declared return type changes `bool` →
  `ProcessResult`; new owned `video::Deinterlacer m_deinterlacer` member
  (constructed `FieldParity::Top`, caller's `coeffs`); constructor gains
  a required `video::DeinterlaceCoefficients coeffs` parameter, inserted
  before the already-defaulted `coverageCallback` parameter (so
  `coverageCallback` stays defaultable and `coeffs` cannot be skipped).
- `io/decklink_capture_consumer.cpp`: constructor initialises
  `m_deinterlacer(video::FieldParity::Top, coeffs)`; `run()`'s
  processed-xor-failed `if`/`else` becomes a three-case `switch` on
  `ProcessResult`; `processOne()`'s body is unchanged through
  `StartAccess`/`GetBytes`, then calls
  `scatter::runFrameBytesDeinterlaced(m_deinterlacer, latticeSnapshot,
  ...)` in place of `scatter::runFrameBytes(...)`, checks its `bool`
  return, still calls `EndAccess` unconditionally first; on `EndAccess`
  failure returns `ProcessResult::Failed` as before; on a `false`
  (stream-start) return with `EndAccess` successful, returns
  `ProcessResult::StreamStart` **without** touching `m_latestFrame` and
  **without** firing the coverage callback (confirmed by reading
  `core/pipeline.cpp`'s `runFrameBytesDeinterlaced()` body directly:
  it returns before `runFrame()` is ever called on that path, so neither
  `dstBytes` nor `weightOut`/coverage is ever written); otherwise
  publishes `m_latestFrame`, fires the coverage callback exactly as
  before, and returns `ProcessResult::Processed`.
- `tests/test_decklink_capture_consumer.cpp`: added `#include
  "video/deinterlace.hpp"`; constructor call now passes
  `scatter::video::DeinterlaceCoefficients::Complex` explicitly; the
  `framesProcessed + framesFailed == framesPopped` invariant widened to a
  third term, `framesStreamStart`; two new checks —
  `framesStreamStart <= 1` unconditionally, and `framesStreamStart == 1`
  whenever `framesProcessed > 0` (reasoning: the first popped frame that
  ever survives `QueryInterface`/`StartAccess`/`GetBytes` is
  unconditionally the one and only call to `m_deinterlacer.push()` that
  can ever return `false`, so any processed frame implies exactly one
  prior stream-start); the diagnostic `fprintf` now also prints
  `framesStreamStart`.

**Member initialisation order double-checked against declaration
order** (a real risk with a new non-trivially-constructed member,
`video::Deinterlacer` has no default constructor): `m_deinterlacer` is
declared after `m_coverageCallback` and before `m_thread` in the header,
and the constructor's initialiser list follows that same order — no
`-Wreorder` risk, verified by inspection since there is no compiler here
to catch it.

**Wrote `src/io/decklink_capture_consumer.hpp`,
`src/io/decklink_capture_consumer.cpp`,
`tests/test_decklink_capture_consumer.cpp` and `WORK-UNITS.md` to the
real repository via the device bridge, then confirmed each byte-for-byte
against this session's own edited copies via SHA-256 checksum through
the device bridge's own shell access** (the usual re-stage-and-diff path
was blocked this session by a stale device sign-in — `device_stage_files`
returned `untrusted_device`; checksums through `device_bash` served the
same confirming role) — all four matched exactly.

**Correction mid-session, logged as `CORRECTIONS.md` C-028: Steve's own
real-terminal `cmake --build build` failed.** `CaptureConsumer`'s
constructor signature changed (new required `DeinterlaceCoefficients`
parameter), but this session's own close-out had only updated the one
call site `WORK-UNITS.md`'s `Files:` line named
(`tests/test_decklink_capture_consumer.cpp`) — two more real
construction sites existed and were never searched for:
`tests/test_decklink_live_output.cpp:136` and
`tests/test_decklink_live_sphere.cpp:565`. Both fixed in this same
session once Steve reported the compiler errors: both now pass
`scatter::video::DeinterlaceCoefficients::Complex` explicitly (matching
ADR-081), `test_decklink_live_sphere.cpp`'s own call also reordered so
the pre-existing `coverageCallback` argument moves to its own new,
later position after `coeffs`. Both written to the real repository and
checksum-confirmed byte for byte the same way as the first four files.
See `CORRECTIONS.md` C-028 for the full account and the general lesson
(grep the whole repository for every real call site before closing out
a session that changes a public constructor/function signature — a
`Files:` line written during an earlier *scoping* session is a plan, not
a fact already checked against the real tree).

**Second correction, same session, logged as `CORRECTIONS.md` C-029:
Steve's own re-run build succeeded, but `ctest` then failed exactly two
tests** -- `test_decklink_live_output` and `test_decklink_live_sphere`,
both on the same stale `CHECK`:
`framesProcessed + framesFailed == framesPopped`, missing the third term
`framesStreamStart` this session's own WU-23b2b work had already added
to `test_decklink_capture_consumer.cpp` but never propagated to these
other two files' own independent copies of the same invariant. The C-028
fix's own header-comment claim ("this test never reads
`CaptureConsumerStats::framesStreamStart`") was flatly wrong -- both
files read it implicitly through this very `CHECK`, and Steve's own real
counts proved it (`test_decklink_live_output`: `framesPopped=82`,
`framesProcessed=81`, `framesFailed=0`, the missing 1 being the
stream-start frame; `test_decklink_live_sphere`: same pattern,
`framesPopped=420`/`framesProcessed=419`). Fixed the same session: both
`CHECK`s widened to the same three-term form
`test_decklink_capture_consumer.cpp` already uses, both `fprintf`
diagnostics extended to print `framesStreamStart`, and the wrong header
comment in both files corrected in place. Both written to the real
repository and checksum-confirmed. **General lesson, sharpening C-028's
own:** grepping for a changed function's own name only catches sites
that fail to *compile* -- a behavioural change to a class's observable
state (a new outcome moving population from one counter to another) can
silently break a caller's own runtime invariant even at a call site that
compiles cleanly. The real check is "does every reader of the changed
class's own state still hold correct assumptions," not just "does every
caller still compile" -- this session needed a second, real `ctest`
failure to find that out, and should have grepped for
`framesProcessed`/`framesFailed` (the touched counters) repository-wide
the first time, not just for `CaptureConsumer(` (the changed
constructor).

`git status --short` against the real repository now shows exactly `M
CORRECTIONS.md`, `M DECISIONS.md`, `M WORK-UNITS.md`, `M
src/io/decklink_capture_consumer.cpp`, `M
src/io/decklink_capture_consumer.hpp`, `M
tests/test_decklink_capture_consumer.cpp`, `M
tests/test_decklink_live_output.cpp`, `M
tests/test_decklink_live_sphere.cpp`, plus this `HANDOFF.md`, and
nothing else — confirmed immediately before finalising this block, not
earlier in the session.

## Where we are

**WU-23b2b is written; build succeeds; ctest previously failed two
tests on a stale invariant, now fixed but not yet re-verified.**
`io/decklink_capture_consumer.hpp`/`.cpp` carry the new
`ProcessResult`-returning `processOne()`, the owned `m_deinterlacer`
member and the `Complex`-coefficients constructor parameter;
`tests/test_decklink_capture_consumer.cpp`,
`tests/test_decklink_live_output.cpp` and
`tests/test_decklink_live_sphere.cpp` all now carry the same widened,
three-term accounting invariant and construct `CaptureConsumer` with
`DeinterlaceCoefficients::Complex` explicitly. `WORK-UNITS.md`'s
WU-23b2b entry and the Phase 6 summary paragraph above WU-23b1 both
reflect the design (not yet the build/ctest history, below).
`DECISIONS.md` now carries ADR-081 (both open points from ADR-080,
settled). `CORRECTIONS.md` now carries C-028 (the two missed call sites
that broke the build) and C-029 (the same two files' own stale
accounting invariant that then broke `ctest`) — otherwise unchanged
through C-027; nothing ADR-080 itself assumed about
`io/decklink_capture_consumer.hpp`/`.cpp`'s own shape turned out wrong.
`INVARIANTS.md` unchanged through I11. All nine changed/created files
are staged as local, uncommitted changes on the
Mac as of this sentence — see "Steve's own next steps" below. **Not yet
re-confirmed against a real build/test run** — the C-029 fix (and C-028's
own fix, which did compile cleanly the second time but was never
retested after C-029's own further edit to the same two files) has not
itself been rebuilt or rerun; re-run both below before trusting either.

## Next work unit

**WU-23b2b's own real-hardware `Accept:` line still needs Steve's own
Monitor 3G → Recorder 3G loopback run** (see below) before this thread
can be called done. Once that lands and is tagged `wu-23b2b-green`, the
Weston 3-field de-interlace thread (WU-23b1/WU-23b2a/WU-23b2b) is
complete. Everything named in Session 43's own "Next work unit" section
(WU-28d, WU-27, WU-33, WU-35, WU-37) is unchanged and still pickable.
**Steve's own explicit stay-in-SD-domain scope decision (WU-24/WU-25
skipped until he says otherwise) is unchanged and still in force.**

## Open questions

Unchanged from Session 42/43's own list (`kCaptureRingCapacity`, Q3, Q4,
Task A1, Task D6, ADR-070's own open question, WU-35's
`compositeLayered()` question) — this session did not touch any of them.
Both of WU-23b2b's own open points (`DeinterlaceCoefficients`, the
`processOne()`/`run()` outcome mechanism) are now closed — see ADR-081.

## Blocked / red

`ctest` was red twice this session on Steve's own real terminal (C-028's
own build failure; C-029's own two `CHECK` failures after the build
fix), both now source-fixed but neither re-verified. Not otherwise
blocked.

## Environment check

This session had no DeckLink-capable compiler anywhere in its own reach
(see above) — the device-bridge sandbox used for git/file operations
(distinct from any Linux cloud sandbox) had ordinary read/write access
this session, no `.git/index.lock` stray files encountered, `git push`
was not attempted (nothing pushed this session). C-024's standing
condition (PSU out, `tools/close.sh` cannot succeed for any unit on
Steve's own real terminal because of the PSU/two-device-architecture
mismatch) is unchanged and unaffected by this session.

## Append to DECISIONS.md

**ADR-081**, appended this session — see above and the real
`DECISIONS.md` for the full text. Covers both of ADR-080's own open
points for this unit: `DeinterlaceCoefficients::Complex` (Steve's
choice) and the `ProcessResult` enum class mechanism (with the
`framesStreamStart` counter name).

## Append to CORRECTIONS.md

**C-028**, appended this session — see above and the real
`CORRECTIONS.md` for the full text. `WORK-UNITS.md`'s WU-23b2b `Files:`
line undercounted the real blast radius of changing `CaptureConsumer`'s
constructor signature: two more real call sites
(`tests/test_decklink_live_output.cpp`, `tests/test_decklink_live_sphere.cpp`)
existed outside its own named test file and broke Steve's own
real-terminal build. Fixed the same session; general lesson logged for
future signature changes.

**C-029**, appended this session — see above and the real
`CORRECTIONS.md` for the full text. The C-028 fix only addressed the
compile error; both of the same two files also carry their own copy of
the `framesProcessed + framesFailed == framesPopped` accounting
invariant, which the new `framesStreamStart` outcome genuinely
invalidates — caught by Steve's own real `ctest` run, not by this
session. Fixed the same session; sharper general lesson logged
(grep for the touched *state*, not just the changed *signature*, after a
behavioural change to an existing state machine).

## Closed out this session

**WU-23b2b, written; build confirmed clean, ctest not yet re-confirmed.**
`io/decklink_capture_consumer.hpp` (new include, new
`framesStreamStart` counter, new private `ProcessResult` enum, new owned
`Deinterlacer` member, new required `coeffs` constructor parameter),
`io/decklink_capture_consumer.cpp` (constructor, `run()`,
`processOne()`), `tests/test_decklink_capture_consumer.cpp` (new
include, `Complex` passed explicitly, widened accounting invariant, two
new stream-start checks). Also, two rounds of correcting real-terminal
failures Steve reported: `tests/test_decklink_live_output.cpp` and
`tests/test_decklink_live_sphere.cpp`, first updated to pass
`DeinterlaceCoefficients::Complex` explicitly at their own
`CaptureConsumer` construction call (C-028, fixed a build error), then
updated again to widen their own copy of the accounting invariant to the
same three-term form and correct a now-false header-comment claim
(C-029, fixed two `ctest` failures). `WORK-UNITS.md` updated (WU-23b2b's
own status line and design paragraph, the Phase 6 summary paragraph).
`DECISIONS.md` ADR-081 appended. `CORRECTIONS.md` C-028 and C-029
appended. This `HANDOFF.md`.

## Steve's own next steps

At your own real terminal, **re-run build and test** — you already
confirmed `cmake --build build` compiles clean (C-028's fix held); what
failed after that was `ctest`, on the stale accounting invariant C-029
now fixes. Neither the C-029 edit itself nor its interaction with the
rest of the suite has been rebuilt or rerun yet:

```
cd ~/src/scatter-dve
cmake --build build
ctest --test-dir build --output-on-failure
```

Expect `test_decklink_device`'s own `test_at_least_one_device_is_full_duplex`
check to fail regardless (the standing PSU/two-device exception,
`DECISIONS.md` ADR-034/035/037, `CORRECTIONS.md` C-024) — that one is not
this unit's own problem, and was already the only pre-existing failure
before this unit touched anything. `test_decklink_live_output` and
`test_decklink_live_sphere` should now pass their own accounting
`CHECK`s (C-029) — if either still fails, read what it actually printed
(`framesPopped`/`framesProcessed`/`framesFailed`/`framesStreamStart`)
rather than assuming C-029's own fix was sufficient; there may be a
third thing this session still missed. For `test_decklink_capture_consumer`
itself, with the Monitor 3G's SDI output patched into the Recorder 3G's
SDI input (ADR-037's own established loopback rig), also check by eye
(the test's own `fprintf` line) that `framesStreamStart` reads exactly
`1` — the automated `CHECK`s already gate on this, but the "increments
before `framesProcessed` ever does" ordering itself is not independently
observable from final counts alone, so a glance at the printed line is
worth it. If anything still fails or the build errors out, that is real
feedback for the next session, not a reason to force a tag past it — do
not run `./tools/close.sh`.

**Only once you've confirmed a real, green (modulo the standing duplex
exception) build and test run**, close out with:

```
cd ~/src/scatter-dve
git add CORRECTIONS.md DECISIONS.md WORK-UNITS.md src/io/decklink_capture_consumer.cpp src/io/decklink_capture_consumer.hpp tests/test_decklink_capture_consumer.cpp tests/test_decklink_live_output.cpp tests/test_decklink_live_sphere.cpp HANDOFF.md
git commit -m "WU-23b2b: CaptureConsumer wiring (ADR-080, ADR-081); C-028, C-029"
git tag -a wu-23b2b-green -m "WU-23b2b: CaptureConsumer wiring (ADR-080, ADR-081); C-028, C-029"
git push origin main
git push origin --tags
```

**File paths above match this session's own real `git status --short`,
re-diffed immediately before writing this block, not merely recalled
from earlier in the session** (`CORRECTIONS.md` C-026's own lesson).
