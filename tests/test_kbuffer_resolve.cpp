// WU-28b: k-buffer resolve, depth-ordered opaque/blend composite
// (core/resolve.hpp's compositeKBuffer(), DECISIONS.md ADR-059/ADR-061).
// Consumes WU-28a's per-cell occupied-slot set (KSlot/TileKBufferAccum) --
// not exercised end to end until this unit. Three parts, matching
// WORK-UNITS.md's own WU-28b accept line:
//
// Part A -- Opaque, hand-built KSlot arrays: nearest slot wins exactly,
// matching composite() of that one AccumCell; others are provably ignored;
// a firstSeenZ tie breaks deterministically; empty -> pure background.
//
// Part B -- Blend, hand-built KSlot arrays: two slots cross-checked against
// compositeLayered() itself (WU-12b/ADR-029) with upperTag forced equal to
// opaqueTag -- an independent already-tested reference, not
// compositeKBuffer()'s own arithmetic; three slots cross-checked against a
// hand-ordered chain of direct composite() calls; a "known ratio" case
// (WORK-UNITS.md's own wording) checked against an exact hand-computed
// blend value; empty -> pure background.
//
// Part C -- the full pipeline (runFrame() end to end) at --threads 1 byte-
// identical to --threads 8, for a real folding-sphere frame (WU-21g/h's own
// pole-to-pole wrap geometry, ADR-053) with kBufferMode == Blend: I6 for
// the completed feature, now that resolveOneTile() actually wires the
// k-buffer path into the real thread pool.

#include "core/resolve.hpp"
#include "core/shapes/shapes.hpp"
#include "harness.hpp"
#include "testpat.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

using namespace scatter;
using namespace scatter::shapes;

namespace {

// Duplicated locally from tests/test_layered_composite.cpp per
// SESSION-PROTOCOL.md rule 2 (one unit, one test, no cross-file sharing):
// a synthetic AccumCell whose accumulated colour is an exact multiple of
// its own weight, so normaliseCell()/divideRounded() recovers (y, cb, cr)
// with no rounding fuzz.
AccumCell makeUniformCell(Sample y, Sample cb, Sample cr, WeightAccum w) noexcept {
    AccumCell c{};
    c.R = ColourAccum(y) * ColourAccum(w);
    c.G = ColourAccum(cb) * ColourAccum(w);
    c.B = ColourAccum(cr) * ColourAccum(w);
    c.w = w;
    return c;
}

KSlot makeSlot(std::uint8_t tag, std::uint16_t z, const AccumCell& cell) noexcept {
    KSlot s;
    s.tag = tag;
    s.occupied = true;
    s.firstSeenZ = z;
    s.cell = cell;
    return s;
}

}  // namespace

// ---------------------------------------------------------------------------
// Part A -- Opaque mode.
// ---------------------------------------------------------------------------

static void test_opaque_nearest_wins_exactly() {
    const AccumCell near = makeUniformCell(fromCode10(900), fromCode10(500),
                                            fromCode10(600), WeightAccum(kWeightUnity));
    const AccumCell mid = makeUniformCell(fromCode10(400), fromCode10(200),
                                           fromCode10(100), WeightAccum(kWeightUnity));
    const AccumCell far = makeUniformCell(fromCode10(50), fromCode10(700),
                                           fromCode10(800), WeightAccum(kWeightUnity));

    std::array<KSlot, kBufferK> slots{makeSlot(1, /*z=*/500, far),
                                       makeSlot(2, /*z=*/10, near),
                                       makeSlot(3, /*z=*/250, mid), KSlot{}};

    const CompositedCell got = compositeKBuffer(slots, KBufferResolveMode::Opaque);
    const CompositedCell expected = composite(near);  // z=10 is nearest ("near = 0")

    CHECK(got.R == expected.R);
    CHECK(got.G == expected.G);
    CHECK(got.B == expected.B);
}

