// scatter-dve — WU-10: single-frame pipeline orchestration; WU-16 adds
// per-tile threading to PASS 2. Entry points declared in core/resolve.hpp
// (see that file and DECISIONS.md ADR-026 for why this unit has no
// pipeline.hpp of its own for runFrame()/runFrameFile() themselves); the
// thread pool WU-16 adds is declared in core/pipeline.hpp instead (see
// that header and DECISIONS.md ADR-040).
//
// This is architecture.md section 3's signal path, assembled end to end
// for the first time: v210 unpack -> chroma upsample -> PASS 1 (lattice
// eval, Jacobian, fragment generation and tile binning -- WU-06/07/08) ->
// PASS 2 (four-bank splat and bank-resolve, WU-09; normalise and
// composite, WU-10) -> chroma downsample -> v210 pack. runFrame() covers
// PASS 1 and PASS 2 over already-4:4:4 rasters; runFrameFile() adds the
// v210/chroma stages either side of it.
//
// WU-16 (ADR-040): PASS 1 (generateFragments()) is unchanged and still
// runs single-threaded, always -- see core/pipeline.hpp's own file
// comment for why. PASS 2 -- the per-tile splat/resolve/normalise/
// composite loop below -- runs on ThreadPool when
// PipelineParams::threads > 1, one worker per available thread, tiles
// partitioned across them by a fixed, static interleaving
// (tileIndex % threads). Both the single-threaded and threaded paths call
// the same resolveOneTile() below, so they cannot silently diverge from
// each other; the single-threaded path additionally never touches
// ThreadPool, core/pipeline.hpp's synchronisation primitives, or
// setWorkerQoS() at all, so it stays exactly what ADR-015 already needs
// it to be: an independently simple oracle, provably correct on its own
// terms, that a threading bug in the new machinery cannot silently
// corrupt. I6 (integer addition is associative) is why any partition of
// tiles across any number of workers, in any completion order, still
// writes every destination pixel exactly once with exactly the same
// value the single-threaded path would -- no per-tile result depends on
// any other tile's, and normalise/composite's own arithmetic
// (core/resolve.cpp) is unchanged, called identically either way.
#include "core/resolve.hpp"

#include "core/pipeline.hpp"
#include "core/splat.hpp"
#include "video/chroma.hpp"
#include "video/v210.hpp"

#include <algorithm>
#include <cstddef>
#include <vector>

#if defined(__APPLE__)
#include <pthread.h>
#include <pthread/qos.h>
#endif

namespace scatter {

// ---------------------------------------------------------------------------
// ThreadPool (core/pipeline.hpp) -- WU-16.
// ---------------------------------------------------------------------------

void setWorkerQoS() noexcept {
#if defined(__APPLE__)
    // architecture.md section 6, "Apple Silicon gotcha", quoted verbatim
    // in core/pipeline.hpp's own comment. UNVERIFIED by this session: no
    // AppleClang/Xcode toolchain exists in the Linux cloud sandbox this
    // unit was implemented in (the same gap ADR-031/032 already flagged
    // for WU-14/WU-15a's own Apple-only surfaces) -- needs building and
    // running at the real terminal before this call can be confirmed to
    // even compile, let alone schedule correctly. See DECISIONS.md
    // ADR-040 and HANDOFF.md.
    pthread_set_qos_class_self_np(QOS_CLASS_USER_INTERACTIVE, 0);
#endif
}

ThreadPool::ThreadPool(int numThreads) : numThreads_(numThreads) {
    workers_.reserve(std::size_t(numThreads_));
    for (int i = 0; i < numThreads_; ++i) {
        workers_.emplace_back([this, i] { workerLoop(i); });
    }
}

ThreadPool::~ThreadPool() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        shutdown_ = true;
    }
    cvDispatch_.notify_all();
    for (std::thread& t : workers_) {
        t.join();
    }
}

void ThreadPool::workerLoop(int workerIndex) {
    setWorkerQoS();
    unsigned myGen = 0;
    for (;;) {
        std::function<void(int)> task;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            cvDispatch_.wait(lock, [this, myGen] {
                return shutdown_ || generation_ != myGen;
            });
            if (shutdown_) {
                return;
            }
            myGen = generation_;
            task = task_;
        }
        task(workerIndex);
        {
            std::lock_guard<std::mutex> lock(mutex_);
            --pending_;
            if (pending_ == 0) {
                cvDone_.notify_one();
            }
        }
    }
}

