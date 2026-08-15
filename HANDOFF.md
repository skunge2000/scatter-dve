# Handoff
Overwritten at the end of every session. This is the first thing to read.

---

**Session:** 25
**Tag:** `wu-19a-green` stood, confirmed at session open (Phase 4 fully
closed out per session 24's own handoff). `wu-20a-green` is NOT yet
tagged — this session's own work is implemented and verified in full in
the sandbox, but per this project's own "the assistant does not run
`close.sh`" rule, tagging is Steve's own next action; see "What to run at
your terminal," below.
**Phase:** 5 (Live capture) is now under way. WU-20a (ring buffer) is done
in the sandbox this session, `wip` pending Steve's real-terminal close;
WU-20b (DeckLink capture object) is scoped as a sketch but deliberately not
built this session — needs the real SDK/AppleClang, same gap as WU-14/
WU-15a. `DECISIONS.md` now runs through ADR-046; `CORRECTIONS.md`
unchanged this session, still through C-015.

## This session in full

**Reading, before anything else.** Per `SESSION-PROTOCOL.md`'s own reading
table: `HANDOFF.md`, `INVARIANTS.md`, `DECISIONS.md` in full (through
ADR-045), `CORRECTIONS.md` (through C-015), `WORK-UNITS.md`. Then, per this
project's own established practice for a new hardware surface (WU-14/
ADR-031, WU-15a/ADR-032 — read the real SDK before scoping, not
`architecture.md`'s own summary, which ADR-039 already flags as describing
the superseded single-full-duplex-device design): the real Blackmagic
DeckLink SDK's own `DeckLinkAPI.h` (`IDeckLinkInput`, `IDeckLinkInputCallback`,
`VideoInputFrameArrived`, `VideoInputFormatChanged`, `EnableVideoInput`,
`StartStreams`/`StopStreams`, `bmdVideoInputEnableFormatDetection`'s own
operational implications, `IDeckLinkVideoInputFrame`/`IDeckLinkVideoBuffer`'s
own split — `GetBytes()` is not directly on the frame, extending ADR-032's
own output-side finding to input), and `Mac/Samples/`'s three real capture
samples (`CaptureStills`, `InputLoopThrough`, `CapturePreview` — the
`CapturePreview` guess flagged going in was confirmed real). Also reread
this project's own `src/io/decklink_device.hpp`/`.cpp`,
`src/io/com_ptr.hpp` (ADR-031 in full), `src/io/decklink_output.hpp`/`.cpp`
(WU-15a) for established idioms, and `architecture.md` sections 3, 6, 7, 9,
12 plus ADR-037/039 (real target: **UltraStudio Recorder 3G**, not 4K Mini).

**The split decision.** `WORK-UNITS.md`'s own WU-20 line named three
pieces — format-detection-aware `EnableVideoInput`, a capture callback
implementing `IDeckLinkInputCallback` with correct frame retention, and a
ring buffer — with no `Files:`/`Accept:` scoping at all. The real-SDK
reading found these do not share a verifiability profile: the first two
need the Blackmagic SDK and AppleClang/Xcode to build or even compile at
all (the same gap ADR-031/032 already established for WU-14/WU-15a), while
the ring buffer architecture.md 6 requires ("never blocks, never
allocates," on the capture callback thread) has zero DeckLink or platform
dependency — ordinary portable C++20, the same as every other `core/`
file — and, notably, **no real SDK sample this project read actually
provides one**: `CaptureStills` hands frames off via a
`std::queue`/`std::mutex`/`std::condition_variable` (allocates every push,
would block a producer against a bounded queue, though that sample never
bounds its own); `InputLoopThrough` and `CapturePreview` both invoke a
`std::function` callback synchronously on the driver's own callback thread,
which is not "push and return immediately" at all. architecture.md's own
requirement is stricter than anything sampled, so the ring buffer is this
project's own design, not adapted from an SDK idiom — and unlike WU-14/
WU-15a, it is fully buildable and runnable, including under
ThreadSanitizer with a real concurrent producer and consumer, in this
project's own Linux cloud sandbox. Split into WU-20a (ring buffer, built
this session) and WU-20b (the DeckLink-specific capture object, sketched
but deferred), the same "split after reading, not before or by guessing"
discipline WU-12a/b, WU-15a/b, WU-16a/b and WU-19a/b all used for their own
splits. See `DECISIONS.md` ADR-046 for the full record.

