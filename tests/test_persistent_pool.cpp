// scatter-dve — WU-19a: persistent, caller-owned ThreadPool
// (Phase 4; DECISIONS.md ADR-044, completing ADR-040/ADR-041's own
// deferred "a persistent, caller-owned ThreadPool that runFrame() can
// reuse across many calls instead of constructing one per call... WU-19's
// own job").
//
// WU-16a/16b (ADR-040/041) already established I6 (output bit-identical
// to the threads<=1 oracle) for a *freshly constructed, per-call*
// ThreadPool -- PipelineParams::threads > 1 with PipelineParams::pool
// left at its default nullptr -- checked by tests/test_threading.cpp and
// tests/test_row_band.cpp, both unchanged and still green (see this
// unit's own DECISIONS.md ADR-044 for the verification method: the full
// pre-existing suite passing unmoved is what proves runThreaded()'s own
// extraction in core/pipeline.cpp changed nothing about that case).
//
// What this file checks is new: that the same correctness holds when a
// single ThreadPool is constructed once, outside runFrame() entirely, and
// reused across many runFrame() calls -- including calls against
// different frame geometries, and calls whose PipelineParams::threads
// field deliberately disagrees with the pool's own size() -- and that
// this reuse pattern is itself clean (no hang, no leak: a ThreadPool that
// mishandled a second runOnAll() round after a first would most likely
// hang, which ctest reports as a failure/timeout, not a silently wrong
// answer).
#include "core/pipeline.hpp"
#include "core/resolve.hpp"
#include "core/shapes/shapes.hpp"
#include "harness.hpp"
#include "testpat.hpp"

#include <cstddef>
#include <vector>

using namespace scatter;

