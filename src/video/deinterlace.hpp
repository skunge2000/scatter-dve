// scatter-dve — WU-23b1: Weston 3-field de-interlace, filter core
// (docs/architecture.md section 3's own "[de-interlace to frame]" stage;
// module layout section 8 names this video/, alongside interlace.hpp/
// chroma.hpp — video-format-level processing, no lattice, no warp, no
// DeckLink dependency, DECISIONS.md ADR-078).
//
// Implements libavfilter/vf_w3fdif.c's own frame-rate mode ("Weston 3
// Field De-Interlacing Filter" -- Jim Easterbrook for BBC R&D, based on
// the process described by Martin Weston -- not vf_bwdif.c, a materially
// more complex, later filter Steve's own preference has ruled out for
// period accuracy). See DECISIONS.md ADR-078 (scoping, source
// confirmation, multi-frame-history resolution) and ADR-079 (this unit's
// own build: exact interface, and a data-flow correction) for the full
// account. CORRECTIONS.md C-025 records why the data flow below differs
// from ADR-078's own first description.
//
// push() takes one full-height ("weave") interlaced frame at a time --
// the same shape video::extractField()'s own `frame` parameter documents
// ("a full-height interlaced frame"), both field parities' rows genuinely
// present -- not a half-height, already-split field. This is required by
// the algorithm itself, not a design preference: the temporal high-pass
// term reads the *current* frame's own missing-parity rows (real data,
// since a weave frame carries both parities) alongside the *previous*
// frame's -- a field-native, single-parity input has no missing-parity
// rows to supply that half of the computation at all (C-025).
//
// One instance always reconstructs the same fixed field parity (chosen at
// construction) as its output's own anchor -- that parity's rows are
// copied through unchanged every push; the other parity's rows are
// synthesised every push. Fully reusable by any caller with a sequence of
// weave frames to feed it: the live-capture path (WU-23b2, not this unit)
// and any future file-sequence driver alike, with zero DeckLink or
// lattice/pipeline dependency of its own to thread through either
// (ADR-078).
#pragma once

#include <cstddef>
#include <optional>

#include "video/interlace.hpp"
#include "video/raster.hpp"

namespace scatter::video {

// Which of the real source's two coefficient sets to use -- ADR-078/079's
// own "simple" (2 low-pass + 3 high-pass taps) versus "complex" (4 + 5
// taps) -- fixed at construction; no caller in this project needs it to
// vary mid-stream.
enum class DeinterlaceCoefficients {
    Simple,
    Complex,
};

// Owns its own three-frame (prev/cur/next) history internally -- the
// direct analogue of libavfilter/vf_w3fdif.c's own W3FDIFContext fields of
// the same names, including their own stream-start convention (see
// push() below) -- so a caller only ever needs to feed it frames in
// sequence, never manage history itself.
class Deinterlacer {
public:
    Deinterlacer(FieldParity anchorParity, DeinterlaceCoefficients coeffs) noexcept;

    // Pushes one full-height weave frame (both parities' own real rows
    // present -- video::extractField()'s own "full-height interlaced
    // frame" shape). Shifts the internal prev/cur/next history exactly as
    // libavfilter/vf_w3fdif.c's own filter_frame() does (DECISIONS.md
    // ADR-079): the first call ever made has no prev yet and returns
    // false, leaving outFrame untouched. Every call from the second
    // onward returns true and writes one reconstructed full-height
    // progressive frame into outFrame -- for the very first output only,
    // reconstructed using a duplicate of its own single available frame
    // as its own temporal partner (the real source's own
    // duplicate-as-own-prev stream-start convention, reproduced exactly);
    // from the second output onward, using the previously-pushed frame as
    // the genuine temporal partner.
    //
    // Precondition, unchecked (this codebase's own convention for
    // caller-sized output -- e.g. core/resolve.hpp's runFrame() dest):
    // outFrame is already sized to weaveFrame's own width/height, and
    // every weaveFrame pushed across this instance's own lifetime shares
    // one consistent width/height.
    bool push(const Raster444& weaveFrame, Raster444& outFrame);

private:
    FieldParity anchorParity_;
    DeinterlaceCoefficients coeffs_;

    // Raster444 has no default constructor (its own only constructor
    // takes width/height), so these three slots need an explicit "not
    // holding a frame yet" state; std::optional is core/ring_buffer.hpp's
    // own tool for exactly that, reused here for internal state rather
    // than as push()'s own return type (DECISIONS.md ADR-079 explains the
    // distinction between the two uses).
    std::optional<Raster444> prev_, cur_, next_;
};

}  // namespace scatter::video
