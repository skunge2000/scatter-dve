// scatter-dve — WU-33c2a: a static v210 file as a pluggable "back" source,
// standing in for a second live capture feed on the back side of a
// self-folding surface (PipelineParams::backSrc, WU-33a/WU-33b, DECISIONS.md
// ADR-092/ADR-093).
//
// DECISIONS.md ADR-095 amends WU-33c2's original scope (a single, hard-wired
// second live DeckLink device) to a *pluggable* back source, config-
// selectable among at least two concrete producers. This is the first —
// split out as its own sibling unit, WU-33c2a, for exactly the reason
// WU-33c1 was already split out of the original WU-33c note (ADR-094): it is
// genuinely hardware-independent, so it is the one sandbox-buildable-and-
// testable this session, versus WU-33c2b's Blackmagic-capture-sub-device
// producer, device-bridge hand-off only, not built this session.
//
// Built directly on WU-33c1's own unpackSourceRaster()/OwnedSourceRaster
// (core/resolve.hpp/core/pipeline.cpp) — this class does no v210/chroma/RGB
// conversion work of its own beyond a plain file read; everything past the
// raw bytes reuses that already-tested primitive.
//
// Pluggability itself is not a virtual interface — this codebase has no
// precedent of its own for one (IDeckLinkInputCallback and friends are COM
// interfaces the Blackmagic SDK mandates, not a pattern this project chose
// for its own internal seams). Instead this class exposes a plain
// zero-argument accessor matching CaptureConsumer::CoverageCallback's own
// std::function shape exactly (io/decklink_capture_consumer.hpp, WU-22c,
// DECISIONS.md ADR-058): WU-33c3's future front-consumer wiring (not this
// unit) is expected to take any std::function<OwnedSourceRaster()> as its
// back-source callable, and this class's own currentSourceRaster() matches
// that signature directly — a caller wraps
// `[&fileSource]{ return fileSource.currentSourceRaster(); }` (or binds it)
// to get one. WU-33c2b's future DeckLink-linked producer is expected to
// expose the same signature, for the same reason.
//
// See DECISIONS.md ADR-095 for the full account of why this accessor
// returns a real OwnedSourceRaster copy rather than a cheap SourceRaster
// view (as OwnedSourceRaster::view() itself does): a live-capture producer's
// own buffer is concurrently mutated by its own consumer thread, so *that*
// producer's equivalent accessor must already return a genuine snapshot
// copy, not a view into memory that could change mid-warp. Returning
// OwnedSourceRaster uniformly from every concrete producer — including this
// always-static one — keeps the callable's own safety contract identical
// regardless of which concrete producer sits behind it, at the deliberate,
// named cost of one avoidable buffer copy per query for this producer's own
// unchanging case.
#pragma once

#include "core/resolve.hpp"  // OwnedSourceRaster, unpackSourceRaster()

#include <optional>
#include <string>

namespace scatter::io {

// Reads one static v210 frame from `path` — packed, tight-stride
// (v210::rowBytesMin(width) bytes per row), width x height, no file
// header of any kind — once, at construction via create() below, and hands
// back a copy of that same frame's own OwnedSourceRaster from every
// currentSourceRaster() call thereafter.
//
// WU-33c2a's own scope is a single static frame only (Steve's own choice,
// this session, alongside the decision to split a future looped/advancing
// sequence out as its own separate sibling unit rather than build it now —
// see WORK-UNITS.md's own WU-33c2a entry and DECISIONS.md ADR-095). A
// caller wanting genuine video-rate playback from a file, not a fixed still,
// needs that future unit, not this class.
class FileBackSource {
public:
    // Returns std::nullopt if the file could not be read at the given
    // width/height: `path` does not exist or cannot be opened, the file is
    // shorter than v210::rowBytesMin(width) * height bytes, or width
    // fails v210::isSupportedWidth() (even and positive) or height is
    // not positive — the same failure conditions video::readV210File()
    // (video/raster.hpp, WU-05) already checks for an equivalent shape,
    // deliberately not reused here (see file_back_source.cpp for why).
    static std::optional<FileBackSource> create(const std::string& path, int width, int height);

    // Matches the std::function<OwnedSourceRaster()> shape WU-33c3's future
    // front-consumer wiring is expected to take (DECISIONS.md ADR-095) —
    // see this header's own file comment above for why this returns a real
    // copy, not a view. Always returns a copy of the same frame read at
    // construction — this class's own always-static scope; the returned
    // object's own view() is safe to use for as long as the returned copy
    // itself is kept alive, independent of this FileBackSource instance.
    OwnedSourceRaster currentSourceRaster() const;

private:
    explicit FileBackSource(OwnedSourceRaster frame);

    OwnedSourceRaster m_frame;
};

}  // namespace scatter::io
