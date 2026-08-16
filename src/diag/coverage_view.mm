// scatter-dve — WU-22b: diagnostic coverage view, the Metal/Cocoa window
// itself (architecture.md section 8's src/diag/coverage_view.cpp; Phase 5's
// own "done when" line, "diagnostic coverage view on the Mac display").
// DECISIONS.md ADR-057 has the full design and the reasoning behind every
// choice below -- read that before changing anything here.
//
// UNVERIFIED: this cannot be compiled or run anywhere in this project's own
// Linux cloud sandbox (no Cocoa, no Metal, no AppleClang/Xcode toolchain at
// all) -- reasoned through against Apple's own documented MTKView/
// NSApplication/Metal APIs, not built or tested by the session that wrote
// it. Build and run this at your own real terminal, and report back exactly
// what happens -- see ADR-057's own "known risk points" section for what is
// most likely to need a fix on first build. The single most likely first
// bug is the fragment shader's UV convention (flagged again at
// kShaderSource below) -- if the displayed image looks vertically flipped,
// that is the fix to make first.

#include "diag/coverage_view.hpp"

#import <Cocoa/Cocoa.h>
#import <MetalKit/MetalKit.h>
#import <Metal/Metal.h>

#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

// Mirrors scatter::kWeightUnity (core/types.hpp) without pulling core/
// headers into this Objective-C++ translation unit -- ADR-057: the .mm file
// stays free of this project's own core:: headers so the platform boundary
// stays exactly at coverage_view.hpp/.mm, the same shape com_ptr.hpp/
// decklink_device.hpp already keep for the Blackmagic SDK (ADR-031). Any
// change to core::kWeightUnity must be mirrored here by hand; there is
// nothing to enforce that mechanically without breaking the boundary, so a
// static_assert cannot check this constant against the real one from here
// -- noted as a known risk point in ADR-057.
constexpr std::int32_t kWeightUnityLocal = 32768;

// weight -> [0, 255] grayscale, clipped above kWeightUnityLocal (ADR-057:
// black at 0, white at 1.0x coverage, clipped to white above).
std::uint8_t weightToGray(std::int32_t w) {
    if (w <= 0) return 0;
    if (w >= kWeightUnityLocal) return 255;
    // w in (0, kWeightUnityLocal): scale to [0, 255].
    const std::int64_t scaled = (std::int64_t(w) * 255) / kWeightUnityLocal;
    return std::uint8_t(scaled);
}

// Full-screen-triangle vertex shader (3 vertices, no vertex buffer, per-
// vertex position/uv derived from vertex_id alone) plus a grayscale-
// sampling fragment shader. Compiled at runtime via
// newLibraryWithSource:options:error: -- this project's first inline MSL,
// chosen over a bundled .metal/.metallib so coverage_view.mm stays a single
// self-contained translation unit with no Xcode build-phase/resource-copy
// step required (ADR-057).
//
// KNOWN RISK POINT: the uv computed here assumes Metal's texture origin is
// top-left and NDC y grows upward, which means uv.y is derived as
// (1 - ndc.y)/2 below to land row 0 of the weight buffer (top row, matching
// the row-major dy*destWidth+dx layout PipelineParams::weightOut already
// uses) at the top of the window. If the first build shows the image
// upside down, flip that sign here (drop the "1 -") rather than anywhere
// else -- this is the one place the row convention is decided.
const char* const kShaderSource = R"(
#include <metal_stdlib>
using namespace metal;

struct VertexOut {
    float4 position [[position]];
    float2 uv;
};

vertex VertexOut vs_main(uint vertexID [[vertex_id]]) {
    // Three vertices covering the whole screen: (-1,-1), (3,-1), (-1,3).
    float2 positions[3] = {
        float2(-1.0, -1.0),
        float2( 3.0, -1.0),
        float2(-1.0,  3.0)
    };
    VertexOut out;
    float2 p = positions[vertexID];
    out.position = float4(p, 0.0, 1.0);
    out.uv = float2((p.x + 1.0) * 0.5, (1.0 - p.y) * 0.5);
    return out;
}

fragment float4 fs_main(VertexOut in [[stage_in]],
                         texture2d<float> weightTex [[texture(0)]]) {
    constexpr sampler s(mag_filter::nearest, min_filter::nearest);
    float gray = weightTex.sample(s, in.uv).r;
    return float4(gray, gray, gray, 1.0);
}
)";

}  // namespace

// ---------------------------------------------------------------------------
// ScatterCoverageMTKView -- handles keypress-quit (Q) and window-close, both
// funnelled through one requestQuit method (ADR-054/055's own keypress-quit
// convention, applied to a Cocoa window instead of a terminal).
// ---------------------------------------------------------------------------

