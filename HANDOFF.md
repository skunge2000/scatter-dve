# Handoff
Overwritten at the end of every session. This is the first thing to read.

---

**Session:** 22
**Tag:** `wu-16b-green` is still the most recent *tag* — Steve's own tagging
step from session 21's own handoff (see that session's "Next work unit") is
presumably done by now but not confirmed by this session, which never
touched threading code. This session's own unit, WU-17, stays `wip` in
`WORK-UNITS.md` — genuinely implemented and verified, but not yet built by
Steve at the real terminal (see below).
**Phase:** 4 (Threading and NEON). WU-16a/WU-16b (threading) both closed out
last session. This session opens Phase 4's *other* half: WU-17, NEON v210
unpack and pack — the first NEON unit in the project. `DECISIONS.md` now
runs through ADR-042; `CORRECTIONS.md` is unchanged this session, still
through C-015 (nothing was claimed and found wrong this session — see
"Corrections this session," below).

**This session's own first job, per Steve's own brief and
`SESSION-PROTOCOL.md`'s own discipline: real scoping before any code.**
`WORK-UNITS.md`'s own WU-17 line was as bare going in as WU-16's own line
was before WU-16a/16b split it (ADR-040) — a title and one accept criterion
("bit-identical to scalar reference"), no `Files:`. Read `src/video/v210.hpp`/
`.cpp` (the WU-02 scalar reference, frozen) and `tests/test_v210.cpp`
closely first, per the brief.

**The scoping question this session actually had to answer, not assume:**
whether this session's own Linux cloud sandbox (x86_64) can do anything
better than ADR-031/032's DeckLink precedent ("reasoned through against
headers, unverified until the real terminal") for a NEON unit — NEON
intrinsics are ARM64 *machine instructions*, not an absent SDK, so the
sandbox's own x86_64 CPU cannot execute them natively regardless of
compiler availability. **Checked directly, not assumed:** `apt-cache
policy` showed `g++-aarch64-linux-gnu` and `qemu-user-static` both
installable in this sandbox; installed and smoke-tested (a small NEON
`vaddq_u16` program, cross-compiled and run under `qemu-aarch64-static`) —
**this sandbox can genuinely execute real AArch64 NEON code**, not merely
cross-compile it blind. Materially stronger than ADR-031/032's DeckLink
units, which could not even compile against the real SDK here at all. Also
checked: Clang cross-compiles the same way; GCC's ASan (`-fsanitize=address`)
specifically crashes `qemu-aarch64-static` itself (a known QEMU user-mode
limitation, confirmed twice, not a code defect) while UBSan alone runs
clean. See `DECISIONS.md` ADR-042 for the full record, including why this
capability was used for genuine execution-verified cross-compilation rather
than left at "unverified until the real terminal."

**1. `src/video/v210.hpp`/`.cpp`, extended (not new).** New NEON-suffixed
siblings — `unpackRowNeon`, `packRowNeon`, `unpackImageNeon`,
`packImageNeon` — guarded by `#if defined(__ARM_NEON)` in both files. The
scalar `unpackRow`/`packRow`/`unpackImage`/`packImage` (WU-02) are
untouched, forever — internal platform-dispatch inside the existing names
was considered and rejected, since this unit's own accept criterion needs
"the scalar implementation, specifically" to remain callable to diff
against. One `vld1q_u32` loads a whole 16-byte v210 group (4 words) into
one 128-bit register; three vector ops extract all 12 of that group's
10-bit fields at once (replacing 12 scalar `component()` calls); the
field-to-Y/Cb/Cr placement itself (an irregular per-word interleave, no
clean vector shuffle across it — the same "no scatter instruction"
reasoning `docs/architecture.md` already gives for the splat) stays 12
scalar lane reads/writes. `readGroup()`/`writeGroup()` turned out to have
no dependency on `width` at all, so `unpackRowNeon`/`packRowNeon` needed no
separate "NEON fast path, scalar tail" structure — they are
`unpackRow`/`packRow`'s own loop bodies with only the group codec
substituted, so ADR-018's short-final-group behaviour is reused exactly,
not re-derived. `toCode10`/`fromCode10` (I2's clamp, the offset-binary
shift) are called identically on both paths, not reimplemented. One stale
comment corrected in place (`v210.hpp`, "Row operations — the primitives
WU-17 replaces" → "...WU-17's NEON path is diffed against", matching the
sibling-function design actually chosen; comment-only, no signature change).

**2. `tests/test_v210_neon.cpp`, new.** Direct diffs of every NEON function
against its scalar counterpart — never a hand-derived expected value, since
this unit's whole accept criterion is the diff itself: `unpackRowNeon` vs
`unpackRow` and `packRowNeon` vs `packRow` over widths covering every
residue mod 6 an even width can take (both the full-group and short-final-
group paths) and, for pack, the full 10-bit domain including the
TRS-reserved codes at I2's clamp boundary; `unpackImageNeon`/`packImageNeon`
vs `unpackImage`/`packImage` over a whole 720x576 frame, plus NEON's own
round trip. 53 checks.

