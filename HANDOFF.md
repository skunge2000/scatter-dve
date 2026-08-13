# Handoff
Overwritten at the end of every session. This is the first thing to read.

---

**Session:** 5
**Tag:** `wu-05-green` — confirmed. `./tools/close.sh 05` ran clean on the
M1 Max with AppleClang (Release, tile 2^5, the config `close.sh` builds) and
tagged it.
**Phase:** 1 — portable core, file to file, 576p25, single-threaded

**Tests:** All five green on the M1 Max: `test_smoke`, `test_v210`,
`test_chroma`, `test_ramp_roundtrip`, `test_testpat`. `test_ramp_roundtrip`
is new this session (4459 checks); the other four are unchanged (WU-05
touched none of their files).

Before that, this session verified on Clang 18 and GCC 13 in a Linux
sandbox (no AppleClang there), both under the project's exact warning set
(`-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror`), Release
and Debug, `SCATTER_TILE_LOG2` 4 and 5 (four configurations × two compilers,
all green — eight total counting both tile sizes) — same practice as prior
sessions. Also ran the full suite under GCC 13 with
`-fsanitize=address,undefined -fno-sanitize-recover=all` (Debug): clean, no
ASan or UBSan report anywhere.

**Build:** clean under `-Werror -Wconversion -Wsign-conversion` on AppleClang
(M1 Max), Clang 18 and GCC 13.

## Where we are

WU-05 done and closed green. `src/video/raster.hpp` adds `Plane`/`ConstPlane`
(pointer + stride + width + height, self-describing per plane) and two owning
frame types, `Raster422` (tight-packed 4:2:2) and `Raster444` (tight-packed
4:4:4) — the general-purpose counterpart to `tools/testpat.hpp`'s `Frame`,
which stays scoped to the tool and its own test per ADR-019.
`src/io/file_source.cpp` and `src/io/file_sink.cpp` implement
`readV210File`/`writeV210File` (declared in `raster.hpp` — see deviation
note below), raw `.v210`, no header, stride always
`v210::rowBytesMin(width)`. `tests/test_ramp_roundtrip.cpp` checks three
separate things, not one — see below, this is the session's main finding.

**The WU-05 accept criterion in `WORK-UNITS.md` was wrong, and is corrected
this session (CORRECTIONS.md C-006).** It read: "ramp and excursion patterns
round-trip bit-exactly through unpack → upsample → downsample → pack."
Multiplied out with the actual coefficients ADR-020 froze at WU-04 — which
postdates whenever that line was drafted — this is false for non-flat
chroma: `chroma::downsampleImage(chroma::upsampleImage(x))` does not
recover `x` except for a flat field. The downsample filter is a half-band
low-pass chosen for anti-aliasing, not built as a perfect-reconstruction
pair with the upsample filter, and ADR-020 says as much (ringing, overshoot)
without ever claiming round-trip identity. Deviations on the excursion
pattern reach several hundred codes at its sharp transitions. This is
correct filter behaviour, not a bug — nothing about it violates I2 or
reopens ADR-020. `tests/test_ramp_roundtrip.cpp` now checks what is
actually true: (1) `writeV210File`→`readV210File` alone is bit-exact for any
pattern; (2) the full chain, file to file, is bit-exact for a flat field
(ADR-020) and (3) for luma always, on any pattern, since chroma resampling
never touches Y; chroma on ramp/excursion is checked only for staying
within the v210 protocol range (I2), not for equality. `WORK-UNITS.md`'s
WU-05 accept line is rewritten to state these three properties instead of
blanket bit-exactness.

**Deviation from the WU-05 file list in `WORK-UNITS.md`:** that list named
`src/video/raster.hpp`, `src/io/file_source.cpp`, `src/io/file_sink.cpp`,
`tests/test_ramp_roundtrip.cpp` — no headers for the two `.cpp` files.
`readV210File`/`writeV210File` are declared in `raster.hpp` instead of two
new near-empty headers; see the comment at the top of `raster.hpp`.
`CMakeLists.txt` was also touched, same mechanical registration as every
prior unit: `file_source.cpp`/`file_sink.cpp` added to `scatter-core`'s
sources (not `scatter` — see ADR-021, appended this session) and
`test_ramp_roundtrip` registered via `scatter_test()`.

While reading the current state at session start, `WORK-UNITS.md` still
marked WU-04 `wip` even though `wu-04-green` already existed as a git tag —
same doc-sync slip as WU-03 had at the start of session 4. Corrected this
session, alongside marking WU-05 `green`.

**Delivery mechanics note, not a design matter:** the remote-device bridge
used to get these changes onto this machine still can't unlink files under
the mounted folder (not just via `git commit`, as session 4 found, but for
git's own `index.lock`/`HEAD.lock`/temp-object cleanup on any git
invocation through that bridge). `mv` works where `rm`/`unlink` doesn't, so
stale lock files got moved into `_to_delete/` rather than removed —
gitignored this session so it doesn't block `close.sh`'s clean-tree check.
Safe to `rm -rf _to_delete/` (and a stray `~/leftover_scatter_lockfiles`
outside the repo) by hand at the real terminal. `./tools/close.sh 05`
itself was, as before, run at the real terminal, not through the bridge.

## Next work unit

**WU-06 — Lattice and Jacobian**, per `WORK-UNITS.md`. 129×129 control
lattice, Catmull-Rom expansion, analytic first derivatives.
**Files:** `src/core/lattice.hpp`, `src/core/lattice.cpp`,
`tests/test_jacobian.cpp`.
**Accept:** analytic derivatives agree with central differences to 1e-6
relative across the lattice interior and at edges. Unstarted.

## Open questions

Unchanged: Q1 (tile size, WU-09), Q2 (4K Mini program outputs, WU-14), Q3
(macOS/Desktop Video version, WU-14).

## Blocked / red

Nothing. WU-05 closed green.

## Environment check still outstanding

Unchanged from session 2 — Desktop Video / UltraStudio 4K Mini smoke test,
independent of WU-06 and costs no session time.

## Append to DECISIONS.md

ADR-021 was appended in full during this session; see `DECISIONS.md`.

## Append to CORRECTIONS.md

C-006 was appended in full during this session; see `CORRECTIONS.md`.
