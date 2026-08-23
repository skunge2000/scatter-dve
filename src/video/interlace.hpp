// scatter-dve — WU-23a: field split and interleave (Phase 6; architecture.md
// section 5's own "Interlace" note: "de-interlace to frames for warping,
// re-interlace on output... Provide a field mode that warps each field
// independently... what Mirage actually did"; module layout section 8 names
// this video/interlace.hpp/.cpp)
//
// WU-23's own WORK-UNITS.md entry named two genuinely different code paths
// under one bare line: de-interlace-to-frame (a real temporal reconstruction
// filter -- Steve's own stated preference is Weston 3-field, for period
// accuracy -- followed by a trivial re-interlace decimate on the way out)
// and field mode (no reconstruction at all: warp each field independently
// at its own native vertical sampling, then interleave the two
// independently-warped results back into one frame). The scoping session
// that opened this unit split them: this is WU-23a, field mode's own half;
// de-interlace-to-frame is WU-23b, not started, gated on actually working
// out the Weston 3-field algorithm first (its own research/ADR, not yet
// done).
//
// Further split discovered while building, not before this session: field
// mode itself splits again. This file is field mode's own data-layout half
// only -- split an interlaced frame into its two field-native rasters, and
// recombine two independently-produced field rasters back into one
// interlaced frame -- a pure row copy, no lattice, no warp, no colour
// change (I2's "everything else passes through untouched" extended to the
// field-split step itself). It does NOT yet drive the lattice/warp pipeline
// per field: doing that honestly means source pixel (px, py) within one
// extracted field must map to the *same* lattice parameter v it would if
// the whole frame were warped as one raster -- so the two fields' outputs
// interleave with the correct half-line vertical phase between them, not
// two independently-renormalised copies of the same shape -- and
// core/binner.hpp's generateFragmentsRowRange() already keys its own
// v-parameter denominator to src.height in full (see its own comment on
// why: "every u/v lattice-parameter calculation stays keyed to
// src.width/src.height in full... never to a caller-shortened
// SourceRaster"), which is exactly the field-height-vs-frame-height
// mismatch a naive "extract a half-height field raster, run today's
// unchanged generateFragments() on it" approach would hit. That needs a new
// core/binner.hpp/.cpp sibling entry point (the same "new sibling, not a
// changed one" shape WU-16b/WU-28c already established for that file),
// which together with this file's own two would exceed
// SESSION-PROTOCOL.md's "3 source files" cap for one unit -- so it is
// WU-23a2, not this unit. See WORK-UNITS.md.
#pragma once

#include "video/raster.hpp"

namespace scatter::video {

// Which set of an interlaced frame's rows a field is made of. Top is the
// first-transmitted field and owns row 0 (and, for an odd frame height, the
// extra row -- see fieldRowCount() below); Bottom owns the rows in between.
enum class FieldParity {
    Top,     // frame rows 0, 2, 4, ...
    Bottom,  // frame rows 1, 3, 5, ...
};

// How many of a frameHeight-row frame's rows belong to `parity`. Top and
// Bottom always account for every row between them exactly once -- no row
// lost, none double-counted, the same discipline core/binner.hpp's
// BinStats holds fragment generation to, applied here to the field-split
// step. Every broadcast standard this project targets (576i, 1080i) has an
// even frame height, where the two counts are equal; an odd frameHeight is
// not a case this project's own formats produce, but the split is still
// exact for one -- Top gets the extra row (it owns row 0 and steps by 2, so
// it reaches the last row of an odd-height frame before Bottom does).
constexpr int fieldRowCount(int frameHeight, FieldParity parity) noexcept {
    return parity == FieldParity::Top ? (frameHeight + 1) / 2 : frameHeight / 2;
}

// Copies field-parity rows only, from a full-height interlaced frame
// (`frame`) into a field-native raster (`outField`) -- a pure data-layout
// copy, no resampling, no colour change. Row fy of outField is row
// (2*fy + (parity == Bottom ? 1 : 0)) of frame.
//
// Precondition, unchecked (this codebase's own convention for
// caller-sized output, e.g. core/resolve.hpp's runFrame() dest): outField
// is already sized fieldRowCount(frame.height, parity) rows by frame.width
// columns.
void extractField(const Raster444& frame, FieldParity parity, Raster444& outField);

// Inverse of extractField(), both parities at once: writes topField's own
// row fy into dest row 2*fy, and bottomField's own row fy into dest row
// 2*fy+1 -- a pure data-layout copy, the exact inverse operation
// extractField() performs, no resampling, no colour change.
//
// Preconditions, unchecked: topField.width == bottomField.width ==
// dest.width; dest is already sized so dest.height == topField.height +
// bottomField.height (the only relationship fieldRowCount() ever produces
// from one common frameHeight -- topField.height == bottomField.height, or
// topField.height == bottomField.height + 1 for an odd frameHeight).
void interleaveFields(const Raster444& topField, const Raster444& bottomField,
                       Raster444& dest);

}  // namespace scatter::video
