# Handoff

Overwritten at the end of every session. This is the first thing to read.

---

**Session:** 1
**Tag:** `wu-01-green`
**Phase:** 1 — portable core, file to file, 576p25, single-threaded
**Tests:** GREEN — `test_smoke`, 2076 checks
**Build:** clean under `-Werror -Wconversion -Wsign-conversion`, Release and
Debug, at `SCATTER_TILE_LOG2` 4 and 5

## Where we are

WU-01 done. `src/core/types.hpp` holds the colour representation, `Frag`,
accumulator types and tiling constants, with invariants I2/I3/I4 encoded as
`static_assert`. `tests/harness.hpp` provides `CHECK`/`CHECK_ONCE`.
`CMakeLists.txt` builds `scatter-core` and registers tests.

No algorithm yet. Nothing reads or writes video.

## Next work unit

**WU-02 — v210 unpack and pack, scalar reference.**

Deliver: `src/video/v210.hpp`, `src/video/v210.cpp`, `tests/test_v210.cpp`,
plus the `CMakeLists.txt` edit described below.

Scope notes:

- v210 packs exactly 6 pixels per 16 bytes: four 32-bit little-endian words,
  three 10-bit components each in the low 30 bits. Component order per group
  of four words is Cb-Y-Cr, Y-Cb-Y, Cr-Y-Cb, Y-Cr-Y.
- Unpack to three separate 16-bit planes via `fromCode10`. Pack via
  `toCode10`, which rounds and applies the protocol clamp — the only clamp
  permitted anywhere (I2).
- Row stride is not derivable from width in general. Take it as a parameter.
  For 720 it is 1920 bytes and for 1920 it is 5120, both exact and already
  128-byte aligned, but do not rely on that for arbitrary widths.
- Scalar only. NEON is WU-17 and must be diffed against this implementation.
- **`CMakeLists.txt` change:** `scatter-core` becomes `STATIC` now that there
  is a translation unit. Replace `add_library(scatter-core INTERFACE)` and
  change every `INTERFACE` keyword on that target to `PUBLIC`. Add
  `scatter_test(test_v210)`.

**Session open command:**

```
./tools/open.sh CMakeLists.txt src/core/types.hpp
```

`types.hpp` is needed because WU-02 uses its converters and constants.
`harness.hpp` and `test_smoke.cpp` are not — do not pass them.

## Open questions

- **Q1.** Tile size 16×16 (32 KB across four banks) versus 32×32 (128 KB).
  Now a CMake cache variable `SCATTER_TILE_LOG2`, default 5, and both
  configurations are verified to build and pass. Benchmark at WU-09. Do not
  resolve before then.
- **Q2.** Whether the 4K Mini's two program outputs are genuinely mirrored.
  Blocks nothing before WU-14.
- **Q3.** macOS version and matching Desktop Video release. Blocks WU-14.

## Blocked / red

Nothing.

## Environment check still outstanding

Phase 0 from `docs/architecture.md` section 10: install Desktop Video, approve
the system extension, confirm the UltraStudio 4K Mini enumerates with both
input and output, capture and play a clip in Media Express. Independent of the
next several work units — can be done in parallel and costs no session time.
Resolves Q3.
