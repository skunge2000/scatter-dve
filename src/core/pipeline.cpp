// scatter-dve — WU-10: single-frame pipeline orchestration; WU-16a added
// per-tile threading to PASS 2; WU-16b adds row-band threading to PASS 1.
// Entry points declared in core/resolve.hpp (see that file and
// DECISIONS.md ADR-026 for why this unit has no pipeline.hpp of its own
// for runFrame()/runFrameFile() themselves); the thread pool WU-16a adds
// is declared in core/pipeline.hpp instead (see that header and
// DECISIONS.md ADR-040).
//
// This is architecture.md section 3's signal path, assembled end to end
// for the first time: v210 unpack -> chroma upsample -> PASS 1 (lattice
// eval, Jacobian, fragment generation and tile binning -- WU-06/07/08) ->
// PASS 2 (four-bank splat and bank-resolve, WU-09; normalise and
// composite, WU-10) -> chroma downsample -> v210 pack. runFrame() covers
// PASS 1 and PASS 2 over already-4:4:4 rasters; runFrameFile() adds the
// v210/chroma stages either side of it.
//
// WU-40 (DECISIONS.md ADR-085) adds a new RGB boundary conversion
// immediately adjacent to chroma upsample/downsample, on both sides, in
// every function below that owns that middle sequence
// (runFrameBytes()/runFrameBytesDeinterlaced()/runFrameFile()): chroma
// upsample -> chroma::ycbcrToRgbImage() -> chroma::rgbToYcbcrImage() ->
// [PASS 1/2 as before, still reading/writing YCbCr-labelled Raster444
// planes -- SourceRaster/sampleBilinear() are not RGB-native yet, that is
// WU-41's own job] -> chroma::ycbcrToRgbImage() -> chroma::rgbToYcbcrImage()
// -> chroma downsample. The round trip back to YCbCr immediately after each
// conversion is deliberate, not a no-op left in by mistake: PASS 1/2 still
// expect Raster444's own Y/Cb/Cr planes on both sides (that does not change
// until WU-41/WU-42 land), so this unit's own job is limited to proving the
// new conversion is wired in and exercised for real at the actual boundary,
// not to reshaping what runs between the two conversions -- see WORK-UNITS.md's
// WU-40 entry and HANDOFF.md for which existing tests this was found to
// affect (the round trip is not bit-exact in general, per I2 -- see
// video/chroma.hpp's own comment on the new functions).
//
// WU-16a (ADR-040) added ThreadPool and threaded PASS 2 alone, deferring
// PASS 1's own row-band parallelism (architecture.md section 6's fuller
// two-pass design) as WU-16b, deliberately, because it needed a
// row-range-aware core/binner.hpp/.cpp entry point that unit's own file
// budget had no room for. WU-16b (ADR-041) is that entry point
// (generateFragmentsRowRange(), core/binner.hpp) plus the row-band
// dispatch and per-worker-arena merge below: when PipelineParams::threads
// > 1, PASS 1 now partitions src's rows into params.threads contiguous
// bands (architecture.md 6's own "Pass 1 partitions the source by row
// bands"), one worker per band, each writing into its own whole-frame
// TileBins -- a private "generation-time bin arena" -- via
// generateFragmentsRowRange(); a second ThreadPool::runOnAll() call is
// architecture.md 6's own "barrier" for free (core/pipeline.hpp's own doc
// comment already named this exact use); PASS 2 then partitions by tile
// exactly as WU-16a already did, except resolveOneTile() below now reads
// every worker's own arena for a given tile, in fixed worker order,
// instead of one shared TileBins -- architecture.md 6's own "each worker
// reads all 8 workers' lists for its tiles, in fixed worker order". The
// threads<=1 path is completely unchanged in its own arithmetic (see
// resolveOneTile()'s own comment below) and still never touches
// ThreadPool, core/pipeline.hpp's synchronisation primitives, or
// setWorkerQoS() at all, so it stays exactly what ADR-015 needs it to be:
// an independently simple oracle, provably correct on its own terms, that
// a threading bug in the new machinery -- now spanning both passes, not
// just one -- cannot silently corrupt. I6 (integer addition is
// associative) is why any partition of rows into bands, any partition of
// tiles across workers, and any completion order, still write every
// destination pixel exactly once with exactly the same value the
// threads<=1 path would: splatTile() (core/splat.cpp, WU-09) accumulates
// into an already-cleared TileAccum regardless of how many sources it is
// called against or in what order, and normalise/composite's own
// arithmetic (core/resolve.cpp) is unchanged, called identically in every
// case. See DECISIONS.md ADR-041.
//
// WU-19a (ADR-044) completes ADR-040's own deferred "persistent,
// caller-owned ThreadPool" item: PipelineParams::pool (core/resolve.hpp),
// when non-null, lets a caller construct one ThreadPool once and reuse it
// across many runFrame() calls instead of paying WU-16a/16b's own
// per-call spawn/join cost on every one of them. The row-band/tile
// dispatch itself -- everything from "per-worker generation-time bin
// arena" through the final resolveOneTile() loop -- did not need to
// change to support this: it was already written entirely in terms of a
// ThreadPool& and how many workers to partition across (pool.size()),
// never in terms of who constructed that pool or how long it will go on
// existing afterward. That whole body is factored out below into
// runThreaded(), called with either a freshly-constructed, this-call-only
// ThreadPool (params.pool == nullptr, WU-16a/16b's own unchanged
// behaviour) or the caller's own externally-owned one (params.pool !=
// nullptr, new this unit) -- see runFrame()'s own three-way branch below,
// and DECISIONS.md ADR-044 for the full design and for why this is scoped
// to the lifecycle change alone, not to any throughput measurement (which
// this project's Linux cloud sandbox cannot meaningfully produce for the
// real target, the M1 Max).
//
// WU-22a (Phase 5, DECISIONS.md ADR-056) adds PipelineParams::weightOut
// (core/resolve.hpp): an optional, caller-owned full-frame buffer that
// resolveOneTile() below writes each destination cell's raw AccumCell::w
// into, alongside its existing composite() call -- the diagnostic coverage
// view's (WU-22b, src/diag/, not built this session) own data source.
// Default nullptr, checked once per destination cell inside the existing
// per-tile loop; every branch above (threads<=1, params.pool != nullptr,
// the per-call ThreadPool) already funnels through this one
// resolveOneTile() function, so nothing else in this file changes.
//
// WU-28b (Phase 7, DECISIONS.md ADR-059, ADR-061) adds
// PipelineParams::kBufferMode (core/resolve.hpp): resolveOneTile() below now
// branches on it, splatting into and resolving from WU-28a's own
// TileKBufferAccum/KSlot storage via compositeKBuffer() instead of the
// plain TileAccum/AccumCell/composite() path, when set. Default Off leaves
// the plain path unchanged and never constructs WU-28a's storage -- every
// branch above still funnels through the one resolveOneTile(), now with two
// additional, nullptr-when-off scratch parameters mirroring accum/
// tileCells' own shape, extending I6 through this unit's new code.
//
// WU-23a2b (Phase 6, DECISIONS.md ADR-077) adds runFrameField() (core/
// resolve.hpp), field mode's own runFrame()-level driver: PASS 1 via
// core/binner.hpp's generateFragmentsFieldRows() (WU-23a2a, ADR-076) in
// place of generateFragments(), run once per parity, each resolved through
// this file's own resolveOneTile() exactly as runFrame()'s threads<=1
// branch already does (single-threaded only, this unit -- see
// runFrameField()'s own comment below); video/interlace.hpp's
// extractField()/interleaveFields() (WU-23a, ADR-075) then decimate and
// recombine the two full-resolution per-parity results into one interlaced
// frame. Reuses resolveOneTile() unchanged -- no new PASS-2 arithmetic --
// the same "share the one tile-resolve function, do not hand-duplicate it"
// discipline WU-16a's own file comment already established for its own two
// paths, now a fourth caller.
#include "core/resolve.hpp"

