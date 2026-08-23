// scatter-dve — WU-23b1 tests: Weston 3-field de-interlace, filter core
// (video/deinterlace.hpp). See DECISIONS.md ADR-078/ADR-079 and
// CORRECTIONS.md C-025 for the full design account this test checks
// against.
//
// Four things, each checked directly rather than only inferred from the
// others:
//
//   1. The three-frame state machine: the first push produces no output;
//      every push from the second onward does; a distinguishing per-frame
//      marker baked into each pushed frame's own content lets a mismatch
//      show up as wrong *content*, not just a wrong true/false return.
//   2. Full-frame reconstruction, for a 6-frame sequence under both
//      coefficient sets and both field parities, against
//      refReconstruct() below -- a separately-written reference
//      implementation (per-pixel, vector-based coefficient lists, its own
//      copy of the reflect/round-half-up arithmetic) that does not call
//      into video/deinterlace.cpp at all, so this is a genuine
//      independent check, not two copies of the same bug agreeing.
//   3. Two explicit, hand-computed (and cross-checked with a standalone
//      Python script during this unit's own development -- see
//      HANDOFF.md) edge-row values, one per coefficient set, at the
//      top-adjacent and bottom rows of an 8-row frame -- the literal
//      "edge rows... checked against a directly-computed expected value"
//      Accept: line, kept separate from the refReconstruct() comparison
//      above so it stands on its own even if that helper had a shared
//      bug with the production code.
//   4. One real-geometry (720x576, this project's own 625i50 standard,
//      docs/architecture.md section 3's buffer-size table) sanity
//      sequence, against refReconstruct() again -- SD, not 1080p, per
//      this project's own current stay-in-SD-domain scope (HANDOFF.md).
//
// Deliberately not exercised here: any lattice/warp/pipeline involvement,
// or DeckLink wiring -- this unit is the filter core alone, no
// dependency on either (WU-23b2's own job, not started).

#include <cstddef>
#include <cstdint>
#include <vector>

#include "core/types.hpp"
#include "harness.hpp"
#include "video/deinterlace.hpp"
#include "video/interlace.hpp"
#include "video/raster.hpp"

using scatter::Sample;
namespace video = scatter::video;

namespace {

// ---------------------------------------------------------------------------
// Marked weave-frame construction -- every (frameId, plane, row, col)
// combination maps to a distinct value (well inside Sample's 16-bit range
// for the small geometry used below), so a sample landing in the wrong
// place -- wrong frame, wrong plane, wrong row -- shows up as a mismatch
// rather than coincidentally matching. Same discipline
// tests/test_interlace.cpp's own makeMarkedFrame() already uses, extended
// with a frameId axis since this unit pushes a *sequence* of frames, not
// one.
// ---------------------------------------------------------------------------

Sample marker(int frameId, int plane, int row, int col) {
    return Sample(frameId * 1000 + plane * 200 + row * 10 + col);
}

video::Raster444 makeMarkedWeaveFrame(int frameId, int width, int height) {
    video::Raster444 f(width, height);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const std::size_t idx = std::size_t(y) * std::size_t(width) + std::size_t(x);
            f.Y[idx]  = marker(frameId, 0, y, x);
            f.Cb[idx] = marker(frameId, 1, y, x);
            f.Cr[idx] = marker(frameId, 2, y, x);
        }
    }
    return f;
}

// ---------------------------------------------------------------------------
// Independent reference implementation -- deliberately structured
// differently from video/deinterlace.cpp's own reconstructPlane() (per-
// pixel rather than per-row-then-per-pixel, std::vector<int> coefficient
// lists rather than a fixed-size struct, its own copy of the reflect and
// round-half-up helpers) and never calls into it. Real w3fdif coefficient
// values, re-typed here from DECISIONS.md ADR-079's own table, not
// #included from production code.
// ---------------------------------------------------------------------------

