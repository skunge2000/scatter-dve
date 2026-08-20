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

#include <array>
#include <cstdint>
#include <string>

namespace scatter {

// Forward-declared, not #include "core/pipeline.hpp": PipelineParams below
// only ever holds a ThreadPool*, never calls a member on it or needs its
// size — a pointer type needs no complete type. Keeps this header's own
// dependency graph exactly as it was (core/binner.hpp, core/types.hpp,
// video/raster.hpp) rather than adding an edge to core/pipeline.hpp for a
// forward-declarable type; the same minimal-footprint judgement ADR-021/
// ADR-026/ADR-030 already applied to "does this need a new header",
// applied here to "does this need a new #include" instead. A caller that
// actually constructs a ThreadPool (to pass its address into
// PipelineParams::pool, below) must already #include "core/pipeline.hpp"
// itself to do so.
class ThreadPool;

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
// K-buffer resolve -- WU-28b (DECISIONS.md ADR-059, ADR-061). Consumes
// WU-28a's own per-cell occupied-slot set (core/splat.hpp's
// TileKBufferAccum/splatTileKBuffer()/sumBanksKBuffer(), core/types.hpp's
// KSlot/kBufferK) -- new, additive alongside composite()/compositeLayered()
// above, not a change to either's existing contract.
// ---------------------------------------------------------------------------

// Which of WU-28a's own k-buffer storage a runFrame() call should resolve
// through, if any -- see PipelineParams::kBufferMode below. One field
// covers both ADR-059's "opacity mode" and "blend control": Off leaves
// today's plain path (composite() against one AccumCell) completely
// untouched; Opaque and Blend each select one of ADR-059's own two
// resolve-time outcomes.
enum class KBufferResolveMode {
    Off,     // default -- WU-28a's storage is not even allocated (see
             // PipelineParams::kBufferMode below)
    Opaque,  // nearest occupied tag-slot wins outright, the rest discarded
    Blend,   // every occupied slot composited back-to-front by first-seen z
};

// Resolves one destination cell's up to kBufferK tag-slots (WU-28a's own
// sumBanksKBuffer() output) into one CompositedCell, per `mode` -- ADR-059's
// two resolve-time outcomes; the exact mechanism, this unit's own job to
// design per ADR-059's own deferral (see resolve.cpp):
//
// - Opaque: among the occupied slots, the one with the smallest
//   KSlot::firstSeenZ ("near = 0", Frag::z's own convention) is
//   composite()'d against `bg` alone, as if it were the cell's only
//   surface; every other slot is discarded.
// - Blend: every occupied slot, sorted front-to-back by firstSeenZ,
//   composited back-to-front -- the farthest against `bg` first, each
//   nearer slot then composited over the accumulated result using its own
//   coverage as alpha. Literally compositeLayered()'s own "read, then
//   write over the read" mechanism above, generalised from exactly two
//   caller-ordered layers to however many slots are occupied, ordered by
//   depth instead of by caller: two occupied slots reduce to one
//   compositeLayered() call with upperTag forced equal to opaqueTag --
//   tests/test_kbuffer_resolve.cpp checks this directly, an independent
//   cross-check against an already-tested function.
//
// Both modes break a firstSeenZ tie (two distinct tags' first-seen
// fragments landing at the same quantised z -- routine, the same
// quantisation ADR-059 already names for same-tag ties during
// accumulation) by smallest `tag`: a fixed, deterministic order over an
// already-fixed `slots` array, so the same input always produces the same
// output regardless of thread count or tile-visitation order -- I6,
// extended from WU-28a's own accumulation step through this resolve step.
//
// A cell with no occupied slot returns composite(AccumCell{}, bg) -- pure
// background, the same "Σw == 0" case composite() already handles.
CompositedCell compositeKBuffer(const std::array<KSlot, kBufferK>& slots,
                                 KBufferResolveMode mode,
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

    // WU-19a (Phase 4, DECISIONS.md ADR-044): an optional, caller-owned,
    // already-constructed ThreadPool (core/pipeline.hpp) to reuse across
    // many runFrame() calls, completing ADR-040's own deferral ("a
    // persistent, caller-owned ThreadPool that runFrame() can reuse
    // across many calls instead of constructing one per call... WU-19's
    // own job"). Default nullptr: every existing caller (WU-10 through
    // WU-18, none of which this unit touches) keeps compiling and
    // behaving exactly as before, byte for byte -- nullptr takes the same
    // threads-only branch WU-16a/16b already established (construct a
    // fresh ThreadPool for this one call if threads > 1, or the plain
    // single-threaded oracle loop if threads <= 1).
    //
    // When non-null, runFrame() dispatches both of its own runOnAll()
    // rounds against *pool directly instead of constructing and joining a
    // local ThreadPool for the call -- the per-call spawn/join overhead
    // WU-16a's own file comment already named as "real overhead a
    // genuinely persistent, reused-across-frames pool would avoid". In
    // that case pool->size() alone decides how many workers this call
    // actually partitions its row bands and tiles across; `threads` above
    // is not consulted at all (see core/pipeline.cpp's own runFrame() for
    // the precise branch order) -- a caller does not need to keep the two
    // fields in sync, and there is no way for a mismatch between them to
    // silently corrupt output: I6 (integer addition is associative) holds
    // regardless of how many workers a given call actually used, and
    // *this* unit's own tests/test_persistent_pool.cpp checks the
    // specific case where they disagree, directly.
    //
    // Ownership and lifetime are the caller's own responsibility, the
    // same unchecked-precondition convention every other non-owning
    // pointer in this codebase already uses (e.g. SourceRaster's own
    // Sample* fields): *pool must outlive this call, and must not be
    // concurrently driven by another runFrame() call or any other
    // runOnAll() caller at the same time -- ThreadPool itself supports
    // only one in-flight runOnAll() dispatch at a time, not concurrent
    // ones.
    ThreadPool* pool = nullptr;

    // WU-22a (Phase 5, DECISIONS.md ADR-056): an optional, caller-owned
    // full-frame weight-capture buffer -- the diagnostic coverage view's
    // (WU-22b, src/diag/, a Mac-only Metal window, not built this
    // session) own data source. Default nullptr: every existing caller
    // (WU-10 through WU-21i, none of which this unit touches) keeps
    // compiling and behaving exactly as before, byte for byte -- the same
    // "non-owning pointer, default nullptr, zero cost and zero behaviour
    // change when absent" shape PipelineParams::pool (WU-19a, ADR-044)
    // already established for a different optional extra, applied here to
    // an optional extra *output* instead of an optional extra input.
    //
    // When non-null, weightOut must point to at least
    // destWidth * destHeight WeightAccum values, tight-packed, row-major
    // (dy * destWidth + dx) -- video::Raster444's own "tight-packed unless
    // there is a real reason otherwise" convention, simpler than Plane's
    // arbitrary-stride support since nothing here needs to match a
    // DeckLink-supplied row stride the way v210 output does; this buffer
    // never leaves the process as a video signal.
    //
    // Written with each destination cell's raw AccumCell::w -- the exact
    // architecture.md 4.5/4.8 coverage-weight accumulator composite()'s
    // own alpha (cell.w clamped to [0, kWeightUnity]) is computed from,
    // captured *before* that clamp, so a caller can see how far above
    // unity a heavily-overlapped cell's real coverage reached, not merely
    // whether it saturated -- see architecture.md 4.4's own "order 1000
    // fragments" note on what heavy compression does to a single cell's
    // weight. A cell this unit's own composite() would treat as
    // uncovered (cell.w <= 0) captures its literal AccumCell::w
    // unchanged, not a sentinel -- 0 for the ordinary case, and the
    // defensive cell.w < 0 case core/resolve.hpp's own normaliseCell()
    // comment already names, should it ever occur.
    //
    // Never read internally by this unit's own composite()/normaliseCell()
    // path, which is completely unchanged by this field's presence or
    // absence -- this is a pure side-channel capture, not a second input,
    // and tests/test_coverage_capture.cpp's own
    // test_capture_is_side_effect_free() checks exactly that: identical
    // composited dest output whether weightOut is null or supplied.
    WeightAccum* weightOut = nullptr;

    // WU-28b (Phase 7, DECISIONS.md ADR-059, ADR-061): which of WU-28a's
    // own k-buffer storage this call's own PASS 2 should resolve through,
    // via compositeKBuffer() above, in place of the plain TileAccum/
    // AccumCell/composite() path. Default Off: every existing caller keeps
    // compiling and behaving exactly as before, byte for byte --
    // core/pipeline.cpp's resolveOneTile() does not even construct
    // TileKBufferAccum when this is left at its default, the same
    // "opt-in, default-off, zero-cost-when-absent" shape
    // PipelineParams::pool (ADR-044) and weightOut (ADR-056) above already
    // established. weightOut is not written when this field is not Off --
    // a k-buffer cell has no single AccumCell::w for its contract to
    // capture (see core/pipeline.cpp's resolveOneTile()).
    KBufferResolveMode kBufferMode = KBufferResolveMode::Off;
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

// Full in-memory path -- architecture.md section 3's complete signal path
// (v210 unpack, chroma upsample, runFrame() above, chroma downsample, v210
// pack), operating on a caller-supplied packed-v210 byte buffer instead of
// an already-4:4:4 raster (runFrame(), above) or a file (runFrameFile(),
// below). WU-21's own reason for existing (DECISIONS.md ADR-048): a
// captured IDeckLinkVideoInputFrame's own pixel bytes live in a
// DeckLink-owned buffer for the duration of one StartAccess/EndAccess
// bracket (ADR-032's own finding, extended to input by ADR-046/047) --
// never a file -- so the file-to-file path below cannot be this project's
// route from a retained capture frame into its own warp pipeline. This is
// the exact shared middle both runFrame() callers already use (v210
// unpack, chroma upsample, runFrame(), chroma downsample, v210 pack) with
// the file-I/O bookends replaced by caller-supplied source/destination
// pointers and strides.
//
// Preconditions, unchecked -- the same convention runFrame() above and
// video::v210::unpackImage/packImage already use throughout this codebase:
// srcBytes holds at least srcRowBytes * srcHeight bytes of packed v210
// (video::v210::unpackImage's own precondition, unchanged); dstBytes holds
// at least dstRowBytes * params.destHeight bytes, and dstRowBytes is at
// least video::v210::rowBytesMin(params.destWidth) -- a caller passing a
// DeckLink-buffer stride that legitimately exceeds the minimum (alignment
// padding beyond one row's own packed content) is fine, exactly as
// io/decklink_output.cpp's own fillFrameBuffer() already trusts the SDK's
// own RowBytesForPixelFormat() rather than assuming rowBytesMin() agrees
// (ADR-032). No bool return the way runFrameFile() has: unlike that
// function, there is no file open/read/write step here that can fail --
// runFrame() itself has no failure return, by construction, and neither do
// v210::unpackImage/packImage or chroma::upsample/downsampleImage.
void runFrameBytes(const Lattice& lattice,
                    const std::uint8_t* srcBytes, std::ptrdiff_t srcRowBytes,
                    int srcWidth, int srcHeight,
                    const PipelineParams& params,
                    std::uint8_t* dstBytes, std::ptrdiff_t dstRowBytes);

// Full file-to-file path -- architecture.md section 3's complete signal
// path: v210 unpack, chroma upsample, runFrame() above, chroma downsample,
// v210 pack. This is what this unit's own I7 identity-map check exercises,
// matching tests/test_ramp_roundtrip.cpp's own file-to-file pattern for
// the pre-warp chain it already covers. srcWidth/srcHeight describe
// srcPath's v210 geometry; params.destWidth/destHeight describe dstPath's.
// Returns false, writing nothing durable, if either v210 file operation
// fails -- matching readV210File/writeV210File's own convention.
//
// Deliberately not implemented in terms of runFrameBytes() above, despite
// running the identical middle sequence (unpack, upsample, runFrame,
// downsample, pack) between its own file-read and file-write: the two
// functions were written independently to keep this one's own existing,
// already-tested body (WU-10, untouched by WU-21) at zero risk from a
// refactor this unit's own accept criterion does not need -- unlike
// ADR-040/041's resolveOneTile() extraction, where two hand-written copies
// of genuinely complex, evolving multi-worker orchestration logic really
// could quietly drift apart, this is a short, linear sequence of calls to
// already independently-tested functions (v210::unpackImage/packImage,
// chroma::upsample/downsampleImage, runFrame() itself), duplicated once,
// not hand-rolled twice. Flagged here, not fixed speculatively, as a
// candidate for consolidation if a third caller ever needs the same
// sequence -- this project's own "not decided here" convention (e.g.
// ADR-042/043's own deferred denser NEON schemes) applied to a much
// smaller question.
bool runFrameFile(const Lattice& lattice, const std::string& srcPath,
                   int srcWidth, int srcHeight, const PipelineParams& params,
                   const std::string& dstPath);

}  // namespace scatter
