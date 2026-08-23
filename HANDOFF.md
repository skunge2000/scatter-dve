# Handoff
Overwritten at the end of every session. This is the first thing to read.

---

**Session:** 49 (WU-23b2a build — Weston 3-field de-interlace:
`runFrameBytesDeinterlaced()` orchestration entry point).

**Tag:** `wu-23b1-green` is still the newest real tag on the repository as
of this session's own start (confirmed directly). This session's own new
code is built and tested green in the cloud sandbox but not yet tagged —
that is Steve's own next step, below (`wu-23b2a-green`).

## Before doing anything else in the next session

Run `git tag --sort=creatordate`, `git log --oneline -10`, `git rev-parse
HEAD origin/main` and `git status --short` directly against
`~/src/scatter-dve` — do not trust this file's own account without
checking it against the real repository first.

## This session in full

Opened with a continuation prompt whose own job was resolving the
immediately preceding session's own unpushed-commits loose end, then
building WU-23b2a.

**Repository state, confirmed directly before reading anything else:**
`git tag --sort=creatordate` (newest: `wu-23b1-green`, matching the
continuation prompt's own expected state exactly), `git log --oneline
-10` (`HEAD` = `dcca567`, "WU-23b2 scoping: split into WU-23b2a/WU-23b2b
(ADR-080), corrects prior wiring assumption (C-027)", immediately
preceded by `982e3e2`, "Session 47 close-out..." — both matching the
continuation prompt's own expected short hashes exactly), `git status
--short` (clean at session start).

**The continuation prompt's own flagged loose end was already closed by
the time this session started:** `git rev-parse HEAD origin/main`
returned the same hash twice
(`dcca56740b6caeaedc381557c84afa99599a0af3`) — Steve had already pushed
the immediately preceding session's own doc-only commits before this
session began. No push was needed or attempted this session for that;
said so plainly per the continuation prompt's own instruction, and moved
straight to this session's own actual job.

**Read directly, not from memory or paraphrase, before writing anything**
(continuation prompt's own list): `SESSION-PROTOCOL.md`, `HANDOFF.md`,
`INVARIANTS.md`, `DECISIONS.md` (ADR-080 in full), `CORRECTIONS.md`
(C-026/C-027 in full), `WORK-UNITS.md` (WU-23b2a's own entry in full),
`core/resolve.hpp`, `core/pipeline.cpp`'s `runFrameBytes()` (confirmed at
the stated ~lines 630-680), `video/deinterlace.hpp`,
`video/interlace.hpp`/`.cpp`, `tests/test_pipeline_bytes.cpp`,
`tests/test_deinterlace.cpp` (for `Deinterlacer`'s own real call
convention and `push()`'s own one-frame-latency state machine),
`tools/testpat.hpp`, `tests/harness.hpp`.

**Re-derived, not merely trusted, ADR-080's own
`extractField()`/`interleaveFields()` no-op proof before relying on it**
(continuation prompt's own explicit instruction): read `video/interlace.cpp`
directly — `extractField()` copies frame row `2*fy + rowOffset` into
field row `fy`; `interleaveFields()` writes field row `fy` back into dest
row `2*fy + rowOffset`. Applying both parities' `extractField()` and then
`interleaveFields()` to the same source frame reproduces every row at its
own original index exactly — confirmed algebraically, not assumed from
the ADR's own paraphrase, before writing `runFrameBytesDeinterlaced()`'s
own body to rely on it. This is also checked directly at runtime now, not
merely proven on paper — see `tests/test_pipeline_bytes.cpp`'s new
`test_deinterlaced_reinterlace_noop_matches_explicit_reinterlace()`,
below.

**Built WU-23b2a exactly as ADR-080/`WORK-UNITS.md` already froze it — no
redesign, no new `DECISIONS.md` entry needed:**

- `core/resolve.hpp`: added `#include "video/deinterlace.hpp"`; declared
  `runFrameBytesDeinterlaced(video::Deinterlacer&, const Lattice&, const
  std::uint8_t* srcBytes, std::ptrdiff_t srcRowBytes, int srcWidth, int
  srcHeight, const PipelineParams&, std::uint8_t* dstBytes,
  std::ptrdiff_t dstRowBytes)`, the frozen signature verbatim, between
  `runFrameBytes()` and `runFrameFile()`.
- `core/pipeline.cpp`: defined it — reproduces `runFrameBytes()`'s own
  sequence (v210 unpack, chroma upsample into a local weave `Raster444`,
  chroma downsample, v210 pack) exactly, with
  `deinterlacer.push(weave, progressive)` inserted between the chroma
  upsample and `runFrame()`; returns `false` immediately (`dstBytes`
  completely untouched) if `push()` returns `false`; `runFrame()` is
  called against the reconstructed `progressive` frame, not the raw
  `weave`; the output side sends `runFrame()`'s own `warped` result
  straight to chroma downsample with no `extractField()`/
  `interleaveFields()` call, per ADR-080's own no-op proof.
- `tests/test_pipeline_bytes.cpp`: extended (not replaced) with four new
  checks, one per `WORK-UNITS.md`'s own WU-23b2a `Accept:` bullets: (1)
  a freshly constructed `Deinterlacer`'s first push leaves `dstBytes`
  byte-for-byte unchanged and returns `false`, checked together with (2)
  from the second push onward matching a hand-composed independent
  reference (`unpackImage` → `upsampleImage` → `push()` → `runFrame()` →
  `downsampleImage` → `packImage`, written independently of
  `core/pipeline.cpp`'s own body) — both in
  `test_deinterlaced_matches_reference_and_first_push_is_a_noop()`; (3)
  the re-interlace no-op path matches an explicit `extractField()`x2 +
  `interleaveFields()` variant byte for byte, in
  `test_deinterlaced_reinterlace_noop_matches_explicit_reinterlace()`;
  (4) anchor-parity rows survive `runFrameBytesDeinterlaced()` unchanged
  under an identity lattice (I7 does not apply directly to a lossy
  de-interlaced round trip, so this substitutes for it), in
  `test_deinterlaced_anchor_rows_survive_identity_round_trip()`; (5)
  625i50 (720x576) geometry exercised directly, in
  `test_deinterlaced_sd_geometry_sanity()`, reusing the same reference
  cross-check at full SD size.

**Built and tested in this project's own Linux cloud sandbox, the full
standard portable-unit matrix WU-23b1 already established — all 10
configurations green:** GCC 13.3.0 and Clang 18.1.3, Release and Debug,
`SCATTER_TILE_LOG2` 4 and 5, plus GCC 13
`-fsanitize=address,undefined -fno-sanitize-recover=all` at both tile
sizes. `ctest`: 26 of 26 in every configuration (25 pre-existing tests,
unaffected, plus `test_pipeline_bytes`'s own now-larger suite — 42
checks, up from 2 tests' worth previously). No ASan/UBSan findings in any
configuration. This is a pre-flight check, not a substitute for Steve's
own real-terminal build — `SESSION-PROTOCOL.md`'s own "sandbox edits are
not delivered until pushed" discipline, extended to "cloud-sandbox green
is not real-terminal green" the same way WU-23b1's own `HANDOFF.md`
entry already stated it. See "Steve's own next steps" below.

**Wrote `core/resolve.hpp`, `core/pipeline.cpp`, `tests/test_pipeline_bytes.cpp`
and `WORK-UNITS.md` to the real repository via the device bridge, then
re-staged each from the device and diffed against this session's own
edited copies before writing this sentence** (`SESSION-PROTOCOL.md`'s own
rule 8) — all four came back byte-for-byte identical. `git status
--short` against the real repository shows exactly `M WORK-UNITS.md`,
`M src/core/pipeline.cpp`, `M src/core/resolve.hpp`,
`M tests/test_pipeline_bytes.cpp` and nothing else (a stray
`_scatter_dve_src_tmp.tar.gz`, an artifact of this session's own
cloud-sandbox transfer mechanism, was created at the repository root and
then moved into `_to_delete/` within the same session, not left in the
tree). This `HANDOFF.md` is written the same way.

No `DECISIONS.md` or `CORRECTIONS.md` entries this session: WU-23b2a's
own implementation matched ADR-080's frozen design in every particular,
including the `extractField()`/`interleaveFields()` no-op proof
re-derived above — nothing ADR-080 assumed turned out wrong once the
code was actually written.

## Where we are

**WU-23b2a is built and cloud-sandbox green, not yet tagged.**
`core/resolve.hpp`/`core/pipeline.cpp` carry the new
`runFrameBytesDeinterlaced()`; `tests/test_pipeline_bytes.cpp` carries
its own new checks. `WORK-UNITS.md`'s WU-23b2a entry status line and the
Phase 6 summary paragraph above it reflect this. `DECISIONS.md`
unchanged through ADR-080; `CORRECTIONS.md` unchanged through C-027;
`INVARIANTS.md` unchanged through I11. All four changed files are staged
as local, uncommitted changes on the Mac as of this sentence — see
"Steve's own next steps" below for the commit/tag/push this session's
own close-out hands him.

## Next work unit

**WU-23b2b** (`CaptureConsumer` wiring, `io/decklink_capture_consumer.hpp`/`.cpp`)
is the natural next pick once WU-23b2a is tagged `wu-23b2a-green` — fully
scoped (`DECISIONS.md` ADR-080, `WORK-UNITS.md`), genuinely depends on
WU-23b2a's own actual built interface, which now exists. Real-hardware-gated
for its own `Accept:` line (per `WORK-UNITS.md`), so building it does not
itself require the PSU or a real capture device, but verifying it fully
eventually will. Everything named in Session 43's own "Next work unit"
section (WU-28d, WU-27, WU-33, WU-35, WU-37) is unchanged and still
pickable if the interlace thread is set aside instead.

**Steve's own explicit stay-in-SD-domain scope decision (WU-24/WU-25
skipped until he says otherwise) is unchanged and still in force** —
carried forward unmodified this session, reiterated here per that
decision's own standing request not to let it be lost silently.

## Open questions

Unchanged from Session 42/43's own list (`kCaptureRingCapacity`, Q3, Q4,
Task A1, Task D6, ADR-070's own open question, WU-35's
`compositeLayered()` question) — this session did not touch any of them.
`DeinterlaceCoefficients` (Simple vs Complex) for `CaptureConsumer`'s own
new constructor parameter (ADR-080) is also unchanged and still open —
WU-23b2b's own build session should raise it with Steve, not default it
silently; this session's own job was WU-23b2a alone, which has no such
parameter of its own to decide (it takes an already-constructed
`Deinterlacer&`).

One aside, not this session's own job to fix: `WORK-UNITS.md`'s own
WU-23b1 entry still read (before this session's own edits above, which
did not touch it) "built this session, unverified — needs a real
build/`ctest` run... before `wu-23b1-green`," even though the real
repository's own newest tag confirms `wu-23b1-green` already exists —
the same kind of stale status line a past correction sweep (`0dc2247`,
"WORK-UNITS.md: correct six stale status lines") already fixed once for
other units. Left alone this session (out of scope — one session, one
work unit) but worth a future sweep.

## Blocked / red

Nothing red. WU-23b2b is blocked on WU-23b2a by design (not a problem)
until Steve's own real-terminal tag lands — the same sequencing WU-23a2b
was blocked on WU-23a2a, and WU-23b2 itself on WU-23b1, before it.

## Environment check

This session built and ran real code in this project's own Linux cloud
sandbox (Ubuntu 24.04, GCC 13.3.0 / Clang 18.1.3 both present, matching
the versions this project's own standard matrix already names) — no
DeckLink SDK configured there (`BLACKMAGIC_SDK_DIR` unset), so
`scatter-decklink` and every DeckLink-linked test, including
`test_decklink_device`, are not even configured in that sandbox; this
unit touches no DeckLink-linked file and needs none of them regardless
(same scope as WU-23b1). The standing condition from prior sessions
(C-024: `tools/close.sh` cannot currently succeed for any unit on
Steve's own real terminal, where the DeckLink SDK *is* configured,
because of the PSU/two-device-architecture mismatch — `DECISIONS.md`
ADR-034/035/037, `CORRECTIONS.md` C-024) is unchanged and unaffected by
this session — Steve's own real-terminal `ctest` run below should still
show `test_decklink_device`'s own `test_at_least_one_device_is_full_duplex`
failing, expected, not blocking. The device-bridge sandbox used this
session for git/file operations (distinct from the Linux cloud sandbox
above) had ordinary read/write access this time — no `.git/index.lock`
stray files encountered, and `git push` was not attempted from it
(nothing needed pushing, per the loose-end check above) so its own
network-egress behaviour this session is untested one way or the other;
do not assume either way next session.

## Append to DECISIONS.md

Nothing this session — WU-23b2a's own implementation matched ADR-080's
already-frozen design exactly.

## Append to CORRECTIONS.md

Nothing this session — no assumption ADR-080 made turned out wrong once
the code was written, including the `extractField()`/`interleaveFields()`
no-op proof, re-derived directly and confirmed (see "This session in
full," above, and `tests/test_pipeline_bytes.cpp`'s own new
`test_deinterlaced_reinterlace_noop_matches_explicit_reinterlace()`).

## Closed out this session

**WU-23b2a.** `core/resolve.hpp` (new include, new declaration),
`core/pipeline.cpp` (new definition), `tests/test_pipeline_bytes.cpp`
(four new checks) — built and tested green across this project's own
full 10-configuration portable-unit matrix in the Linux cloud sandbox.
`WORK-UNITS.md` updated (WU-23b2a's own status line, Phase 6 summary
paragraph). This `HANDOFF.md`. No `DECISIONS.md`/`CORRECTIONS.md`
entries needed. Also confirmed and closed the continuation prompt's own
flagged loose end: the immediately preceding session's own unpushed
commits had already reached `origin` by the time this session started —
no action needed.

## Steve's own next steps

At your own real terminal:

```
cd ~/src/scatter-dve
cmake --build build
ctest --test-dir build --output-on-failure
```

Confirm nothing **other than** `test_decklink_device`'s own
`test_at_least_one_device_is_full_duplex` check fails (the standing
PSU/two-device exception, `DECISIONS.md` ADR-034/035/037,
`CORRECTIONS.md` C-024) — this unit touches no DeckLink-linked file, so
nothing about its own build/test should disturb that. Then:

```
cd ~/src/scatter-dve
git add src/core/pipeline.cpp src/core/resolve.hpp tests/test_pipeline_bytes.cpp WORK-UNITS.md HANDOFF.md
git commit -m "WU-23b2a: runFrameBytesDeinterlaced() orchestration entry point (ADR-080)"
git tag -a wu-23b2a-green -m "WU-23b2a: runFrameBytesDeinterlaced() orchestration entry point (ADR-080)"
git push origin main
git push origin --tags
```

**File paths above match this session's own real `git status --short`,
re-diffed immediately before writing this block, not merely recalled
from earlier in the session** — `git status --short` at the moment this
block was finalised showed exactly `M HANDOFF.md`, `M WORK-UNITS.md`,
`M src/core/pipeline.cpp`, `M src/core/resolve.hpp`,
`M tests/test_pipeline_bytes.cpp`, all five accounted for above
(`CORRECTIONS.md` C-026's own general lesson — an earlier draft of this
very block omitted `HANDOFF.md` itself from the `git add` line, caught
by this same diff-against-reality step before being sent).
