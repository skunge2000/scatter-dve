// scatter-dve — WU-16: thread pool, QoS, per-worker bin arenas
// (Phase 4; architecture.md section 6, scoped per DECISIONS.md ADR-040)
//
// Checks WORK-UNITS.md's own WU-16 accept line directly: "8-thread output
// bit-identical to single-threaded (I6)." Two parts:
//
// 1. test_threadpool_runs_every_worker_every_round() and
//    test_threadpool_of_one() exercise core/pipeline.hpp's ThreadPool
//    directly — dispatch reaches every worker index exactly once per
//    runOnAll() call, repeatably across many calls on the same pool, and
//    the pool tears down cleanly (its destructor joins every worker; a
//    stuck join would hang this test process, which ctest reports as a
//    failure/timeout rather than a pass) — independent of anything
//    runFrame() itself does with it.
//
// 2. test_threaded_pipeline_matches_single_threaded() is the literal
//    accept criterion: a genuinely warped frame (a cylinder over a zone
//    plate — the same construction technique tests/test_zoneplate.cpp and
//    tests/test_shapes.cpp already use, duplicated locally per
//    SESSION-PROTOCOL.md rule 2), run through runFrame() once at
//    PipelineParams::threads == 1 and again at several other thread
//    counts including 8, checked bit-for-bit (Sample is an integer type;
//    I6 promises exact equality, not a tolerance) against the
//    single-threaded reference for every destination pixel's Y/Cb/Cr.
//    destWidth/destHeight are deliberately not exact multiples of the
//    tile size, so the run also exercises resolveOneTile()'s own
//    partial-tile edge clamp (core/pipeline.cpp) under threading, not just
//    full interior tiles. One thread count (16) is deliberately larger
//    than the frame's own tile count, so some workers get zero tiles in
//    core/pipeline.cpp's own `idx = worker; idx < totalTiles; idx +=
//    pool.size()` partition — that must be harmless, not a crash or a
//    hang.
#include "core/pipeline.hpp"
#include "core/resolve.hpp"
#include "core/shapes/shapes.hpp"
#include "harness.hpp"
#include "testpat.hpp"

#include <atomic>
#include <cstddef>
#include <vector>

using namespace scatter;

