// scatter-dve — WU-10: normalise, composite (architecture.md section 4.8;
// module layout section 8 names this core/resolve.hpp/.cpp)
//
// This is pass 2's second half (section 3's signal path: "PASS 2: tile
// resolve -> four-bank splat -> accumulate -> normalise"), and the
// consumer of WU-09's sumBanks() output -- one AccumCell per destination
// cell, per tile. A different "resolve" than sumBanks()'s own bank-resolve
// (see core/splat.hpp's file header for why): architecture.md's signal
// path uses "resolve" for the whole of pass 2, but WORK-UNITS.md splits it
// in two; WU-09 already picked sumBanks() specifically so this unit's own
// name would not collide with it.
//
// architecture.md 4.8, verbatim:
//
//   Per output cell: out = Σ(w · colour) / Σw, computed as an int64
//   divide, with the zero-weight case flagged rather than silently
//   producing black... Then, and only then, composite against the
//   background using Σw as alpha.
//
// I5 is this unit's own rationale for that ordering ("normalise before
// compositing... compositing premultiplied offset-binary values against
// zero gives a green fringe on every partially covered edge, because
// Y=0, Cb=Cr=0 is not black"): AccumCell's Y/Cb/Cr fields are premultiplied
// by weight (Σ(w·colour), not Σcolour), so compositing them directly
// against a background would reproduce exactly that bug. normaliseCell()
// divides first, unconditionally; composite() calls it and never sees the
// premultiplied fields itself.
//
// 4.8 fixes the normalise formula and the compositing order but not what
// "the background" actually is -- nothing upstream of this unit produces
// a second image to composite against (that is Phase 2's k-buffer,
// WU-28); see ADR-026 for the choice this session makes instead, and for
// why core/pipeline.cpp's own orchestration entry points are declared
// below rather than in a separate pipeline.hpp.
//
// Integer arithmetic throughout (I4, I6), same as core/splat.hpp: this is
// still the fixed-point accumulation path, one step further along. Never
// introduce floating point here.
#pragma once

#include "core/binner.hpp"
#include "core/types.hpp"
#include "video/raster.hpp"

#include <cstdint>
#include <string>

