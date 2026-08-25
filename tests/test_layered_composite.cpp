// WU-12b: page turn, priority-tag opaque (core/resolve.hpp's
// compositeLayered(), DECISIONS.md ADR-028's sketch, ADR-029's frozen
// signature/behaviour) -- reproduces US 4,563,703 FIG. 5's "opaque with
// priority tag set" mode (architecture.md 4.7 phase 2, section 9's own
// test-plan entry) for exactly two caller-ordered layers, not WU-28's
// general k-buffer (ADR-009 unchanged).
//
// Two parts:
//
// Part A -- direct, hand-built AccumCell unit tests of compositeLayered()
// itself, decoupled from any real splat/pipeline geometry, checking the
// read-replace-write formula exactly (bit-for-bit -- this is integer
// arithmetic throughout, I6, so exact equality is safe here unlike two
// independently-derived floating-point expressions; see CORRECTIONS.md
// C-012 for why that distinction matters and doesn't apply to this file):
//
//   A1. Tag mismatch -- falls back to WU-12a's own accumulation-sums
//       default, exactly equal to composite() of the two cells summed.
//   A2. Tag match, upper fully covered (upper.w == kWeightUnity exactly)
//       -- result depends only on upper's own resolved colour, provably
//       independent of both `lower` and `bg` (blend() at alpha == unity
//       reduces to the colour term exactly, worked through in
//       core/resolve.hpp's own composite() doc comment).
//   A3. Tag match, upper uncovered (upper.w == 0) -- result exactly equals
//       composite(lower, bg), i.e. "unaffected" -- the read, un-replaced.
//   A4. Tag match, partial alpha -- result exactly matches a locally
//       duplicated re-derivation of the read-replace-write blend formula
//       (SESSION-PROTOCOL.md rule 2: duplicated, not shared, and not
//       calling resolve.cpp's own file-local blend()/divideRounded(),
//       which are anonymous-namespace and not linkable from here anyway).
//
// Part B -- the pipeline-level scenario HANDOFF.md's own "Next work unit"
// section suggests: WU-12a's own two-layer construction (a page-turn flap
// over a full-canvas page behind), duplicated locally from
// tests/test_pageturn.cpp per SESSION-PROTOCOL.md rule 2 (this is
// WU-12a's own test file; WU-12b owns a new one, not an extension of it),
// checking the opaque path specifically against real splatted AccumCells:
// at the flap's own well-covered pixel the composited result is exact
// (not merely close) to the read-replace-write formula applied to the real
// data, and is shown to differ meaningfully from the transparent-sum
// default there; at a pixel where only the page behind has coverage, the
// result is exactly unaffected.

#include "core/binner.hpp"
#include "core/lattice.hpp"
#include "core/resolve.hpp"
#include "core/shapes/shapes.hpp"
#include "core/splat.hpp"
#include "harness.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

using namespace scatter;
using namespace scatter::shapes;

namespace {

// ---------------------------------------------------------------------------
// Independent local re-derivations of resolve.cpp's own file-local
// arithmetic (SESSION-PROTOCOL.md rule 2: duplicated, not shared -- and
// resolve.cpp's blend()/divideRounded() are anonymous-namespace, so not
// linkable from here even if this file wanted to reuse them). Same formula,
// re-typed, so comparisons against compositeLayered()'s actual output can
// use exact equality (integer arithmetic throughout, no rounding ambiguity
// of the kind C-012 warns about for differently-shaped floating-point
// expressions).
// ---------------------------------------------------------------------------

Sample expectedDivide(ColourAccum colour, WeightAccum weight) noexcept {
    const std::int64_t w = std::int64_t(weight);
    const std::int64_t r = (colour + w / 2) / w;
    return Sample(r);
}

Sample expectedBlend(Sample colour, Sample bg, std::int64_t alpha) noexcept {
    const std::int64_t unity = std::int64_t(kWeightUnity);
    const std::int64_t sum = std::int64_t(colour) * alpha +
                              std::int64_t(bg) * (unity - alpha) +
                              unity / 2;
    return Sample(sum / unity);
}

AccumCell expectedSum(const AccumCell& a, const AccumCell& b) noexcept {
    AccumCell out{};
    out.R = a.R + b.R;
    out.G = a.G + b.G;
    out.B = a.B + b.B;
    out.w = a.w + b.w;
    return out;
}

// A synthetic AccumCell whose accumulated colour is an exact multiple of
// its own weight, so normaliseCell()/divideRounded() recovers (y, cb, cr)
// with no rounding fuzz: (y*w + w/2) / w == y exactly for any w > 0 (floor
// division of a*w + r by w, with 0 <= r < w, is always a). Lets Part A
// assert exact expected colours without re-deriving divideRounded() first.
AccumCell makeUniformCell(Sample y, Sample cb, Sample cr, WeightAccum w) noexcept {
    AccumCell c{};
    c.R = ColourAccum(y)  * ColourAccum(w);
    c.G = ColourAccum(cb) * ColourAccum(w);
    c.B = ColourAccum(cr) * ColourAccum(w);
    c.w = w;
    return c;
}

}  // namespace