void ThreadPool::runOnAll(const std::function<void(int)>& fn) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        task_ = fn;
        pending_ = numThreads_;
        ++generation_;
    }
    cvDispatch_.notify_all();
    std::unique_lock<std::mutex> lock(mutex_);
    cvDone_.wait(lock, [this] { return pending_ == 0; });
}

// ---------------------------------------------------------------------------
// runFrame() / runFrameFile() -- WU-10, threaded PASS 2 added at WU-16.
// ---------------------------------------------------------------------------

namespace {

// One tile's worth of PASS 2: bank-resolve (WU-09's sumBanks(), reading
// `accum`/`tileCells` as reusable scratch -- see core/splat.hpp's own
// TileAccum::clear() comment, which already anticipated exactly this
// reuse), then normalise and composite every covered cell into `dest`
// (WU-10's own composite(), core/resolve.hpp, unchanged). `accum` is
// cleared here, unconditionally, before every tile -- equivalent to
// WU-10's original per-tile `TileAccum accum;` (fresh construction
// zero-initialises identically, per TileAccum's own constructor; see
// DECISIONS.md ADR-040) but reusable across many tiles without
// reallocating either scratch buffer, which is what makes this safe and
// cheap to call once per tile from a single persistent per-worker arena
// in WU-16's own threaded path below.
void resolveOneTile(const TileBins& bins, const PipelineParams& params,
                     video::Raster444& dest, TileAccum& accum,
                     std::vector<AccumCell>& tileCells, int tx, int ty) {
    accum.clear();
    splatTile(bins.tile(tx, ty), accum);
    sumBanks(accum, tileCells.data());

    const int originX = tx * kTileSize;
    const int originY = ty * kTileSize;
    // A tile at the destination raster's own right/bottom edge can extend
    // past destWidth/destHeight (TileBins sizes tilesX_/tilesY_ by
    // tileCount()'s ceiling division, core/binner.hpp); cells beyond the
    // raster have no destination pixel to write and are simply not
    // visited, the same "no fragment lost or duplicated, nothing
    // fabricated past the edge" discipline ADR-024's off-raster-drop
    // choice already uses one stage earlier.
    const int localWidth = std::min(kTileSize, params.destWidth - originX);
    const int localHeight = std::min(kTileSize, params.destHeight - originY);

    for (int ly = 0; ly < localHeight; ++ly) {
        for (int lx = 0; lx < localWidth; ++lx) {
            const AccumCell& cell =
                tileCells[std::size_t(ly) * std::size_t(kTileSize) + std::size_t(lx)];
            const CompositedCell out = composite(cell, params.background);

            const int dx = originX + lx;
            const int dy = originY + ly;
            const std::size_t idx =
                std::size_t(dy) * std::size_t(dest.width) + std::size_t(dx);
            dest.Y[idx] = out.Y;
            dest.Cb[idx] = out.Cb;
            dest.Cr[idx] = out.Cr;
        }
    }
}

}  // namespace