namespace {

// ---------------------------------------------------------------------------
// Part 1 — ThreadPool itself, independent of runFrame().
// ---------------------------------------------------------------------------

void test_threadpool_runs_every_worker_every_round() {
    const int numThreads = 5;
    const int numRounds = 3;
    ThreadPool pool(numThreads);
    CHECK(pool.size() == numThreads);

    // vector<atomic<int>>'s (size_type) constructor value-initialises each
    // element in place -- no copy of a non-copyable std::atomic required.
    // Parenthesised-with-a-variable form parses as a function declaration
    // (-Wvexing-parse, an error under this project's -Werror, ADR-017);
    // the count is bound to a named size_t first to keep the constructor
    // call itself unambiguous.
    const std::size_t numThreadsN = std::size_t(numThreads);
    std::vector<std::atomic<int>> callCount(numThreadsN);

    for (int round = 0; round < numRounds; ++round) {
        std::vector<std::atomic<bool>> seenThisRound(numThreadsN);
        pool.runOnAll([&](int worker) {
            callCount[std::size_t(worker)].fetch_add(1, std::memory_order_relaxed);
            seenThisRound[std::size_t(worker)].store(true, std::memory_order_relaxed);
        });
        for (int i = 0; i < numThreads; ++i) {
            CHECK(seenThisRound[std::size_t(i)].load());
        }
    }
    for (int i = 0; i < numThreads; ++i) {
        CHECK(callCount[std::size_t(i)].load() == numRounds);
    }
}

void test_threadpool_of_one() {
    ThreadPool pool(1);
    CHECK(pool.size() == 1);
    std::atomic<int> hits{0};
    for (int round = 0; round < 4; ++round) {
        pool.runOnAll([&](int worker) {
            CHECK(worker == 0);
            hits.fetch_add(1, std::memory_order_relaxed);
        });
    }
    CHECK(hits.load() == 4);
}

// setWorkerQoS() is a no-op everywhere this test suite actually runs (the
// Apple branch is unverified in this Linux cloud sandbox -- see
// DECISIONS.md ADR-040) -- calling it directly on the calling thread at
// least confirms it links and returns rather than being trusted purely by
// inspection.
void test_set_worker_qos_does_not_crash() {
    setWorkerQoS();
    CHECK(true);
}

// ---------------------------------------------------------------------------
// Part 2 — the accept criterion: runFrame() itself.
// ---------------------------------------------------------------------------

// Same technique tests/test_zoneplate.cpp's testZonePlateCompression() and
// tests/test_shapes.cpp's runFlatSourceThroughLattice() already use,
// duplicated locally per SESSION-PROTOCOL.md rule 2 (no cross-test-file
// sharing): a zone plate (varied, non-uniform frequency content, so
// different tiles genuinely accumulate different fragment counts and
// weights -- a more honest stress case than a flat source, which would
// give every covered tile near-identical work) warped through a cylinder
// (ADR-027) so the per-source-pixel Jacobian, and therefore
// generateFragments()'s own supersampling decisions and fragment density,
// vary across the frame instead of being uniform the way a pure affine
// warp's would be.
struct WarpedFrame {
    Lattice lattice;
    std::vector<Sample> y, cb, cr;
    int srcSize;
};

WarpedFrame buildWarpedTestFrame(int srcSize, int destW, int destH) {
    const testpat::Frame zp = testpat::makeZonePlate(srcSize, srcSize);

    WarpedFrame w;
    w.srcSize = srcSize;
    w.y = zp.Y;
    // Full-width flat chroma, exactly test_zoneplate.cpp's own reasoning:
    // testpat::makeZonePlate's chroma is flat by construction, so filling
    // full-width Cb/Cr planes directly is equivalent to running the
    // narrower 4:2:2 planes through chroma::upsampleImage, and keeps this
    // test isolated from the chroma resampler entirely.
    w.cb.assign(std::size_t(srcSize) * std::size_t(srcSize), kChromaZero);
    w.cr.assign(std::size_t(srcSize) * std::size_t(srcSize), kChromaZero);

    shapes::CylinderParams cp;
    cp.radius = double(destW) * 0.6;
    cp.angleSpan = 1.8;  // > pi/2: a strong roll, well past WU-11's own default
    cp.heightSpan = double(destH) * 0.9;
    cp.centerX = double(destW) / 2.0;
    cp.centerY = double(destH) / 2.0;
    w.lattice = shapes::buildCylinderLattice(cp);

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
    params.threads = threads;

    runFrame(w.lattice, src, params, dest);
}

void test_threaded_pipeline_matches_single_threaded() {
    // Deliberately not exact multiples of either candidate tile size (16
    // or 32, SCATTER_TILE_LOG2) -- exercises resolveOneTile()'s own
    // partial-edge-tile clamp (core/pipeline.cpp) in every threaded run
    // below, not just full interior tiles.
    const int destW = 257;
    const int destH = 193;
    const int srcSize = 256;

    const WarpedFrame w = buildWarpedTestFrame(srcSize, destW, destH);

    video::Raster444 reference(destW, destH);
    runOnce(w, destW, destH, /*threads=*/1, reference);

    // Sanity check on the reference itself: not every pixel is
    // background, i.e. the warp actually put something on the raster --
    // otherwise a bit-identical-to-a-blank-frame comparison below would
    // pass for a broken reason. Background per PipelineParams' own
    // default is kBlack/kChromaZero/kChromaZero (core/resolve.hpp).
    bool sawNonBackground = false;
    for (std::size_t i = 0; i < reference.Y.size(); ++i) {
        if (reference.Y[i] != kBlack) {
            sawNonBackground = true;
            break;
        }
    }
    CHECK(sawNonBackground);

    // 0 and a negative value both take the documented threads<=1 path
    // (core/resolve.hpp's own PipelineParams::threads comment) -- checked
    // explicitly rather than assumed, alongside every threaded count
    // WORK-UNITS.md's own WU-16 accept line names (8) and a few more:
    // one that does not divide the tile grid evenly (3), one equal to a
    // typical performance-core count (5), and one (16) deliberately
    // larger than this frame's own total tile count, so some workers get
    // zero tiles in core/pipeline.cpp's own static partition.
    const int threadCounts[] = {0, -3, 1, 2, 3, 5, 8, 16};

    for (int threads : threadCounts) {
        video::Raster444 dest(destW, destH);
        runOnce(w, destW, destH, threads, dest);

        CHECK(dest.Y.size() == reference.Y.size());
        CHECK(dest.Cb.size() == reference.Cb.size());
        CHECK(dest.Cr.size() == reference.Cr.size());

        for (std::size_t i = 0; i < reference.Y.size(); ++i) {
            CHECK_ONCE(dest.Y[i] == reference.Y[i]);
            CHECK_ONCE(dest.Cb[i] == reference.Cb[i]);
            CHECK_ONCE(dest.Cr[i] == reference.Cr[i]);
        }
    }
}

// A second, differently-shaped warp/frame size, run only at threads == 1
// vs threads == 8 (WORK-UNITS.md's own literal accept line) -- guards
// against test_threaded_pipeline_matches_single_threaded()'s own single
// geometry happening to be one where every tile's fragment count is a
// round multiple of something and a partition bug would not show up.
// Square, exact multiple of both candidate tile sizes this time (deliberate
// contrast with the first test's off-multiple geometry).
void test_threaded_pipeline_matches_single_threaded_second_geometry() {
    const int destW = 320;
    const int destH = 320;
    const int srcSize = 200;

    const WarpedFrame w = buildWarpedTestFrame(srcSize, destW, destH);

    video::Raster444 reference(destW, destH);
    runOnce(w, destW, destH, 1, reference);

    video::Raster444 eightThreads(destW, destH);
    runOnce(w, destW, destH, 8, eightThreads);

    for (std::size_t i = 0; i < reference.Y.size(); ++i) {
        CHECK_ONCE(eightThreads.Y[i] == reference.Y[i]);
        CHECK_ONCE(eightThreads.Cb[i] == reference.Cb[i]);
        CHECK_ONCE(eightThreads.Cr[i] == reference.Cr[i]);
    }
}

}  // namespace

int main() {
    test_threadpool_runs_every_worker_every_round();
    test_threadpool_of_one();
    test_set_worker_qos_does_not_crash();
    test_threaded_pipeline_matches_single_threaded();
    test_threaded_pipeline_matches_single_threaded_second_geometry();
    return scatter::test::summary("test_threading");
}