// ---------------------------------------------------------------------------
// Part A: direct unit tests of compositeLayered() against hand-built cells.
// ---------------------------------------------------------------------------

static void test_compositeLayered_tag_mismatch_sums_then_composites() {
    const AccumCell lower = makeUniformCell(fromCode10(300), fromCode10(500),
                                             fromCode10(700), WeightAccum(kWeightUnity));
    const AccumCell upper = makeUniformCell(fromCode10(800), fromCode10(200),
                                             fromCode10(100),
                                             WeightAccum(kWeightUnity / 2));
    const Background bg{fromCode10(100), fromCode10(480), fromCode10(560)};

    const CompositedCell expected = composite(expectedSum(lower, upper), bg);
    const CompositedCell actual = compositeLayered(lower, upper, 7, 3, bg);

    CHECK_ONCE(actual.R == expected.R);
    CHECK_ONCE(actual.G == expected.G);
    CHECK_ONCE(actual.B == expected.B);
}

static void test_compositeLayered_opaque_full_alpha_ignores_lower_and_bg() {
    const AccumCell upper = makeUniformCell(fromCode10(900), fromCode10(150),
                                             fromCode10(800),
                                             WeightAccum(kWeightUnity));
    const AccumCell lowerCovered = makeUniformCell(
        fromCode10(300), fromCode10(500), fromCode10(700), WeightAccum(kWeightUnity));
    const AccumCell lowerUncovered{};  // w == 0
    const Background bg1{fromCode10(64), fromCode10(512), fromCode10(512)};
    const Background bg2{fromCode10(940), fromCode10(64), fromCode10(940)};

    // alpha == kWeightUnity exactly -> blend reduces to the colour term
    // alone (core/resolve.hpp's composite() doc comment), so the result
    // should equal upper's own resolved colour directly, regardless of
    // lower or bg.
    const CompositedCell a = compositeLayered(lowerCovered, upper, 2, 2, bg1);
    const CompositedCell b = compositeLayered(lowerCovered, upper, 2, 2, bg2);
    const CompositedCell c = compositeLayered(lowerUncovered, upper, 2, 2, bg1);

    CHECK_ONCE(a.R == fromCode10(900));
    CHECK_ONCE(a.G == fromCode10(150));
    CHECK_ONCE(a.B == fromCode10(800));
    CHECK_ONCE(a.R == b.R && a.G == b.G && a.B == b.B);
    CHECK_ONCE(a.R == c.R && a.G == c.G && a.B == c.B);
}

static void test_compositeLayered_opaque_zero_alpha_equals_lower_alone() {
    const AccumCell lower = makeUniformCell(fromCode10(300), fromCode10(500),
                                             fromCode10(700), WeightAccum(kWeightUnity));
    const AccumCell upperZero{};  // w == 0: no flap coverage at this cell
    const Background bg{fromCode10(200), fromCode10(600), fromCode10(400)};

    const CompositedCell expected = composite(lower, bg);
    const CompositedCell actual = compositeLayered(lower, upperZero, 9, 9, bg);

    CHECK_ONCE(actual.R == expected.R);
    CHECK_ONCE(actual.G == expected.G);
    CHECK_ONCE(actual.B == expected.B);
}

