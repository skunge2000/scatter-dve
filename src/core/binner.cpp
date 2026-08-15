// scatter-dve — WU-08: see core/binner.hpp for the design note and ADR-024
// for the choices this file makes that architecture.md leaves open.
#include "core/binner.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace scatter {

TileBins::TileBins(int destWidth, int destHeight)
    : destWidth_(destWidth),
      destHeight_(destHeight),
      tilesX_(tileCount(destWidth)),
      tilesY_(tileCount(destHeight)),
      bins_(std::size_t(tilesX_) * std::size_t(tilesY_)) {}

const std::vector<Frag>& TileBins::tile(int tx, int ty) const noexcept {
    return bins_[std::size_t(ty) * std::size_t(tilesX_) + std::size_t(tx)];
}

std::vector<Frag>& TileBins::tile(int tx, int ty) noexcept {
    return bins_[std::size_t(ty) * std::size_t(tilesX_) + std::size_t(tx)];
}

namespace {

// ADR-024: source pixel index -> continuous lattice parameter, linear
// across the whole lattice range regardless of the source raster's actual
// resolution ("the lattice covers the source raster", architecture.md
// 4.1). dim <= 1 has no meaningful span; pin to the lattice origin rather
// than divide by zero.
double pixelToLattice(double pixel, int dim) noexcept {
    if (dim <= 1) return 0.0;
    return pixel * double(kLatticeMax) / double(dim - 1);
}

// architecture.md 4.2 defines J directly in terms of the lattice's own
// (u, v) parametrisation; core/lattice.hpp's u, v range over the fixed
// [0, kLatticeMax] control-vertex index space regardless of the source
// raster's actual resolution (pixelToLattice() above). K and the
// supersampling decision need the Jacobian with respect to *source
// pixels*, not lattice-parameter units: "the local compression ratio"
// (4.2) is a statement about how many source pixels land per destination
// pixel, not about the arbitrary 129-vertex control mesh, and the two
// differ by a constant per-axis scale factor that grows with raster
// resolution (kLatticeMax / (dim - 1), the same constant pixelToLattice()
// applies going the other way) -- large enough at 1920-wide rasters
// (kLatticeMax / 1919 =~ 0.0667 per axis, =~ 0.00445 on the determinant)
// that using the raw lattice-parameter Jacobian directly would make K and
// the supersampling decision depend on source resolution instead of on
// the actual warp. The chain rule gives the pixel-space Jacobian directly
// from lattice.jacobian()'s output, with no extra lattice evaluation. See
// ADR-024.
Jacobian pixelJacobian(const Jacobian& latticeJ, int srcWidth, int srcHeight) noexcept {
    const double scaleU = (srcWidth  > 1) ? double(kLatticeMax) / double(srcWidth  - 1) : 0.0;
    const double scaleV = (srcHeight > 1) ? double(kLatticeMax) / double(srcHeight - 1) : 0.0;
    Jacobian j;
    j.dxdu = latticeJ.dxdu * scaleU;
    j.dxdv = latticeJ.dxdv * scaleV;
    j.dydu = latticeJ.dydu * scaleU;
    j.dydv = latticeJ.dydv * scaleV;
    return j;
}

// architecture.md 4.6: pick the supersampling factor N (1, 2 or 4) for one
// source sample's pixel-space Jacobian (pixelJacobian() above).
// threshold2x2 anchors to 4.6's own literal "det J > 1"; threshold4x4 and
// the hard cap are ADR-024.
int chooseSupersample(double detJ, const SupersampleConfig& ss) noexcept {
    const double mag = std::fabs(detJ);
    int n = 1;
    if (mag > ss.threshold2x2) n = 2;
    if (mag > ss.threshold4x4) n = 4;
    return std::min(n, std::max(1, ss.maxSupersample));
}

struct Colour {
    double y, cb, cr;
};

// Bilinear source-colour lookup at a fractional (px, py), clamped to the
// raster before splitting into cell + fraction so out-of-range positions
// (a supersampled sub-sample can fall up to half a pixel outside [0, dim))
// replicate the boundary sample rather than blending with whatever
// happens to sit at a wrapped or reflected index — the same edge
// convention ADR-020 and ADR-022 use for their own boundary lookups.
Colour sampleBilinear(const SourceRaster& src, double px, double py) noexcept {
    const double cx = std::clamp(px, 0.0, double(src.width - 1));
    const double cy = std::clamp(py, 0.0, double(src.height - 1));
    const int x0 = int(std::floor(cx));
    const int y0 = int(std::floor(cy));
    const int x1 = std::min(x0 + 1, src.width - 1);
    const int y1 = std::min(y0 + 1, src.height - 1);
    const double fx = cx - double(x0);
    const double fy = cy - double(y0);

    auto at = [&](const Sample* plane, int x, int yy) noexcept -> double {
        return double(plane[std::size_t(yy) * std::size_t(src.width) + std::size_t(x)]);
    };
    auto lerp2 = [&](const Sample* plane) noexcept -> double {
        const double top = at(plane, x0, y0) * (1.0 - fx) + at(plane, x1, y0) * fx;
        const double bot = at(plane, x0, y1) * (1.0 - fx) + at(plane, x1, y1) * fx;
        return top * (1.0 - fy) + bot * fy;
    };
    return Colour{lerp2(src.y), lerp2(src.cb), lerp2(src.cr)};
}

Sample toSample(double v) noexcept {
    const double lo = 0.0;
    const double hi = double(std::numeric_limits<Sample>::max());
    const double r  = std::round(std::clamp(v, lo, hi));
    return Sample(r);
}

Weight toWeight(double w) noexcept {
    const double scaled = w * double(kWeightUnity);
    const double r = std::round(std::clamp(scaled, 0.0, double(kWeightMax)));
    return Weight(r);
}

// Depth quantisation is not yet fixed by any ADR (nothing downstream reads
// Frag::z before WU-28's k-buffer) and WU-08's accept criteria do not
// exercise it; round-to-nearest, saturate at the uint16 range, near = 0,
// matching Vec3::z's "near = 0" convention (core/lattice.hpp) with the
// simplest possible mapping until a real need fixes it properly.
std::uint16_t toDepth(double z) noexcept {
    const double hi = double(std::numeric_limits<std::uint16_t>::max());
    const double r  = std::round(std::clamp(z, 0.0, hi));
    return std::uint16_t(r);
}

// ADR-024: SubPos is 12.4 fixed and unsigned, but a fragment replicated
// into a neighbouring tile can sit up to one whole pixel outside that
// tile's own [0, kTileSize) span — exactly as far as the four-bank
// splat's base+1 / base+stride corners reach (architecture.md 4.5).
// Biasing every stored coordinate by exactly one pixel (kSubPixelOne)
// gives that headroom on the low side while a fragment's ordinary,
// non-replicated position still fits comfortably under kTileSize's actual
// range (16 or 32): stored = (position_relative_to_this_tile + 1px) in
// 12.4 fixed. WU-09's splat un-biases by subtracting kSubPixelOne before
// taking the integer base cell.
SubPos encodeTileLocal(double relativePixels) noexcept {
    const double biased = (relativePixels + 1.0) * double(kSubPixelOne);
    const double hi = double(std::numeric_limits<SubPos>::max());
    const double r  = std::round(std::clamp(biased, 0.0, hi));
    return SubPos(r);
}

}  // namespace

