// scatter-dve — WU-16: thread pool, QoS, per-worker bin arenas
// (Phase 4; architecture.md section 6's threading model, scoped down for
// this unit — see DECISIONS.md ADR-040 for the full design and for what
// is deliberately not built here.)
//
// Arrives now, exactly as ADR-026 anticipated when it declared
// runFrame()/runFrameFile() in core/resolve.hpp instead of a pipeline.hpp
// of their own: "When WU-16 actually adds thread-pool state, pipeline.hpp
// arrives with it and these declarations move there." They do not move —
// ADR-026 also said "nothing about runFrame()'s own signature is expected
// to change when that happens," and nothing here changes it. This header
// adds only the thread-pool machinery core/pipeline.cpp's own runFrame()
// now uses internally when PipelineParams::threads (core/resolve.hpp) is
// greater than 1.
//
// Scope, per ADR-040: this unit parallelises PASS 2 alone — the per-tile
// splat/bank-resolve/normalise/composite loop core/pipeline.cpp already
// runs once per tile, reading TileBins (core/binner.hpp), which pass 1
// (generateFragments(), unchanged, still single-threaded this unit) has
// already fully populated by the time any worker thread is started. Every
// tile's own bin is independent of every other tile's, and every tile
// writes a disjoint block of the destination raster — the "natural unit
// of per-thread work" ADR-040 reasons through in full. Pass 1's own
// row-band partitioning (architecture.md section 6's fuller two-pass
// sketch, with a private per-tile bin arena *per worker* during
// generation itself) is not built here — see ADR-040's own deferral to a
// future WU-16b.
//
// "Per-worker bin arena," in this unit's own scope, names the one
// TileAccum (core/splat.hpp) plus one AccumCell scratch buffer each
// worker thread owns for the whole of runOnAll()'s call, reused (cleared,
// not reconstructed) across every tile that worker is assigned — exactly
// what TileAccum::clear()'s own doc comment already anticipated ("e.g.
// for reuse across tiles (WU-16's per-worker bin arenas will want
// this)"). See core/pipeline.cpp.
#pragma once

#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

namespace scatter {

// architecture.md section 6, "Apple Silicon gotcha": threads inherit
// default QoS, which may schedule work onto the M1 Max's 2 efficiency
// cores instead of its 8 performance cores — a 3-4x slowdown with no
// error, nothing to catch in a test. The documented fix is
// `pthread_set_qos_class_self_np(QOS_CLASS_USER_INTERACTIVE, 0)`, called
// once by the calling thread itself at startup — an Apple-only API
// (<pthread/qos.h>), so this function is a no-op everywhere else, the
// same "fail soft on a platform that does not have this surface at all"
// shape ADR-031's BLACKMAGIC_SDK_DIR guard already uses for a different
// Apple-only dependency, and for the same underlying reason: ADR-013
// requires scatter-core to keep building and testing in the Linux cloud
// sandbox, with no Apple toolchain and no hardware, unchanged.
//
// Purely a scheduling hint. Never changes this project's own output —
// that is I6's job, not this function's — so it has no return value and
// nothing meaningful for a test to assert about beyond "building and
// calling it on every supported platform does not fail."
void setWorkerQoS() noexcept;

// A fixed-size pool of persistent worker threads (spawned once, joined on
// destruction — not respawned per frame, since architecture.md section 6
// describes "8 worker threads" as a standing resource, not a per-frame
// cost). Each worker calls setWorkerQoS() exactly once, at thread
// startup, before ever running caller work.
//
// runOnAll(fn) is this pool's only operation: it calls fn(workerIndex)
// once for every workerIndex in [0, size()), one call per worker thread,
// and blocks the calling thread until every one of those calls has
// returned. Two calls to runOnAll() from the same calling thread are
// therefore already a barrier in the architecture.md section 6 sense —
// the second call's own work is never dispatched to any worker until the
// first call has fully returned to the caller — without this class
// needing a separate barrier primitive of its own. This unit only ever
// calls runOnAll() once per runFrame() invocation (pass 2 alone, per this
// header's own file comment); the two-call barrier shape is here because
// it falls out of the same mechanism for free, for whichever future unit
// (ADR-040's own named WU-16b) parallelises pass 1 the same way and
// actually needs it.
//
// fn must not throw: a worker thread that lets an exception escape its
// own dispatched call terminates the process (ordinary std::thread
// behaviour for an uncaught exception), and nothing in this class catches
// on a worker's behalf — the same "caller's own bug, not guarded against
// here" convention this codebase already uses throughout for unchecked
// preconditions (e.g. Lattice::at()'s row/col bounds).
class ThreadPool {
public:
    // numThreads must be >= 1. core/pipeline.cpp only ever constructs one
    // when PipelineParams::threads > 1 (see that file); a caller wanting
    // single-threaded execution does not construct a pool at all, so that
    // path never depends on this class — or on runOnAll()'s own dispatch
    // machinery — being correct. See ADR-040's own reasoning for why the
    // threads<=1 path stays a plain, separate loop rather than routing
    // through ThreadPool(1).
    explicit ThreadPool(int numThreads);
    ~ThreadPool();

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool(ThreadPool&&) = delete;
    ThreadPool& operator=(ThreadPool&&) = delete;

    int size() const noexcept { return numThreads_; }

    void runOnAll(const std::function<void(int)>& fn);

private:
    int numThreads_;
    std::vector<std::thread> workers_;

    std::mutex mutex_;
    std::condition_variable cvDispatch_;
    std::condition_variable cvDone_;
    std::function<void(int)> task_;
    unsigned generation_ = 0;
    int pending_ = 0;
    bool shutdown_ = false;

    void workerLoop(int workerIndex);
};

}  // namespace scatter