#include "core/pipeline.hpp"
#include "core/splat.hpp"
#include "video/chroma.hpp"
#include "video/interlace.hpp"
#include "video/v210.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
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
// in the threaded paths below.
//
// `sources` is every PASS-1 arena this tile's own bin should be read
// from, splatted into the same cleared `accum` in the order given
// (architecture.md 6's own "in fixed worker order") -- WU-16a's own
// single-source threads<=1 and threads>1-PASS-2-only paths both passed
// exactly one TileBins here; WU-16b's own row-band-parallel PASS 1
// (below) passes one per worker instead, since a row band's own
// fragments can land in any tile regardless of which band generated
// them. splatTile() does not clear `accum` itself (core/splat.hpp's own
// contract) and integer addition is associative (I6), so summing however
// many sources into one cleared accum here is exact regardless of their
// count or order -- one source (the threads<=1 case) is not a special
// case of this loop, just the N == 1 instance of it, which is what lets
// every path in this file keep sharing one resolveOneTile() instead of
// hand-duplicating its own copy per path, the same "cannot silently
// diverge" property WU-16a's own file comment already relied on for its
// two paths, now extended to three.
void resolveOneTile(std::span<const TileBins* const> sources,
                     const PipelineParams& params, video::Raster444& dest,
                     TileAccum& accum, std::vector<AccumCell>& tileCells,
                     TileKBufferAccum* kAccum,
                     std::vector<std::array<KSlot, kBufferK>>* tileKCells,
                     int tx, int ty) {
    const int originX = tx * kTileSize;
    const int originY = ty * kTileSize;
    // A tile at the destination raster's own right/bottom edge can extend
    // past destWidth/destHeight (TileBins sizes tilesX_/tilesY_ by
    // tileCount()'s ceiling division, core/binner.hpp); cells beyond the
    // raster have no destination pixel to write and are simply not
    // visited, the same "no fragment lost or duplicated, nothing
    // fabricated past the edge" discipline ADR-024's off-raster-drop
    // choice already uses one stage earlier. Shared by both branches below.
    const int localWidth = std::min(kTileSize, params.destWidth - originX);
    const int localHeight = std::min(kTileSize, params.destHeight - originY);

    // WU-28b (DECISIONS.md ADR-059, ADR-061): the k-buffer resolve path --
    // splatTileKBuffer()/sumBanksKBuffer() in place of splatTile()/
    // sumBanks(), compositeKBuffer() in place of composite() below.
    // kAccum/tileKCells are guaranteed non-null here -- every caller only
    // constructs them when params.kBufferMode != Off, mirroring accum/
    // tileCells' own unconditional construction, so the plain path pays
    // nothing extra when off.
    if (params.kBufferMode != KBufferResolveMode::Off) {
        kAccum->clear();
        for (const TileBins* bins : sources) {
            splatTileKBuffer(bins->tile(tx, ty), *kAccum);
        }
        sumBanksKBuffer(*kAccum, tileKCells->data());

        for (int ly = 0; ly < localHeight; ++ly) {
            for (int lx = 0; lx < localWidth; ++lx) {
                const std::array<KSlot, kBufferK>& slots =
                    (*tileKCells)[std::size_t(ly) * std::size_t(kTileSize) +
                                  std::size_t(lx)];
                const CompositedCell out =
                    compositeKBuffer(slots, params.kBufferMode, params.background);

                const int dx = originX + lx;
                const int dy = originY + ly;
                const std::size_t idx =
                    std::size_t(dy) * std::size_t(dest.width) + std::size_t(dx);
                dest.Y[idx] = out.Y;
                dest.Cb[idx] = out.Cb;
                dest.Cr[idx] = out.Cr;

                // WU-22a's weightOut is deliberately not written along
                // this path -- see PipelineParams::kBufferMode's own doc
                // comment (core/resolve.hpp).
            }
        }
        return;
    }

    accum.clear();
    for (const TileBins* bins : sources) {
        splatTile(bins->tile(tx, ty), accum);
    }
    sumBanks(accum, tileCells.data());

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

            // WU-22a (DECISIONS.md ADR-056): opt-in side-channel capture of
            // the same cell's raw AccumCell::w that composite() above just
            // read to build `out` -- see PipelineParams::weightOut's own
            // doc comment (core/resolve.hpp) for why this is a pointer
            // field on params rather than a second output raster threaded
            // through every runFrame()/runFrameBytes()/runFrameFile()
            // signature. Indexed by params.destWidth, not dest.width --
            // the two are always equal by runFrame()'s own precondition
            // (core/resolve.hpp's runFrame() doc comment: "the caller
            // sizes [dest] to params.destWidth x params.destHeight"), but
            // weightOut's own buffer is sized and documented in terms of
            // params.destWidth specifically, so this reads that field
            // rather than relying on the equality holding silently.
            if (params.weightOut != nullptr) {
                params.weightOut[std::size_t(dy) * std::size_t(params.destWidth) +
                                  std::size_t(dx)] = cell.w;
            }
        }
    }
}