static void test_opaque_z_tie_breaks_by_smallest_tag() {
    const AccumCell a = makeUniformCell(fromCode10(300), fromCode10(300),
                                         fromCode10(300), WeightAccum(kWeightUnity));
    const AccumCell b = makeUniformCell(fromCode10(700), fromCode10(700),
                                         fromCode10(700), WeightAccum(kWeightUnity));

    // Same firstSeenZ, different tags, in both possible physical array
    // orders -- the result must depend only on slot content (smallest tag
    // wins the tie), never on which array index either slot happens to
    // occupy.
    std::array<KSlot, kBufferK> order1{makeSlot(9, 42, b), makeSlot(2, 42, a), KSlot{}, KSlot{}};
    std::array<KSlot, kBufferK> order2{makeSlot(2, 42, a), makeSlot(9, 42, b), KSlot{}, KSlot{}};

    const CompositedCell got1 = compositeKBuffer(order1, KBufferResolveMode::Opaque);
    const CompositedCell got2 = compositeKBuffer(order2, KBufferResolveMode::Opaque);
    const CompositedCell expected = composite(a);  // tag 2 < tag 9

    CHECK(got1.R == expected.R && got1.G == expected.G && got1.B == expected.B);
    CHECK(got2.R == expected.R && got2.G == expected.G && got2.B == expected.B);
}

static void test_no_occupied_slot_is_pure_background_both_modes() {
    std::array<KSlot, kBufferK> empty{};
    const Background bg{fromCode10(100), fromCode10(200), fromCode10(300)};

    for (KBufferResolveMode mode : {KBufferResolveMode::Opaque, KBufferResolveMode::Blend}) {
        const CompositedCell got = compositeKBuffer(empty, mode, bg);
        CHECK(got.R == bg.R);
        CHECK(got.G == bg.G);
        CHECK(got.B == bg.B);
    }
}

// ---------------------------------------------------------------------------
// Part B -- Blend mode.
// ---------------------------------------------------------------------------

static void test_blend_two_slots_matches_compositeLayered() {
    const AccumCell farther = makeUniformCell(fromCode10(200), fromCode10(600),
                                               fromCode10(150), WeightAccum(kWeightUnity));
    const AccumCell nearer = makeUniformCell(fromCode10(800), fromCode10(100),
                                              fromCode10(900), WeightAccum(kWeightUnity / 3));

    std::array<KSlot, kBufferK> slots{makeSlot(4, /*z=*/1000, farther),
                                       makeSlot(6, /*z=*/50, nearer), KSlot{}, KSlot{}};

    const CompositedCell got = compositeKBuffer(slots, KBufferResolveMode::Blend);

    // compositeLayered() with upperTag forced == opaqueTag always takes the
    // "read lower against bg, write upper over that" branch (ADR-028/029)
    // -- exactly the two-slot case of compositeKBuffer()'s own mechanism.
    const CompositedCell expected = compositeLayered(farther, nearer, /*upperTag=*/6,
                                                       /*opaqueTag=*/6);

    CHECK(got.R == expected.R);
    CHECK(got.G == expected.G);
    CHECK(got.B == expected.B);
}

static void test_blend_three_slots_matches_hand_ordered_chain() {
    const AccumCell back = makeUniformCell(fromCode10(100), fromCode10(100),
                                            fromCode10(100), WeightAccum(kWeightUnity));
    const AccumCell mid = makeUniformCell(fromCode10(500), fromCode10(400),
                                           fromCode10(300), WeightAccum(kWeightUnity / 2));
    const AccumCell front = makeUniformCell(fromCode10(900), fromCode10(50),
                                             fromCode10(700), WeightAccum(kWeightUnity / 4));

    std::array<KSlot, kBufferK> slots{makeSlot(1, /*z=*/900, back), makeSlot(2, /*z=*/50, front),
                                       makeSlot(3, /*z=*/400, mid), KSlot{}};

    const CompositedCell got = compositeKBuffer(slots, KBufferResolveMode::Blend);

    // Independent re-derivation: chain composite() calls farthest to
    // nearest, not a call into compositeKBuffer() itself.
    const CompositedCell step1 = composite(back);
    const Background afterBack{step1.R, step1.G, step1.B};
    const CompositedCell step2 = composite(mid, afterBack);
    const Background afterMid{step2.R, step2.G, step2.B};
    const CompositedCell expected = composite(front, afterMid);

    CHECK(got.R == expected.R);
    CHECK(got.G == expected.G);
    CHECK(got.B == expected.B);
}