static void test_compositeLayered_opaque_partial_alpha_matches_readreplacewrite_formula() {
    const AccumCell lower = makeUniformCell(fromCode10(300), fromCode10(500),
                                             fromCode10(700), WeightAccum(kWeightUnity));
    const WeightAccum quarterUnity = WeightAccum(kWeightUnity / 4);
    const AccumCell upper = makeUniformCell(fromCode10(900), fromCode10(150),
                                             fromCode10(800), quarterUnity);
    const Background bg{fromCode10(64), fromCode10(512), fromCode10(512)};

    // lower is fully covered, so the "read" step's own alpha is unity too:
    // afterRead is exactly lower's own colour, independent of bg.
    const CompositedCell afterRead = composite(lower, bg);
    CHECK_ONCE(afterRead.R == fromCode10(300));
    CHECK_ONCE(afterRead.G == fromCode10(500));
    CHECK_ONCE(afterRead.B == fromCode10(700));

    const std::int64_t alpha = std::int64_t(quarterUnity);
    const Sample expectedY = expectedBlend(fromCode10(900), afterRead.R, alpha);
    const Sample expectedCb = expectedBlend(fromCode10(150), afterRead.G, alpha);
    const Sample expectedCr = expectedBlend(fromCode10(800), afterRead.B, alpha);

    const CompositedCell actual = compositeLayered(lower, upper, 4, 4, bg);

    CHECK_ONCE(actual.R == expectedY);
    CHECK_ONCE(actual.G == expectedCb);
    CHECK_ONCE(actual.B == expectedCr);
}

// ---------------------------------------------------------------------------
// Part B: the HANDOFF.md-suggested pipeline scenario -- a page-turn flap
// (upper, opaque-tagged) over a full-canvas page behind (lower) -- reusing
// WU-12a's own two-layer construction, duplicated locally from
// tests/test_pageturn.cpp per SESSION-PROTOCOL.md rule 2.
// ---------------------------------------------------------------------------