namespace {

// Same construction technique tests/test_threading.cpp's own
// buildWarpedTestFrame() uses -- duplicated locally, per SESSION-PROTOCOL.md
// rule 2 (no cross-test-file sharing): a zone plate (non-uniform
// frequency content, so different tiles genuinely accumulate different
// fragment counts and weights) warped through a cylinder (ADR-027), so
// the per-source-pixel Jacobian -- and therefore generateFragments()'s
// own supersampling decisions and fragment density -- vary across the
// frame instead of being uniform the way a pure affine warp's would be.
struct WarpedFrame {
    Lattice lattice;
    std::vector<Sample> y, cb, cr;
    int srcSize;
};

WarpedFrame buildWarpedTestFrame(int srcSize, int destW, int destH,
                                  double angleSpan) {
    const testpat::Frame zp = testpat::makeZonePlate(srcSize, srcSize);

    WarpedFrame w;
    w.srcSize = srcSize;
    w.y = zp.Y;
    // Full-width flat chroma, exactly test_zoneplate.cpp's/test_threading.cpp's
    // own reasoning: testpat::makeZonePlate's chroma is flat by
    // construction, so this is equivalent to running the narrower 4:2:2
    // planes through chroma::upsampleImage, keeping this test isolated
    // from the chroma resampler entirely.
    w.cb.assign(std::size_t(srcSize) * std::size_t(srcSize), kChromaZero);
    w.cr.assign(std::size_t(srcSize) * std::size_t(srcSize), kChromaZero);

    shapes::CylinderParams cp;
    cp.radius = double(destW) * 0.6;
    cp.angleSpan = angleSpan;
    cp.heightSpan = double(destH) * 0.9;
    cp.centerX = double(destW) / 2.0;
    cp.centerY = double(destH) / 2.0;
    w.lattice = shapes::buildCylinderLattice(cp);

    return w;
}

void runOnce(const WarpedFrame& w, int destW, int destH, int threads,
             ThreadPool* pool, video::Raster444& dest) {
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
    params.pool = pool;

    runFrame(w.lattice, src, params, dest);
}

bool hasNonBackground(const video::Raster444& r) {
    for (std::size_t i = 0; i < r.Y.size(); ++i) {
        if (r.Y[i] != kBlack) {
            return true;
        }
    }
    return false;
}

void checkMatches(const video::Raster444& dest, const video::Raster444& reference) {
    CHECK(dest.Y.size() == reference.Y.size());
    CHECK(dest.Cb.size() == reference.Cb.size());
    CHECK(dest.Cr.size() == reference.Cr.size());
    for (std::size_t i = 0; i < reference.Y.size(); ++i) {
        CHECK_ONCE(dest.Y[i] == reference.Y[i]);
        CHECK_ONCE(dest.Cb[i] == reference.Cb[i]);
        CHECK_ONCE(dest.Cr[i] == reference.Cr[i]);
    }
}

// ---------------------------------------------------------------------------
// 1. A persistent pool, constructed once, reused across many runFrame()
//    calls against the *same* geometry, matches the threads<=1/pool==nullptr
//    oracle on every single call -- not just the first, which is the
//    specific thing WU-16a/16b's own per-call-construction tests could
//    never have exercised (their own ThreadPool never outlives one call).
// ---------------------------------------------------------------------------
void test_persistent_pool_matches_single_threaded_across_many_calls() {
    const int destW = 257;  // deliberately not a multiple of either
    const int destH = 193;  // candidate tile size (16 or 32) -- exercises
                             // resolveOneTile()'s partial-edge-tile clamp.
    const int srcSize = 256;
    const int numCalls = 10;

    const WarpedFrame w = buildWarpedTestFrame(srcSize, destW, destH, 1.8);

    video::Raster444 reference(destW, destH);
    runOnce(w, destW, destH, /*threads=*/1, /*pool=*/nullptr, reference);
    CHECK(hasNonBackground(reference));

    ThreadPool pool(8);
    for (int call = 0; call < numCalls; ++call) {
        video::Raster444 dest(destW, destH);
        // threads is left at 8 here (matching pool.size()) -- test 2,
        // below, is where the two are deliberately made to disagree.
        runOnce(w, destW, destH, /*threads=*/8, &pool, dest);
        checkMatches(dest, reference);
    }
    // pool goes out of scope here (destructor joins all 8 workers) --
    // a stuck join after ten prior runOnAll() round-trips would hang this
    // test process, which ctest reports as a failure/timeout rather than
    // a pass, so reaching the end of main() (see below) is itself part of
    // what this test checks.
}

// ---------------------------------------------------------------------------
// 2. PipelineParams::pool's own documented contract (core/resolve.hpp):
//    when non-null, pool->size() alone decides the worker count actually
//    used, and PipelineParams::threads is not consulted at all in that
//    branch. Checked directly, not just asserted in a comment: a pool
//    deliberately sized differently from `threads` still produces correct
//    output. This is a genuine regression guard, not a tautology --
//    core/pipeline.cpp's own row-band partition
//    (`(worker * src.height) / numWorkers`) and tile partition
//    (`idx += numWorkers`) both use whatever `numWorkers` they are given;
//    if runFrame() mistakenly partitioned using params.threads while only
//    dispatching pool.size() worker callbacks (or vice versa), rows or
//    tiles would be dropped or double-covered, which this bit-identity
//    check would catch -- I6 guarantees a *correct* partition is
//    order/count-independent, not that an *inconsistent* one still adds
//    up.
// ---------------------------------------------------------------------------
void test_persistent_pool_size_governs_partition_not_threads_field() {
    const int destW = 320;
    const int destH = 320;
    const int srcSize = 200;

    const WarpedFrame w = buildWarpedTestFrame(srcSize, destW, destH, 1.2);

    video::Raster444 reference(destW, destH);
    runOnce(w, destW, destH, /*threads=*/1, /*pool=*/nullptr, reference);
    CHECK(hasNonBackground(reference));

    // Pool size (3) and the params.threads field (99, then 1 -- both
    // values that would themselves take a *different* code path if they,
    // rather than pool->size(), actually governed anything) are
    // deliberately made to disagree.
    ThreadPool pool(3);

    video::Raster444 destA(destW, destH);
    runOnce(w, destW, destH, /*threads=*/99, &pool, destA);
    checkMatches(destA, reference);

    video::Raster444 destB(destW, destH);
    runOnce(w, destW, destH, /*threads=*/1, &pool, destB);
    checkMatches(destB, reference);
}

// ---------------------------------------------------------------------------
// 3. A persistent pool reused across *different* frame geometries in
//    sequence -- proving no per-frame-geometry state leaks from one
//    runFrame() call into the next by way of the shared pool. Every
//    per-frame arena (workerBins, TileAccum, tileCells) is freshly
//    allocated inside runThreaded() on each call (core/pipeline.cpp), not
//    cached on ThreadPool itself, so this is expected to simply work --
//    checked directly rather than only reasoned through, the same
//    "empirically, not just by inspection" standard this project applied
//    to its own first concurrent code (WU-16a's ThreadSanitizer run,
//    CORRECTIONS.md C-011/C-012's general lesson).
// ---------------------------------------------------------------------------
void test_persistent_pool_reused_across_different_geometries() {
    ThreadPool pool(4);

    const WarpedFrame w1 = buildWarpedTestFrame(180, 200, 150, 0.9);
    video::Raster444 ref1(200, 150);
    runOnce(w1, 200, 150, 1, nullptr, ref1);
    CHECK(hasNonBackground(ref1));
    video::Raster444 dest1(200, 150);
    runOnce(w1, 200, 150, 4, &pool, dest1);
    checkMatches(dest1, ref1);

    // Second geometry: different source size, different destination
    // extent (larger, and not a multiple of either candidate tile size),
    // different warp strength -- run through the *same* pool object
    // immediately afterward.
    const WarpedFrame w2 = buildWarpedTestFrame(311, 401, 233, 2.4);
    video::Raster444 ref2(401, 233);
    runOnce(w2, 401, 233, 1, nullptr, ref2);
    CHECK(hasNonBackground(ref2));
    video::Raster444 dest2(401, 233);
    runOnce(w2, 401, 233, 4, &pool, dest2);
    checkMatches(dest2, ref2);

    // Back to the first geometry once more, same pool, to confirm the
    // second call didn't leave the pool (or anything it touched) in a
    // state that corrupts a subsequent, differently-shaped-again call.
    video::Raster444 dest1Again(200, 150);
    runOnce(w1, 200, 150, 4, &pool, dest1Again);
    checkMatches(dest1Again, ref1);
}

}  // namespace

int main() {
    test_persistent_pool_matches_single_threaded_across_many_calls();
    test_persistent_pool_size_governs_partition_not_threads_field();
    test_persistent_pool_reused_across_different_geometries();
    return scatter::test::summary("test_persistent_pool");
}