static void test_blend_z_tie_breaks_by_smallest_tag() {
    const AccumCell a = makeUniformCell(fromCode10(200), fromCode10(200),
                                         fromCode10(200), WeightAccum(kWeightUnity / 2));
    const AccumCell b = makeUniformCell(fromCode10(800), fromCode10(800),
                                         fromCode10(800), WeightAccum(kWeightUnity / 2));

    std::array<KSlot, kBufferK> order1{makeSlot(5, 77, b), makeSlot(1, 77, a), KSlot{}, KSlot{}};
    std::array<KSlot, kBufferK> order2{makeSlot(1, 77, a), makeSlot(5, 77, b), KSlot{}, KSlot{}};

    // Tag 1 < tag 5, so a is "nearer" and composited last (on top).
    const CompositedCell step = composite(b);
    const Background afterB{step.R, step.G, step.B};
    const CompositedCell expected = composite(a, afterB);

    const CompositedCell got1 = compositeKBuffer(order1, KBufferResolveMode::Blend);
    const CompositedCell got2 = compositeKBuffer(order2, KBufferResolveMode::Blend);

    CHECK(got1.R == expected.R && got1.G == expected.G && got1.B == expected.B);
    CHECK(got2.R == expected.R && got2.G == expected.G && got2.B == expected.B);
}

// "Known ratio": a fully-covered back slot, an exactly half-covered front
// slot -- alpha == kWeightUnity/2 blends front and back exactly in half,
// checked against an exact hand-computed value.
static void test_blend_known_ratio() {
    const AccumCell back =
        makeUniformCell(fromCode10(200), fromCode10(300), fromCode10(400),
                         WeightAccum(kWeightUnity));  // fully covered
    const AccumCell front =
        makeUniformCell(fromCode10(1000), fromCode10(100), fromCode10(50),
                         WeightAccum(kWeightUnity / 2));  // exactly half covered

    std::array<KSlot, kBufferK> slots{makeSlot(1, /*z=*/1000, back),
                                       makeSlot(2, /*z=*/0, front), KSlot{}, KSlot{}};
    const CompositedCell got = compositeKBuffer(slots, KBufferResolveMode::Blend);

    // back is fully covered, so composite(back, bg) == back's own resolved
    // colour regardless of bg; front then blends 50/50 over that exactly.
    const std::int64_t half = std::int64_t(kWeightUnity) / 2;
    const std::int64_t unity = std::int64_t(kWeightUnity);
    auto expectChannel = [&](Sample frontC, Sample backC) {
        return Sample((std::int64_t(frontC) * half + std::int64_t(backC) * (unity - half) +
                        unity / 2) /
                       unity);
    };
    CHECK(got.R == expectChannel(fromCode10(1000), fromCode10(200)));
    CHECK(got.G == expectChannel(fromCode10(100), fromCode10(300)));
    CHECK(got.B == expectChannel(fromCode10(50), fromCode10(400)));
}

// ---------------------------------------------------------------------------
// Part D -- WU-35a: manualTransp (DECISIONS.md ADR-087, WORK-UNITS.md
// WU-35a). **[P]-tier, not a confirmed historical mechanism** -- see this
// unit's own entries in DECISIONS.md/WORK-UNITS.md. All four tests below
// re-derive their expected values independently of compositeKBuffer()'s
// own blendBetweenSheets() helper (resolve.cpp), the same "hand-computed,
// not a call back into the function under test" discipline
// test_blend_known_ratio() above already uses.
// ---------------------------------------------------------------------------

