// scatter-dve — WU-22b: diagnostic coverage view, the Metal/Cocoa window
// itself (architecture.md section 8's src/diag/coverage_view.cpp; Phase 5's
// own "done when" line, "diagnostic coverage view on the Mac display").
// DECISIONS.md ADR-057 has the full design and the reasoning behind every
// choice below -- read that before changing anything here. WU-22c (ADR-058)
// adds requestQuit() below, wiring this window into the live capture/output
// pipeline -- see ADR-058 for that unit's own design. WU-22c's own
// same-session follow-up (ADR-058's own addendum, after Steve's first real
// build/run reported the coverage window not responding to any control key
// but Q) adds SpecialKey and setKeyHandler() below: keyboard focus is
// per-window in macOS, so the terminal's own stdin channel this file's own
// header comment already describes never receives a keystroke typed while
// the coverage window itself has focus -- setKeyHandler() is how a caller
// (WU-22c) lets this window's own -keyDown: drive the same application
// logic the terminal channel already does, so either window can be
// clicked into and used.
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
#include <functional>
#include <memory>

namespace scatter::diag {

// WU-22c follow-up: the four arrow keys, named here rather than left as
// raw characters, since Cocoa's own NSEvent already decodes them fully
// (see coverage_view.mm's own -keyDown: for the NSUpArrowFunctionKey-style
// constants this maps from) -- unlike the terminal's own ESC-sequence
// encoding, which is why this window's own key handling needs no
// byte-at-a-time state machine the way a caller's own stdin channel might.
enum class SpecialKey { None, ArrowUp, ArrowDown, ArrowLeft, ArrowRight };

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
    // could relax. WU-22c (ADR-058) is the caller that marshals a
    // background-thread-produced buffer onto the main thread before calling
    // this, via dispatch_async(dispatch_get_main_queue(), ...) -- which is
    // also where ADR-057's own "double-buffer / atomic swap" threading
    // answer actually lives: a background thread fills a fresh buffer and
    // hands ownership of it across in one dispatch_async block, so
    // CoverageWindow itself only ever sees one buffer at a time, on one
    // thread, with no locking of its own.
    void updateWeights(const std::int32_t* weights, int width, int height);

    // Shows the window and runs the Cocoa event loop until the user presses
    // Q or closes the window, or until requestQuit() below is called from
    // any thread -- the same keypress-quit convention this project's own
    // live-sphere demo already established (ADR-054/055). Blocks the
    // calling thread; call this last, after any setup and the first
    // updateWeights() call.
    void run();

    // WU-22c (ADR-058): requests that run() return, as if the user had
    // pressed Q in the coverage window or closed it -- the mechanism by
    // which a *different* quit channel (tests/test_decklink_live_sphere.cpp's
    // own stdin keypress loop, unified with this window's own Cocoa run
    // loop per ADR-058) can also end this window's run() call, so that one
    // Q, from either channel, quits the whole live-sphere session.
    //
    // Safe to call from any thread. Internally this forwards to the same
    // -requestQuit ObjC method ScatterCoverageMTKView's own -keyDown: and
    // -windowWillClose: already call (coverage_view.mm) -- [NSApp stop:nil]
    // plus a dummy posted event are both documented by Apple as safe to
    // call from any thread; there is no new thread-safety obligation this
    // method takes on beyond what that existing mechanism already provides.
    // A no-op if run() has not been called yet or has already returned.
    void requestQuit();

    // WU-22c follow-up: called from the main thread only (Cocoa's own
    // -keyDown: dispatch, the same thread run()/updateWeights() already
    // require), once per keydown this window's own view receives while it
    // has keyboard focus -- for every key except Q or a window-close,
    // which this class continues to handle internally as quit, unchanged
    // since WU-22b/WU-22c; `handler` is never invoked for either of those.
    //
    // Exactly one of the two parameters carries information on any one
    // call: for a plain character key, `asciiChar` is the character Cocoa
    // reported (e.g. 'X', 'x', 'a', ...) and `special` is SpecialKey::None;
    // for one of the four arrow keys, `asciiChar` is '\0' and `special`
    // names which arrow. This class has no vocabulary of its own for what
    // any particular key should *do* -- unlike the terminal channel's own
    // Key enum (an application-level concept, defined in
    // tests/test_decklink_live_sphere.cpp, not here) -- so an unrecognized
    // key (anything not Q, not an arrow, not one of a caller's own
    // meaningful letters) still reaches `handler` with whatever character
    // Cocoa reported and SpecialKey::None; deciding what that means, if
    // anything, is entirely the caller's own job.
    //
    // Default (no handler set): keydowns other than Q are silently
    // ignored, matching this class's own behavior before this method
    // existed. Safe to call from any thread; for a defined effect on the
    // very next keydown, call this before run() -- a handler installed
    // while run() is already blocking takes effect starting with the next
    // keydown event after it is set, not retroactively.
    using KeyHandler = std::function<void(char asciiChar, SpecialKey special)>;
    void setKeyHandler(KeyHandler handler);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace scatter::diag