// WU-19a (ADR-044): the threaded PASS-1/PASS-2 body, factored out of
// runFrame() unchanged -- byte-for-byte the same statements WU-16b's own
// threads>1 branch already ran -- so it can run against either a
// ThreadPool this call constructs and joins itself (WU-16a/16b's own
// per-call behaviour, still exactly what happens whenever
// PipelineParams::pool is left at its default nullptr) or one the caller
// already owns and is reusing across many runFrame() calls
// (PipelineParams::pool, new this unit). Nothing here reads how `pool`
// came to exist or how long it will go on existing afterward -- only
// `pool.size()` and `pool.runOnAll()`, both already part of
// core/pipeline.hpp's own public ThreadPool interface, unchanged by this
// unit.
void runThreaded(const Lattice& lattice, const SourceRaster& src,
                  const PipelineParams& params, video::Raster444& dest,
                  ThreadPool& pool, int tilesX, int totalTiles,
                  std::size_t tilePixelsN) {
    const int numWorkers = pool.size();
    // Bound to a named std::size_t first: `std::vector<T> v(std::size_t(
    // numWorkers))` with a plain identifier inside the inner parentheses
    // parses as a function declaration (-Wvexing-parse, -Werror under
    // ADR-017) -- the same pitfall tests/test_threading.cpp's own
    // callCount/seenThisRound already document and work around.
    const std::size_t numWorkersN = std::size_t(numWorkers);

    // Per-worker "generation-time bin arena": one whole-frame TileBins per
    // worker, not a partial one -- a row band's own fragments can land in
    // any tile depending on the warp, so there is no way to know in
    // advance which tiles a given band will touch. All numWorkers arenas
    // are fully constructed here, serially, before any worker thread
    // starts (no reallocation happens once PASS 1 dispatch begins below),
    // the same "preallocate before the hot parallel section" shape
    // WU-16a's own per-worker TileAccum/tileCells arenas already used one
    // stage later. Freshly allocated on every runThreaded() call, whether
    // `pool` is this call's own local one or a caller's persistent one --
    // WU-19a reuses the *pool*, not any per-frame arena, so a pool reused
    // across differently-sized destination rasters or differently-shaped
    // sources needs nothing special here: each call sizes its own arenas
    // for its own params.destWidth/destHeight, same as WU-16a/16b already
    // did per call.
    std::vector<TileBins> workerBins;
    workerBins.reserve(std::size_t(numWorkers));
    for (int i = 0; i < numWorkers; ++i) {
        workerBins.emplace_back(params.destWidth, params.destHeight);
    }

    // PASS 1: contiguous row bands (architecture.md 6's own "partitions
    // the source by row bands", as distinct from PASS 2's interleaved
    // tileIndex % numWorkers below -- see DECISIONS.md ADR-041 for why the
    // two passes partition differently). rowStart/rowEnd via
    // (worker * src.height) / numWorkers bounds every band exactly once,
    // covers [0, src.height) exactly, and degrades harmlessly to an empty
    // band (rowStart == rowEnd, generateFragmentsRowRange() emits nothing)
    // whenever numWorkers exceeds src.height -- the same "some workers get
    // zero work, harmlessly" property WU-16a's own PASS-2 tile partition
    // already relies on when numWorkers exceeds the tile count. Each
    // worker writes only into its own workerBins[worker] -- no shared
    // mutable state between workers during this phase, so no locking is
    // needed (lattice and src are read-only throughout, same as WU-16a's
    // own PASS 2 read of a single shared TileBins).
    pool.runOnAll([&](int worker) {
        const int rowStart = (worker * src.height) / numWorkers;
        const int rowEnd = ((worker + 1) * src.height) / numWorkers;
        generateFragmentsRowRange(lattice, src, params.maxK, params.supersample,
                                   params.tag, rowStart, rowEnd,
                                   workerBins[std::size_t(worker)]);
    });

    // Barrier: this is architecture.md 6's own barrier between pass 1 and
    // pass 2, free per core/pipeline.hpp's own ThreadPool doc comment --
    // runOnAll()'s second call below is never dispatched to any worker
    // until the first has fully drained on this calling thread, so every
    // workerBins[i] is completely populated, with a happens-before edge to
    // every PASS-2 worker's own first read of it, before PASS 2 starts.
    // This holds exactly the same way whether `pool` is a fresh,
    // this-call-only ThreadPool or a caller's persistent one -- the
    // barrier is a property of two runOnAll() calls on the same pool
    // instance, not of the pool's own age.

    std::vector<const TileBins*> allArenas(numWorkersN);
    for (int i = 0; i < numWorkers; ++i) {
        allArenas[std::size_t(i)] = &workerBins[std::size_t(i)];
    }

    // PASS 2: tile-parallel, exactly WU-16a's own static interleaved
    // partition (tileIndex % numWorkers) -- unchanged from ADR-040. Each
    // worker owns one persistent TileAccum + tileCells pair, its own
    // "per-worker bin arena" in WU-16a's sense, reused (cleared, never
    // reallocated) across every tile it is assigned. The only change from
    // WU-16a: resolveOneTile() now reads every worker's own PASS-1 arena
    // for a given tile (allArenas, fixed order 0..numWorkers-1 --
    // architecture.md 6's own "in fixed worker order") instead of one
    // shared TileBins, splatting all of them into the same cleared accum
    // before bank-resolving. Tiles are read-only during this phase (PASS 1
    // above has already fully returned on this same calling thread, with
    // the happens-before edge the barrier note above describes), and each
    // tile owns a disjoint block of `dest`'s pixels, so no two workers
    // ever read or write the same memory. See I6: the result is
    // bit-identical to the threads<=1 path regardless of how many row
    // bands or tile-workers ran, in which order any of them finished, or
    // whether `pool` itself was constructed for this call alone or is
    // being reused across many -- splatTile() accumulates into an
    // already-cleared accum regardless of source count or order, and no
    // per-tile result depends on any other tile's.
    std::vector<TileAccum> arenas(numWorkersN);
    std::vector<std::vector<AccumCell>> cellBufs(
        numWorkersN, std::vector<AccumCell>(tilePixelsN));

    // WU-28b: per-worker k-buffer scratch, mirroring arenas/cellBufs above
    // -- allocated only when params.kBufferMode != Off, so the plain-mode
    // path pays nothing extra and every threading path keeps funnelling
    // through the one resolveOneTile() (WU-16a's own "cannot silently
    // diverge" property).
    std::vector<std::optional<TileKBufferAccum>> kArenas;
    std::vector<std::vector<std::array<KSlot, kBufferK>>> kCellBufs;
    if (params.kBufferMode != KBufferResolveMode::Off) {
        kArenas.resize(numWorkersN);
        for (auto& a : kArenas) {
            a.emplace();
        }
        kCellBufs.assign(numWorkersN,
                          std::vector<std::array<KSlot, kBufferK>>(tilePixelsN));
    }

    pool.runOnAll([&](int worker) {
        TileAccum& accum = arenas[std::size_t(worker)];
        std::vector<AccumCell>& tileCells = cellBufs[std::size_t(worker)];
        TileKBufferAccum* kAccum = nullptr;
        std::vector<std::array<KSlot, kBufferK>>* tileKCells = nullptr;
        if (params.kBufferMode != KBufferResolveMode::Off) {
            kAccum = &*kArenas[std::size_t(worker)];
            tileKCells = &kCellBufs[std::size_t(worker)];
        }
        for (int idx = worker; idx < totalTiles; idx += numWorkers) {
            const int tx = idx % tilesX;
            const int ty = idx / tilesX;
            resolveOneTile(allArenas, params, dest, accum, tileCells, kAccum,
                            tileKCells, tx, ty);
        }
    });
}

}  // namespace

