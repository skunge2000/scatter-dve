// scatter-dve — WU-22a: opt-in weight-capture plumbing for the diagnostic
// coverage view (architecture.md section 8's src/diag/coverage_view.cpp,
// Phase 5's own "done when" line; DECISIONS.md ADR-056 for the full design
// this file checks).
//
// This unit's own job is narrow: PipelineParams::weightOut (core/resolve.hpp)
// and its one write site in core/pipeline.cpp's resolveOneTile() move each
// destination cell's already-computed AccumCell::w out to a caller-supplied
// buffer instead of letting it fall on the floor after composite() reads it.
// The coverage-weight arithmetic itself (architecture.md 4.5's splat, 4.8's
// normalise/composite) is WU-08/09/10's, already checked in
// tests/test_binner.cpp, tests/test_splat.cpp and tests/test_zoneplate.cpp;
// this file does not re-derive it. What it does check, directly:
//
// 1. Capturing has zero effect on the pipeline's existing, already-verified
//    output — weightOut is a pure side channel, never a second input.
// 2. The captured buffer is genuinely zero at guaranteed-uncovered pixels
//    and genuinely near kWeightUnity at guaranteed-fully-covered ones —
//    the two coarse, hand-derivable landmarks a real coverage capture must
//    hit, reusing tests/test_zoneplate.cpp's own offset-placement
//    construction (its own file header derives exactly which destination
//    columns land in each regime; duplicated here per SESSION-PROTOCOL.md
//    rule 2, not shared across translation units).
// 3. The captured value at every pixel is bit-for-bit the same AccumCell::w
//    resolveOneTile() itself accumulated — proven by an independent
//    recomputation through the same public PASS-1/PASS-2 primitives
//    (generateFragments(), splatTile(), sumBanks() — core/binner.hpp,
//    core/splat.hpp), not by re-deriving the arithmetic a different way.
//    This is a plumbing check (did the right value reach the right index),
//    not a fresh proof that splatTile()/sumBanks() are correct — that proof
//    already exists.
// 4. Capture is unaffected by PipelineParams::threads — the same I6
//    (integer addition is associative) discipline tests/test_threading.cpp
//    already established for `dest`, extended here to weightOut, since
//    WU-22a adds a second output PASS 2 writes into per tile.
#include "core/binner.hpp"
#include "core/resolve.hpp"
#include "core/splat.hpp"
#include "harness.hpp"

#include <cstdint>
#include <vector>

using namespace scatter;

