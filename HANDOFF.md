# Handoff
Overwritten at the end of every session. This is the first thing to read.

---

**Session:** 23
**Tag:** `wu-17-green` is still the most recent *tag* — session 22's own
"Next work unit" asked Steve to tag it by hand; presumably done by now but
not confirmed by this session, which never touched v210/NEON-precedent code
beyond reading it for scoping. WU-18 is now implemented and verified in this
session's own Linux cloud sandbox (below) but not yet built or run at the
real terminal at all — unlike WU-17's own session, this session never had
device-bridge shell access to the real Mac, so there is no real-terminal
result to report yet, only the sandbox one.
**Phase:** 4 (Threading and NEON), second half. WU-17 (NEON v210) closed out
green last session. This session is WU-18, NEON chroma resampling — the
second and last NEON unit in Phase 4 (WU-19, "Real time at 576i25," is next
and is not a NEON unit itself). `DECISIONS.md` now runs through ADR-043;
`CORRECTIONS.md` is unchanged this session, still through C-015 (nothing was
claimed and found wrong this session in the sense `CORRECTIONS.md` tracks —
see "Corrections this session," below).

**This session's own first job, per Steve's own brief and
`SESSION-PROTOCOL.md`'s own discipline: real scoping before any code.**
`WORK-UNITS.md`'s own WU-18 line was barer going in than WU-17's own line
was before ADR-042 (WU-17 at least already had one accept criterion written
— "bit-identical to scalar reference"; WU-18 had neither that nor a
`Files:` line, nor even a one-line accept criterion). Read `src/video/
chroma.hpp`/`.cpp` (the WU-04 scalar reference, ADR-020's own frozen filter
coefficients/rounding/edge-handling, a frozen interface) and `tests/
test_chroma.cpp` closely first, per the brief.

**The module-layout question this session actually had to check, not
assume the same answer as WU-17's own session.** `docs/architecture.md`
section 8's own module-layout sketch names `v210.hpp/.cpp` as "unpack/pack,
scalar reference + NEON" explicitly, but `chroma.hpp/.cpp`'s own line reads
only "422↔444 polyphase" — no "+ NEON" mentioned, a real difference from
v210's own line. Checked directly rather than assumed the same "extend the
existing file pair" answer carries over: two pieces of evidence, both
already in the repository before this session started, resolve it the same
way anyway — `chroma.hpp`'s own top comment (written at WU-04) already says
"the scalar reference here is the oracle WU-18's NEON path is diffed
against — the same relationship v210.cpp already has to WU-17," and
architecture.md's own Phase 4 "done when" line names "NEON v210 and chroma
paths" together, chroma being one of exactly two modules this document ever
names for a NEON path at all. The module-layout table's per-line comments
are simply not uniformly detailed (`splat.hpp/.cpp`'s own line likewise
says nothing about NEON, with the actual "stays scalar" statement living in
a different section entirely), not a deliberate signal chroma's NEON code
belongs somewhere else. See `DECISIONS.md` ADR-043 for the full record.

**One correction this session was careful not to walk into.**
`CORRECTIONS.md` C-006 already established that chroma's own upsample→
downsample round trip is not bit-exact for non-flat fields (the downsample
filter is a deliberately lossy anti-aliasing stage, not a perfect-
reconstruction pair with upsample). This unit's own accept criterion is
therefore diffing NEON output against scalar output of the *same* filter
call (`upsampleRow` vs `upsampleRowNeon`, `downsampleRow` vs
`downsampleRowNeon`), never a round trip — `tests/test_chroma_neon.cpp`'s
own header comment states this explicitly.