void runFrame(const Lattice& lattice, const SourceRaster& src,
              const PipelineParams& params, video::Raster444& dest) {
    // tileCount() (core/binner.hpp) is the exact function TileBins's own
    // constructor already calls internally to derive tilesX_/tilesY_ --
    // computing it here directly, rather than via a shared TileBins'
    // tilesX()/tilesY() the way WU-16a's own code did, is behaviour-
    // preserving (same function, same arguments) and lets every branch
    // below share it.
    const int tilesX = tileCount(params.destWidth);
    const int tilesY = tileCount(params.destHeight);
    const int totalTiles = tilesX * tilesY;
    const std::size_t tilePixelsN = std::size_t(kTilePixels);

    if (params.pool == nullptr && params.threads <= 1) {
        // The determinism oracle (ADR-015, I6): a plain, single-threaded
        // loop with no dependency on ThreadPool, its synchronisation
        // primitives, or setWorkerQoS() at all -- see this file's own
        // header comment and DECISIONS.md ADR-040/ADR-041. PASS 1 runs
        // once, synchronously, over the whole raster -- exactly WU-08's
        // own generateFragments(), unchanged -- into one TileBins; one
        // scratch TileAccum/tileCells pair, reused (cleared) across every
        // tile in turn, exactly the loop WU-10 originally wrote.
        // resolveOneTile() below takes a *list* of PASS-1 sources
        // (WU-16b); wrapping this single TileBins in a one-element span
        // is arithmetically identical to WU-16a's own direct call --
        // splatTile() is still called exactly once per tile, against the
        // same tile bin, in the same order. Unaffected by WU-19a: a
        // caller must supply a non-null pool to leave this branch at all
        // (see below), so a default-constructed PipelineParams -- pool
        // still nullptr -- lands here exactly as it always has.
        TileBins bins(params.destWidth, params.destHeight);
        generateFragments(lattice, src, params.maxK, params.supersample,
                           params.tag, bins);

        TileAccum accum;
        std::vector<AccumCell> tileCells(tilePixelsN);

        // WU-28b: k-buffer scratch, only constructed when
        // params.kBufferMode != Off -- see runThreaded()'s own matching
        // comment above for why this keeps the plain path's own oracle loop
        // at zero extra cost when the mode is left at its default.
        std::optional<TileKBufferAccum> kAccumOpt;
        std::vector<std::array<KSlot, kBufferK>> tileKCells;
        TileKBufferAccum* kAccum = nullptr;
        std::vector<std::array<KSlot, kBufferK>>* tileKCellsPtr = nullptr;
        if (params.kBufferMode != KBufferResolveMode::Off) {
            kAccumOpt.emplace();
            tileKCells.resize(tilePixelsN);
            kAccum = &*kAccumOpt;
            tileKCellsPtr = &tileKCells;
        }

        const std::array<const TileBins*, 1> soloSource{&bins};
        for (int ty = 0; ty < tilesY; ++ty) {
            for (int tx = 0; tx < tilesX; ++tx) {
                resolveOneTile(soloSource, params, dest, accum, tileCells, kAccum,
                                tileKCellsPtr, tx, ty);
            }
        }
        return;
    }

    if (params.pool != nullptr) {
        // WU-19a (ADR-044): the caller already owns a ThreadPool and is
        // reusing it across (presumably) many runFrame() calls -- no
        // construct/join overhead this call. pool->size() alone decides
        // the worker count actually used; params.threads is deliberately
        // not consulted anywhere in this branch (see PipelineParams::pool's
        // own doc comment in core/resolve.hpp for why, and
        // tests/test_persistent_pool.cpp for the check that a mismatched
        // `threads` value cannot silently change this branch's own
        // output).
        runThreaded(lattice, src, params, dest, *params.pool, tilesX,
                    totalTiles, tilePixelsN);
        return;
    }

    // WU-16a/16b's own per-call ThreadPool, unchanged: params.pool is
    // nullptr and params.threads > 1 (the only remaining case, the first
    // branch above having already handled pool == nullptr && threads <=
    // 1), so a fresh pool of exactly params.threads workers is
    // constructed for the duration of this one call and joined
    // (ThreadPool's own destructor) before returning.
    ThreadPool localPool(params.threads);
    runThreaded(lattice, src, params, dest, localPool, tilesX, totalTiles,
                tilePixelsN);
}

