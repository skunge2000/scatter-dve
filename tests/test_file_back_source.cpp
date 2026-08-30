// scatter-dve — WU-33c2a tests: FileBackSource (io/file_back_source.hpp/.cpp;
// DECISIONS.md ADR-095, amending WU-33c2's original single-hard-wired-
// second-live-device scope to a pluggable back source). This is the first
// of at least two concrete producers ADR-095 calls for — a static v210 file
// — split out as its own genuinely hardware-independent, sandbox-buildable-
// and-testable sibling unit, the same way WU-33c1 was already split out of
// the original WU-33c note (ADR-094).
//
// Portable, zero DeckLink dependency — this file adds one new scatter-core
// source file (src/io/file_back_source.cpp) plus this new test executable.
//
// Six checks:
//
//   1. currentSourceRaster()'s own output, read back from a real file on
//      disk, reproduces unpackSourceRaster()'s own output (WU-33c1) bit-for-
//      bit for the exact same packed bytes held only in memory — proving
//      FileBackSource::create() reads the file honestly rather than merely
//      plausibly, the same "never call the trusted path itself, only its
//      own already-independently-tested building blocks" discipline
//      tests/test_unpack_source_raster.cpp's own check 1 already uses.
//   2. currentSourceRaster() called twice returns identical content both
//      times — the "always the same static frame" contract this class's
//      own header comment documents.
//   3. create() returns std::nullopt for a path that does not exist.
//   4. create() returns std::nullopt for a file shorter than
//      video::v210::rowBytesMin(width) * height bytes (truncated).
//   5. create() returns std::nullopt for an unsupported width (odd).
//   6. create() returns std::nullopt for a non-positive height.

#include "core/resolve.hpp"
#include "io/file_back_source.hpp"
#include "video/v210.hpp"
#include "harness.hpp"
#include "testpat.hpp"

#include <cstdint>
#include <cstdio>
#include <fstream>
#include <ios>
#include <string>
#include <vector>

using namespace scatter;
using scatter::io::FileBackSource;

namespace {

// Unique-enough scratch path for this test executable alone — no other test
// translation unit reads or writes it (SESSION-PROTOCOL.md rule 2: no shared
// fixture across test translation units), and it is removed at the end of
// each test function that creates it, success or failure, so a re-run never
// sees a stale file left by a prior run.
std::string scratchPath(const char* suffix) {
    return std::string("/tmp/scatter_dve_test_file_back_source_") + suffix + ".v210";
}

std::vector<std::uint8_t> packZonePlateV210(int width, int height) {
    const testpat::Frame src = testpat::makeZonePlate(width, height);
    const std::size_t rowBytes = v210::rowBytesMin(width);
    std::vector<std::uint8_t> packed(rowBytes * std::size_t(height));
    v210::packImage(src.Y.data(), src.yStride(), src.Cb.data(), src.Cr.data(),
                     src.cStride(), width, height, packed.data(),
                     std::ptrdiff_t(rowBytes));
    return packed;
}

void writeFile(const std::string& path, const std::vector<std::uint8_t>& bytes) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    out.write(reinterpret_cast<const char*>(bytes.data()), std::streamsize(bytes.size()));
}

bool sameRaster(const SourceRaster& a, const SourceRaster& b) {
    if (a.width != b.width || a.height != b.height) return false;
    const std::size_t n = std::size_t(a.width) * std::size_t(a.height);
    for (std::size_t i = 0; i < n; ++i) {
        if (a.r[i] != b.r[i] || a.g[i] != b.g[i] || a.b[i] != b.b[i]) return false;
    }
    return true;
}

}  // namespace

// ---------------------------------------------------------------------------
// 1. Reads back bit-for-bit identical to unpackSourceRaster() on the same
//    bytes.
// ---------------------------------------------------------------------------

static void test_file_back_source_matches_unpack_source_raster_for_same_bytes() {
    constexpr int W = 64, H = 48;
    const std::string path = scratchPath("check1");

    const std::vector<std::uint8_t> packed = packZonePlateV210(W, H);
    writeFile(path, packed);

    auto provider = FileBackSource::create(path, W, H);
    CHECK(bool(provider));
    if (!provider) { std::remove(path.c_str()); return; }

    const OwnedSourceRaster viaFile = provider->currentSourceRaster();
    const OwnedSourceRaster viaMemory =
        unpackSourceRaster(packed.data(), std::ptrdiff_t(v210::rowBytesMin(W)), W, H);

    CHECK(sameRaster(viaFile.view(), viaMemory.view()));

    std::remove(path.c_str());
}

// ---------------------------------------------------------------------------
// 2. currentSourceRaster() is stable across repeated calls.
// ---------------------------------------------------------------------------

static void test_file_back_source_repeated_calls_return_the_same_frame() {
    constexpr int W = 32, H = 24;
    const std::string path = scratchPath("check2");

    writeFile(path, packZonePlateV210(W, H));

    auto provider = FileBackSource::create(path, W, H);
    CHECK(bool(provider));
    if (!provider) { std::remove(path.c_str()); return; }

    const OwnedSourceRaster first = provider->currentSourceRaster();
    const OwnedSourceRaster second = provider->currentSourceRaster();
    CHECK(sameRaster(first.view(), second.view()));

    std::remove(path.c_str());
}

// ---------------------------------------------------------------------------
// 3-6. Failure paths -- all leave create() returning std::nullopt.
// ---------------------------------------------------------------------------

static void test_file_back_source_missing_file_returns_nullopt() {
    // Not written by this test at all -- if a prior run's own check 1/2
    // scratch file happened to collide (it cannot, distinct suffixes), this
    // would be a false pass; distinct suffixes per check rule this out.
    const std::string path = scratchPath("does_not_exist");
    std::remove(path.c_str());  // defensive: guarantee absence regardless of prior state

    auto provider = FileBackSource::create(path, 64, 48);
    CHECK(!provider);
}

static void test_file_back_source_truncated_file_returns_nullopt() {
    constexpr int W = 64, H = 48;
    const std::string path = scratchPath("check4_truncated");

    std::vector<std::uint8_t> packed = packZonePlateV210(W, H);
    packed.resize(packed.size() / 2);  // deliberately short
    writeFile(path, packed);

    auto provider = FileBackSource::create(path, W, H);
    CHECK(!provider);

    std::remove(path.c_str());
}

static void test_file_back_source_unsupported_width_returns_nullopt() {
    constexpr int W = 63, H = 48;  // odd -- fails video::v210::isSupportedWidth()
    const std::string path = scratchPath("check5_odd_width");

    // Content doesn't matter -- create() must reject on the width check
    // before ever touching the file.
    writeFile(path, std::vector<std::uint8_t>(16, 0));

    auto provider = FileBackSource::create(path, W, H);
    CHECK(!provider);

    std::remove(path.c_str());
}

static void test_file_back_source_non_positive_height_returns_nullopt() {
    const std::string path = scratchPath("check6_bad_height");
    writeFile(path, std::vector<std::uint8_t>(16, 0));

    auto provider = FileBackSource::create(path, 64, 0);
    CHECK(!provider);

    std::remove(path.c_str());
}

int main() {
    test_file_back_source_matches_unpack_source_raster_for_same_bytes();
    test_file_back_source_repeated_calls_return_the_same_frame();
    test_file_back_source_missing_file_returns_nullopt();
    test_file_back_source_truncated_file_returns_nullopt();
    test_file_back_source_unsupported_width_returns_nullopt();
    test_file_back_source_non_positive_height_returns_nullopt();

    return scatter::test::summary("test_file_back_source");
}