// manualTransp's default (0) must leave compositeKBuffer()'s own Blend
// mode byte-identical to every session before WU-35a existed -- checked
// here by passing 0 explicitly as the new fourth argument and comparing
// against the same hand-ordered composite() chain
// test_blend_three_slots_matches_hand_ordered_chain() above already
// establishes as an independent reference.
static void test_manual_transp_zero_reproduces_pre_wu35a_blend() {
    const AccumCell back = makeUniformCell(fromCode10(100), fromCode10(100),
                                            fromCode10(100), WeightAccum(kWeightUnity));
    const AccumCell mid = makeUniformCell(fromCode10(500), fromCode10(400),
                                           fromCode10(300), WeightAccum(kWeightUnity / 2));
    const AccumCell front = makeUniformCell(fromCode10(900), fromCode10(50),
                                             fromCode10(700), WeightAccum(kWeightUnity / 4));

    std::array<KSlot, kBufferK> slots{makeSlot(1, /*z=*/900, back), makeSlot(2, /*z=*/50, front),
                                       makeSlot(3, /*z=*/400, mid), KSlot{}};

    const CompositedCell got =
        compositeKBuffer(slots, KBufferResolveMode::Blend, kDefaultBackground, /*manualTransp=*/0);

    const CompositedCell step1 = composite(back);
    const Background afterBack{step1.R, step1.G, step1.B};
    const CompositedCell step2 = composite(mid, afterBack);
    const Background afterMid{step2.R, step2.G, step2.B};
    const CompositedCell expected = composite(front, afterMid);

    CHECK(got.R == expected.R);
    CHECK(got.G == expected.G);
    CHECK(got.B == expected.B);
}

// T = kWeightUnity ("Ext. Key"/farthest-only end of the coefficient, not a
// named S1 mode itself but the mirror image of Opaque per WU-SM-02.md
// fixture 30): two fully-covered slots, nearer contributes
// (kWeightUnity - T) = 0, so the result must equal the farther slot's own
// resolved colour alone, independent of the nearer slot's colour --
// fixture 30's own "at T = 1 the near hemisphere should fully vanish
// behind the far one".
static void test_manual_transp_one_reverses_two_slot_occlusion() {
    const AccumCell farther = makeUniformCell(fromCode10(200), fromCode10(600),
                                               fromCode10(150), WeightAccum(kWeightUnity));
    const AccumCell nearer = makeUniformCell(fromCode10(800), fromCode10(100),
                                              fromCode10(900), WeightAccum(kWeightUnity));

    std::array<KSlot, kBufferK> slots{makeSlot(4, /*z=*/1000, farther),
                                       makeSlot(6, /*z=*/50, nearer), KSlot{}, KSlot{}};

    const CompositedCell got = compositeKBuffer(slots, KBufferResolveMode::Blend,
                                                  kDefaultBackground, Weight(kWeightUnity));
    const CompositedCell expected = composite(farther);  // fully covered: bg irrelevant

    CHECK(got.R == expected.R);
    CHECK(got.G == expected.G);
    CHECK(got.B == expected.B);
}

// T = kWeightUnity / 2: two fully-covered, equal-weight slots must match
// what an unarbitrated accumulate-then-normalise produces (fixture 30's
// own "at T = 0.5 the result should match ... the same starting point
// WU-28a/WU-28b shipped before any tag-based arbitration existed") -- for
// two equal-weight fully-covered cells that reduces to the plain average
// of their two resolved colours, rounded to nearest exactly the way
// blend()'s own "add half the divisor" convention already rounds.
static void test_manual_transp_half_matches_unarbitrated_accumulate() {
    const AccumCell farther = makeUniformCell(fromCode10(200), fromCode10(600),
                                               fromCode10(150), WeightAccum(kWeightUnity));
    const AccumCell nearer = makeUniformCell(fromCode10(800), fromCode10(100),
                                              fromCode10(900), WeightAccum(kWeightUnity));

    std::array<KSlot, kBufferK> slots{makeSlot(4, /*z=*/1000, farther),
                                       makeSlot(6, /*z=*/50, nearer), KSlot{}, KSlot{}};

    const CompositedCell got = compositeKBuffer(
        slots, KBufferResolveMode::Blend, kDefaultBackground, Weight(kWeightUnity / 2));

    auto expectAverage = [](Sample a, Sample b) {
        return Sample((std::int64_t(a) + std::int64_t(b) + 1) / 2);
    };
    CHECK(got.R == expectAverage(fromCode10(800), fromCode10(200)));
    CHECK(got.G == expectAverage(fromCode10(100), fromCode10(600)));
    CHECK(got.B == expectAverage(fromCode10(900), fromCode10(150)));
}