// ---------------------------------------------------------------------------
// runFrameField() -- WU-23a2b (DECISIONS.md ADR-077).
// ---------------------------------------------------------------------------

void runFrameField(const Lattice& lattice, const SourceRaster& src,
                    const PipelineParams& params, video::Raster444& dest) {
    const int tilesX = tileCount(params.destWidth);
    const int tilesY = tileCount(params.destHeight);
    const std::size_t tilePixelsN = std::size_t(kTilePixels);

    // One field parity's own PASS 1 (generateFragmentsFieldRows(),
    // WU-23a2a -- rowOffset 0 selects source rows 0, 2, 4, ..., 1 selects
    // 1, 3, 5, ...) + PASS 2 (resolveOneTile() above, unchanged), into a
    // temporary full destWidth x destHeight raster -- exactly runFrame()'s
    // own threads<=1 oracle-loop body above, with
    // generateFragmentsFieldRows() standing in for generateFragments() as
    // PASS 1's own entry point. Single-threaded only, this unit (see
    // runFrameField()'s own doc comment, core/resolve.hpp, ADR-077):
    // params.threads/params.pool are not read here, the same incremental
    // staging WU-16a through WU-19a already used before threading a new
    // orchestration path. kAccum/tileKCells are passed as nullptr --
    // resolveOneTile() only dereferences them when params.kBufferMode !=
    // Off, and this unit's own precondition (core/resolve.hpp) requires
    // Off, so this is exactly WU-10's own original plain-mode call shape.
    auto resolveOneParity = [&](int rowOffset) {
        TileBins bins(params.destWidth, params.destHeight);
        generateFragmentsFieldRows(lattice, src, params.maxK, params.supersample,
                                    params.tag, rowOffset, bins);

        video::Raster444 full(params.destWidth, params.destHeight);
        TileAccum accum;
        std::vector<AccumCell> tileCells(tilePixelsN);
        const std::array<const TileBins*, 1> soloSource{&bins};

        for (int ty = 0; ty < tilesY; ++ty) {
            for (int tx = 0; tx < tilesX; ++tx) {
                resolveOneTile(soloSource, params, full, accum, tileCells,
                                /*kAccum=*/nullptr, /*tileKCells=*/nullptr, tx, ty);
            }
        }
        return full;
    };

    const video::Raster444 topFull = resolveOneParity(0);
    const video::Raster444 bottomFull = resolveOneParity(1);

    // Output side (ADR-076): each parity's resolve above already covers the
    // full destination raster -- a field's own samples can scatter to any
    // destination row under a general warp -- so extractField() below picks
    // out that parity's own rows of the *destination* frame, sized against
    // params.destHeight (not src.height -- fieldRowCount()'s first argument
    // is a frame height, and it is dest's own rows being selected here, not
    // src's).
    video::Raster444 topField(
        params.destWidth,
        video::fieldRowCount(params.destHeight, video::FieldParity::Top));
    video::Raster444 bottomField(
        params.destWidth,
        video::fieldRowCount(params.destHeight, video::FieldParity::Bottom));
    video::extractField(topFull, video::FieldParity::Top, topField);
    video::extractField(bottomFull, video::FieldParity::Bottom, bottomField);

    video::interleaveFields(topField, bottomField, dest);
}

