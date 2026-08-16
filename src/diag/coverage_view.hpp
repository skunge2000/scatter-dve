// scatter-dve — WU-22b: diagnostic coverage view, the Metal/Cocoa window
// itself (architecture.md section 8's src/diag/coverage_view.cpp; Phase 5's
// own "done when" line, "diagnostic coverage view on the Mac display").
// DECISIONS.md ADR-057 has the full design and the reasoning behind every
// choice below -- read that before changing anything here.
//
// This project's first Objective-C++ (.mm) file and first Cocoa/Metal
// dependency of any kind -- see ADR-057 for why this header stays plain,
// portable C++20 (no Objective-C type, no Apple framework #include) while
// coverage_view.mm alone carries the actual AppKit/MetalKit/Metal
// dependency, the same "keep the platform dependency behind the .cpp/.mm
// boundary" shape com_ptr.hpp/decklink_device.hpp already use for the
// Blackmagic SDK (ADR-031), applied here to a different platform surface.
//
// UNVERIFIED: this cannot be compiled or run anywhere in this project's own
// Linux cloud sandbox (no Cocoa, no Metal, no AppleClang/Xcode toolchain at
// all) -- reasoned through against Apple's own documented MTKView/
// NSApplication/Metal APIs, not built or tested by the session that wrote
// it. Build and run this at your own real terminal, and report back exactly
// what happens -- see ADR-057's own "known risk points" section for what is
// most likely to need a fix on first build.
#pragma once

#include <cstdint>
#include <memory>

namespace scatter::diag {

// Fixed at construction -- CoverageWindow does not support resizing itself
// (ADR-057: a fixed size, matching the weight buffer's own width/height
// exactly, needs no scaling pass between buffer and window pixels, which is
// both simpler to get right on a first Metal unit and lower per-frame cost
// than a resizable window would be).
struct CoverageWindowConfig {
    int width = 0;
    int height = 0;
    // Not copied past construction -- must outlive the CoverageWindow, the
    // same "caller owns it, we only point at it" convention this project's
    // own non-owning fields already use (PipelineParams::pool/weightOut,
    // ADR-044/056).
    const char* title = "scatter-dve \xE2\x80\x94 coverage view";
};

// A single Cocoa/Metal window rendering a captured weight buffer
// (core/resolve.hpp's PipelineParams::weightOut, WU-22a/ADR-056) as
// grayscale: black at weight 0, white at kWeightUnity (1.0x coverage),
// clipped to white above kWeightUnity (ADR-057).
//
// Not copyable or movable -- owns real Cocoa/Metal resources (a window, a
// Metal device and command queue, a texture) with no defined "what does it
// mean to copy or move an open window" semantics this project needs, the
// same "resource-owning class, no copy/move" shape ThreadPool
// (core/pipeline.hpp, ADR-040) already uses for a different
// non-trivially-duplicable resource.
class CoverageWindow {
public:
    explicit CoverageWindow(const CoverageWindowConfig& config);
    ~CoverageWindow();

    CoverageWindow(const CoverageWindow&) = delete;
    CoverageWindow& operator=(const CoverageWindow&) = delete;
    CoverageWindow(CoverageWindow&&) = delete;
    CoverageWindow& operator=(CoverageWindow&&) = delete;

    // Replaces the displayed buffer and schedules a redraw (ADR-057:
    // MTKView's own enableSetNeedsDisplay/isPaused mode -- the window only
    // redraws when this is called, never on a free-running timer). `weights`
    // must point to exactly `width * height` tight-packed, row-major
    // std::int32_t values (core/resolve.hpp's own PipelineParams::weightOut
    // layout) where width/height match this window's own
    // CoverageWindowConfig exactly -- an unchecked caller precondition, the
    // same convention this codebase already uses throughout (Lattice::at()'s
    // row/col bounds, ThreadPool::runOnAll()'s "fn must not throw").
    //
    // MUST be called from the main thread only. This is a hard Cocoa/AppKit
    // requirement (documented by Apple: AppKit UI work and Metal drawable
    // presentation are main-thread-only), not a design choice this class
    // could relax. A future caller running on a different thread (WU-22c,
    // wiring this to the live capture pipeline, not built by this unit) must
    // marshal onto the main thread itself before calling this -- e.g.
    // dispatch_async(dispatch_get_main_queue(), ...) -- which is also where
    // ADR-057's own "double-buffer / atomic swap" threading answer actually
    // lives: a background thread fills a fresh buffer and hands ownership of
    // it across in one dispatch_async block, so CoverageWindow itself only
    // ever sees one buffer at a time, on one thread, with no locking of its
    // own.
    void updateWeights(const std::int32_t* weights, int width, int height);

    // Shows the window and runs the Cocoa event loop until the user presses
    // Q or closes the window -- the same keypress-quit convention this
    // project's own live-sphere demo already established (ADR-054/055).
    // Blocks the calling thread; call this last, after any setup and the
    // first updateWeights() call.
    void run();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace scatter::diag