**3. `CMakeLists.txt`.** `test_v210_neon` added, gated on
`CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm64|aarch64)$"` — deliberately its own
guard, not folded into the neighbouring Apple-only `-mcpu=apple-m1` block
(that one is tuning, unrelated to whether NEON itself compiles, and must
also match a non-Apple AArch64 Linux build this session's own cross-compile
verification used). No new project-level cross-compilation infrastructure
was added — the aarch64 cross toolchain file used for this session's own
sandbox verification was ad hoc, outside the repository (same
"standalone reproduction, not new project machinery" shape ADR-036 already
used), not committed. Reasoning in `DECISIONS.md` ADR-042.

**Corrections this session:** none. A most-vexing-parse compile error in
this session's own first draft of `test_v210_neon.cpp`
(`std::vector<Sample> Y(std::size_t(width)), ...`, which C++ grammar reads
as a function declaration) was caught by the compiler and fixed immediately
within the same edit, before any claim was ever made based on the broken
draft — routine iteration, not a design/reasoning error of the kind
`CORRECTIONS.md` tracks (same distinction ADR-033 draws for "an item
flagged unverified resolving... is not the same thing as an earlier claim
turning out wrong").

**Tests / Build — this session's own Linux cloud sandbox:**

*Default x86_64 (no toolchain file), confirming this unit leaves the
existing matrix completely unaffected:* Clang 18.1.3 and GCC 13.3.0,
Release and Debug, `SCATTER_TILE_LOG2` 4 and 5 (eight configurations, all
sixteen tests green — `test_v210_neon` correctly absent, per its own
`CMAKE_SYSTEM_PROCESSOR` gate, confirmed via the configure log's own
"skipping test_v210_neon" `STATUS` message) plus GCC 13 with
`-fsanitize=address,undefined -fno-sanitize-recover=all` at both tile sizes
(clean). Zero warnings anywhere under this project's full `-Wall -Wextra
-Wpedantic -Wconversion -Wsign-conversion -Werror` set.

*AArch64 cross-compile + `qemu-aarch64-static` execution — genuine
execution, not just compilation:* GCC 13.3.0 via an ad hoc CMake toolchain
file (`CMAKE_SYSTEM_NAME Linux`, `CMAKE_CROSSCOMPILING_EMULATOR
"qemu-aarch64-static;-L;/usr/aarch64-linux-gnu"`, not committed), Release
and Debug, tile 4 and 5 (four configurations, all **seventeen** tests
green — the sixteen carried over plus `test_v210_neon`'s own 53 checks),
plus GCC 13 with `-fsanitize=undefined -fno-sanitize-recover=all` (clean).
`-fsanitize=address` was tried and reproducibly crashes
`qemu-aarch64-static` itself before any test runs (`uncaught target signal
11`) — a sandbox/emulator limitation, not a code defect; deferred to the
real M1 Max. Clang 18.1.3 cross-compile (`--target=aarch64-linux-gnu
--gcc-toolchain=/usr`, standalone, no CMake — same shape ADR-036's own
verification used): `v210.cpp` + `test_v210_neon.cpp` compiled and run
under `qemu-aarch64-static`, `PASS test_v210_neon (53 checks)`, zero
warnings — confirming both compilers this project's matrix already uses
for x86_64 compile this unit clean, not only GCC.

**Tests / Build — real terminal, M1 Max, AppleClang:** not run this
session. Genuinely different from ADR-031/032's DeckLink units in one
respect (every line this unit adds has already executed correctly for
real, in this session's own sandbox, not merely been reasoned through) but
still outstanding in the two ways any unit's real-terminal run always is:
AppleClang specifically hasn't compiled it yet (this session's own
verification used GCC and Clang's mainline cross target, not AppleClang —
a different compiler, with this project's own `-mcpu=apple-m1` tuning
already applied via the existing CMake block), and ASan on AArch64 hasn't
been tried at all (the one config this session's own sandbox genuinely
could not attempt, per the qemu limitation above — the real M1 Max has no
such limitation).

## Where we are

Phase 3 (SDI output) and Phase 4's threading half (WU-16a/16b) remain done,
unchanged since session 21. Phase 4's NEON half: WU-17 implemented and
verified — genuinely executed as real AArch64 code via cross-compile +
`qemu-aarch64-static`, not merely reasoned through — but not yet built at
the real terminal. `DECISIONS.md` now runs through ADR-042; `CORRECTIONS.md`
unchanged, through C-015.

**Delivery mechanics:** implementation and verification were done in a
separate Linux cloud sandbox (the same one every unit since WU-01 has used,
plus this session's own aarch64 cross-compile + qemu addition); final files
(`src/video/v210.hpp`, `src/video/v210.cpp`, `tests/test_v210_neon.cpp`,
`CMakeLists.txt`, `DECISIONS.md`, `WORK-UNITS.md`, this `HANDOFF.md`) were
written to the real repository via the device bridge. **Not committed by
this session** — per this project's own carried-over operational note,
committing through the device bridge on this machine reliably leaves stale
`.git/index.lock`/`HEAD.lock` files behind (the bridge can write files but
cannot unlink anything on this machine), so Steve runs the commit himself;
exact commands below.

## Next work unit

**Steve's own next action: build and test WU-17 at the real terminal, then
commit.**

```
cd ~/src/scatter-dve
cmake --build build
ctest --test-dir build --output-on-failure
```

If green (expect 17/18 — the one already-known `test_decklink_device`
full-duplex exception, ADR-035, unrelated to WU-17; anything else failing
is a real problem, not an accepted exception):

```
git add src/video/v210.hpp src/video/v210.cpp tests/test_v210_neon.cpp \
        CMakeLists.txt DECISIONS.md WORK-UNITS.md HANDOFF.md
git commit -m "WU-17: NEON v210 unpack/pack, verified via aarch64 cross-compile + qemu in the cloud sandbox (ADR-042)"
./tools/close.sh 17
```

If `close.sh` tags `wu-17-green` on its own (no `test_decklink_device`
exception hit, or Steve accepts it by hand the same way as `wu-15a-green`/
`wu-16a-green`/`wu-16b-green`), WU-17 is done and **WU-18** (NEON chroma
resampling) is next in Phase 4, per `WORK-UNITS.md`'s own ordering — bare
going in, same as WU-17 was; needs the same real-scoping-before-code
treatment against `src/video/chroma.hpp`/`.cpp` (ADR-020's own frozen
filter coefficients) and `tests/test_chroma.cpp`.

If AppleClang rejects anything this session's own GCC/Clang-cross
verification did not catch — genuinely possible, since AppleClang is a
third compiler this session never tried — fix it at the real terminal
(`SESSION-PROTOCOL.md` rule: production code only changes where a real
problem is found, not speculatively) and note what AppleClang caught that
the cross-compile matrix didn't, in `CORRECTIONS.md`, before closing.

## Open questions

Unchanged: Q1 (tile size), Q3 (macOS/Desktop Video version), Q4 (lattice
edge damping, C-008(a)). Q2 remains moot per ADR-037. ADR-037's own
follow-ups #1 (`test_decklink_device.cpp`'s full-duplex check) and #2
(genlock) remain open, unrelated to Phase 4.

New, named but not resolved, per ADR-042's own "considered and set aside"
note: a denser NEON scheme (`vld4q_u32` across 4 groups at once, fully SoA
before any scalar step) — this unit's own accept criterion is correctness,
not throughput (WU-19's job), so the simpler one-group-per-register shape
was kept; worth revisiting if WU-19's own real-time budget work finds v210
unpack/pack is actually a bottleneck.

## Blocked / red

Nothing red. WU-17 is fully green in this session's own sandbox (both the
default x86_64 matrix, unaffected, and genuine AArch64 execution via
cross-compile + qemu); only the real-terminal AppleClang build and
`close.sh 17` are outstanding, Steve's own next step per above.

## Environment check

Unchanged from session 18–21 (ADR-037/039): **UltraStudio Monitor 3G** is
the active, confirmed output target. **UltraStudio Recorder 3G** is in
hand, named (ADR-039) as Phase 5's own input target, still untouched by any
code. **UltraStudio 4K Mini** remains on hold pending a PSU replacement.
None of this is relevant to WU-17's own work — pure `src/video/`, no
DeckLink code touched, no hardware needed to verify it (this session's own
verification used cross-compilation and emulation instead, for a different
reason than the DeckLink units' own "no toolchain at all" — see ADR-042).

## Append to DECISIONS.md

ADR-042 was appended in full this session; see `DECISIONS.md`. Does not
reopen `docs/architecture.md`, ADR-013, ADR-017, ADR-018 or ADR-031 — see
ADR-042's own closing paragraph for the precise relationship to each.

## Append to CORRECTIONS.md

Nothing appended this session — see "Corrections this session," above.

---

## What to run at your terminal

```
cd ~/src/scatter-dve
cmake --build build
ctest --test-dir build --output-on-failure
git add src/video/v210.hpp src/video/v210.cpp tests/test_v210_neon.cpp \
        CMakeLists.txt DECISIONS.md WORK-UNITS.md HANDOFF.md
git commit -m "WU-17: NEON v210 unpack/pack, verified via aarch64 cross-compile + qemu in the cloud sandbox (ADR-042)"
./tools/close.sh 17
```

If `close.sh` refuses to tag because of the already-known
`test_decklink_device` full-duplex exception (ADR-035, unrelated to
WU-17), tag by hand the same way as the last three units:

```
git tag -a wu-17-green -m "WU-17: build green, tests pass (test_decklink_device's full-duplex check is ADR-035's known exception, unrelated to WU-17)"
git push origin HEAD --tags   # if you keep a remote
```
