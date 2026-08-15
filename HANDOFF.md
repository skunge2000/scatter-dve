# Handoff
Overwritten at the end of every session. This is the first thing to read.

---

**Session:** 24
**Tag:** `wu-18-green` stood, confirmed at session open. `wu-19a-green` was
tagged this session per the "What to run at your terminal" runbook this
handoff's own earlier draft gave — see the commands below; not
independently re-verified (this session's own recurring "the assistant
does not run `close.sh`" limit) but expected clean, the same "one known
exception, nothing else" pattern every close since WU-15a has hit.
**Phase:** 4 (Threading and NEON) is now genuinely done in full — WU-16a/
16b, WU-17, WU-18 (all already green going in), WU-19a (this session:
persistent, caller-owned `ThreadPool`, ADR-044) and WU-19b (this session,
via Steve's own real-terminal measurement: confirmed "at frame rate" on
the real M1 Max) all closed out. `DECISIONS.md` now runs through ADR-045;
`CORRECTIONS.md` unchanged this session, still through C-015.

## This session in full

**Part 1 — WU-19a.** `WORK-UNITS.md`'s own WU-19 line was barer going in
than any prior unit's — just a title, "Real time at 576i25." Read
architecture.md section 10's own Phase 4 "done when" line, section 6
(threading model) and section 11 (budget/splat discussion), plus
`core/pipeline.cpp`/`.hpp` and `video/v210.cpp`/`chroma.cpp`, before scoping
anything. The central finding: "at frame rate" is a real-hardware timing
claim this project's own Linux cloud sandbox CPU cannot produce meaningful
evidence about — a different kind of gap than ADR-031/032's own "cannot
compile here at all" (this sandbox compiles and runs the code fine; the
number it would produce just wouldn't mean anything for the M1 Max). Split
into WU-19a (a correctness-preserving refactor this sandbox *can* fully
verify) and WU-19b (the real measurement, Steve's own job).

WU-19a built `PipelineParams::pool` (`core/resolve.hpp`) — an optional,
caller-owned, already-constructed `ThreadPool*`, default `nullptr` — so a
caller can build one `ThreadPool` once and reuse it across many
`runFrame()` calls instead of paying WU-16a/16b's own per-call spawn/join
cost every time. `core/pipeline.cpp`'s own threaded PASS-1/PASS-2 body was
factored out unchanged into `runThreaded(..., ThreadPool&, ...)`, callable
against either a fresh per-call pool (unchanged WU-16a/16b behaviour) or
the caller's persistent one. New `tests/test_persistent_pool.cpp` (2 562
778 checks) verified: a pool reused across ten calls matches the
single-threaded oracle every time; a pool whose size deliberately
disagrees with `PipelineParams::threads` still produces correct output
(the direct regression check that `pool->size()`, not `threads`, governs);
a pool reused across different frame geometries in sequence doesn't leak
state. Full sandbox matrix — Clang 18/GCC 13 x Release/Debug x tile 4/5,
GCC 13 ASan+UBSan both tile sizes, GCC 13 ThreadSanitizer both tile
sizes — all clean, all seventeen tests green, zero warnings. See
`DECISIONS.md` ADR-044.

**Part 2 — WU-19b.** Steve ran a small, deliberately uncommitted
`std::chrono` scratch benchmark (per ADR-044's own reasoning: a sandbox-CPU
benchmark tool would report numbers meaningless for the real question, so
nothing was committed for this) against the real M1 Max, linked directly
against `libscatter-core.a`, using WU-19a's own persistent-pool mechanism,
timing a genuinely warped (cylinder-over-zone-plate) 720x576 frame across
200 iterations after a 20-iteration warmup, at four thread counts, at
**both** tile sizes:

| threads | 2^5 (32x32) ms/frame | 2^4 (16x16) ms/frame |
|---|---|---|
| 1 | 48.766 (over 40ms budget — expected) | 50.269 (over budget) |
| 2 | 24.665 | 25.710 |
| 4 | 13.101 | 14.349 |
| 8 | 6.868 (5.8x headroom) | 7.528 (5.3x headroom) |

architecture.md 10's own Phase 4 "done when" line ("8-thread output is
bit-identical to single-threaded, at frame rate") is now satisfied in full
— bit-identical by WU-16a/16b/19a's own I6 checks, frame-rate by this
measurement, at architecture.md's own named 8-worker configuration, with
wide margin at both tile sizes.

**Part 3 — Q1 settled, unplanned but natural given the data already in
hand.** Tile 2^5 (32x32) beat 2^4 (16x16) at every thread count measured,
consistently by 3-10%, the margin widening as worker count grew — real,
whole-pipeline, real-M1-Max evidence for the question architecture.md 4.5
itself flagged as empirical ("32x32 may still win on reduced edge
replication despite spilling") and that has sat open since WU-09 for lack
of exactly this kind of measurement. `SCATTER_TILE_LOG2=5` — already this
project's own default — is now confirmed and settled as the project's tile
size going forward, not merely an unexamined default. See `DECISIONS.md`
ADR-045; `CMakeLists.txt`'s own tile-size comment updated to match (both
values stay fully configurable and fully exercised by the test matrix —
nothing about the build changed, only the comment recording that the
question is closed).

**Neither NEON deferral reopened.** WU-17's own denser `vld4q_u32` v210
scheme and WU-18's own `downsampleRowNeon` load-count reduction: with
5.3-5.8x headroom at 8 threads, at both tile sizes, there is no evidence
either is a bottleneck. Both stay exactly as deferred as ADR-042/043 left
them.

**Corrections this session:** none. `CORRECTIONS.md` unchanged, still
through C-015. One documentation-only touch-up: `WORK-UNITS.md`'s own
WU-18-session note ("once tagged, Phase 4 is done in full... WU-19 is
next") turned out premature once real scoping found WU-19 needed two
sub-units — corrected in place with a short parenthetical, not erased, the
same convention WU-04's own session used correcting WU-03's stale status
line.

## Where we are

**Phase 4 (Threading and NEON) is done in full.** Every unit from WU-16a
through WU-19b is green (pending only the routine `close.sh 19a` tag
confirmation, per "What was run this session," below). `DECISIONS.md` now
runs through ADR-045; `CORRECTIONS.md` unchanged, through C-015.

**Delivery mechanics:** WU-19a's implementation and verification were done
in this session's own Linux cloud sandbox; WU-19b's own measurement and
Q1's own resolution happened entirely at Steve's real terminal, reported
back into this same session. All files (`resolve.hpp`, `pipeline.cpp`,
`test_persistent_pool.cpp`, `CMakeLists.txt`, `DECISIONS.md`,
`WORK-UNITS.md`, this `HANDOFF.md`) were written to the real repository via
the device bridge. Steve commits and tags at his own terminal — the
standing operational note (device-bridge commits on this machine leave
stale `.git/index.lock`/`HEAD.lock` files) still applies.

## Next work unit

**Phase 5 — Live capture.** `WORK-UNITS.md`'s own WU-20 line
("DeckLink input, format detection, ring buffer") already names its
hardware target (**UltraStudio Recorder 3G**, ADR-039) but has no
`Files:`/`Accept:` scoping at all yet — whichever session starts it should
read the real SDK's own `IDeckLinkInput`/capture-callback shape first, the
same reading-before-scoping discipline ADR-031/032 already used for
enumeration and output, rather than assume from architecture.md's own
Input subsection (which still describes the original single-full-duplex-
device design, unrevised — ADR-039's own note). Given WU-14/WU-15a's own
precedent, expect this to need the same "reasoned through against the real
SDK headers, unverified until the real terminal" shape this sandbox has
used for every DeckLink-touching unit so far — no Blackmagic SDK, no
AppleClang, in this sandbox.

## Open questions

**Q1 (tile size) — closed this session, ADR-045.** No longer open.

Unchanged: Q3 (macOS/Desktop Video version), Q4 (lattice edge damping,
C-008(a)). Q2 remains moot per ADR-037. ADR-037's own follow-ups #1
(`test_decklink_device.cpp`'s full-duplex check) and #2 (genlock) remain
open — both squarely Phase 5's own concern now that Phase 5 is next.

## Blocked / red

Nothing red. WU-19a is green in the sandbox across the full matrix
(including TSAN, both tile sizes); WU-19b and Q1 are both confirmed by real
hardware measurement. Only the routine `close.sh 19a` tag confirmation is
outstanding — same procedural step every unit needs.

## Environment check

Unchanged from sessions 18-23 (ADR-037/039): **UltraStudio Monitor 3G** is
the active, confirmed output target. **UltraStudio Recorder 3G** is in
hand, named (ADR-039) as Phase 5's own input target — about to actually
matter, now that WU-20 is next. **UltraStudio 4K Mini** remains on hold
pending a PSU replacement.

## Append to DECISIONS.md

ADR-044 (WU-19a's own design) and ADR-045 (Q1, tile size, settled) were
both appended in full this session; see `DECISIONS.md`. ADR-045 does not
reopen `docs/architecture.md`, ADR-002 or ADR-044 — see its own closing
paragraph.

## Append to CORRECTIONS.md

Nothing appended this session.

---

## What was run this session

Already done:

```
cd ~/src/scatter-dve
git add src/core/resolve.hpp src/core/pipeline.cpp tests/test_persistent_pool.cpp \
        CMakeLists.txt DECISIONS.md WORK-UNITS.md HANDOFF.md
git commit -m "WU-19a: persistent, caller-owned ThreadPool (ADR-044), verified in the cloud sandbox"
cmake -B build && cmake --build build
ctest --test-dir build --output-on-failure   # 20/21 -- ADR-035's known exception
./tools/close.sh 19a
git tag -a wu-19a-green -m "WU-19a: persistent, caller-owned ThreadPool, verified"
```

Plus, for WU-19b and Q1 (not committed — scratch, per ADR-044/045):

```
cmake -B build-t4 -DSCATTER_TILE_LOG2=4 && cmake --build build-t4
# scratch bench.cpp compiled and linked against both build/libscatter-core.a
# and build-t4/libscatter-core.a, results recorded in WORK-UNITS.md/ADR-045
```

**Still to do, this session's own remaining loose end:** commit this
replacement `HANDOFF.md` (and the WU-19b/Q1 updates to `WORK-UNITS.md`,
`DECISIONS.md` and `CMakeLists.txt`'s own comment, all already written to
the repo via the bridge):

```
cd ~/src/scatter-dve
git add HANDOFF.md WORK-UNITS.md DECISIONS.md CMakeLists.txt
git commit -m "WU-19b + Q1: real-time measurement at 576i25 confirmed on M1 Max; tile size settled (ADR-045)"
git push origin HEAD --tags   # if you keep a remote
```

No new build/test run needed for this commit — nothing in `src/` or
`tests/` changed since the WU-19a commit above, only documentation
recording WU-19b's own measurement and Q1's own resolution. `build-t4/` is
yours to keep or remove as you like; nothing in the repo depends on it
persisting.
