// scatter-dve — WU-16b: PASS 1 row-band parallelism, per-worker
// generation-time bin arenas (architecture.md section 6's fuller two-pass
// design; DECISIONS.md ADR-041, completing ADR-040's own deferred WU-16b).
//
// Two parts:
//
// 1. checkRowRangeReassembly() and its two callers exercise
//    core/binner.hpp's new generateFragmentsRowRange() directly, independent
//    of core/pipeline.cpp — the same "test the new primitive on its own
//    terms, not only through the pipeline" discipline
//    tests/test_threading.cpp's own Part 1 (ThreadPool, direct) already
//    used for WU-16a. Partitioning a source raster's rows into several
//    bands (including more bands than rows) and reassembling every band's
//    own TileBins must reproduce a single whole-raster generateFragments()
//    call exactly — not just the same set of source pixels present per
//    tile, but every fragment's own encoded position/colour/weight fields
//    bit-identical. This is ADR-040/ADR-041's own "honest fix" — the
//    v-parameter's denominator stays keyed to the whole raster regardless
//    of which row band is being generated — checked directly rather than
//    only inferred from the pipeline staying bit-identical to
//    threads == 1 (which would already fail if this were wrong, but
//    wouldn't localise the failure to this specific mechanism).
//
// 2. test_threaded_pipeline_more_workers_than_source_rows() is the
//    pipeline-level edge case tests/test_threading.cpp's own thread-count
//    matrix (max source rows: 256, 200) never exercises: a source
//    genuinely shorter than several tested thread counts, so multiple
//    workers get an empty row band in core/pipeline.cpp's own PASS-1
//    dispatch — must be harmless, not a crash or a hang, the same standard
//    tests/test_threading.cpp already established for PASS 2's own
//    more-workers-than-tiles case (threads == 16 there).
#include "core/binner.hpp"
#include "core/pipeline.hpp"
#include "core/resolve.hpp"
#include "core/shapes/shapes.hpp"
#include "harness.hpp"

#include <cstddef>
#include <cstdint>
#include <map>
#include <utility>
#include <vector>

using namespace scatter;

