# HANDOFF — Session 83

## Where we are

**Verified repo state at session start, not trusted from the incoming prompt — this time it matched.** `HEAD` and `origin/main` both `aac353c` ("WU-33c6 scoped (not built) -- DECISIONS.md ADR-100"), working tree dirty only in `HANDOFF.md` (Session 82's own draft, never itself a blocker per `SESSION-PROTOCOL.md`), `wu-45-green`/`wu-46-green`/`wu-47-green` all still real tags at their expected commits. No correction needed at session open.

**Job (`WU-33c6`, built and `green` this session):** Session 82 scoped this unit (`DECISIONS.md` ADR-100) but did not build it — `OwnedSourceRaster::rgb` (`core/resolve.hpp`, WU-33c1/ADR-094) changes from an owned-by-value `video::RasterRGB` to a `std::shared_ptr<video::RasterRGB>`, so copying an `OwnedSourceRaster` becomes an atomic refcount increment instead of a three-`std::vector` deep copy (~2.5 MB at 720x576) — the real, named cost `FileBackSource::currentSourceRaster()` (WU-33c2a) pays on every single processed frame for a static frame that never changes, one of the leads ADR-099 parked against Steve's own reported frame-rate drop.

This session re-derived the scope against the real current code before writing anything (per the incoming instruction and per `SESSION-PROTOCOL.md`'s own standing rule), and found one small discrepancy in ADR-100/`WORK-UNITS.md`'s own prior note: `core/pipeline.cpp`'s own `unpackSourceRaster()` has two touched lines, not five (four `.` -> `->` conversions across those two lines) — described in full in the new `DECISIONS.md` ADR-101, not logged as a `CORRECTIONS.md` entry (cosmetic miscount, not a wrong `Files:`/`Accept:` line or a code/test defect). Everything else in ADR-100's own design was built exactly as scoped.

**Built:** `src/core/resolve.hpp` (`OwnedSourceRaster::rgb` is now `std::shared_ptr<video::RasterRGB>` — first `std::shared_ptr` anywhere in `src/`; constructor uses `std::make_shared`; `view()` updated to `rgb->...`; `#include <memory>` added; header comment extended, not replaced). `src/core/pipeline.cpp` (`unpackSourceRaster()`'s two `out.rgb.` lines become `out.rgb->`, mechanical only). `tests/test_unpack_source_raster.cpp` (WU-33c1's own two existing checks kept, their now-stale "deep copy"/"genuinely freed" comments corrected in place; two new checks added — (3) a copy's own `view().r/.g/.b` compare pointer-equal to the original's; (4) two independently-constructed `OwnedSourceRaster`s compare pointer-unequal).

## Build/test verification (this session, cloud sandbox + device bridge's own Linux VM)

Built and tested for real, not merely scoped — full account in `DECISIONS.md` ADR-101.

- Baseline confirmed first: fresh `git clone` of `origin` at `aac353c` built clean, `test_unpack_source_raster` 7/7 green, GCC 13.3.0 Release — matching WU-33c1's own already-`green` state exactly.
- After this session's three edits, three configurations, full 32/32 `ctest` suite green in every one, zero compiler warnings:
  - GCC 13.3.0 Release — `test_unpack_source_raster` now 13 checks (up from 7).
  - Clang 18.1.3 Release — 32/32 green.
  - GCC 13.3.0 Debug, `-fsanitize=address,undefined -fno-sanitize-recover=all` — 32/32 green; `nm -D` found 42 asan/ubsan symbol references, `ldd` confirmed `libasan.so.8`/`libubsan.so.1` genuinely linked.
- Regression-detection proved twice, not assumed: (a) temporarily reverted `rgb` to an owned-by-value member — exactly check 3's own three assertions failed, nothing else; (b) temporarily made the constructor share one `static` `shared_ptr` across every instance — exactly check 4's own three assertions failed, check 3 unaffected. Both mutations reverted and confirmed byte-identical to the pre-mutation file before rebuilding back to 13/13 green; neither mutation was ever written to the real repository.
- All three changed files written into the real repository via the device bridge, then re-staged and diffed byte-for-byte against the cloud sandbox's own confirmed-green copy — identical.
- Independent confirmation beyond the diff: compiled directly on the device bridge's own Linux VM (bare `g++ 11.4.0`, no CMake there, confirmed again this session) against this project's own full `-std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror` flag set, from the real just-written files, not the sandbox copy: `test_unpack_source_raster` 13/13 green; `test_file_back_source` (WU-33c2a) 8/8, unchanged; `test_pipeline_backsrc` (WU-33a/b) 17/17, unchanged. Scratch object/binary files this produced were written outside the mounted repository folder and deleted before session end; none were ever committed.

**Not yet built, run, tagged or pushed at Steve's own real terminal.**

## What's next (Steve's own to run)

1. Review the diff — `git status --short` should show exactly six modified files (`src/core/resolve.hpp`, `src/core/pipeline.cpp`, `tests/test_unpack_source_raster.cpp`, `WORK-UNITS.md`, `DECISIONS.md`, `HANDOFF.md`), nothing else:
   ```
   cd ~/src/scatter-dve
   git status --short
   git diff -- src/core/resolve.hpp src/core/pipeline.cpp tests/test_unpack_source_raster.cpp WORK-UNITS.md DECISIONS.md
   ```
2. Commit everything (including this `HANDOFF.md`, so the tree is clean before `close.sh` runs — it refuses on any uncommitted change, `HANDOFF.md` included), then let `tools/close.sh` build, test, tag, and push in one step:
   ```
   git add src/core/resolve.hpp src/core/pipeline.cpp tests/test_unpack_source_raster.cpp WORK-UNITS.md DECISIONS.md HANDOFF.md
   git commit -m "WU-33c6: copy-on-write OwnedSourceRaster via std::shared_ptr<RasterRGB> -- DECISIONS.md ADR-101"
   ./tools/close.sh 33c6
   ```
   `close.sh` builds (default Release config, whatever `BLACKMAGIC_SDK_DIR` is already cached in `build/` from earlier sessions — this unit is core-only and does not depend on it either way), runs the full `ctest` suite, tags `wu-33c6-green` only if everything passes, and pushes `HEAD --tags` to `origin` automatically on success. If it reports `WARNING: push failed; commit is local only.` (this session's own device-bridge shell cannot push — no stored credentials there — but `close.sh` runs at your own real terminal, where `git push` has worked in every prior session), push by hand:
   ```
   git push origin main
   git push origin --tags
   ```
3. This session's own cloud-sandbox/device-bridge-VM verification is a genuine, independent confirmation (three sandbox configurations plus a fourth compiler on the real hardware, all green) but is not a substitute for your own `close.sh` run at your real terminal, same standing convention every session uses.
4. **Optional, unrelated to this unit, still parked from Session 82 (ADR-099):** if you want to chase whether this unit actually moves the `framesPushed`/`framesArrived` ratio, rebuild and rerun `test_decklink_live_backsrc_demo` (DeckLink-linked, real-terminal only) and compare against the 114/752 baseline `DECISIONS.md` ADR-099/ADR-100 already cite. Nothing in this session claims that ratio moved — see ADR-101's own explicit non-claim.

## What's broken / flagged, not fixed (out of this session's own scope)

**`docs/wu-audit-2026-08.md` line 113 is still stale — re-checked directly this session, not fixed, same standing instruction Sessions 72-82 already followed, now eleven-plus sessions running (72 through 83).** It still reads: "The real-content gap (single tag per call) was already found and logged as C-020/ADR-062 before this sweep, and closed by WU-28c. Nothing new." Still incorrect: `WU-28c` only ever built the `TagByFacing` functions; the frame-mode gap was not closed until `WU-35a4`, and field mode's own equivalent gap was not closed until `WU-47`. Left unfixed — outside this session's own scope (`src/core/resolve.hpp`, `src/core/pipeline.cpp`, `tests/test_unpack_source_raster.cpp`, `WORK-UNITS.md`, `DECISIONS.md`), same restraint every session since 72 has already applied.

**Two items parked at Steve's own request since Session 82 (ADR-099), untouched by this session, still open:**
- Which capture device is actually fed, and whether/how to make `firstFormatDetectionCapableInput()` (or the existing `selectFormatDetectionCapableInput()` from WU-33c2b) let Steve choose it explicitly instead of always taking the first match. Still blocks WU-33c5.
- Whether `FileBackSource`'s own per-frame copy (now fixed by this unit) was actually the cause of the reported frame-rate drop — this unit proves the copy was real and is now gone; it does not and cannot prove that was the actual bottleneck. Confirming that against real hardware is Steve's own job (item 4 above).

## Untouched, deliberately

`INVARIANTS.md` (not touched — grepped again this session for `OwnedSourceRaster`/`RasterRGB`/`shared_ptr`, no match, no invariant this session's work bears on). `ADR-059/062/065/069/070/072/073/074/077/082` through `099` (not reopened — ADR-094 extended by ADR-100 already, ADR-100 itself extended by this session's new ADR-101, neither revised). `CORRECTIONS.md` (no new entry — this session's own one discrepancy found, the "five touched lines" miscount, was judged cosmetic per the threshold explained in ADR-101, not a `Files:`/`Accept:`/code/test error). `io/file_back_source.hpp`/`.cpp`, `io/decklink_back_source.hpp`/`.cpp`, `io/decklink_capture_consumer.hpp`/`.cpp` (re-confirmed zero-touch by grep this session, not edited). `video::RasterRGB` itself (`video/raster.hpp`, untouched — still genuine local scratch storage elsewhere in `core/pipeline.cpp`, confirmed unrelated to `OwnedSourceRaster::rgb` by this session's own line-by-line review). `core/binner.hpp`/`SourceRaster` (already read-only, needs no change). `docs/wu-audit-2026-08.md` (flagged above, not fixed). `tests/test_decklink_live_backsrc_demo.cpp` and every DeckLink-linked file from Sessions 78-82's own WU-33c2b/c3/c4/c5 work (not read or touched this session — this unit's whole point was that it needed none of them).