**WU-20a itself.** `src/core/ring_buffer.hpp` (new): a fixed-capacity,
single-producer/single-consumer, allocation-free `RingBuffer<T, Capacity>`,
the classic circular-buffer one-slot-always-empty technique, `std::atomic`
head/tail indices with acquire/release ordering (mirroring
`io/decklink_output.hpp`'s own `PlaybackStats` atomics convention for the
relaxed `droppedCount()`), drop-not-block on a full ring. `tests/
test_ring_buffer.cpp` (new): FIFO order, full-ring drop/count behaviour,
`capacity()` reporting usable slots not backing storage, 10000-cycle
wraparound with an instrumented move-only `Tracked` type checking for
leaks/duplication, and — the check this unit cares about most — a genuine
two-`std::thread` producer/consumer run of 200000 items, verified under
ThreadSanitizer. One genuine first-draft compile bug caught and fixed
before any claim was made (a raw template-argument-list comma inside
`CHECK(...)`, a single-argument function-like macro — worked around with a
local `using` alias, documented inline; not a `CORRECTIONS.md` entry, per
the WU-17/18 precedent for routine first-draft mistakes caught same-session).
`CMakeLists.txt`: `test_ring_buffer` wired in via `scatter_test()`, plus a
comment documenting the WU-20a/WU-20b split and that WU-20b is deferred.