namespace scatter {

// ---------------------------------------------------------------------------
// Normalise -- architecture.md 4.8's divide, in isolation.
// ---------------------------------------------------------------------------

// One resolved destination cell: the coverage-weighted average colour, or a
// flag that Σw was zero -- 4.8's "flagged rather than silently producing
// black". Y/Cb/Cr are meaningful only when covered is true; a caller that
// reads them under covered == false is the same class of bug as reading
// TileBins::tile() past its bounds elsewhere in this codebase (not checked
// here, by the same convention).
struct ResolvedCell {
    Sample Y = 0, Cb = 0, Cr = 0;
    bool covered = false;
};

// out = Σ(w·colour) / Σw (architecture.md 4.8), an int64 divide, rounded to
// nearest via the same "add half the divisor, then divide" convention
// core/types.hpp's toCode10 and ADR-020's chroma filters already use.
// Never clamps: a weighted average of values already within Sample's
// representable range cannot leave it, so I2 needs no help here, unlike
// v210::pack.
//
// cell.w <= 0 returns ResolvedCell{} with covered == false rather than
// dividing by zero -- 4.8's flagged case. (Strictly <= 0, not == 0: w is a
// sum of non-negative per-corner contributions and should never go
// negative, but a defensive cell.w < 0 is treated the same as uncovered
// rather than read as a valid divisor.)
ResolvedCell normaliseCell(const AccumCell& cell) noexcept;

// ---------------------------------------------------------------------------
// Composite -- architecture.md 4.8's second half, "using Σw as alpha".
// ---------------------------------------------------------------------------

// The flat backing colour a destination cell composites against wherever it
// has no coverage, or partial coverage. See ADR-026: architecture.md names
// "the background" without saying what it is, and nothing upstream of this
// unit produces a second image to composite against -- that is Phase 2's
// k-buffer (WU-28, architecture.md 4.7). This unit defines it instead as a
// caller-supplied constant colour. Default is legal black -- I3's
// kBlack/kChromaZero.
struct Background {
    Sample Y = kBlack, Cb = kChromaZero, Cr = kChromaZero;
};

inline constexpr Background kDefaultBackground{};

struct CompositedCell {
    Sample Y = 0, Cb = 0, Cr = 0;
};

// Normalises `cell` (calling normaliseCell() exactly once -- so a caller
// never normalises it separately first, enforcing 4.8's own "then, and
// only then" ordering structurally rather than by convention) and
// composites the result against `bg` using cell.w as alpha.
//
// alpha is cell.w clamped to [0, kWeightUnity] (ADR-026): Σw is coverage,
// not itself a [0, 1] fraction -- under compression, this unit's own
// zone-plate accept criterion routinely accumulates Σw far above
// kWeightUnity in a single cell (of order 1000x under 32:1 compression,
// per CORRECTIONS.md C-007), and even a single, non-overlapping surface's
// weight legitimately exceeds unity wherever many source samples land in
// one destination cell -- core/types.hpp's own Weight comment says as
// much. So alpha saturates at "fully covered" rather than growing without
// bound or inverting the blend: an unclamped alpha above unity would give
// the background a *negative* contribution, which is not what "coverage"
// means. A cell with covered == false (Σw == 0) composites as pure
// background without ever reading its meaningless colour fields -- the
// entire reason 4.8 asks for the zero-weight case to be flagged rather
// than defaulting to black: black is a colour choice, and this unit does
// not get to make it, the background does.
CompositedCell composite(const AccumCell& cell,
                          const Background& bg = kDefaultBackground) noexcept;

// ---------------------------------------------------------------------------
// Layered composite -- WU-12b, priority-tag opacity (architecture.md 4.7
// phase 2's own phrase, "opaque with priority tag set"; section 13's own
// "priority tag for forcing opacity"). DECISIONS.md ADR-028 sketches this
// mechanism at WU-12a; ADR-029 freezes the name, signature and behaviour
// below, once written and tested, per ADR-028's own closing note that this
// was left for this unit to do.
//
// Not a k-buffer (ADR-009 unchanged, WU-28's own job): exactly two layers,
// caller-ordered (lower, upper), no per-pixel depth sorting among more than
// two surfaces. This is the narrowest thing that can honestly be called
// "opaque with priority tag set" for a page-turn flap (upper) over a page
// behind (lower) -- the only two-surface case WU-12a's own accept criterion
// exercises.
// ---------------------------------------------------------------------------

// Combines two already-splatted AccumCell layers -- `lower` and `upper`, in
// that caller-fixed order -- against `bg`, taking one of two branches
// depending on whether `upperTag` equals the caller-configured `opaqueTag`.
// Only `upper`'s tag is consulted; `lower`'s own tag (if a caller is
// tracking one) plays no part in the decision -- see ADR-029.
//
// - upperTag == opaqueTag: architecture.md 4.7 phase 2's own
//   "read-replace-write". `lower` is composited against `bg` first -- the
//   "read" -- and `upper`'s own resolved colour is then composited over
//   *that* result using upper's own alpha (`upper.w` clamped to
//   [0, kWeightUnity], composite()'s own convention) -- the "write",
//   replacing what was read. Implemented as two calls to composite() above
//   (see resolve.cpp), not a hand-rolled blend: composite(lower, bg) IS the
//   read, and its result, reinterpreted as a Background (both structs are
//   the same three Sample fields), is exactly the surface the second
//   composite() call needs to write onto. This also makes partial upper
//   coverage fall out correctly with no extra branching: where upper.w is
//   0, composite(upper, afterRead) returns afterRead unchanged (lower
//   alone); where upper.w is at or above kWeightUnity, it returns upper's
//   own resolved colour outright, independent of afterRead -- exactly
//   HANDOFF.md's own "flap's own well-covered pixels resolve close to the
//   flap's own colour" and "pixels with only page-behind coverage are
//   unaffected".
// - upperTag != opaqueTag: WU-12a's own accumulation-sums default
//   (architecture.md 4.7 phase 1) -- `lower` and `upper` summed
//   component-wise first (exact, I6: integer addition is associative, the
//   same identity tests/test_pageturn.cpp's own
//   test_pipeline_pageturn_transparent_accumulates_over_page_behind()
//   already establishes), then composited once via composite() above.
CompositedCell compositeLayered(const AccumCell& lower, const AccumCell& upper,
                                 std::uint8_t upperTag, std::uint8_t opaqueTag,
                                 const Background& bg = kDefaultBackground) noexcept;

// ---------------------------------------------------------------------------
// Pipeline orchestration -- WU-10; implemented in src/core/pipeline.cpp.
//
// architecture.md section 3's signal path, run for one whole frame,
// single-threaded. Declared here rather than in a separate pipeline.hpp,
// even though section 8's module-layout sketch names "pipeline.hpp/.cpp
// # orchestration, thread pool, barriers" as a pair the same way it names
// every other core/ module: the thread pool and barriers that comment
// describes are WU-16's (Phase 4), not built yet, and this unit's own
// orchestration is one function with no state of its own to expose beyond
// it. SESSION-PROTOCOL.md's work-unit cap ("touch at most 3 source files
// plus its test") is already spent by resolve.hpp, resolve.cpp and
// pipeline.cpp; a fourth header has nowhere to go without exceeding it.
// ADR-021 already established the precedent for exactly this situation --
// declaring a new .cpp's public entry points in an existing, related
// header instead of a header of its own, for file_source.cpp/
// file_sink.cpp in video/raster.hpp -- and ADR-026 records the same choice
// made here. When WU-16 actually adds thread-pool state, pipeline.hpp
// arrives with it, and these declarations move there.
// ---------------------------------------------------------------------------

// Per-frame configuration for runFrame()/runFrameFile(): the destination
// raster's extent, plus the same parameters WU-07/WU-08 already require a
// caller to supply (maxK, adaptive supersampling) passed straight through
// unchanged -- this unit has no more grounds to invent an operating point
// for them than WU-08 did -- and this unit's own tag and background.
//
// supersample defaults to WU-08's own architecture.md-anchored threshold
// values (SupersampleConfig's own defaults, core/binner.hpp: threshold2x2
// = 1.0, threshold4x4 = 4.0 -- ADR-024) plus a small safety margin, not
// those bare values themselves. See CORRECTIONS.md C-008: an affine map
// whose true pixel-space determinant sits exactly at 1.0 -- this unit's
// own I7 identity-map check is exactly that -- has its *computed*
// determinant land on either side of 1.0, pixel by pixel, essentially at
// random, purely from ordinary floating-point rounding in
// core/lattice.cpp's bicubic evaluation (the affine-reproduction identity
// a Catmull-Rom basis satisfies is exact algebraically, not bit-for-bit).
// WU-08's chooseSupersample() is correct on its own terms -- a bare `>`
// comparison is exactly what 4.6 asks for -- but that makes it exactly as
// sensitive as a `>` comparison against a noisy value always is. The
// margin (1e-6) is far larger than the noise this session measured
// (below 1e-10) and far smaller than any determinant difference a real
// magnifying warp would ever need treated differently for.
inline constexpr double kSupersampleThresholdMargin = 1e-6;

inline SupersampleConfig defaultPipelineSupersample() noexcept {
    SupersampleConfig ss;
    ss.threshold2x2 = 1.0 + kSupersampleThresholdMargin;
    ss.threshold4x4 = 4.0 + kSupersampleThresholdMargin;
    return ss;
}

struct PipelineParams {
    int destWidth = 0;
    int destHeight = 0;
    double maxK = 1000.0;
    SupersampleConfig supersample = defaultPipelineSupersample();
    std::uint8_t tag = 0;
    Background background = kDefaultBackground;