**1. `src/video/chroma.hpp`/`.cpp`, extended (not new).** New NEON-suffixed
siblings — `upsampleRowNeon`, `downsampleRowNeon`, `upsampleImageNeon`,
`downsampleImageNeon` — guarded by `#if defined(__ARM_NEON)` in both files,
same guard v210.hpp/.cpp already use. The scalar `upsampleRow`/
`downsampleRow`/`upsampleImage`/`downsampleImage` (WU-04) are untouched.
Unlike v210's own NEON path (a fixed per-group bit interleave, identically
shaped for every group regardless of width), chroma's own irregularity is
`clampIndex()`'s boundary replication (ADR-020) — a no-op everywhere in a
row's interior, present only at a handful of indices near either end. The
new functions vectorise exactly the interior (four lanes of `int32x4_t`
multiply-accumulate per batch, matching the scalar functions' own
`std::int32_t` accumulator width, coefficients and rounding exactly, lane
for lane); the edge indices where a tap's `clampIndex()` call actually
replicates a boundary sample are computed scalar, calling the identical
`clampIndex()`/`roundShift()` helpers the scalar functions already use.
`downsampleRowNeon`'s own load pattern uses `vld2` deinterleaving loads at
six fixed offsets (the filter decimates by two, so consecutive output lanes
need input positions two apart — a plain contiguous load does not line up)
to extract its seven tap positions; its output is contiguous, so the result
is stored with a single `vst1_u16`. `upsampleRowNeon`'s co-sited samples
never need clamping at all and are left a plain scalar copy — no arithmetic
there to diverge on. Full design, including the associativity argument for
why NEON's own multiply-accumulate term order is still guaranteed
bit-exact (not the `CORRECTIONS.md` C-012 hazard, which is specific to
floating point) — see `DECISIONS.md` ADR-043.