void runFrameBytes(const Lattice& lattice,
                    const std::uint8_t* srcBytes, std::ptrdiff_t srcRowBytes,
                    int srcWidth, int srcHeight,
                    const PipelineParams& params,
                    std::uint8_t* dstBytes, std::ptrdiff_t dstRowBytes) {
    // v210 unpack (WU-02) straight out of the caller's own buffer -- no
    // file, no intermediate copy of the packed bytes themselves.
    video::Raster422 in(srcWidth, srcHeight);
    v210::unpackImage(srcBytes, srcRowBytes, srcWidth, srcHeight,
                              in.Y.data(), in.planeY().strideSamples,
                              in.Cb.data(), in.Cr.data(), in.planeCb().strideSamples);

    // Chroma upsample 4:2:2 -> 4:4:4 (ADR-005); luma passes straight
    // through, same as runFrame()'s other two callers.
    video::Raster444 full(srcWidth, srcHeight);
    std::copy(in.Y.begin(), in.Y.end(), full.Y.begin());
    chroma::upsampleImage(in.Cb.data(), in.planeCb().strideSamples,
                           srcWidth, srcHeight,
                           full.Cb.data(), full.planeCb().strideSamples);
    chroma::upsampleImage(in.Cr.data(), in.planeCr().strideSamples,
                           srcWidth, srcHeight,
                           full.Cr.data(), full.planeCr().strideSamples);

    // RGB boundary conversion, input side (WU-40, ADR-085) -- see this
    // file's own header comment above for why this round-trips back to
    // YCbCr immediately rather than feeding SourceRaster with RGB directly.
    {
        video::RasterRGB rgb(srcWidth, srcHeight);
        chroma::ycbcrToRgbImage(full.Y.data(), full.Cb.data(), full.Cr.data(),
                                 full.planeY().strideSamples, srcWidth, srcHeight,
                                 rgb.R.data(), rgb.G.data(), rgb.B.data(),
                                 rgb.planeR().strideSamples);
        chroma::rgbToYcbcrImage(rgb.R.data(), rgb.G.data(), rgb.B.data(),
                                 rgb.planeR().strideSamples, srcWidth, srcHeight,
                                 full.Y.data(), full.Cb.data(), full.Cr.data(),
                                 full.planeY().strideSamples);
    }

    SourceRaster src;
    src.width = srcWidth;
    src.height = srcHeight;
    src.y = full.Y.data();
    src.cb = full.Cb.data();
    src.cr = full.Cr.data();

    video::Raster444 warped(params.destWidth, params.destHeight);
    runFrame(lattice, src, params, warped);

    // RGB boundary conversion, output side (WU-40, ADR-085) -- symmetric
    // with the input side above, immediately before chroma downsample.
    {
        video::RasterRGB rgb(params.destWidth, params.destHeight);
        chroma::ycbcrToRgbImage(warped.Y.data(), warped.Cb.data(), warped.Cr.data(),
                                 warped.planeY().strideSamples,
                                 params.destWidth, params.destHeight,
                                 rgb.R.data(), rgb.G.data(), rgb.B.data(),
                                 rgb.planeR().strideSamples);
        chroma::rgbToYcbcrImage(rgb.R.data(), rgb.G.data(), rgb.B.data(),
                                 rgb.planeR().strideSamples,
                                 params.destWidth, params.destHeight,
                                 warped.Y.data(), warped.Cb.data(), warped.Cr.data(),
                                 warped.planeY().strideSamples);
    }

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

    // v210 pack (WU-02) straight into the caller's own buffer -- no file,
    // no intermediate copy of the packed bytes themselves.
    v210::packImage(out.Y.data(), out.planeY().strideSamples,
                            out.Cb.data(), out.Cr.data(), out.planeCb().strideSamples,
                            params.destWidth, params.destHeight,
                            dstBytes, dstRowBytes);
}