namespace {

// ---------------------------------------------------------------------------
// Part 1 — generateFragmentsRowRange() vs. generateFragments(), direct.
// ---------------------------------------------------------------------------

// Duplicated locally from tests/test_binner.cpp per SESSION-PROTOCOL.md
// rule 2 (no cross-test-file sharing) — see that file for the fuller
// rationale of both helpers below.

Lattice makePixelAffineLattice(double scaleX, double scaleY, double offX,
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

struct SignatureRaster {
    int width, height;
    std::vector<Sample> y, cb, cr;

    explicit SignatureRaster(int w, int h) : width(w), height(h) {
        y.resize(std::size_t(w) * std::size_t(h));
        cb.resize(std::size_t(w) * std::size_t(h));
        cr.resize(std::size_t(w) * std::size_t(h));
        for (int py = 0; py < h; ++py) {
            for (int px = 0; px < w; ++px) {
                const std::size_t i = std::size_t(py) * std::size_t(w) + std::size_t(px);
                y[i]  = Sample(px);
                cb[i] = Sample(py);
                cr[i] = 0;
            }
        }
    }

    SourceRaster view() const noexcept {
        SourceRaster r;
        r.width = width;
        r.height = height;
        r.y = y.data();
        r.cb = cb.data();
        r.cr = cr.data();
        return r;
    }
};

using FragKey = std::pair<int, int>;  // (px, py), decoded from R/G (renamed
                                       // from Y/Cb, WU-39, ADR-085 -- the
                                       // decoded values are unaffected: this
                                       // is a pure field-name rename, not a
                                       // change to what binner.cpp stores)

FragKey decode(const Frag& f) noexcept { return {int(f.R), int(f.G)}; }

bool fragBitIdentical(const Frag& a, const Frag& b) noexcept {
    return a.x == b.x && a.y == b.y && a.R == b.R && a.G == b.G &&
           a.B == b.B && a.w == b.w && a.z == b.z && a.tag == b.tag &&
           a.reserved == b.reserved;
}

// Generates fragments for `src`/`lat` in `numBands` row bands — the exact
// (worker * height) / numBands boundary formula core/pipeline.cpp's own
// runFrame() uses — and checks the union of every band's own TileBins,
// tile by tile, against a single whole-raster generateFragments() call.
void checkRowRangeReassembly(const Lattice& lat, const SourceRaster& src,
                              int destW, int destH, int numBands) {
    TileBins whole(destW, destH);
    generateFragments(lat, src, /*maxK=*/1000.0, SupersampleConfig{}, /*tag=*/0, whole);

    std::vector<TileBins> bands;
    bands.reserve(std::size_t(numBands));
    for (int b = 0; b < numBands; ++b) {
        bands.emplace_back(destW, destH);
        const int rowStart = (b * src.height) / numBands;
        const int rowEnd = ((b + 1) * src.height) / numBands;
        generateFragmentsRowRange(lat, src, /*maxK=*/1000.0, SupersampleConfig{},
                                   /*tag=*/0, rowStart, rowEnd, bands.back());
    }

    for (int ty = 0; ty < whole.tilesY(); ++ty) {
        for (int tx = 0; tx < whole.tilesX(); ++tx) {
            std::map<FragKey, Frag> wantMap;
            for (const Frag& f : whole.tile(tx, ty)) {
                wantMap[decode(f)] = f;
            }
            // No internal duplication in the whole-raster reference itself
            // (same property tests/test_binner.cpp already checks for this
            // construction) — a precondition for the per-key comparison
            // below to mean what it says.
            CHECK_ONCE(wantMap.size() == whole.tile(tx, ty).size());

            std::map<FragKey, Frag> gotMap;
            std::size_t gotCount = 0;
            for (const TileBins& band : bands) {
                for (const Frag& f : band.tile(tx, ty)) {
                    gotMap[decode(f)] = f;
                    ++gotCount;
                }
            }
            CHECK_ONCE(gotCount == wantMap.size());
            CHECK_ONCE(gotMap.size() == wantMap.size());

            for (const auto& kv : wantMap) {
                const auto it = gotMap.find(kv.first);
                if (it == gotMap.end()) {
                    CHECK_ONCE(false);
                    continue;
                }
                CHECK_ONCE(fragBitIdentical(it->second, kv.second));
            }
        }
    }
}

void test_row_range_reassembles_to_whole_raster_uneven_bands() {
    const int W = 41, H = 23;
    SignatureRaster src(W, H);
    // Compression on both axes — differing supersampling decisions are not
    // needed here (test_binner.cpp's own dedicated test already covers
    // 4.6's thresholds directly); this test's own job is the row-range
    // API's reassembly property, at a source height (23) that does not
    // divide evenly into 7 bands, so at least one band's own row count
    // differs from its neighbours'.
    Lattice lat = makePixelAffineLattice(0.6, 0.6, 5.0, 5.0, W, H);
    checkRowRangeReassembly(lat, src.view(), 96, 64, /*numBands=*/7);
}

void test_row_range_reassembles_with_more_bands_than_rows() {
    const int W = 12, H = 5;
    SignatureRaster src(W, H);
    // Compression, same reason test_row_range_reassembles_to_whole_raster_
    // uneven_bands() above uses it: a magnifying map here would trigger
    // 4.6's own supersampling (chooseSupersample(), core/binner.cpp), and
    // a source pixel's own multiple sub-samples can then legitimately
    // collapse onto the same destination cell — decode()'s (px, py)
    // signature does not distinguish sub-samples, so that would produce a
    // same-tile signature collision this test's own reassembly check is
    // not built to interpret, not a WU-16b defect. det J < 1 keeps N == 1
    // everywhere, matching tests/test_binner.cpp's own convention for
    // exactly this reason.
    Lattice lat = makePixelAffineLattice(0.5, 0.5, 3.0, 3.0, W, H);
    // numBands (11) > H (5): several bands are empty (rowStart == rowEnd)
    // — must be harmless, not a crash, the same "harmless, not a bug"
    // standard tests/test_threading.cpp already established for PASS 2's
    // own more-workers-than-tiles case (threads == 16 there).
    checkRowRangeReassembly(lat, src.view(), 64, 64, /*numBands=*/11);
}

// ---------------------------------------------------------------------------
// Part 2 — runFrame() itself, more workers than source rows.
// ---------------------------------------------------------------------------

struct TinyWarpedFrame {
    Lattice lattice;
    std::vector<Sample> y, cb, cr;
    int srcSize;
};

TinyWarpedFrame buildTinyWarpedFrame(int srcSize, int destW, int destH) {
    TinyWarpedFrame w;
    w.srcSize = srcSize;
    w.y.resize(std::size_t(srcSize) * std::size_t(srcSize));
    w.cb.assign(std::size_t(srcSize) * std::size_t(srcSize), kChromaZero);
    w.cr.assign(std::size_t(srcSize) * std::size_t(srcSize), kChromaZero);
    for (int py = 0; py < srcSize; ++py) {
        for (int px = 0; px < srcSize; ++px) {
            const std::size_t i = std::size_t(py) * std::size_t(srcSize) + std::size_t(px);
            w.y[i] = Sample((px * 997 + py * 131) % 65536);
        }
    }

    shapes::CylinderParams cp;
    cp.radius = double(destW) * 0.6;
    cp.angleSpan = 1.4;
    cp.heightSpan = double(destH) * 0.9;
    cp.centerX = double(destW) / 2.0;
    cp.centerY = double(destH) / 2.0;
    w.lattice = shapes::buildCylinderLattice(cp);
    return w;
}

void runOnce(const TinyWarpedFrame& w, int destW, int destH, int threads,
             video::Raster444& dest) {
    SourceRaster src;
    src.width = w.srcSize;
    src.height = w.srcSize;
    src.y = w.y.data();
    src.cb = w.cb.data();
    src.cr = w.cr.data();

    PipelineParams params;
    params.destWidth = destW;
    params.destHeight = destH;
    params.maxK = 1000.0;
    params.threads = threads;

    runFrame(w.lattice, src, params, dest);
}

void test_threaded_pipeline_more_workers_than_source_rows() {
    // Deliberately tiny source height (4) against thread counts well past
    // it (8, 16) — forces several row bands to be empty
    // ((worker * src.height) / numWorkers gives rowStart == rowEnd for the
    // high-index workers) in core/pipeline.cpp's own PASS-1 dispatch, not
    // just PASS 2's already-tested empty-tile-share case.
    const int destW = 61;
    const int destH = 47;
    const int srcSize = 4;

    const TinyWarpedFrame w = buildTinyWarpedFrame(srcSize, destW, destH);

    video::Raster444 reference(destW, destH);
    runOnce(w, destW, destH, /*threads=*/1, reference);

    const int threadCounts[] = {2, 3, 8, 16};
    for (int threads : threadCounts) {
        video::Raster444 dest(destW, destH);
        runOnce(w, destW, destH, threads, dest);
        CHECK(dest.Y.size() == reference.Y.size());
        for (std::size_t i = 0; i < reference.Y.size(); ++i) {
            CHECK_ONCE(dest.Y[i] == reference.Y[i]);
            CHECK_ONCE(dest.Cb[i] == reference.Cb[i]);
            CHECK_ONCE(dest.Cr[i] == reference.Cr[i]);
        }
    }
}

}  // namespace

int main() {
    test_row_range_reassembles_to_whole_raster_uneven_bands();
    test_row_range_reassembles_with_more_bands_than_rows();
    test_threaded_pipeline_more_workers_than_source_rows();
    return scatter::test::summary("test_row_band");
}