BinStats generateFragmentsRowRange(const Lattice& lattice, const SourceRaster& src,
                                    double maxK, const SupersampleConfig& ss,
                                    std::uint8_t tag, int rowStart, int rowEnd,
                                    TileBins& outBins) {
    BinStats stats;

    // WU-16b (ADR-041): the loop bound is the caller's own row band
    // (rowStart/rowEnd); every u/v calculation below still reads
    // src.width/src.height in full (pixelToLattice(), pixelJacobian()) —
    // the same fields generateFragments() passes when it covers the whole
    // raster in one call below — so a given source pixel generates the
    // identical fragment whether this function is called once for its own
    // row band or once for the whole raster in one sweep. This is the
    // "honest fix" ADR-040 named and left for this unit: the row-loop
    // bound and the v-parameter's own denominator are two independently
    // controllable things here, not the same field, unlike the naive
    // "call generateFragments() once per band against a shortened
    // SourceRaster::height" alternative ADR-040 already ruled out.
    for (int py = rowStart; py < rowEnd; ++py) {
        for (int px = 0; px < src.width; ++px) {
            const double u0 = pixelToLattice(double(px), src.width);
            const double v0 = pixelToLattice(double(py), src.height);
            const Jacobian centreJ =
                pixelJacobian(lattice.jacobian(u0, v0), src.width, src.height);
            const double centreDet =
                centreJ.dxdu * centreJ.dydv - centreJ.dxdv * centreJ.dydu;
            const int n = chooseSupersample(centreDet, ss);
            const double invN2 = 1.0 / double(n * n);

            for (int sy = 0; sy < n; ++sy) {
                for (int sx = 0; sx < n; ++sx) {
                    ++stats.sourceSamples;

                    // Sub-cell centres within the source pixel's own unit
                    // footprint [px-0.5, px+0.5) x [py-0.5, py+0.5): for
                    // n == 1 this reduces to exactly (px, py).
                    const double subPx =
                        double(px) - 0.5 + (double(sx) + 0.5) / double(n);
                    const double subPy =
                        double(py) - 0.5 + (double(sy) + 0.5) / double(n);

                    const double u = pixelToLattice(subPx, src.width);
                    const double v = pixelToLattice(subPy, src.height);
                    const Vec3 dest = lattice.eval(u, v);

                    if (dest.x < 0.0 || dest.x >= double(outBins.destWidth()) ||
                        dest.y < 0.0 || dest.y >= double(outBins.destHeight())) {
                        ++stats.droppedOffRaster;
                        continue;
                    }

                    // n == 1: reuse centreJ, the same Jacobian that chose
                    // it, rather than evaluating jacobian() twice at an
                    // identical (u, v). n > 1: each sub-sample gets its
                    // own local pixel-space Jacobian for K, since it is no
                    // longer representative of the whole source pixel.
                    const Jacobian J = (n == 1)
                        ? centreJ
                        : pixelJacobian(lattice.jacobian(u, v), src.width, src.height);
                    const double k = densityCompensation(J, maxK);
                    const double weight = k * invN2;

                    const Colour c = sampleBilinear(src, subPx, subPy);

                    const int baseX = int(std::floor(dest.x));
                    const int baseY = int(std::floor(dest.y));
                    const double fracX = dest.x - double(baseX);
                    const double fracY = dest.y - double(baseY);

                    const int tileX = baseX / kTileSize;
                    const int tileY = baseY / kTileSize;
                    const int localX = baseX - tileX * kTileSize;
                    const int localY = baseY - tileY * kTileSize;

                    // architecture.md 4.4/4.5: the four-bank splat always
                    // touches base, base+1, base+stride and
                    // base+stride+1, regardless of the fractional
                    // position — so replication is needed whenever the
                    // base cell itself is the tile's last row/column,
                    // independent of fracX/fracY.
                    const bool straddleX =
                        (localX == kTileSize - 1) && (tileX + 1 < outBins.tilesX());
                    const bool straddleY =
                        (localY == kTileSize - 1) && (tileY + 1 < outBins.tilesY());

                    Frag frag{};
                    frag.Y = toSample(c.y);
                    frag.Cb = toSample(c.cb);
                    frag.Cr = toSample(c.cr);
                    frag.w = toWeight(weight);
                    frag.z = toDepth(dest.z);
                    frag.tag = tag;
                    frag.reserved = 0;

                    const int dtyMax = straddleY ? 1 : 0;
                    const int dtxMax = straddleX ? 1 : 0;
                    for (int dty = 0; dty <= dtyMax; ++dty) {
                        for (int dtx = 0; dtx <= dtxMax; ++dtx) {
                            Frag f = frag;
                            f.x = encodeTileLocal(double(localX) + fracX -
                                                   double(dtx * kTileSize));
                            f.y = encodeTileLocal(double(localY) + fracY -
                                                   double(dty * kTileSize));
                            outBins.tile(tileX + dtx, tileY + dty).push_back(f);
                            if (dtx == 0 && dty == 0) {
                                ++stats.primaryFragments;
                            } else {
                                ++stats.replicaFragments;
                            }
                        }
                    }
                }
            }
        }
    }

    return stats;
}

// WU-16b: a thin wrapper — the whole raster is exactly the row range
// [0, src.height). Signature and behaviour are exactly WU-08's frozen
// ones, unchanged; see ADR-041.
BinStats generateFragments(const Lattice& lattice, const SourceRaster& src,
                            double maxK, const SupersampleConfig& ss,
                            std::uint8_t tag, TileBins& outBins) {
    return generateFragmentsRowRange(lattice, src, maxK, ss, tag, 0, src.height, outBins);
}

}  // namespace scatter