**Verification.** Full project (all 51 build targets, eighteen tests, the
seventeen carried over unchanged plus `test_ring_buffer`, 20036 checks) —
GCC 13 and Clang 18, Release and Debug, `SCATTER_TILE_LOG2` 4 and 5, all
clean, zero warnings under this project's full `-Wall -Wextra -Wpedantic
-Wconversion -Wsign-conversion -Werror` set. Both compilers' own ASan+UBSan
runtimes, both tile sizes: clean. Both compilers' own **ThreadSanitizer**,
both tile sizes: clean, no data race — the first unit in this project to
verify a concurrent data structure under TSan with two different
compilers' own sanitizer runtimes, not just GCC's (Clang's
`libclang_rt.tsan`/`.asan` needed a manual `apt-get download`/`dpkg -i`
workaround for an unrelated broken transitive dependency — `libc6-i386` —
in the sandbox's own package mirror; not a project code issue). This is a
materially stronger verification than WU-14/WU-15a's own SDK-gap units
could get, extending ADR-042's own "genuinely verify everything possible"
ethos (established for the NEON units' AArch64 cross-compile) to a
portable, SDK-independent data structure instead.

**Explicitly stated, per this project's own discipline (ADR-031/032):**
this sandbox has no Blackmagic SDK and no AppleClang/Xcode toolchain at
all. Anything landing in the `scatter-decklink` target (gated on
`BLACKMAGIC_SDK_DIR`) — which is all of WU-20b — cannot be compiled here,
only reasoned through against the real SDK headers and samples, exactly as
WU-14/WU-15a already were. WU-20a itself has no such gap; see
"Verification," above.

**Corrections this session:** none logged. The `CHECK`-macro compile bug
above was caught and fixed within the same edit, before any claim was
made — routine first-draft mistake, not a `CORRECTIONS.md`-worthy
correction, per the WU-17/18 precedent this project already established
for that category.

## Where we are

**Phase 5 (Live capture) is under way.** WU-20a (ring buffer) is done in
the sandbox, `wip` pending Steve's real-terminal close. WU-20b (DeckLink
capture object: format-detection-aware `EnableVideoInput`,
`IDeckLinkInputCallback` implementation, pushing arrived frames into a
`WU-20a::RingBuffer`) is sketched in `WORK-UNITS.md` but not built —
deliberately deferred, needs the real SDK/AppleClang. `DECISIONS.md` now
runs through ADR-046; `CORRECTIONS.md` unchanged, through C-015.

**Delivery mechanics:** WU-20a's implementation and full verification
matrix were done entirely in this session's own Linux cloud sandbox — no
real-hardware or real-terminal step was needed for WU-20a itself, unlike
every DeckLink-touching unit before it. All files (`ring_buffer.hpp`,
`test_ring_buffer.cpp`, `CMakeLists.txt`, `DECISIONS.md`, `WORK-UNITS.md`,
this `HANDOFF.md`) were written to the real repository via the device
bridge. Steve commits and tags at his own terminal — the standing
operational note (device-bridge commits on this machine leave stale
`.git/index.lock`/`HEAD.lock` files) still applies; git commands are not
run via the bridge this session either.

## Next work unit

**WU-20b — DeckLink capture: format-detection-aware `EnableVideoInput`,
`IDeckLinkInputCallback` implementation.** Sketched, not frozen, in
`WORK-UNITS.md` this session (files: `src/io/decklink_input.hpp`/`.cpp`,
`tests/test_decklink_input.cpp`). Needs the real SDK headers/samples this
session already read (cited in `DECISIONS.md` ADR-046) plus AppleClang/
Xcode to build or run at all — expect the same "reasoned through, entirely
unverified until the real terminal" shape WU-14/WU-15a already used.
Real `Files:`/`Accept:` scoping is that session's own first job, same as
every unit in this project, this sketch included.

Once WU-20b closes, Phase 5's own remaining units are WU-21 (full loop
through at 576i25) and WU-22 (diagnostic coverage view) — both `todo`,
unscoped.

## Open questions

Unchanged from session 24: Q3 (macOS/Desktop Video version), Q4 (lattice
edge damping, C-008(a)) remain open. Q2 remains moot per ADR-037. ADR-037's
own follow-up #2 (genlock) remains open — now squarely WU-20b's own
concern. Follow-up #1 (`test_decklink_device.cpp`'s full-duplex check) is
already the known, accepted ADR-035 exception, unrelated to Phase 5's own
new work.

## Blocked / red

Nothing red. WU-20a is green in the sandbox across the full matrix
(GCC+Clang, Release+Debug, tile 4+5, ASan+UBSan, and — both compilers —
ThreadSanitizer). Only the routine `close.sh 20a` tag confirmation is
outstanding, same procedural step every unit needs. WU-20b is not started;
not blocked, deliberately deferred (needs real SDK/AppleClang).

## Environment check

Unchanged from sessions 18-24 (ADR-037/039): **UltraStudio Monitor 3G**
remains the active, confirmed output target. **UltraStudio Recorder 3G**
is in hand, named (ADR-039) as Phase 5's own input target — WU-20b's own
job to actually exercise it, next. **UltraStudio 4K Mini** remains on hold
pending a PSU replacement.

## Append to DECISIONS.md

ADR-046 (the real `IDeckLinkInput`/`IDeckLinkInputCallback`/
`IDeckLinkVideoInputFrame` shape, the two `architecture.md` 7 inaccuracies
found, the three real capture samples surveyed, the WU-20a/WU-20b split
decision and reasoning, the full `RingBuffer<T, Capacity>` design freeze,
and the full verification record) was appended in full this session; see
`DECISIONS.md`. ADR-046 does not reopen `docs/architecture.md`, ADR-009,
013, 021, 030, 031, 032, 034, 039 or 040 — see its own closing paragraph.

## Append to CORRECTIONS.md

Nothing appended this session. The `CHECK`-macro compile bug in
`tests/test_ring_buffer.cpp`'s first draft was caught and fixed within the
same edit, before any claim was made based on the broken draft — a routine
first-draft mistake, not a genuine correction in the `CORRECTIONS.md`
sense, per the precedent WU-17/18 already established for this exact
category (see `HANDOFF.md` session 22/23 and `DECISIONS.md` ADR-042/043).

---

## What to run at your terminal

Nothing has been committed yet — same standing operational note as every
prior session: committing through the device bridge on this repo reliably
leaves stale `.git/index.lock`/`HEAD.lock` files behind, so this was left
for you to run directly:

```
cd ~/src/scatter-dve
git add src/core/ring_buffer.hpp tests/test_ring_buffer.cpp CMakeLists.txt \
        DECISIONS.md WORK-UNITS.md HANDOFF.md
git commit -m "WU-20a: portable SPSC ring buffer (ADR-046), verified in the cloud sandbox incl. TSan"
cmake -B build && cmake --build build
ctest --test-dir build --output-on-failure   # expect 20/21 -- ADR-035's known exception, unrelated
./tools/close.sh 20a
git tag -a wu-20a-green -m "WU-20a: portable SPSC ring buffer, verified"
```

WU-20b is deliberately not started — its own real scoping, and the real
SDK/AppleClang build it needs, are next session's first job (or yours, if
you want to get ahead of it: read `DECISIONS.md` ADR-046's own WU-20b
sketch and `WORK-UNITS.md`'s own WU-20b entry first).