struct RefCoefs {
    std::vector<std::int64_t> low;
    std::vector<std::int64_t> high;
};

RefCoefs refCoefsFor(video::DeinterlaceCoefficients c) {
    if (c == video::DeinterlaceCoefficients::Simple) {
        return RefCoefs{{16384, 16384}, {-2048, 4096, -2048}};
    }
    return RefCoefs{{-852, 17236, 17236, -852}, {1016, -3801, 5570, -3801, 1016}};
}

int refReflect(int y, int height) {
    while (y < 0) y += 2;
    while (y >= height) y -= 2;
    return y;
}

Sample refPixel(const std::vector<Sample>& curPlane, const std::vector<Sample>& prevPlane,
               int width, int height, int row, int col, int anchorRowParity,
               const RefCoefs& coefs) {
    if (row % 2 == anchorRowParity) {
        return curPlane[std::size_t(row) * std::size_t(width) + std::size_t(col)];
    }
    std::int64_t sum = 0;
    const int nLow = int(coefs.low.size());
    for (int j = 0; j < nLow; ++j) {
        const int yin = refReflect((row + 1) + 2 * j - nLow, height);
        sum += std::int64_t(curPlane[std::size_t(yin) * std::size_t(width) + std::size_t(col)]) * coefs.low[std::size_t(j)];
    }
    const int nHigh = int(coefs.high.size());
    for (int j = 0; j < nHigh; ++j) {
        const int yin = refReflect((row + 1) + 2 * j - nHigh, height);
        const std::size_t idx = std::size_t(yin) * std::size_t(width) + std::size_t(col);
        sum += std::int64_t(curPlane[idx]) * coefs.high[std::size_t(j)];
        sum += std::int64_t(prevPlane[idx]) * coefs.high[std::size_t(j)];
    }
    const std::int64_t rounded = (sum + (std::int64_t(1) << 14)) >> 15;
    return Sample(rounded);
}

video::Raster444 refReconstruct(const video::Raster444& cur, const video::Raster444& prev,
                                video::FieldParity anchor, video::DeinterlaceCoefficients c) {
    const int anchorRowParity = (anchor == video::FieldParity::Bottom) ? 1 : 0;
    const RefCoefs coefs = refCoefsFor(c);
    video::Raster444 out(cur.width, cur.height);
    for (int y = 0; y < cur.height; ++y) {
        for (int x = 0; x < cur.width; ++x) {
            const std::size_t idx = std::size_t(y) * std::size_t(cur.width) + std::size_t(x);
            out.Y[idx]  = refPixel(cur.Y,  prev.Y,  cur.width, cur.height, y, x, anchorRowParity, coefs);
            out.Cb[idx] = refPixel(cur.Cb, prev.Cb, cur.width, cur.height, y, x, anchorRowParity, coefs);
            out.Cr[idx] = refPixel(cur.Cr, prev.Cr, cur.width, cur.height, y, x, anchorRowParity, coefs);
        }
    }
    return out;
}

// ---------------------------------------------------------------------------
// 1 & 2. State machine + full-frame reconstruction, against refReconstruct().
//
// The expected (cur, prev) pair at push k (0-indexed) follows directly
// from DECISIONS.md ADR-079's own shift (traced in CORRECTIONS.md C-025):
// push 0 -> no output; push 1 -> cur = frame 0, prev = frame 0 (the
// stream-start duplicate); push m (m >= 2) -> cur = frame (m-1),
// prev = frame (m-2).
// ---------------------------------------------------------------------------