namespace {

Lattice makeAffineLattice(double scaleX, double scaleY, double offX,
                           double offY, int srcWidth, int srcHeight) {
    Lattice lat;
    const double su = (srcWidth  > 1) ? scaleX * double(srcWidth  - 1) / double(kLatticeMax) : 0.0;
    const double sv = (srcHeight > 1) ? scaleY * double(srcHeight - 1) / double(kLatticeMax) : 0.0;
    for (int row = 0; row < kLatticeSize; ++row) {
        for (int col = 0; col < kLatticeSize; ++col) {
            Vec3& p = lat.at(row, col);
            p.x = offX + su * double(col);
            p.y = offY + sv * double(row);
            p.z = 0.0;
        }
    }
    return lat;
}

// Pass 1 + pass 2 up to (but not including) normalise/composite -- same
// tile loop core/pipeline.cpp's runFrame() uses internally, duplicated here
// (as tests/test_pageturn.cpp already does) because runFrame() always
// composites a single layer before returning, and this unit needs the raw
// per-cell AccumCell for each of the two layers separately.
std::vector<AccumCell> accumFromBins(TileBins& bins, int destW, int destH) {
    std::vector<AccumCell> grid(std::size_t(destW) * std::size_t(destH));
    std::vector<AccumCell> tileCells(std::size_t(kTilePixels), AccumCell{});

    for (int ty = 0; ty < bins.tilesY(); ++ty) {
        for (int tx = 0; tx < bins.tilesX(); ++tx) {
            TileAccum accum;
            splatTile(bins.tile(tx, ty), accum);
            sumBanks(accum, tileCells.data());

            const int originX = tx * kTileSize;
            const int originY = ty * kTileSize;
            const int localWidth = std::min(kTileSize, destW - originX);
            const int localHeight = std::min(kTileSize, destH - originY);
            for (int ly = 0; ly < localHeight; ++ly) {
                for (int lx = 0; lx < localWidth; ++lx) {
                    const int dx = originX + lx;
                    const int dy = originY + ly;
                    grid[std::size_t(dy) * std::size_t(destW) + std::size_t(dx)] =
                        tileCells[std::size_t(ly) * std::size_t(kTileSize) + std::size_t(lx)];
                }
            }
        }
    }
    return grid;
}

std::vector<AccumCell> accumOneLayer(const Lattice& lat, const SourceRaster& src,
                                      std::uint8_t tag, double maxK,
                                      const SupersampleConfig& ss,
                                      int destW, int destH) {
    TileBins bins(destW, destH);
    generateFragments(lat, src, maxK, ss, tag, bins);
    return accumFromBins(bins, destW, destH);
}

struct TwoLayers {
    std::vector<AccumCell> pageOnly;
    std::vector<AccumCell> flapOnly;
};

constexpr int kDest = 128;
constexpr int kSrc  = 128;
constexpr std::uint8_t kPageTag = 0;
constexpr std::uint8_t kFlapTag = 1;
constexpr std::uint8_t kOpaqueTag = kFlapTag;  // the flap is the opaque upper layer

TwoLayers buildPageAndFlap() {
    // "Page behind": a flat affine map covering the entire destination
    // canvas at 1:1, uniform colour -- full coverage everywhere.
    const Lattice pageLat = makeAffineLattice(1.0, 1.0, 0.0, 0.0, kSrc, kSrc);
    std::vector<Sample> pageY(std::size_t(kSrc) * std::size_t(kSrc), fromCode10(200));
    std::vector<Sample> pageCb(std::size_t(kSrc) * std::size_t(kSrc), fromCode10(600));
    std::vector<Sample> pageCr(std::size_t(kSrc) * std::size_t(kSrc), fromCode10(300));
    SourceRaster pageSrc;
    pageSrc.width = kSrc;
    pageSrc.height = kSrc;
    pageSrc.r = pageY.data();
    pageSrc.g = pageCb.data();
    pageSrc.b = pageCr.data();

    // The flap: a genuinely curling page turn, sized comfortably inside the
    // destination canvas, with a larger source raster than its own
    // on-screen footprint so compression dominates rather than
    // magnification (C-011's own precaution).
    PageTurnParams flapParams;
    flapParams.width        = 80.0;
    flapParams.heightSpan   = 80.0;
    flapParams.radius       = 40.0;
    flapParams.turnProgress = 0.5;
    flapParams.centerX      = 64.0;
    flapParams.centerY      = 64.0;
    const Lattice flapLat = buildPageTurnLattice(flapParams);
    std::vector<Sample> flapY(std::size_t(kSrc) * std::size_t(kSrc), fromCode10(850));
    std::vector<Sample> flapCb(std::size_t(kSrc) * std::size_t(kSrc), fromCode10(450));
    std::vector<Sample> flapCr(std::size_t(kSrc) * std::size_t(kSrc), fromCode10(900));
    SourceRaster flapSrc;
    flapSrc.width = kSrc;
    flapSrc.height = kSrc;
    flapSrc.r = flapY.data();
    flapSrc.g = flapCb.data();
    flapSrc.b = flapCr.data();

    constexpr double kMaxK = 1000.0;
    const SupersampleConfig ss{};

    TwoLayers layers;
    layers.pageOnly = accumOneLayer(pageLat, pageSrc, kPageTag, kMaxK, ss, kDest, kDest);
    layers.flapOnly = accumOneLayer(flapLat, flapSrc, kFlapTag, kMaxK, ss, kDest, kDest);
    return layers;
}

}  // namespace

