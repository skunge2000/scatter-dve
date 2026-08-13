// scatter-dve — WU-03: test pattern generator
//
// Writes one of the three WU-03 patterns to a raw .v210 file. No header on
// the file: width, height and stride are metadata the caller already chose
// on the command line, consistent with the project's transport being v210
// alone, not a container format.
//
//   ./make_testpat ramp       out.v210 [width] [height]
//   ./make_testpat zoneplate  out.v210 [width] [height]
//   ./make_testpat excursion  out.v210 [width] [height]
//   ./make_testpat all        out-dir/ [width] [height]
//
// width/height default to 720x576 — Phase 1's format (ADR-007).
#include <cstdio>
#include <cstdlib>
#include <string>

#include "testpat.hpp"

namespace {

void printUsage(const char* argv0) {
    std::fprintf(stderr,
        "usage: %s <ramp|zoneplate|excursion|all> <output> [width] [height]\n"
        "  ramp | zoneplate | excursion   writes <output> as a single .v210 file\n"
        "  all                            writes ramp.v210, zoneplate.v210 and\n"
        "                                 excursion.v210 into <output>, a directory\n"
        "  width, height default to 720x576 (Phase 1's format, ADR-007)\n",
        argv0);
}

bool writeOne(const std::string& pattern, const std::string& path,
              int width, int height) {
    scatter::testpat::Frame f =
        pattern == "ramp"      ? scatter::testpat::makeRamp(width, height)
      : pattern == "zoneplate" ? scatter::testpat::makeZonePlate(width, height)
      :                          scatter::testpat::makeExcursion(width, height);

    if (!scatter::testpat::writeV210(f, path)) {
        std::fprintf(stderr, "error: could not write %s\n", path.c_str());
        return false;
    }

    const std::size_t stride = scatter::v210::rowBytesMin(width);
    std::printf("%-10s -> %s  (%dx%d, stride %zu bytes, %zu bytes total)\n",
               pattern.c_str(), path.c_str(), width, height, stride,
               stride * std::size_t(height));
    return true;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 3) {
        printUsage(argv[0]);
        return 2;
    }

    const std::string pattern = argv[1];
    const std::string output  = argv[2];
    const int width  = argc > 3 ? std::atoi(argv[3]) : 720;
    const int height = argc > 4 ? std::atoi(argv[4]) : 576;

    if (!scatter::v210::isSupportedWidth(width) || height <= 0) {
        std::fprintf(stderr, "error: unsupported width/height %dx%d\n", width, height);
        return 2;
    }
    if (pattern != "ramp" && pattern != "zoneplate" &&
        pattern != "excursion" && pattern != "all") {
        printUsage(argv[0]);
        return 2;
    }

    if (pattern == "all") {
        const std::string dir =
            (output.empty() || output.back() == '/') ? output : output + "/";
        bool ok = true;
        ok = writeOne("ramp",      dir + "ramp.v210",      width, height) && ok;
        ok = writeOne("zoneplate", dir + "zoneplate.v210", width, height) && ok;
        ok = writeOne("excursion", dir + "excursion.v210", width, height) && ok;
        return ok ? 0 : 1;
    }

    return writeOne(pattern, output, width, height) ? 0 : 1;
}