@interface ScatterCoverageMTKView : MTKView <NSWindowDelegate>
- (void)requestQuit;
@end

@implementation ScatterCoverageMTKView

- (BOOL)acceptsFirstResponder {
    return YES;
}

- (void)keyDown:(NSEvent*)event {
    NSString* chars = [event charactersIgnoringModifiers];
    if ([chars length] > 0) {
        unichar c = [chars characterAtIndex:0];
        if (c == 'q' || c == 'Q') {
            [self requestQuit];
            return;
        }
    }
    [super keyDown:event];
}

- (void)windowWillClose:(NSNotification*)notification {
    (void)notification;
    [self requestQuit];
}

- (void)requestQuit {
    [NSApp stop:nil];
    // [NSApp stop:] only takes effect once the run loop processes another
    // event -- a documented Cocoa gotcha. Post a dummy event so -[NSApp run]
    // actually returns immediately instead of waiting for the next real
    // user input.
    NSEvent* dummy = [NSEvent otherEventWithType:NSEventTypeApplicationDefined
                                         location:NSZeroPoint
                                    modifierFlags:0
                                        timestamp:0
                                     windowNumber:0
                                          context:nil
                                          subtype:0
                                            data1:0
                                            data2:0];
    [NSApp postEvent:dummy atStart:YES];
}

@end

// ---------------------------------------------------------------------------
// ScatterCoverageDelegate -- MTKViewDelegate, draws the current texture as a
// full-screen triangle each time the view is told to redraw (ADR-057:
// enableSetNeedsDisplay/isPaused mode, so this only fires from
// updateWeights(), never on a free-running timer).
// ---------------------------------------------------------------------------

@interface ScatterCoverageDelegate : NSObject <MTKViewDelegate>
@property(nonatomic, strong) id<MTLCommandQueue> commandQueue;
@property(nonatomic, strong) id<MTLRenderPipelineState> pipelineState;
@property(nonatomic, strong) id<MTLTexture> weightTexture;
@end

@implementation ScatterCoverageDelegate

- (void)mtkView:(MTKView*)view drawableSizeWillChange:(CGSize)size {
    (void)view;
    (void)size;
    // Fixed-size window (ADR-057) -- nothing to do here.
}

- (void)drawInMTKView:(MTKView*)view {
    id<CAMetalDrawable> drawable = view.currentDrawable;
    MTLRenderPassDescriptor* rpd = view.currentRenderPassDescriptor;
    if (!drawable || !rpd) {
        return;
    }
    id<MTLCommandBuffer> cmdBuf = [self.commandQueue commandBuffer];
    id<MTLRenderCommandEncoder> encoder =
        [cmdBuf renderCommandEncoderWithDescriptor:rpd];
    [encoder setRenderPipelineState:self.pipelineState];
    [encoder setFragmentTexture:self.weightTexture atIndex:0];
    [encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];
    [encoder endEncoding];
    [cmdBuf presentDrawable:drawable];
    [cmdBuf commit];
}

@end

// ---------------------------------------------------------------------------
// CoverageWindow::Impl
// ---------------------------------------------------------------------------