static void test_pipeline_pageturn_opaque_flap_hides_page_behind() {
    const TwoLayers layers = buildPageAndFlap();
    const Background bg = kDefaultBackground;

    // The flap's own most solidly covered destination pixel -- found
    // empirically (C-011's own discipline: do not assume where coverage is
    // densest), same technique tests/test_pageturn.cpp's own transparent-
    // mode check already uses.
    std::size_t bestIdx = 0;
    WeightAccum bestW = -1;
    for (std::size_t i = 0; i < layers.flapOnly.size(); ++i) {
        if (layers.flapOnly[i].w > bestW) {
            bestW = layers.flapOnly[i].w;
            bestIdx = i;
        }
    }
    CHECK(bestW > 0);
    CHECK(layers.pageOnly[bestIdx].w > 0);  // page behind covers everywhere

    const AccumCell& lower = layers.pageOnly[bestIdx];
    const AccumCell& upper = layers.flapOnly[bestIdx];

    // Exact expected value: re-derive the read-replace-write formula
    // directly from the real splatted data, independent of
    // compositeLayered()'s own implementation.
    const CompositedCell afterRead = composite(lower, bg);
    const std::int64_t alpha =
        std::clamp<std::int64_t>(std::int64_t(upper.w), 0, std::int64_t(kWeightUnity));
    const Sample upperY  = expectedDivide(upper.R, upper.w);
    const Sample upperCb = expectedDivide(upper.G, upper.w);
    const Sample upperCr = expectedDivide(upper.B, upper.w);
    const Sample expectedY  = expectedBlend(upperY, afterRead.R, alpha);
    const Sample expectedCb = expectedBlend(upperCb, afterRead.G, alpha);
    const Sample expectedCr = expectedBlend(upperCr, afterRead.B, alpha);

    const CompositedCell actual = compositeLayered(lower, upper, kFlapTag, kOpaqueTag, bg);
    CHECK_ONCE(actual.R == expectedY);
    CHECK_ONCE(actual.G == expectedCb);
    CHECK_ONCE(actual.B == expectedCr);

    // This pixel really is "well covered" by the flap, per HANDOFF.md's own
    // language -- alpha within 10% of full unity -- otherwise the exact
    // check above is true but vacuous for what this test is meant to show.
    CHECK(alpha >= std::int64_t(kWeightUnity) * 9 / 10);

    // The opaque result differs meaningfully from what the transparent
    // (tag-mismatch) default would have produced at the same pixel --
    // proving the opaque branch actually replaces rather than blends in,
    // the same "more than ordinary rounding" margin
    // tests/test_pageturn.cpp's own transparent-mode check uses.
    const CompositedCell transparentResult = composite(expectedSum(lower, upper), bg);
    constexpr int kRoundingMargin = 8;
    const int dY = std::abs(int(actual.R) - int(transparentResult.R));
    const int dCb = std::abs(int(actual.G) - int(transparentResult.G));
    const int dCr = std::abs(int(actual.B) - int(transparentResult.B));
    CHECK(dY > kRoundingMargin || dCb > kRoundingMargin || dCr > kRoundingMargin);
}

static void test_pipeline_pageturn_opaque_unaffected_where_flap_absent() {
    const TwoLayers layers = buildPageAndFlap();
    const Background bg = kDefaultBackground;

    // A destination cell the flap never reaches at all, but the (full-
    // canvas) page behind does.
    std::size_t idx = layers.flapOnly.size();
    for (std::size_t i = 0; i < layers.flapOnly.size(); ++i) {
        if (layers.flapOnly[i].w == 0 && layers.pageOnly[i].w > 0) {
            idx = i;
            break;
        }
    }
    CHECK(idx < layers.flapOnly.size());

    const CompositedCell expected = composite(layers.pageOnly[idx], bg);
    const CompositedCell actual = compositeLayered(layers.pageOnly[idx], layers.flapOnly[idx],
                                                     kFlapTag, kOpaqueTag, bg);

    CHECK_ONCE(actual.R == expected.R);
    CHECK_ONCE(actual.G == expected.G);
    CHECK_ONCE(actual.B == expected.B);
}

int main() {
    test_compositeLayered_tag_mismatch_sums_then_composites();
    test_compositeLayered_opaque_full_alpha_ignores_lower_and_bg();
    test_compositeLayered_opaque_zero_alpha_equals_lower_alone();
    test_compositeLayered_opaque_partial_alpha_matches_readreplacewrite_formula();
    test_pipeline_pageturn_opaque_flap_hides_page_behind();
    test_pipeline_pageturn_opaque_unaffected_where_flap_absent();
    return scatter::test::summary("test_layered_composite");
}