// T applies only to the between-sheet contribution, gated by the nearer
// slot's own coverage exactly the way a plain composite() call already
// scales by coverage -- a half-covered nearer slot at T = 0.5 should not
// simply average with the farther slot at full weight; the general
// two-value blend formula (test_blend_known_ratio()'s own idiom above)
// applied at the coverage-discounted alpha WU-35a's own design computes
// gives the independent expected value here.
static void test_manual_transp_gated_by_own_coverage_known_ratio() {
    const AccumCell back =
        makeUniformCell(fromCode10(200), fromCode10(300), fromCode10(400),
                         WeightAccum(kWeightUnity));  // fully covered
    const AccumCell front =
        makeUniformCell(fromCode10(1000), fromCode10(100), fromCode10(50),
                         WeightAccum(kWeightUnity / 2));  // exactly half covered

    std::array<KSlot, kBufferK> slots{makeSlot(1, /*z=*/1000, back),
                                       makeSlot(2, /*z=*/0, front), KSlot{}, KSlot{}};
    const CompositedCell got = compositeKBuffer(
        slots, KBufferResolveMode::Blend, kDefaultBackground, Weight(kWeightUnity / 2));

    // back is fully covered, so the base step resolves to back's own
    // colour regardless of bg (test_blend_known_ratio()'s own reasoning).
    // The between-sheet alpha is front's own half coverage further
    // discounted by (kWeightUnity - T) = kWeightUnity / 2, i.e.
    // coverageAlpha * nearerShare / kWeightUnity, rounded.
    const std::int64_t unity = std::int64_t(kWeightUnity);
    const std::int64_t coverageAlpha = unity / 2;   // front's own coverage
    const std::int64_t nearerShare = unity - unity / 2;  // kWeightUnity - T
    const std::int64_t alpha = (coverageAlpha * nearerShare + unity / 2) / unity;
    auto expectChannel = [&](Sample frontC, Sample backC) {
        return Sample((std::int64_t(frontC) * alpha + std::int64_t(backC) * (unity - alpha) +
                        unity / 2) /
                       unity);
    };
    CHECK(got.R == expectChannel(fromCode10(1000), fromCode10(200)));
    CHECK(got.G == expectChannel(fromCode10(100), fromCode10(300)));
    CHECK(got.B == expectChannel(fromCode10(50), fromCode10(400)));
}

// PipelineParams::manualTransp's own default -- WU-35a's binding
// constraint that T defaults to 0 (Opaque), checked directly against the
// struct rather than assumed from the header comment alone.
static void test_pipeline_params_manual_transp_defaults_to_zero() {
    const PipelineParams params;
    CHECK(params.manualTransp == 0);
}

// ---------------------------------------------------------------------------
// Part C -- full pipeline I6: --threads 1 byte-identical to --threads 8, a
// real folding-sphere frame, kBufferMode == Blend.
// ---------------------------------------------------------------------------

