# Handoff
Overwritten at the end of every session. This is the first thing to read.

---

**Session:** 23
**Tag:** `wu-17-green` stands, unchanged. `wu-18-green` was tagged this
session — Steve ran the `git tag -a wu-18-green ...` command given below,
and a second attempt (pasted again) correctly errored "tag already exists,"
confirming the first attempt succeeded silently (`git tag -a` prints
nothing on success). Not independently re-verified by this session that the
tag points at the WU-18 commit specifically (`4ea23b8...`, per Steve's own
`git commit` output below) rather than some stale prior tag — Steve was
asked to confirm with `git show wu-18-green --no-patch --format="%H %s"`
against `git log -1`; if that check comes back clean, there is nothing
further to do here.
**Phase:** 4 (Threading and NEON) is now done in full, pending only that
tag-identity confirmation above. WU-16a/16b (threading) and WU-17 (NEON
v210) were already green going in; this session closed out WU-18 (NEON
chroma resampling), the phase's second and last NEON unit. `DECISIONS.md`
now runs through ADR-043; `CORRECTIONS.md` is unchanged this session, still
through C-015 (nothing was claimed and found wrong this session in the
sense `CORRECTIONS.md` tracks — see "Corrections this session," below).

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
defect, deferred to the real M1 Max (see below — untried there too, not
required by this unit's own accept criterion). Clang 18.1.3 cross-compile
(`--target=aarch64-linux-gnu --gcc-toolchain=/usr`, standalone, no CMake —
same shape ADR-042's own Clang check used): `chroma.cpp` + `v210.cpp` +
`test_chroma_neon.cpp` compiled and linked clean, run under
`qemu-aarch64-static`, `PASS test_chroma_neon (34 checks)`, zero warnings —
confirming both compilers this project's matrix already uses for x86_64
compile this unit clean, not only GCC.

**Tests / Build — real terminal, M1 Max, AppleClang (Steve's own turn, this
session):** `cmake -B build && cmake --build build` succeeded clean
(configure log confirms `test_v210_neon, test_chroma_neon configured` and
the DeckLink SDK found, same as every session since WU-14) — `chroma.cpp`
and `test_chroma_neon.cpp` compiled clean under AppleClang, the one
compiler this session's own sandbox verification could not try (GCC/
Clang-cross, not AppleClang). Full suite: **19 of 20 passing.**
`test_chroma_neon` itself green (0.14s on the fresh build, 0.00s on
`close.sh`'s incremental rebuild) — the real-terminal confirmation this
unit's own bit-identical-to-scalar claim needed, now genuinely established
on real M1 Max hardware, not only via cross-compile + qemu. The one
failure, `test_decklink_device.cpp:53`,
`test_at_least_one_device_is_full_duplex` (`foundDuplexDevice` staying
false) — this is **ADR-035's own already-named, already-accepted
exception**, unrelated to WU-18: the UltraStudio Monitor 3G is
playback-only by design, the same "N-1 of N" pattern every close since
WU-15a has hit. WU-18 touches no `src/io/`, `decklink_*` or
`test_decklink_*` file at all, so this is not a regression this unit
introduced — `test_decklink_output` itself passed both its checks (5.28s
fresh, 5.16s on rebuild), confirming the DeckLink-side mechanics this unit
doesn't touch are still fine. `./tools/close.sh 18` correctly refused to
tag (its own gate cannot distinguish an accepted exception from a real
failure, by design), the same way it correctly refused for WU-15a, WU-16a,
WU-16b and WU-17. Steve was given the `git tag -a wu-18-green ...` command
and ran it; see "Tag," above, for the one loose end (tag-identity
confirmation) left for this handoff's own next check. ASan on AArch64 (the
one config this session's own sandbox genuinely could not attempt, per the
qemu limitation above) remains untried on any platform — not required for
this unit's own accept criterion, and not blocking; flagged as a possible
future check, not scheduled, the same status WU-17's own handoff left it
at.

## Where we are

Phase 3 (SDI output) remains done, unchanged. Phase 4 (Threading and NEON)
is now done in full: WU-16a/16b (threading) and both NEON units (WU-17
v210, WU-18 chroma) are all green at the real terminal. `DECISIONS.md` now
runs through ADR-043; `CORRECTIONS.md` unchanged, through C-015.

**Delivery mechanics:** implementation and verification were done in this
session's own Linux cloud sandbox; final files were written to the real
repository via the device bridge (`chroma.hpp`, `chroma.cpp`,
`test_chroma_neon.cpp`, `CMakeLists.txt`, `DECISIONS.md`, `WORK-UNITS.md`,
an earlier draft of this `HANDOFF.md`). Steve then committed, built, tested
and tagged at his own terminal — commit `4ea23b8`, per the standing
operational note (device-bridge commits leave stale
`.git/index.lock`/`HEAD.lock` files on this machine — the bridge can write
files but cannot unlink anything here). This replacement `HANDOFF.md`
itself still needs writing back and committing — see "What to run at your
terminal," below; working tree will be dirty with just this one file until
then.

## Next work unit

**Steve's own immediate next action: the one loose end from "Tag," above —
confirm `wu-18-green` points at the WU-18 commit, not a stale prior tag:**

```
git show wu-18-green --no-patch --format="%H %s"
git log -1 --format="%H %s"
```

If those match, nothing further — just `git push origin HEAD --tags` if a
remote is kept, and commit this replacement `HANDOFF.md` (see "What to run
at your terminal," below).

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

Nothing red. WU-18 is green at both the Linux sandbox and the real
terminal. Only the tag-identity confirmation above is outstanding, and
that is Steve's own quick check per "Next work unit."

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

Already done, this session:

```
cd ~/src/scatter-dve
git add src/video/chroma.hpp src/video/chroma.cpp tests/test_chroma_neon.cpp \
        CMakeLists.txt DECISIONS.md WORK-UNITS.md HANDOFF.md
git commit -m "WU-18: NEON chroma resampling, verified via aarch64 cross-compile + qemu in the cloud sandbox (ADR-043)"
cmake -B build && cmake --build build
ctest --test-dir build --output-on-failure    # 19/20 -- ADR-035's known exception
./tools/close.sh 18                            # correctly refused to tag -- by design
git tag -a wu-18-green -m "..."                # run, apparently successfully (see "Tag," above)
```

Still to do:

```
git show wu-18-green --no-patch --format="%H %s"   # confirm this matches:
git log -1 --format="%H %s"
git push origin HEAD --tags                         # if you keep a remote

git add HANDOFF.md
git commit -m "WU-18: session close, real-terminal confirmation recorded"
```
