# Handoff
Overwritten at the end of every session. This is the first thing to read.

---

**Session:** 21
**Tag:** `wu-16a-green` is the most recent *tag*. WU-16b (this session) is
implemented and verified in a Linux cloud sandbox but not yet built at the
real terminal — same shape WU-16a's own first turn left things in, and the
same next action: Steve runs the build/test/`close.sh` sequence below,
then tags `wu-16b-green` by hand once he has (this unit adds no Apple-only
surface, so there is no reason to expect the real-terminal run to differ
from the sandbox, but the assistant does not run `close.sh` — see
"What to run at your terminal," below).
**Phase:** 3 (SDI output) remains done in full, unchanged since session
18. Phase 4 (Threading and NEON): WU-16a is confirmed green
(`wu-16a-green`, tagged by Steve since the last session's own handoff).
WU-16b (PASS 1 row-band parallelism, per-worker generation-time bin
arenas — the other half of Phase 4's threading work, named but not scoped
at WU-16a's own close) is implemented and verified this session; its own
`WORK-UNITS.md` line stays `wip` pending the real-terminal run above.

**This session's own first job, per Steve's own brief and
`SESSION-PROTOCOL.md`'s own discipline: read `core/binner.hpp`/`.cpp`
closely (`generateFragments()`'s row loop, `pixelToLattice()`) before
writing anything, and verify ADR-040's own diagnosis against the real
code rather than assume it.** It checks out exactly as ADR-040 described:
the row loop's bound and `pixelToLattice()`'s own `v`-denominator both
read the same `SourceRaster::height` field, so a row-band-parallel PASS 1
cannot be built by calling `generateFragments()` once per band against a
`SourceRaster` whose own `height` was shortened to the band's extent —
that moves the `v`-parameter's whole domain, not just which rows are
visited. See `DECISIONS.md` ADR-041 for the full design this session
implements from that finding.

**1. `src/core/binner.hpp`/`.cpp`, extended.** New
`generateFragmentsRowRange()` — same parameters as `generateFragments()`
plus `rowStart`/`rowEnd`; the row loop's bound becomes `rowStart`/
`rowEnd` while every `pixelToLattice()`/`pixelJacobian()` call inside
stays keyed to `src.width`/`src.height` in full — the "honest fix" ADR-040
named. `generateFragments()` itself is now a thin wrapper,
`generateFragmentsRowRange(..., 0, src.height, outBins)` — WU-08's own
frozen signature and behaviour, unchanged.

**2. `src/core/pipeline.cpp`, extended.** `runFrame()`'s `threads > 1`
branch: PASS 1 now partitions `src`'s rows into `params.threads`
contiguous bands (`(worker * src.height) / numWorkers` boundaries —
architecture.md 6's own "partitions the source by row bands," contrasted
with PASS 2's own unchanged interleaved `tileIndex % threads` split), one
worker per band, each writing into its own whole-frame `TileBins` (a
private "generation-time bin arena" — not a partial one, since a row
band's own fragments can land in any tile depending on the warp) via
`generateFragmentsRowRange()`. A second `ThreadPool::runOnAll()` call is
architecture.md 6's own barrier, free — `core/pipeline.hpp`'s own doc
comment already anticipated this exact use, unused until now; no
`pipeline.hpp` change needed. PASS 2 (tile-parallel, unchanged
partitioning) now reads every worker's own PASS-1 arena for a given tile,
in fixed worker order, instead of one shared `TileBins`.
`resolveOneTile()` is generalised to take `std::span<const TileBins*
const>` instead of a single `TileBins&` — the `threads <= 1` path wraps
its own single `TileBins` in a one-element `std::array`, arithmetically
identical to before (verified by the full pre-existing test suite still
passing unchanged against this refactored path, not just reasoned) —
extending WU-16a's own "both paths share one function, so they cannot
silently diverge" property from two paths to three, rather than
hand-duplicating a second bank-resolve/normalise/composite body for the
multi-source case.

**3. `tests/test_row_band.cpp`, new.** Direct checks of
`generateFragmentsRowRange()` against `generateFragments()` (row bands
reassembled, tile by tile, bit-identical to a whole-raster call — two
constructions: uneven bands, and more bands than source rows) plus the
pipeline-level edge case `tests/test_threading.cpp`'s own thread-count
matrix never exercised: `runFrame()` at thread counts exceeding the
source raster's own row count, so several workers get an empty PASS-1
row band.

**4. `CMakeLists.txt`.** `test_row_band` added (`scatter_test()`); no new
target dependency.

**Correction this session:** C-015 (`CORRECTIONS.md`) — this session's
own first draft of `test_row_range_reassembles_with_more_bands_than_rows()`
used a magnifying map, which triggers 4.6's own supersampling and breaks
the decode()-by-colour-signature technique's own "at most one Frag per
signature per tile" assumption (several sub-samples of one source pixel
can land on the same destination cell); caught by a standalone diagnostic
run before the test was relied on, fixed by switching to a compressive
map (matching `tests/test_binner.cpp`'s own convention). No production
code was implicated.

**Tests / Build — Linux cloud sandbox (this session):** all sixteen
tests green (fourteen carried over from before WU-16a, plus
`test_threading` and the new `test_row_band`) across Clang 18 and GCC 13,
Release and Debug, `SCATTER_TILE_LOG2` 4 and 5 — eight configurations,
zero warnings under this project's full `-Wall -Wextra -Wpedantic
-Wconversion -Wsign-conversion -Werror` set — plus GCC 13 with
`-fsanitize=address,undefined -fno-sanitize-recover=all` at both tile
sizes (clean) and GCC 13 with `-fsanitize=thread` (clean — no data race;
run both across the full suite and standalone against
`test_threading`/`test_row_band`). Unlike WU-16a, this unit adds no new
Apple-only surface at all (`setWorkerQoS()` is untouched), so there is no
open verification question the way `setWorkerQoS()`'s own `#ifdef
__APPLE__` branch was going into WU-16a's own second turn — only the
procedural real-terminal `close.sh` run remains, per this project's own
"the assistant does not run `close.sh`" rule.

## Where we are

Phase 3 (SDI output) remains done in full (WU-14/15a/15b, session 18).
Phase 4: WU-16a green (`wu-16a-green`). WU-16b implemented and verified in
the Linux cloud sandbox this session; `WORK-UNITS.md` stays `wip` pending
Steve's own real-terminal run and `close.sh 16b`. `DECISIONS.md` now runs
through ADR-041; `CORRECTIONS.md` now runs through C-015.

**Corrections this session:** C-015 (see above).

**Delivery mechanics:** implemented and verified in this session's own
Linux cloud sandbox (Ubuntu 24.04, Clang 18.1.3, GCC 13.3.0), not the
device bridge's own more limited Linux VM. Final files were written to
the real repository via the device bridge. **Not committed this
session** — per the operational note from the WU-16a session (repeated
below), committing through the device bridge on this repo reliably
leaves stale `.git/index.lock`/`HEAD.lock` files behind, since the bridge
can write files but cannot unlink anything on Steve's machine. Rather
than proxy `git add`/`git commit` through the bridge again, this
session's own close hands Steve the exact commands to run at his own
terminal instead (below) — an untested convenience, but one that avoids
repeating a known friction point.

## Next work unit

**Steve's own next action: build, test and (if green) `close.sh` WU-16b
at his own terminal, then commit and tag by hand.** Exact commands under
"What to run at your terminal," below.

After that: **WU-17** (NEON v210 unpack and pack) is next in Phase 4, per
`WORK-UNITS.md`'s own ordering — its own `Accept:` line ("bit-identical to
scalar reference") is already written; no other scoping is recorded for
it yet.

## Open questions

Unchanged: Q1 (tile size), Q3 (macOS/Desktop Video version), Q4 (lattice
edge damping, C-008(a)). Q2 remains moot per ADR-037. ADR-037's own
follow-ups #1 (`test_decklink_device.cpp`'s full-duplex check) and #2
(genlock) remain open, unrelated to this session's own work — Phase 5's
problem, not Phase 4's.

New, named but not resolved, per ADR-041's own "not decided here"
section: per-row-band load balancing (PASS 1's own contiguous bands can
carry uneven fragment-generation cost under a warp whose magnification
varies sharply across the frame, the same class of concern C-011 already
raised for a shape's own front-facing point); bump-allocated/preallocated
bin arenas (architecture.md 6's own "preallocated, bump-allocated" phrase
is not what `TileBins`' own `std::vector<Frag>::push_back()` growth
implements, for either WU-16a's single arena or this unit's `numWorkers`
arenas); a persistent, caller-owned `ThreadPool` reused across frames
(WU-19's own job, unchanged from ADR-040's own deferral).

## Blocked / red

Nothing red. WU-16b is fully green in the Linux cloud sandbox; only the
real-terminal `close.sh` run and the commit/tag are outstanding, both
Steve's own next steps per "Next work unit," above.

## Environment check

Unchanged from session 18–20 (ADR-037/039): **UltraStudio Monitor 3G** is
the active, confirmed output target. **UltraStudio Recorder 3G** is in
hand, named (ADR-039) as Phase 5's own input target, still untouched by
any code. **UltraStudio 4K Mini** remains on hold pending a PSU
replacement. None of this is relevant to WU-16b's own work — pure
`src/core/`, no DeckLink code touched, no hardware needed to verify it.

## Append to DECISIONS.md

ADR-041 was appended in full this session; see `DECISIONS.md`. Does not
reopen `docs/architecture.md`, ADR-002, ADR-008, ADR-013, ADR-015,
ADR-017, ADR-024, ADR-026, ADR-029, ADR-031 or ADR-040 — see ADR-041's own
closing paragraph for the precise relationship to each.

## Append to CORRECTIONS.md

C-015 was appended in full this session; see `CORRECTIONS.md`. A
test-authoring assumption (decode()-by-colour-signature reassembly checks
require `chooseSupersample()` to return 1 everywhere) caught and fixed
within this same session, before being relied on as evidence of anything;
no production code implicated.

---

## What to run at your terminal

```
cd ~/src/scatter-dve
cmake -B build && cmake --build build
ctest --test-dir build --output-on-failure
./tools/close.sh 16b
```

If `close.sh 16b` reports the same, already-understood
`test_decklink_device` full-duplex exception ADR-035 already covers
(15/16 or 16/17, not a clean N/N) and nothing else fails, that exception
is expected and does not block tagging — the same call Steve made for
`wu-15a-green` and `wu-16a-green`. If `close.sh` itself tags cleanly,
nothing further is needed. If it refuses (its own gate cannot distinguish
an accepted exception from a real failure, by design), tag by hand:

```
git add src/core/binner.hpp src/core/binner.cpp src/core/pipeline.cpp \
        tests/test_row_band.cpp CMakeLists.txt \
        DECISIONS.md CORRECTIONS.md WORK-UNITS.md HANDOFF.md
git commit -m "WU-16b: PASS 1 row-band parallelism, per-worker generation-time bin arenas (ADR-041)"
git tag -a wu-16b-green -m "WU-16b: build green, tests pass (test_decklink_device's full-duplex check is ADR-035's known exception, unrelated to WU-16b)"
git push origin HEAD --tags   # if you keep a remote
```

Adjust the exact exception wording in the tag message if the real run's
own result differs from what's described above — the message should say
what actually happened, not what this handoff predicted would happen.