namespace {

constexpr double kPi = 3.14159265358979323846;

// WU-21g/h's own pole-to-pole, seamless-360-degree-wrap geometry (ADR-053):
// angleSpanV == pi reaches both poles without folding on its own;
// angleSpanH == 2*pi wraps fully around, deliberately folding the rest of
// the way -- opposite sides of the sphere land in the same destination
// cells at different depths. One tag only (PipelineParams::tag is
// single-valued per call, so one runFrame() call cannot itself route
// fragments into more than one k-buffer slot per cell -- Parts A/B above
// already cover the multi-slot resolve arithmetic directly); this test's
// own job is solely I6 through the new threaded k-buffer code path.
struct WarpedFrame {
    Lattice lattice;
    std::vector<Sample> y, cb, cr;
    int srcSize;
};

WarpedFrame buildFoldingSphereFrame(int srcSize, int destW, int destH) {
    const testpat::Frame zp = testpat::makeZonePlate(srcSize, srcSize);
    WarpedFrame w;
    w.srcSize = srcSize;
    w.y = zp.Y;
    w.cb.assign(std::size_t(srcSize) * std::size_t(srcSize), kChromaZero);
    w.cr.assign(std::size_t(srcSize) * std::size_t(srcSize), kChromaZero);

    SphereParams sp;
    sp.radius = double(destW) * 0.35;
    sp.angleSpanH = 2.0 * kPi;
    sp.angleSpanV = kPi;
    sp.centerX = double(destW) / 2.0;
    sp.centerY = double(destH) / 2.0;
    w.lattice = buildSphereLattice(sp);
    return w;
}

void runOnce(const WarpedFrame& w, int destW, int destH, int threads,
             video::Raster444& dest) {
    SourceRaster src;
    src.width = w.srcSize;
    src.height = w.srcSize;
    src.r = w.y.data();
    src.g = w.cb.data();
    src.b = w.cr.data();

    PipelineParams params;
    params.destWidth = destW;
    params.destHeight = destH;
    params.maxK = 1000.0;
    params.tag = 5;
    params.threads = threads;
    params.kBufferMode = KBufferResolveMode::Blend;

    runFrame(w.lattice, src, params, dest);
}

}  // namespace

static void test_kbuffer_pipeline_threads_1_matches_threads_8() {
    // Not exact multiples of either candidate tile size (16 or 32) --
    // exercises resolveOneTile()'s own partial-edge-tile clamp under the
    // new k-buffer branch too, the same reason test_threading.cpp's own
    // geometry is deliberately off-multiple.
    const int destW = 161;
    const int destH = 129;
    const int srcSize = 96;

    const WarpedFrame w = buildFoldingSphereFrame(srcSize, destW, destH);

    video::Raster444 reference(destW, destH);
    runOnce(w, destW, destH, /*threads=*/1, reference);

    bool sawNonBackground = false;
    for (std::size_t i = 0; i < reference.Y.size(); ++i) {
        if (reference.Y[i] != kBlack) {
            sawNonBackground = true;
            break;
        }
    }
    CHECK(sawNonBackground);

    const int threadCounts[] = {2, 3, 8};
    for (int threads : threadCounts) {
        video::Raster444 dest(destW, destH);
        runOnce(w, destW, destH, threads, dest);
        for (std::size_t i = 0; i < reference.Y.size(); ++i) {
            CHECK_ONCE(dest.Y[i] == reference.Y[i]);
            CHECK_ONCE(dest.Cb[i] == reference.Cb[i]);
            CHECK_ONCE(dest.Cr[i] == reference.Cr[i]);
        }
    }
}

int main() {
    test_opaque_nearest_wins_exactly();
    test_opaque_z_tie_breaks_by_smallest_tag();
    test_blend_two_slots_matches_compositeLayered();
    test_blend_three_slots_matches_hand_ordered_chain();
    test_blend_z_tie_breaks_by_smallest_tag();
    test_blend_known_ratio();
    test_manual_transp_zero_reproduces_pre_wu35a_blend();
    test_manual_transp_one_reverses_two_slot_occlusion();
    test_manual_transp_half_matches_unarbitrated_accumulate();
    test_manual_transp_gated_by_own_coverage_known_ratio();
    test_pipeline_params_manual_transp_defaults_to_zero();
    test_no_occupied_slot_is_pure_background_both_modes();
    test_kbuffer_pipeline_threads_1_matches_threads_8();
    return scatter::test::summary("test_kbuffer_resolve");
}