namespace {

// Same technique as tests/test_zoneplate.cpp's own makeAffineLattice() —
// duplicated locally per SESSION-PROTOCOL.md rule 2. dest.x = offX + scaleX
// * px, dest.y = offY + scaleY * py, exact for any continuous (u, v) since
// Catmull-Rom reproduces an affine function of its control points exactly.
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

std::vector<Sample> flatPlane(int w, int h, Sample v) {
    return std::vector<Sample>(std::size_t(w) * std::size_t(h), v);
}

// ---------------------------------------------------------------------------
// 1. Capture is side-effect-free on the composited output.
// ---------------------------------------------------------------------------

void test_capture_is_side_effect_free() {
    const int srcSize = 32, destSize = 48;
    std::vector<Sample> y  = flatPlane(srcSize, srcSize, fromCode10(700));
    std::vector<Sample> cb = flatPlane(srcSize, srcSize, fromCode10(300));
    std::vector<Sample> cr = flatPlane(srcSize, srcSize, fromCode10(800));

    SourceRaster src;
    src.width = srcSize;
    src.height = srcSize;
    src.r = y.data();
    src.g = cb.data();
    src.b = cr.data();

    // Deliberately compressive (2:1) and off-centre, so both fully-covered
    // and uncovered destination cells exist — a livelier regression check
    // than an identity map's own degenerate one-fragment-per-cell case.
    const Lattice lat = makeAffineLattice(0.5, 0.5, 8.0, 8.0, srcSize, srcSize);

    PipelineParams paramsNoCapture;
    paramsNoCapture.destWidth = destSize;
    paramsNoCapture.destHeight = destSize;
    paramsNoCapture.maxK = 1000.0;

    PipelineParams paramsWithCapture = paramsNoCapture;
    std::vector<WeightAccum> weightBuf(std::size_t(destSize) * std::size_t(destSize),
                                        WeightAccum(-1));  // sentinel: every cell must be overwritten
    paramsWithCapture.weightOut = weightBuf.data();

    video::Raster444 destNoCapture(destSize, destSize);
    video::Raster444 destWithCapture(destSize, destSize);
    runFrame(lat, src, paramsNoCapture, destNoCapture);
    runFrame(lat, src, paramsWithCapture, destWithCapture);

    CHECK(destNoCapture.Y == destWithCapture.Y);
    CHECK(destNoCapture.Cb == destWithCapture.Cb);
    CHECK(destNoCapture.Cr == destWithCapture.Cr);

    // Every cell in a fully covered/uncovered destination raster of this
    // construction is visited by resolveOneTile()'s own loop (it does not
    // skip cells), so the sentinel must be gone everywhere.
    for (WeightAccum w : weightBuf) {
        CHECK_ONCE(w != WeightAccum(-1));
    }
}

// ---------------------------------------------------------------------------
// 2. Zero at guaranteed-uncovered pixels; near kWeightUnity at
//    guaranteed-fully-covered ones. Reuses tests/test_zoneplate.cpp's own
//    64x64-into-128x64, half-pixel-offset (32.5) construction — see that
//    file's header for the full derivation of which destination columns
//    land in which regime. Duplicated here per SESSION-PROTOCOL.md rule 2.
// ---------------------------------------------------------------------------

void test_capture_zero_at_uncovered_and_near_unity_at_full_coverage() {
    const int srcSize = 64;
    const int destW = 128, destH = 64;

    std::vector<Sample> y  = flatPlane(srcSize, srcSize, fromCode10(900));
    std::vector<Sample> cb = flatPlane(srcSize, srcSize, fromCode10(150));
    std::vector<Sample> cr = flatPlane(srcSize, srcSize, fromCode10(850));

    SourceRaster src;
    src.width = srcSize;
    src.height = srcSize;
    src.r = y.data();
    src.g = cb.data();
    src.b = cr.data();

    const Lattice lat = makeAffineLattice(1.0, 1.0, 32.5, 0.0, srcSize, srcSize);
    PipelineParams params;
    params.destWidth = destW;
    params.destHeight = destH;
    params.maxK = 1000.0;

    std::vector<WeightAccum> weightBuf(std::size_t(destW) * std::size_t(destH));
    params.weightOut = weightBuf.data();

    video::Raster444 dest(destW, destH);
    runFrame(lat, src, params, dest);

    auto at = [&](int x, int y2) noexcept {
        return std::size_t(y2) * std::size_t(destW) + std::size_t(x);
    };

    // Zero-coverage columns are checked across every row, including row 0
    // and row destH-1: emptiness does not depend on the Jacobian at all,
    // so CORRECTIONS.md C-008(a) below has nothing to do with this half of
    // the check.
    for (int yRow = 0; yRow < destH; ++yRow) {
        for (int x = 0; x < 32; ++x) {
            CHECK_ONCE(weightBuf[at(x, yRow)] == 0);
        }
        for (int x = 97; x < destW; ++x) {
            CHECK_ONCE(weightBuf[at(x, yRow)] == 0);
        }
    }

    // Full-coverage columns [33, 96) sum two adjacent source columns' own
    // half-contributions to one full-weight cell -- but only once both
    // contributing source columns, *and* the source row itself, are clear
    // of CORRECTIONS.md C-008(a)'s edge-derivative damping: this
    // construction's source and destination share the same (u, v) <->
    // (px, py) mapping at scale 1, so destination row 0/destH-1 map
    // straight onto source row py=0/63 and destination column 33/95 pull
    // in source column px=0/63 -- exactly the lattice-parameter positions
    // (u or v = 0 or kLatticeMax) ADR-022's edge-replication clamp damps
    // the analytic derivative at, up to 50% (measured directly for this
    // exact construction: K doubles from 1.0 to 2.0 at the single-axis
    // edge, verified against core/jacobian.hpp's own densityCompensation()
    // before writing this check). That is a genuine, already-frozen
    // property of this codebase's lattice edge handling, not something
    // this unit's capture is responsible for reproducing faithfully rather
    // than routing around -- tests/test_zoneplate.cpp's own
    // test_pipeline_partial_coverage_no_fringe() hits the identical effect
    // on *colour* and works around it with a rounding margin instead, since
    // colour normalises the weight distortion away; weight itself has
    // nothing to normalise against, so the honest fix here is the same one
    // test_i7_identity_full_pipeline() already uses for a different reason
    // -- stay one full row/column inside the raster's own edge, where the
    // Jacobian is undamped and the true value (kWeightUnity, exactly, not
    // merely approximately) is what a correct capture must show.
    for (int yRow = 1; yRow < destH - 1; ++yRow) {
        for (int x = 34; x < 95; ++x) {
            CHECK_ONCE(weightBuf[at(x, yRow)] == WeightAccum(kWeightUnity));
        }
    }
}

// ---------------------------------------------------------------------------
// 3. Captured weight matches an independent recomputation through the same
//    public PASS-1/PASS-2 primitives resolveOneTile() itself calls —
//    generateFragments() (core/binner.hpp), splatTile()/sumBanks()
//    (core/splat.hpp). A genuinely compressive, off-centre warp, so cells
//    at every coverage level (zero, partial, well above unity) are
//    exercised, not just the identity map's degenerate case.
// ---------------------------------------------------------------------------

void test_capture_matches_independent_recomputation() {
    const int srcSize = 96, destSize = 64;

    std::vector<Sample> y(std::size_t(srcSize) * std::size_t(srcSize));
    std::vector<Sample> cb = flatPlane(srcSize, srcSize, kChromaZero);
    std::vector<Sample> cr = flatPlane(srcSize, srcSize, kChromaZero);
    // A ramp, not a flat field: exercises real per-pixel colour variation
    // through the capture path, even though this unit's own check below is
    // about weight, not colour.
    for (int yy = 0; yy < srcSize; ++yy) {
        for (int xx = 0; xx < srcSize; ++xx) {
            const int code = kCode10Min + (xx * (kCode10Max - kCode10Min)) / (srcSize - 1);
            y[std::size_t(yy) * std::size_t(srcSize) + std::size_t(xx)] =
                fromCode10(std::uint16_t(code));
        }
    }

    SourceRaster src;
    src.width = srcSize;
    src.height = srcSize;
    src.r = y.data();
    src.g = cb.data();
    src.b = cr.data();

    // 3:2 compression (96 -> 64), off-centre, so K > 1 everywhere and many
    // source samples land per destination cell — architecture.md 4.4's own
    // "order 1000 fragments under 32:1 compression" territory in miniature.
    const Lattice lat = makeAffineLattice(2.0 / 3.0, 2.0 / 3.0, 3.0, 5.0, srcSize, srcSize);
    PipelineParams params;
    params.destWidth = destSize;
    params.destHeight = destSize;
    params.maxK = 1000.0;

    std::vector<WeightAccum> captured(std::size_t(destSize) * std::size_t(destSize));
    params.weightOut = captured.data();

    video::Raster444 dest(destSize, destSize);
    runFrame(lat, src, params, dest);

    // Independent recomputation: PASS 1 (generateFragments(), the exact
    // same public entry point runFrame()'s own threads<=1 branch calls)
    // into a fresh TileBins, then PASS 2's own bank-resolve
    // (splatTile()/sumBanks(), core/splat.hpp) per tile, into a
    // freestanding AccumCell grid this test owns — never touching
    // core/pipeline.cpp's resolveOneTile() or PipelineParams::weightOut at
    // all. If capture's own indexing (dx/dy against params.destWidth,
    // core/pipeline.cpp) is wrong in any way — transposed, off-by-one,
    // stale across tiles — this diverges from `captured` above; if it is
    // right, I6 (integer addition is associative) makes the two exactly
    // equal, not merely close.
    TileBins bins(destSize, destSize);
    SupersampleConfig ss;  // PipelineParams' own default, replicated: WU-08's
                            // ADR-024 threshold values (1.0/4.0), matching
                            // what defaultPipelineSupersample() (core/
                            // resolve.hpp) adds its small margin on top of.
    // PipelineParams' own default margin is not reproduced here on purpose:
    // this warp's det J is nowhere near 1.0 (2:1-ish compression, not an
    // identity map), so the C-008 floating-point-noise-at-exactly-1.0
    // margin defaultPipelineSupersample() exists for cannot matter to which
    // branch chooseSupersample() takes for any sample this construction
    // generates — the two configs are behaviourally identical here.
    generateFragments(lat, src, params.maxK, ss, params.tag, bins);

    const int tilesX = tileCount(destSize);
    const int tilesY = tileCount(destSize);
    const std::size_t tilePixelsN = std::size_t(kTilePixels);
    std::vector<AccumCell> tileCells(tilePixelsN);
    TileAccum accum;

    bool anyAboveUnity = false;
    bool anyZero = false;

    for (int ty = 0; ty < tilesY; ++ty) {
        for (int tx = 0; tx < tilesX; ++tx) {
            accum.clear();
            splatTile(bins.tile(tx, ty), accum);
            sumBanks(accum, tileCells.data());

            const int originX = tx * kTileSize;
            const int originY = ty * kTileSize;
            const int localWidth = std::min(kTileSize, destSize - originX);
            const int localHeight = std::min(kTileSize, destSize - originY);

            for (int ly = 0; ly < localHeight; ++ly) {
                for (int lx = 0; lx < localWidth; ++lx) {
                    const AccumCell& cell =
                        tileCells[std::size_t(ly) * std::size_t(kTileSize) + std::size_t(lx)];
                    const int dx = originX + lx;
                    const int dy = originY + ly;
                    const std::size_t idx =
                        std::size_t(dy) * std::size_t(destSize) + std::size_t(dx);

                    CHECK_ONCE(captured[idx] == cell.w);

                    if (cell.w > WeightAccum(kWeightUnity)) anyAboveUnity = true;
                    if (cell.w == 0) anyZero = true;
                }
            }
        }
    }

    // Confirms this construction genuinely exercises both regimes — a
    // check that always trivially passed (e.g. an all-zero or all-unity
    // capture from a construction that never left the destination raster,
    // or a degenerate lattice) would not be exercising anything.
    CHECK(anyAboveUnity);
    CHECK(anyZero);
}

// ---------------------------------------------------------------------------
// 4. Capture is unaffected by PipelineParams::threads — I6's own guarantee
//    (tests/test_threading.cpp), extended to this unit's second output.
// ---------------------------------------------------------------------------

void test_capture_matches_across_thread_counts() {
    const int srcSize = 80, destSize = 96;

    std::vector<Sample> y(std::size_t(srcSize) * std::size_t(srcSize));
    for (int yy = 0; yy < srcSize; ++yy) {
        for (int xx = 0; xx < srcSize; ++xx) {
            const int code = kCode10Min + ((xx + yy) * (kCode10Max - kCode10Min)) /
                                               (2 * srcSize - 2);
            y[std::size_t(yy) * std::size_t(srcSize) + std::size_t(xx)] =
                fromCode10(std::uint16_t(code));
        }
    }
    std::vector<Sample> cb = flatPlane(srcSize, srcSize, kChromaZero);
    std::vector<Sample> cr = flatPlane(srcSize, srcSize, kChromaZero);

    SourceRaster src;
    src.width = srcSize;
    src.height = srcSize;
    src.r = y.data();
    src.g = cb.data();
    src.b = cr.data();

    // Magnifying this time (1.5x), off a non-multiple-of-tile-size raster,
    // so PASS 1's supersampling and PASS 2's tile partitioning both do
    // genuine work across the thread counts compared below — the same
    // construction shape tests/test_threading.cpp's own accept criterion
    // favours for exactly this reason.
    const Lattice lat = makeAffineLattice(1.5, 1.5, 4.0, 2.0, srcSize, srcSize);

    std::vector<WeightAccum> capturedSingle(std::size_t(destSize) * std::size_t(destSize));
    {
        PipelineParams params;
        params.destWidth = destSize;
        params.destHeight = destSize;
        params.maxK = 1000.0;
        params.threads = 1;
        params.weightOut = capturedSingle.data();
        video::Raster444 dest(destSize, destSize);
        runFrame(lat, src, params, dest);
    }

    for (int threads : {2, 8}) {
        std::vector<WeightAccum> capturedN(std::size_t(destSize) * std::size_t(destSize));
        PipelineParams params;
        params.destWidth = destSize;
        params.destHeight = destSize;
        params.maxK = 1000.0;
        params.threads = threads;
        params.weightOut = capturedN.data();
        video::Raster444 dest(destSize, destSize);
        runFrame(lat, src, params, dest);

        CHECK(capturedN == capturedSingle);
    }
}

}  // namespace

int main() {
    test_capture_is_side_effect_free();
    test_capture_zero_at_uncovered_and_near_unity_at_full_coverage();
    test_capture_matches_independent_recomputation();
    test_capture_matches_across_thread_counts();

    return scatter::test::summary("test_coverage_capture");
}