// runFrameBytesDeinterlaced() -- WU-23b2a (DECISIONS.md ADR-080). Reproduces
// runFrameBytes()'s own shared sequence above (v210 unpack, chroma upsample,
// runFrame(), chroma downsample, v210 pack) exactly, with `deinterlacer`'s
// own push() inserted between the chroma upsample and runFrame() -- see
// core/resolve.hpp for the full design account and CORRECTIONS.md C-027 for
// why runFrameBytes() itself could not simply be extended in place (its own
// chroma-upsampled weave Raster444 is a local variable, never reachable by
// any caller).
bool runFrameBytesDeinterlaced(video::Deinterlacer& deinterlacer,
                                const Lattice& lattice,
                                const std::uint8_t* srcBytes, std::ptrdiff_t srcRowBytes,
                                int srcWidth, int srcHeight,
                                const PipelineParams& params,
                                std::uint8_t* dstBytes, std::ptrdiff_t dstRowBytes) {
    // v210 unpack (WU-02) straight out of the caller's own buffer -- exactly
    // runFrameBytes()'s own first step above.
    video::Raster422 in(srcWidth, srcHeight);
    v210::unpackImage(srcBytes, srcRowBytes, srcWidth, srcHeight,
                              in.Y.data(), in.planeY().strideSamples,
                              in.Cb.data(), in.Cr.data(), in.planeCb().strideSamples);

    // Chroma upsample 4:2:2 -> 4:4:4 (ADR-005) into the full-height "weave"
    // frame -- exactly the shape video::Deinterlacer::push() requires
    // (video::extractField()'s own precondition vocabulary, video/deinterlace.hpp);
    // luma passes straight through, same as runFrameBytes()'s other callers.
    video::Raster444 weave(srcWidth, srcHeight);
    std::copy(in.Y.begin(), in.Y.end(), weave.Y.begin());
    chroma::upsampleImage(in.Cb.data(), in.planeCb().strideSamples,
                           srcWidth, srcHeight,
                           weave.Cb.data(), weave.planeCb().strideSamples);
    chroma::upsampleImage(in.Cr.data(), in.planeCr().strideSamples,
                           srcWidth, srcHeight,
                           weave.Cr.data(), weave.planeCr().strideSamples);

    // Deinterlace -- the one step runFrameBytes() above does not have.
    // ADR-080: mirrors push()'s own contract exactly. On the very first call
    // ever made against a freshly constructed `deinterlacer`, push() returns
    // false and `progressive` is left whatever Raster444's own constructor
    // zero-initialised it to -- dstBytes must stay completely untouched in
    // that case, so this returns immediately, before touching runFrame() or
    // dstBytes at all.
    video::Raster444 progressive(srcWidth, srcHeight);
    if (!deinterlacer.push(weave, progressive)) {
        return false;
    }

    // RGB boundary conversion, input side (WU-40, ADR-085) -- applied to
    // `progressive`, not `weave`: this sits immediately before SourceRaster
    // is built, the same relative position runFrameBytes() above uses,
    // downstream of deinterlace rather than wrapping it (deinterlace still
    // operates on genuine, unperturbed chroma-upsampled YCbCr). See this
    // file's own header comment above for why this round-trips back to
    // YCbCr immediately rather than feeding SourceRaster with RGB directly.
    {
        video::RasterRGB rgb(srcWidth, srcHeight);
        chroma::ycbcrToRgbImage(progressive.Y.data(), progressive.Cb.data(),
                                 progressive.Cr.data(),
                                 progressive.planeY().strideSamples, srcWidth, srcHeight,
                                 rgb.R.data(), rgb.G.data(), rgb.B.data(),
                                 rgb.planeR().strideSamples);
        chroma::rgbToYcbcrImage(rgb.R.data(), rgb.G.data(), rgb.B.data(),
                                 rgb.planeR().strideSamples, srcWidth, srcHeight,
                                 progressive.Y.data(), progressive.Cb.data(),
                                 progressive.Cr.data(),
                                 progressive.planeY().strideSamples);
    }

    // The reconstructed full-height progressive frame -- not `weave` -- is
    // this call's own source raster for the warp, exactly the same relative
    // position `full` occupies in runFrameBytes() above.
    SourceRaster src;
    src.width = srcWidth;
    src.height = srcHeight;
    src.y = progressive.Y.data();
    src.cb = progressive.Cb.data();
    src.cr = progressive.Cr.data();

    video::Raster444 warped(params.destWidth, params.destHeight);
    runFrame(lattice, src, params, warped);

    // RGB boundary conversion, output side (WU-40, ADR-085) -- symmetric
    // with the input side above, immediately before chroma downsample.
    {
        video::RasterRGB rgb(params.destWidth, params.destHeight);
        chroma::ycbcrToRgbImage(warped.Y.data(), warped.Cb.data(), warped.Cr.data(),
                                 warped.planeY().strideSamples,
                                 params.destWidth, params.destHeight,
                                 rgb.R.data(), rgb.G.data(), rgb.B.data(),
                                 rgb.planeR().strideSamples);
        chroma::rgbToYcbcrImage(rgb.R.data(), rgb.G.data(), rgb.B.data(),
                                 rgb.planeR().strideSamples,
                                 params.destWidth, params.destHeight,
                                 warped.Y.data(), warped.Cb.data(), warped.Cr.data(),
                                 warped.planeY().strideSamples);
    }

    // Output-side "[re-interlace]" is a provable no-op for this project's
    // own frame-rate-only mode (ADR-080: video::extractField() applied for
    // both parities followed by video::interleaveFields() over the *same*
    // source frame reproduces every row at its own original index exactly,
    // an algebraic identity of the row-index arithmetic itself) -- so
    // `warped` goes straight to chroma downsample below, exactly as
    // runFrameBytes() above does with its own (non-deinterlaced) warped
    // output. Not a corner cut: WORK-UNITS.md's own WU-23b2a Accept line
    // checks this identity directly, by building the same output via an
    // explicit extractField() x2 + interleaveFields() pass and comparing
    // byte for byte against the no-op path shipped here.
    video::Raster422 out(params.destWidth, params.destHeight);
    std::copy(warped.Y.begin(), warped.Y.end(), out.Y.begin());
    chroma::downsampleImage(warped.Cb.data(), warped.planeCb().strideSamples,
                             params.destWidth, params.destHeight,
                             out.Cb.data(), out.planeCb().strideSamples);
    chroma::downsampleImage(warped.Cr.data(), warped.planeCr().strideSamples,
                             params.destWidth, params.destHeight,
                             out.Cr.data(), out.planeCr().strideSamples);

    // v210 pack (WU-02) straight into the caller's own buffer -- exactly
    // runFrameBytes()'s own last step above.
    v210::packImage(out.Y.data(), out.planeY().strideSamples,
                            out.Cb.data(), out.Cr.data(), out.planeCb().strideSamples,
                            params.destWidth, params.destHeight,
                            dstBytes, dstRowBytes);
    return true;
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

    // RGB boundary conversion, input side (WU-40, ADR-085) -- see
    // runFrameBytes() above (this file's own header comment covers the full
    // rationale) for why this round-trips back to YCbCr immediately rather
    // than feeding SourceRaster with RGB directly.
    {
        video::RasterRGB rgb(srcWidth, srcHeight);
        chroma::ycbcrToRgbImage(full.Y.data(), full.Cb.data(), full.Cr.data(),
                                 full.planeY().strideSamples, srcWidth, srcHeight,
                                 rgb.R.data(), rgb.G.data(), rgb.B.data(),
                                 rgb.planeR().strideSamples);
        chroma::rgbToYcbcrImage(rgb.R.data(), rgb.G.data(), rgb.B.data(),
                                 rgb.planeR().strideSamples, srcWidth, srcHeight,
                                 full.Y.data(), full.Cb.data(), full.Cr.data(),
                                 full.planeY().strideSamples);
    }

    SourceRaster src;
    src.width = srcWidth;
    src.height = srcHeight;
    src.y = full.Y.data();
    src.cb = full.Cb.data();
    src.cr = full.Cr.data();

    video::Raster444 warped(params.destWidth, params.destHeight);
    runFrame(lattice, src, params, warped);

    // RGB boundary conversion, output side (WU-40, ADR-085) -- symmetric
    // with the input side above, immediately before chroma downsample.
    {
        video::RasterRGB rgb(params.destWidth, params.destHeight);
        chroma::ycbcrToRgbImage(warped.Y.data(), warped.Cb.data(), warped.Cr.data(),
                                 warped.planeY().strideSamples,
                                 params.destWidth, params.destHeight,
                                 rgb.R.data(), rgb.G.data(), rgb.B.data(),
                                 rgb.planeR().strideSamples);
        chroma::rgbToYcbcrImage(rgb.R.data(), rgb.G.data(), rgb.B.data(),
                                 rgb.planeR().strideSamples,
                                 params.destWidth, params.destHeight,
                                 warped.Y.data(), warped.Cb.data(), warped.Cr.data(),
                                 warped.planeY().strideSamples);
    }

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
