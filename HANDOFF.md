# Handoff

Overwritten at the end of every session. This is the first thing to read.

---

**Session:** 2
**Tag:** `wu-02-green`
**Phase:** 1 — portable core, file to file, 576p25, single-threaded
**Tests:** GREEN — `test_smoke` 2076 checks, `test_v210` 635 checks
**Build:** clean under `-Werror -Wconversion -Wsign-conversion`, Release and
Debug, at `SCATTER_TILE_LOG2` 4 and 5

## Where we are

WU-02 done. `src/video/v210.hpp` and `.cpp` hold the scalar v210 unpack and
pack. Row functions are the primitive — `unpackRow` / `packRow` — with
`unpackImage` / `packImage` looping over them. Byte strides on the packed side,
sample strides on the planar side, both taken as parameters. Geometry helpers
are `constexpr`: `groupsPerRow`, `rowBytesMin`, `rowBytesAligned`,
`chromaWidth`, `isSupportedWidth`.

`scatter-core` is now `STATIC`; every usage requirement moved from `INTERFACE`
to `PUBLIC`. `scatter_test(test_v210)` registered.

Planes at this stage are 4:2:2 — luma `width` wide, chroma `width / 2`,
co-sited with even luma. Upsampling to 4:4:4 is WU-04 and happens after unpack,
never during it.

`packRow` is the single clamp site in the pipeline (I2) and the only place
`toCode10` is called on the output path. Nothing legalises.

Still nothing reads or writes files; that is WU-05.

### Measured, this session

- Round trip byte-identical over random legal-code planes at widths 2, 4, 6, 8,
  10, 12, 14, 16, 18, 22, 720, 1920, and over a full 720×576 frame.
- Codes 4 and 1019 survive. Sub-black (5, 63) and super-white (941, 1000) pass
  through untouched. Samples outside the protocol range clamp at pack and
  nowhere else.
- **Cross-checked against FFmpeg.** Our packed output is byte-identical to
  FFmpeg's v210 encoder given the same plane data, at 12×2, 48×3, 720×4 and
  1920×2. This confirms component order, bit positions, endianness and stride
  against a third-party implementation rather than only against ourselves.
  FFmpeg's own stride rule, `ceil(width / 48) × 128`, agrees with
  `rowBytesAligned(width, 128)` — including 1920 bytes at 720 and 5120 at 1920.
- Clean under ASan and UBSan.
- The four-configuration matrix (Release/Debug × tile 4/5) was run under CMake
  and ctest, not just by hand.

### One decision made that is not yet an ADR

A short final group — any width not a multiple of 6 — is padded on pack with
luma at `kCode10Black` and chroma at `kCode10ChromaZero`, and the padding is
discarded on unpack. The values matter only in that they must be deterministic,
or `unpack → pack` would not be byte-identical at such widths. Exposed as
`v210::kPadLuma` and `v210::kPadChroma`. Proposed wording if you want it
recorded:

> **ADR-018 — v210 short-group padding is black, and deterministic.**
> Widths that are not a multiple of 6 leave unused components in the final
> 16-byte group. These are packed as `kCode10Black` for luma and
> `kCode10ChromaZero` for chroma and ignored on unpack. Determinism is the
> requirement — without it `unpack → pack` is not byte-identical at such widths
> and I7 could not be stated for them. Black rather than replication of the
> edge pixel because a decoder that renders the full group should show nothing
> rather than a smear. Neither 720 nor 1920 exercises this path.

## Next work unit

**WU-03 — Test pattern generator.**

Deliver: `tools/make_testpat.cpp`, `tests/test_testpat.cpp`, plus the
`CMakeLists.txt` edit described below.

Scope notes:

- Three patterns. A full-range ramp sweeping code 4 to 1019 on Y, Cb and Cr;
  a zone plate; and a pattern with deliberate sub-black and super-white
  excursions. The ramp must contain codes 4 and 1019 exactly, not merely
  approach them, so the sweep should be constructed to hit both endpoints
  regardless of width.
- Output is raw `.v210`, written via `packImage`. Choose the stride explicitly
  and record it — `rowBytesMin` is the right default for files, and at 720 it
  coincides with the aligned figure anyway.
- The excursion pattern is the interesting one for I2. It must contain codes
  at 4 and 1019 and codes in the footroom and above nominal white, and those
  must still be there after a round trip. It is not a legal-signal test
  pattern and is not meant to be.
- The zone plate is generated in the planar domain at 4:2:2 for now. Its real
  use is WU-10's aliasing check.

**`CMakeLists.txt` change:** `tools/make_testpat.cpp` is the first executable
that is not a test, so it needs its own `add_executable` linking
`scatter-core`, placed after the `scatter_test` function definition. Add
`scatter_test(test_testpat)`.

**Session open command:**

```
./tools/open.sh CMakeLists.txt src/core/types.hpp src/video/v210.hpp
```

`v210.cpp` is not needed — the interface and the geometry helpers are in the
header. `harness.hpp` is not needed.

## Open questions

- **Q1.** Tile size 16×16 (32 KB across four banks) versus 32×32 (128 KB).
  CMake cache variable `SCATTER_TILE_LOG2`, default 5, both configurations
  verified. Benchmark at WU-09. Do not resolve before then.
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