void testStateMachineAndReconstruction(video::FieldParity anchor,
                                       video::DeinterlaceCoefficients coeffs,
                                       int width, int height, int numFrames) {
    video::Deinterlacer d(anchor, coeffs);
    std::vector<video::Raster444> pushed;
    pushed.reserve(std::size_t(numFrames));

    for (int k = 0; k < numFrames; ++k) {
        video::Raster444 frame = makeMarkedWeaveFrame(k, width, height);
        video::Raster444 out(width, height);
        const bool produced = d.push(frame, out);
        pushed.push_back(std::move(frame));

        if (k == 0) {
            CHECK(!produced);
            continue;
        }
        CHECK_ONCE(produced);

        const std::size_t curIdx = std::size_t(k - 1);
        const std::size_t prevIdx = (k == 1) ? 0 : std::size_t(k - 2);
        const video::Raster444 expected =
            refReconstruct(pushed[curIdx], pushed[prevIdx], anchor, coeffs);

        CHECK_ONCE(out.Y == expected.Y);
        CHECK_ONCE(out.Cb == expected.Cb);
        CHECK_ONCE(out.Cr == expected.Cr);
    }
}

// ---------------------------------------------------------------------------
// 3. Explicit hand-computed edge rows -- anchor = Top (missing rows are
// the odd ones: 1, 3, 5, 7 of an 8-row frame), row 1 (needs a top-edge
// reflection: the real formula's own y_in for one high-pass tap goes
// negative and reflects back to 1 itself) and row 7 (needs a
// bottom-edge reflection: both low-pass taps collapse onto row 6). cur
// and prev are simple, distinguishable-by-inspection ramps, not markers
// -- cur[row] = (row+1)*100, prev[row] = (row+1)*100 + 5000 -- chosen so
// the expected sums are easy to re-derive by hand from
// video/deinterlace.hpp's own file comment and DECISIONS.md ADR-079's
// coefficient table, and cross-checked against those two together with a
// standalone script during this unit's own development (see HANDOFF.md).
// Single column: the reconstruction has no horizontal component, so width
// beyond 1 would only repeat the same check.
// ---------------------------------------------------------------------------

void testEdgeRowsHandComputed() {
    constexpr int width = 1;
    constexpr int height = 8;

    video::Raster444 curFrame(width, height), prevFrame(width, height);
    for (int row = 0; row < height; ++row) {
        const Sample cv = Sample((row + 1) * 100);
        const Sample pv = Sample((row + 1) * 100 + 5000);
        curFrame.Y[std::size_t(row)]  = cv;
        curFrame.Cb[std::size_t(row)] = cv;
        curFrame.Cr[std::size_t(row)] = cv;
        prevFrame.Y[std::size_t(row)]  = pv;
        prevFrame.Cb[std::size_t(row)] = pv;
        prevFrame.Cr[std::size_t(row)] = pv;
    }

    // Three pushes line up the internal cur_/prev_ pair this check wants
    // (cur_ == curFrame, prev_ == prevFrame) via the ordinary shift
    // (DECISIONS.md ADR-079): push 1 (prevFrame) warms up with no output
    // (cur_ becomes prevFrame, duplicated as its own prev_ candidate);
    // push 2 (curFrame) produces an output this check does not care about
    // (cur_/prev_ both still prevFrame at that point -- curFrame is only
    // sitting in next_ so far); push 3 (content irrelevant, curFrame
    // reused) shifts next_'s own curFrame into cur_, with prev_ now the
    // real prevFrame from push 1 -- exactly the pairing
    // video/deinterlace.hpp's own file-comment example and this
    // function's own Python-cross-checked arithmetic (see file header)
    // assume.
    for (video::DeinterlaceCoefficients coeffs :
         {video::DeinterlaceCoefficients::Simple, video::DeinterlaceCoefficients::Complex}) {
        video::Deinterlacer d(video::FieldParity::Top, coeffs);
        video::Raster444 out(width, height);

        CHECK(!d.push(prevFrame, out));
        CHECK(d.push(curFrame, out));
        CHECK(d.push(curFrame, out));

        const bool simple = (coeffs == video::DeinterlaceCoefficients::Simple);
        const Sample expectedRow1 = simple ? Sample(175) : Sample(173);
        const Sample expectedRow7 = simple ? Sample(725) : Sample(727);

        // Row 0 and row 2/4/6 are anchor rows (Top), copied through
        // unchanged from cur_ == curFrame; only checking the two
        // hand-computed missing rows here, per this function's own job.
        CHECK(out.Y[1] == expectedRow1);
        CHECK(out.Y[7] == expectedRow7);
        CHECK(out.Cb[1] == expectedRow1);
        CHECK(out.Cb[7] == expectedRow7);
        CHECK(out.Cr[1] == expectedRow1);
        CHECK(out.Cr[7] == expectedRow7);
    }
}