**2. `tests/test_chroma_neon.cpp`, new.** Direct diffs of every NEON
function against its scalar counterpart, full 16-bit `Sample` domain (no
clamp of its own applies here, unlike v210's I2), at widths chosen so
`chromaWidth(width)` sweeps 1 through 14 — covering zero interior batches,
exactly one, and the transition into two, for both filters' own
differently-sized interior margins — plus 720/1920, the two real widths;
plus a whole-frame (720x576) image-wrapper check. 34 checks.

**3. `CMakeLists.txt`.** `test_chroma_neon` added to the exact same
`CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm64|aarch64)$"` guard block
`test_v210_neon` already uses — no new guard invented, no new
cross-compilation infrastructure added (the aarch64 toolchain file used for
this session's own sandbox verification was ad hoc, outside the repository,
same as WU-17's own session).

**Corrections this session:** none logged in `CORRECTIONS.md`, though one
routine-iteration bug is worth recording here (per `DECISIONS.md` ADR-043's
own "genuine bug" section): this session's own first draft of
`tests/test_chroma_neon.cpp` used `std::vector<Sample> in(std::size_t(cw))`
(and the equivalent with `width`) — the identical most-vexing-parse mistake
`HANDOFF.md` recorded for WU-17's own first draft of `test_v210_neon.cpp`.
It compiled clean in every default x86_64 configuration, because
`test_chroma_neon` is gated behind the same `CMAKE_SYSTEM_PROCESSOR`
condition as `test_v210_neon` and is therefore never *compiled* on x86_64
at all — only the aarch64 cross-compile actually built the file and caught
it, on its first attempt, before any claim was made based on the broken
draft. Fixed the same way WU-17's own session fixed it: named a
`const std::size_t` local first. Not a `CORRECTIONS.md` entry, same
"routine iteration, not a design/reasoning error" distinction `HANDOFF.md`
already drew for WU-17's own instance of this — but it is direct evidence
for *why* this session actually ran the aarch64 cross-compile rather than
treating it as a formality: an x86_64-only check would have shipped this
silently.

**Tests / Build — this session's own Linux cloud sandbox:**

*Default x86_64 (no toolchain file), confirming this unit leaves the
existing matrix completely unaffected:* Clang 18.1.3 and GCC 13.3.0,
Release and Debug, `SCATTER_TILE_LOG2` 4 and 5 (eight configurations, all
sixteen tests green — `test_chroma_neon`/`test_v210_neon` both correctly
absent, per the shared `CMAKE_SYSTEM_PROCESSOR` gate) plus GCC 13 with
`-fsanitize=address,undefined -fno-sanitize-recover=all` at both tile sizes
(clean). Zero warnings anywhere under this project's full `-Wall -Wextra
-Wpedantic -Wconversion -Wsign-conversion -Werror` set.

*AArch64 cross-compile + `qemu-aarch64-static` execution — genuine
execution, not just compilation, reusing ADR-042's own established
capability directly (`g++-aarch64-linux-gnu`/`qemu-user-static` installed
fresh in this session's own sandbox, confirmed still available rather than
assumed persistent from WU-17's own session):* GCC 13.3.0 via an ad hoc
CMake toolchain file (not committed), Release and Debug, tile 4 and 5 (four
configurations, all **eighteen** tests green — the sixteen carried over
plus `test_v210_neon` and the new `test_chroma_neon`) — this run is what
caught the most-vexing-parse bug above, on its first attempt. GCC 13 with
`-fsanitize=undefined -fno-sanitize-recover=all` on AArch64: eighteen tests
green, clean. `-fsanitize=address` on AArch64 reproducibly crashes
`qemu-aarch64-static` itself (`uncaught target signal 11`), reconfirmed —
the exact sandbox/emulator limitation ADR-042 already named, not a code
defect, deferred to the real M1 Max. Clang 18.1.3 cross-compile
(`--target=aarch64-linux-gnu --gcc-toolchain=/usr`, standalone, no CMake —
same shape ADR-042's own Clang check used): `chroma.cpp` + `v210.cpp` +
`test_chroma_neon.cpp` compiled and linked clean, run under
`qemu-aarch64-static`, `PASS test_chroma_neon (34 checks)`, zero warnings —
confirming both compilers this project's matrix already uses for x86_64
compile this unit clean, not only GCC.

**Tests / Build — real terminal, M1 Max, AppleClang:** not run this
session. Unlike WU-17's own session, this session had no device-bridge
shell access to the real Mac at any point — only the Linux cloud sandbox
above. This is the one thing genuinely outstanding before `WORK-UNITS.md`'s
WU-18 line can go `green`; see "Next work unit," below.

## Where we are

Phase 3 (SDI output) and Phase 4's threading half (WU-16a/16b) remain done,
unchanged since session 21/22. WU-17 (NEON v210) remains green, unchanged
this session. WU-18 (NEON chroma resampling): fully implemented and
verified in this session's own Linux cloud sandbox, both the default x86_64
matrix and genuine AArch64 execution via cross-compile + `qemu-aarch64-
static` — not yet run at the real terminal at all. `DECISIONS.md` now runs
through ADR-043; `CORRECTIONS.md` unchanged, through C-015.

**Delivery mechanics:** implementation and verification were done in this
session's own Linux cloud sandbox; final files were written to the real
repository via the device bridge (`chroma.hpp`, `chroma.cpp`,
`test_chroma_neon.cpp`, `CMakeLists.txt`, `DECISIONS.md`, `WORK-UNITS.md`,
this `HANDOFF.md`). **Not committed by this session.** Per the standing
operational note (carried over several sessions now): committing through
the device bridge on this repository reliably leaves stale
`.git/index.lock`/`HEAD.lock` files behind — the bridge can write files but
cannot unlink anything on Steve's machine. The exact commands Steve needs
to run himself are below, under "What to run at your terminal."

## Next work unit

**Steve's own next action: build and test at the real terminal, then close
out WU-18.**

```
cd ~/src/scatter-dve
cmake -B build && cmake --build build
ctest --test-dir build --output-on-failure
```

Expect 18 or 19 of 19 (or 18, depending on whether `test_decklink_*` is
configured) passing — the only anticipated failure is `test_decklink_device`'s
already-known, already-accepted full-duplex exception (ADR-035, the
UltraStudio Monitor 3G is playback-only), unrelated to this unit, the same
"N-1 of N" pattern every session since WU-15a has hit. `test_chroma_neon`
itself should show green alongside `test_v210_neon`. If AppleClang disagrees
with this session's own GCC/Clang-cross verification (no such disagreement
is expected — WU-17's own session had none — but WU-11's own C-012 shows it
has happened before), that is this unit's own bug to fix at the real
terminal per `SESSION-PROTOCOL.md`, not `DECISIONS.md` ADR-043 to relax.

Then, per this project's own established discipline (device-bridge commits
leave stale lock files — see "Delivery mechanics," above), commit and close
out by hand — exact commands under "What to run at your terminal," below.

After that: **WU-19** ("Real time at 576i25") is next, per `WORK-UNITS.md`'s
own ordering — the last unit in Phase 4, and the first unit whose own job is
throughput rather than correctness (both WU-17 and WU-18 explicitly deferred
performance tuning to it — WU-17's own denser `vld4q_u32` scheme, WU-18's
own `downsampleRowNeon` load-count redundancy, and PASS 1/2's per-frame
`ThreadPool` construction, ADR-040's own deferral). `WORK-UNITS.md`'s own
WU-19 line is bare going in — the same real-scoping-before-code treatment
every bare line in this project has needed so far.

## Open questions

Unchanged: Q1 (tile size), Q3 (macOS/Desktop Video version), Q4 (lattice
edge damping, C-008(a)). Q2 remains moot per ADR-037. ADR-037's own
follow-ups #1 (`test_decklink_device.cpp`'s full-duplex check) and #2
(genlock) remain open, unrelated to Phase 4.

Carried over from WU-17's own session, still open, now doubly relevant with
WU-18 also deferring throughput to WU-19: a denser NEON scheme for v210
(`vld4q_u32` across 4 groups, fully SoA) and, new this session, chroma's own
`downsampleRowNeon` load-count redundancy (six `vld2` loads for seven tap
positions, some overlap) — neither decided or scoped, both flagged for
WU-19 to pick up if its own real-time budget work finds either is actually
a bottleneck, not before.

## Blocked / red

Nothing red. WU-18 is fully green in this session's own Linux cloud
sandbox, both x86_64 (unaffected) and genuine AArch64 execution
(cross-compile + qemu, GCC and Clang, including UBSan). Blocked only on the
real-terminal run — see "Next work unit," above.

## Environment check

Unchanged from session 18–22 (ADR-037/039): **UltraStudio Monitor 3G** is
the active, confirmed output target. **UltraStudio Recorder 3G** is in
hand, named (ADR-039) as Phase 5's own input target, still untouched by any
code. **UltraStudio 4K Mini** remains on hold pending a PSU replacement.
None of this is relevant to WU-18's own work — pure `src/video/`, no
DeckLink code touched, no hardware needed to verify it (this session's own
verification used cross-compilation and emulation, the same reason WU-17's
own session gave, ADR-042).

## Append to DECISIONS.md

ADR-043 was appended in full this session; see `DECISIONS.md`. Does not
reopen `docs/architecture.md`, ADR-004, ADR-020 or ADR-042 — see ADR-043's
own closing paragraph for the precise relationship to each.

## Append to CORRECTIONS.md

Nothing appended this session — see "Corrections this session," above.

---

## What to run at your terminal

```
cd ~/src/scatter-dve
git status                                # confirm what the device bridge wrote
git add src/video/chroma.hpp src/video/chroma.cpp tests/test_chroma_neon.cpp \
        CMakeLists.txt DECISIONS.md WORK-UNITS.md HANDOFF.md
git commit -m "WU-18: NEON chroma resampling, verified via aarch64 cross-compile + qemu in the cloud sandbox (ADR-043)"
cmake -B build && cmake --build build
ctest --test-dir build --output-on-failure
./tools/close.sh 18
```

If `close.sh` refuses to tag because of the already-known
`test_decklink_device` full-duplex exception (ADR-035) — expected, the same
"N-1 of N" pattern every close since WU-15a has hit — tag it by hand the
same way as the last several units:

```
git tag -a wu-18-green -m "WU-18: build green, tests pass (test_decklink_device's full-duplex check is ADR-035's known exception, unrelated to WU-18)"
git push origin HEAD --tags   # if you keep a remote
```

If AppleClang disagrees with anything (unexpected, but see WU-11's own
C-012 for precedent that it can happen) — fix within `tests/
test_chroma_neon.cpp` or, if a genuine `chroma.cpp` bug, within
`src/video/chroma.cpp` alone; do not relax `DECISIONS.md` ADR-043 to route
around it.
