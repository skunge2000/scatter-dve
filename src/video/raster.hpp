// scatter-dve — WU-05: plane descriptors and raw .v210 file I/O
//
// Two things live here because they are one concern: every function added by
// this unit (v210::unpack/packImage already existed; readV210File and
// writeV210File are new) takes the same (pointer, stride, width, height)
// group per plane, repeated three times, and that repetition is what this
// file removes. `Plane`/`ConstPlane` bundle one plane's descriptor; nothing
// here computes on samples.
//
// `readV210File`/`writeV210File` are declared here and implemented in
// src/io/file_source.cpp and src/io/file_sink.cpp respectively — a
// deviation from WU-05's file list in WORK-UNITS.md, which named those two
// .cpp files but not headers for them. Declaring them next to the plane
// descriptors they're expressed in avoids two near-empty extra headers for
// one function each; see HANDOFF.md for this session. This does not reopen
// ADR-013's core/video split: both .cpp files use only <fstream>, no
// Blackmagic SDK, so they compile into `scatter-core` (see ADR-021) even
// though the module-layout sketch in docs/architecture.md section 8 places
// io/ under the `scatter` application target.
#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "core/types.hpp"
#include "video/v210.hpp"

namespace scatter::video {

// ---------------------------------------------------------------------------
// Plane descriptors
//
// Self-describing: width/height travel with the pointer and stride, unlike
// v210.hpp/chroma.hpp's row/image functions, which take a single width and
// height for all three planes and rely on the caller to have already picked
// the right one (full width for Y and 4:4:4 chroma, v210::chromaWidth(width)
// for 4:2:2 chroma). Two planes of the same image legitimately have
// different width and height (4:2:2 chroma vs luma) or the same (4:4:4, or
// any 4:2:2 plane against another of the same kind) — nothing here assumes
// either.
// ---------------------------------------------------------------------------

struct Plane {
    Sample*        data           = nullptr;
    std::ptrdiff_t strideSamples  = 0;
    int            width          = 0;
    int            height         = 0;
};

struct ConstPlane {
    const Sample*  data           = nullptr;
    std::ptrdiff_t strideSamples  = 0;
    int            width          = 0;
    int            height         = 0;

    ConstPlane() = default;
    ConstPlane(const Sample* d, std::ptrdiff_t s, int w, int h) noexcept
        : data(d), strideSamples(s), width(w), height(h) {}
    // Implicit: every call site that has a mutable Plane and needs a
    // ConstPlane (writeV210File takes only ConstPlane) should not have to
    // spell out a conversion.
    ConstPlane(const Plane& p) noexcept
        : data(p.data), strideSamples(p.strideSamples),
          width(p.width), height(p.height) {}
};

// ---------------------------------------------------------------------------
// Raster422 — owning 4:2:2 frame, tight-packed (stride == plane width).
//
// Structurally the same as tools/testpat.hpp's Frame (ADR-019), which stays
// where it is: that one is shared only by the tool and its own test. This is
// the general-purpose version for everything else, starting with this unit's
// file I/O.
// ---------------------------------------------------------------------------

struct Raster422 {
    std::vector<Sample> Y, Cb, Cr;
    int width = 0, height = 0;

    Raster422(int w, int h)
        : Y(std::size_t(w) * std::size_t(h)),
          Cb(std::size_t(v210::chromaWidth(w)) * std::size_t(h)),
          Cr(std::size_t(v210::chromaWidth(w)) * std::size_t(h)),
          width(w), height(h) {}

    Plane planeY() noexcept  { return {Y.data(),  width,                    width,                   height}; }
    Plane planeCb() noexcept { return {Cb.data(), v210::chromaWidth(width), v210::chromaWidth(width), height}; }
    Plane planeCr() noexcept { return {Cr.data(), v210::chromaWidth(width), v210::chromaWidth(width), height}; }

    ConstPlane planeY() const noexcept  { return {Y.data(),  width,                    width,                   height}; }
    ConstPlane planeCb() const noexcept { return {Cb.data(), v210::chromaWidth(width), v210::chromaWidth(width), height}; }
    ConstPlane planeCr() const noexcept { return {Cr.data(), v210::chromaWidth(width), v210::chromaWidth(width), height}; }
};

// ---------------------------------------------------------------------------
// Raster444 — owning 4:4:4 frame, tight-packed. All three planes share
// `width`: the shape chroma is in between chroma::upsampleImage and
// chroma::downsampleImage (ADR-005).
// ---------------------------------------------------------------------------

struct Raster444 {
    std::vector<Sample> Y, Cb, Cr;
    int width = 0, height = 0;

    Raster444(int w, int h)
        : Y(std::size_t(w) * std::size_t(h)),
          Cb(std::size_t(w) * std::size_t(h)),
          Cr(std::size_t(w) * std::size_t(h)),
          width(w), height(h) {}

    Plane planeY() noexcept  { return {Y.data(),  width, width, height}; }
    Plane planeCb() noexcept { return {Cb.data(), width, width, height}; }
    Plane planeCr() noexcept { return {Cr.data(), width, width, height}; }

    ConstPlane planeY() const noexcept  { return {Y.data(),  width, width, height}; }
    ConstPlane planeCb() const noexcept { return {Cb.data(), width, width, height}; }
    ConstPlane planeCr() const noexcept { return {Cr.data(), width, width, height}; }
};

// ---------------------------------------------------------------------------
// Raw .v210 file I/O — implemented in src/io/file_source.cpp and
// src/io/file_sink.cpp. No file header: width and height are supplied by the
// caller, exactly as v210::unpackImage/packImage require them as parameters
// rather than deriving them (the project's transport is v210 alone, not a
// container format — tools/make_testpat.cpp's usage comment makes the same
// point for its own output).
//
// Both assume the file's row stride is v210::rowBytesMin(width) — the only
// stride this project's own writers, this one and tools/testpat.hpp's
// writeV210, ever produce. A file padded to a different stride is out of
// scope for this unit.
//
// Preconditions, none checked at runtime beyond width/height/open/read-length
// (which do return false rather than trip UB): Cb.strideSamples ==
// Cr.strideSamples, matching v210::unpackImage/packImage's single shared
// chroma stride parameter.
// ---------------------------------------------------------------------------

// Reads exactly v210::rowBytesMin(width) * height bytes from `path` and
// unpacks them into Y/Cb/Cr. Returns false, leaving the planes unmodified,
// if the file cannot be opened, is shorter than expected, or width/height
// fail v210::isSupportedWidth / height > 0.
bool readV210File(const std::string& path, int width, int height,
                   Plane Y, Plane Cb, Plane Cr);

// Packs Y/Cb/Cr and writes exactly v210::rowBytesMin(width) * height bytes
// to `path`, truncating any existing file. Returns false, writing nothing
// durable, if the file cannot be opened for writing, the write is short, or
// width/height fail the same checks as readV210File.
bool writeV210File(const std::string& path, int width, int height,
                    ConstPlane Y, ConstPlane Cb, ConstPlane Cr);

}  // namespace scatter::video