// ---------------------------------------------------------------------------
// 4. Real-geometry (720x576, this project's own 625i50 standard) sanity
// sequence -- SD, per this project's own current stay-in-SD-domain scope
// (HANDOFF.md), not 1920x1080. Simple coefficients only, to keep this
// check's own runtime reasonable; the small-geometry tests above already
// cover Complex exhaustively.
// ---------------------------------------------------------------------------

void testSdGeometrySanity() {
    constexpr int width = 720;
    constexpr int height = 576;
    constexpr int numFrames = 3;

    for (video::FieldParity anchor : {video::FieldParity::Top, video::FieldParity::Bottom}) {
        video::Deinterlacer d(anchor, video::DeinterlaceCoefficients::Simple);
        std::vector<video::Raster444> pushed;
        pushed.reserve(std::size_t(numFrames));

        for (int k = 0; k < numFrames; ++k) {
            // A cheap per-frame pattern, not the full marker scheme (this
            // geometry is large enough that per-pixel uniqueness buys
            // nothing extra over what the small-geometry tests above
            // already establish) -- distinct per frame/plane/row so a
            // gross wiring mistake (wrong plane, wrong row) still shows
            // up as a mismatch.
            video::Raster444 frame(width, height);
            for (int y = 0; y < height; ++y) {
                const Sample v = Sample((k * 3 + y) % 60000);
                const std::size_t rowBase = std::size_t(y) * std::size_t(width);
                for (int x = 0; x < width; ++x) {
                    frame.Y[rowBase + std::size_t(x)]  = v;
                    frame.Cb[rowBase + std::size_t(x)] = Sample(v + 1);
                    frame.Cr[rowBase + std::size_t(x)] = Sample(v + 2);
                }
            }

            video::Raster444 out(width, height);
            const bool produced = d.push(frame, out);
            pushed.push_back(std::move(frame));

            if (k == 0) {
                CHECK(!produced);
                continue;
            }
            CHECK_ONCE(produced);

            const std::size_t curIdx = std::size_t(k - 1);
            const std::size_t prevIdx = (k == 1) ? 0 : std::size_t(k - 2);
            const video::Raster444 expected = refReconstruct(
                pushed[curIdx], pushed[prevIdx], anchor, video::DeinterlaceCoefficients::Simple);

            CHECK_ONCE(out.Y == expected.Y);
            CHECK_ONCE(out.Cb == expected.Cb);
            CHECK_ONCE(out.Cr == expected.Cr);
        }
    }
}

}  // namespace

int main() {
    // Small, hand-tractable geometry (fast, and complex-filter taps need
    // enough rows to exercise a genuine interior alongside both edges) --
    // width 4 keeps the marker arithmetic simple, height 10 gives the
    // complex filter's own ±3-row footprint real interior rows as well as
    // both edges, 6 frames comfortably clears the Accept line's own "5+"
    // sequence length.
    for (video::FieldParity anchor : {video::FieldParity::Top, video::FieldParity::Bottom}) {
        for (video::DeinterlaceCoefficients coeffs :
             {video::DeinterlaceCoefficients::Simple, video::DeinterlaceCoefficients::Complex}) {
            testStateMachineAndReconstruction(anchor, coeffs, 4, 10, 6);
        }
    }

    testEdgeRowsHandComputed();
    testSdGeometrySanity();

    return scatter::test::summary("test_deinterlace");
}