void runFrame(const Lattice& lattice, const SourceRaster& src,
              const PipelineParams& params, video::Raster444& dest) {
    TileBins bins(params.destWidth, params.destHeight);
    generateFragments(lattice, src, params.maxK, params.supersample,
                       params.tag, bins);

    const int tilesX = bins.tilesX();
    const int tilesY = bins.tilesY();
    const int totalTiles = tilesX * tilesY;
    const std::size_t tilePixelsN = std::size_t(kTilePixels);

    if (params.threads <= 1) {
        // The determinism oracle (ADR-015, I6): a plain, single-threaded
        // loop with no dependency on ThreadPool, its synchronisation
        // primitives, or setWorkerQoS() at all -- see this file's own
        // header comment and DECISIONS.md ADR-040. One scratch TileAccum/
        // tileCells pair, reused (cleared) across every tile in turn,
        // exactly the loop WU-10 originally wrote.
        TileAccum accum;
        std::vector<AccumCell> tileCells(tilePixelsN);
        for (int ty = 0; ty < tilesY; ++ty) {
            for (int tx = 0; tx < tilesX; ++tx) {
                resolveOneTile(bins, params, dest, accum, tileCells, tx, ty);
            }
        }
        return;
    }

    // WU-16 (ADR-040): PASS 2 only, tile-parallel. Each worker owns one
    // persistent TileAccum + tileCells pair -- its own "per-worker bin
    // arena" in this unit's own scope -- reused (cleared, never
    // reallocated) across every tile a fixed, static interleaving
    // (tileIndex % threads) assigns it. Tiles are read-only during this
    // phase (PASS 1 above has already fully populated `bins` on this same
    // calling thread, with a happens-before edge to every worker's own
    // first read via ThreadPool's own dispatch synchronisation), and each
    // tile owns a disjoint block of `dest`'s pixels, so no two workers
    // ever read or write the same memory. See I6: the result is
    // bit-identical to the threads<=1 path above regardless of how many
    // workers ran, or in which order any of them finished, because
    // resolveOneTile() is the same function either way and every tile's
    // own contribution to `dest` does not depend on any other tile's.
    ThreadPool pool(params.threads);
    std::vector<TileAccum> arenas(std::size_t(pool.size()));
    std::vector<std::vector<AccumCell>> cellBufs(
        std::size_t(pool.size()), std::vector<AccumCell>(tilePixelsN));

    pool.runOnAll([&](int worker) {
        TileAccum& accum = arenas[std::size_t(worker)];
        std::vector<AccumCell>& tileCells = cellBufs[std::size_t(worker)];
        for (int idx = worker; idx < totalTiles; idx += pool.size()) {
            const int tx = idx % tilesX;
            const int ty = idx / tilesX;
            resolveOneTile(bins, params, dest, accum, tileCells, tx, ty);
        }
    });
}

bool runFrameFile(const Lattice& lattice, const std::string& srcPath,
                   int srcWidth, int srcHeight, const PipelineParams& params,
                   const std::string& dstPath) {
    video::Raster422 in(srcWidth, srcHeight);
    if (!video::readV210File(srcPath, srcWidth, srcHeight, in.planeY(),
                              in.planeCb(), in.planeCr())) {
        return false;
    }

    // Chroma upsample 4:2:2 -> 4:4:4 (ADR-005); luma is never touched by
    // it and passes straight through, same as tests/test_ramp_roundtrip.cpp's
    // own identity chain.
    video::Raster444 full(srcWidth, srcHeight);
    std::copy(in.Y.begin(), in.Y.end(), full.Y.begin());
    chroma::upsampleImage(in.Cb.data(), in.planeCb().strideSamples,
                           srcWidth, srcHeight,
                           full.Cb.data(), full.planeCb().strideSamples);
    chroma::upsampleImage(in.Cr.data(), in.planeCr().strideSamples,
                           srcWidth, srcHeight,
                           full.Cr.data(), full.planeCr().strideSamples);

    SourceRaster src;
    src.width = srcWidth;
    src.height = srcHeight;
    src.y = full.Y.data();
    src.cb = full.Cb.data();
    src.cr = full.Cr.data();

    video::Raster444 warped(params.destWidth, params.destHeight);
    runFrame(lattice, src, params, warped);

    // Chroma downsample 4:4:4 -> 4:2:2 (ADR-005) before pack; luma again
    // passes straight through.
    video::Raster422 out(params.destWidth, params.destHeight);
    std::copy(warped.Y.begin(), warped.Y.end(), out.Y.begin());
    chroma::downsampleImage(warped.Cb.data(), warped.planeCb().strideSamples,
                             params.destWidth, params.destHeight,
                             out.Cb.data(), out.planeCb().strideSamples);
    chroma::downsampleImage(warped.Cr.data(), warped.planeCr().strideSamples,
                             params.destWidth, params.destHeight,
                             out.Cr.data(), out.planeCr().strideSamples);

    return video::writeV210File(dstPath, params.destWidth, params.destHeight,
                                 out.planeY(), out.planeCb(), out.planeCr());
}

}  // namespace scatter