    // WU-16 (Phase 4, DECISIONS.md ADR-040): number of worker threads
    // PASS 2 (bank-resolve, normalise, composite) runs on. Default 1 --
    // every caller from WU-10 through WU-15b, unchanged, gets exactly the
    // single-threaded loop that predates this field, byte for byte (see
    // core/pipeline.cpp's own runFrame(), the "threads <= 1" branch).
    // Values <= 1 take that same single-threaded path; values > 1
    // construct a core/pipeline.hpp ThreadPool of exactly this many
    // workers for the duration of one runFrame() call and partition PASS
    // 2's tiles across them. PASS 1 (fragment generation) is unchanged
    // and always single-threaded regardless of this value -- see
    // ADR-040 for why, and for WU-16b, the deferred follow-up that would
    // change that. I6 (integer addition is associative) is what makes any
    // value here produce output bit-identical to threads == 1 for the
    // same lattice/src/params -- WORK-UNITS.md's own WU-16 accept
    // criterion, checked directly in tests/test_threading.cpp.
    int threads = 1;
};

// Pass 1 (WU-06/07/08) plus pass 2 (WU-09 and this unit) over an
// already-4:4:4 source raster, writing an already-allocated 4:4:4
// destination raster the caller sizes to params.destWidth x
// params.destHeight (Raster444's own constructor does this; dest's width
// must equal params.destWidth for this function's row-major indexing to
// land correctly -- caller's bug, not checked here, matching this
// codebase's existing convention for unchecked preconditions). No v210 or
// chroma 4:2:2 I/O of its own -- see runFrameFile() below for the
// file-to-file wrapper -- so a caller can drive the warp/splat/resolve
// stages directly against a synthetic raster (this unit's own zone-plate
// anti-aliasing check does exactly this) without a v210 round trip in
// between.
void runFrame(const Lattice& lattice, const SourceRaster& src,
              const PipelineParams& params, video::Raster444& dest);

// Full file-to-file path -- architecture.md section 3's complete signal
// path: v210 unpack, chroma upsample, runFrame() above, chroma downsample,
// v210 pack. This is what this unit's own I7 identity-map check exercises,
// matching tests/test_ramp_roundtrip.cpp's own file-to-file pattern for
// the pre-warp chain it already covers. srcWidth/srcHeight describe
// srcPath's v210 geometry; params.destWidth/destHeight describe dstPath's.
// Returns false, writing nothing durable, if either v210 file operation
// fails -- matching readV210File/writeV210File's own convention.
bool runFrameFile(const Lattice& lattice, const std::string& srcPath,
                   int srcWidth, int srcHeight, const PipelineParams& params,
                   const std::string& dstPath);

}  // namespace scatter