namespace scatter::diag {

struct CoverageWindow::Impl {
    NSWindow* window = nil;
    ScatterCoverageMTKView* view = nil;
    ScatterCoverageDelegate* delegate = nil;
    id<MTLDevice> device = nil;
    int width = 0;
    int height = 0;
    std::vector<std::uint8_t> grayscaleScratch;
};

CoverageWindow::CoverageWindow(const CoverageWindowConfig& config)
    : impl_(std::make_unique<Impl>()) {
    impl_->width = config.width;
    impl_->height = config.height;
    impl_->grayscaleScratch.resize(
        std::size_t(config.width) * std::size_t(config.height));

    // NSApplication must be initialised before any window is created --
    // harmless to call sharedApplication repeatedly if a future caller
    // (WU-22c) already did this once for the whole process.
    [NSApplication sharedApplication];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

    impl_->device = MTLCreateSystemDefaultDevice();
    if (!impl_->device) {
        // No Metal-capable GPU -- cannot proceed. Deliberately left as a
        // hard failure (no software-render fallback exists in this design)
        // rather than silently degrading; ADR-057 records this as accepted
        // for a diagnostic tool that only ever runs on Steve's own Mac.
        throw std::runtime_error(
            "scatter::diag::CoverageWindow: MTLCreateSystemDefaultDevice() "
            "returned nil -- no Metal-capable GPU available");
    }

    const NSRect frame = NSMakeRect(0, 0, config.width, config.height);
    const NSWindowStyleMask style = NSWindowStyleMaskTitled |
                                     NSWindowStyleMaskClosable |
                                     NSWindowStyleMaskMiniaturizable;
    // Deliberately NOT NSWindowStyleMaskResizable (ADR-057: fixed size).
    impl_->window = [[NSWindow alloc] initWithContentRect:frame
                                                  styleMask:style
                                                    backing:NSBackingStoreBuffered
                                                      defer:NO];
    [impl_->window setTitle:[NSString stringWithUTF8String:config.title]];
    [impl_->window setReleasedWhenClosed:NO];

    impl_->view = [[ScatterCoverageMTKView alloc] initWithFrame:frame
                                                          device:impl_->device];
    impl_->view.enableSetNeedsDisplay = YES;
    impl_->view.paused = YES;
    impl_->view.colorPixelFormat = MTLPixelFormatBGRA8Unorm;
    [impl_->window setContentView:impl_->view];
    [impl_->window setDelegate:impl_->view];
    [impl_->window makeFirstResponder:impl_->view];

    NSError* error = nil;
    id<MTLLibrary> library =
        [impl_->device newLibraryWithSource:[NSString stringWithUTF8String:kShaderSource]
                                     options:nil
                                       error:&error];
    if (!library) {
        throw std::runtime_error(
            std::string("scatter::diag::CoverageWindow: shader compile failed: ") +
            (error ? [[error localizedDescription] UTF8String] : "unknown error"));
    }
    id<MTLFunction> vertexFn = [library newFunctionWithName:@"vs_main"];
    id<MTLFunction> fragmentFn = [library newFunctionWithName:@"fs_main"];

    MTLRenderPipelineDescriptor* pipelineDesc =
        [[MTLRenderPipelineDescriptor alloc] init];
    pipelineDesc.vertexFunction = vertexFn;
    pipelineDesc.fragmentFunction = fragmentFn;
    pipelineDesc.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;

    id<MTLRenderPipelineState> pipelineState =
        [impl_->device newRenderPipelineStateWithDescriptor:pipelineDesc error:&error];
    if (!pipelineState) {
        throw std::runtime_error(
            std::string("scatter::diag::CoverageWindow: pipeline state failed: ") +
            (error ? [[error localizedDescription] UTF8String] : "unknown error"));
    }

    MTLTextureDescriptor* texDesc =
        [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatR8Unorm
                                                             width:NSUInteger(config.width)
                                                            height:NSUInteger(config.height)
                                                         mipmapped:NO];
    texDesc.usage = MTLTextureUsageShaderRead;
    id<MTLTexture> texture = [impl_->device newTextureWithDescriptor:texDesc];

    impl_->delegate = [[ScatterCoverageDelegate alloc] init];
    impl_->delegate.commandQueue = [impl_->device newCommandQueue];
    impl_->delegate.pipelineState = pipelineState;
    impl_->delegate.weightTexture = texture;
    // MTKView.delegate is a weak reference (documented by Apple) -- Impl
    // must hold delegate strongly itself, which it does via the @property
    // (nonatomic, strong) declarations above plus this ivar reference.
    impl_->view.delegate = impl_->delegate;
}

CoverageWindow::~CoverageWindow() {
    if (impl_ && impl_->window) {
        [impl_->window setDelegate:nil];
        [impl_->window close];
    }
}

void CoverageWindow::updateWeights(const std::int32_t* weights, int width, int height) {
    if (width != impl_->width || height != impl_->height) {
        // Precondition violated by caller -- documented as unchecked in the
        // header, but a thrown exception here is cheap and gives a much
        // clearer first-run diagnostic than silently corrupting memory, so
        // it stays in for this diagnostic-only tool (ADR-057).
        throw std::invalid_argument(
            "scatter::diag::CoverageWindow::updateWeights: size mismatch");
    }
    auto& gray = impl_->grayscaleScratch;
    const std::size_t n = std::size_t(width) * std::size_t(height);
    for (std::size_t i = 0; i < n; ++i) {
        gray[i] = weightToGray(weights[i]);
    }
    const MTLRegion region =
        MTLRegionMake2D(0, 0, NSUInteger(width), NSUInteger(height));
    [impl_->delegate.weightTexture replaceRegion:region
                                      mipmapLevel:0
                                        withBytes:gray.data()
                                      bytesPerRow:NSUInteger(width)];
    [impl_->view setNeedsDisplay:YES];
}

void CoverageWindow::run() {
    [impl_->window makeKeyAndOrderFront:nil];
    [NSApp activateIgnoringOtherApps:YES];
    [NSApp run];
}

}  // namespace scatter::diag
