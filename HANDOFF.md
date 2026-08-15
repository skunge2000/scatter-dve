# Handoff
Overwritten at the end of every session. This is the first thing to read.

---

**Session:** 15
**Tag:** none yet — WU-14 is `wip`, not `green`. Nothing in this unit has
been built or run by this session; see "Delivery mechanics" below for why
that is expected for this specific unit, not a gap.
**Phase:** 3 — SDI output, started. WU-14 (DeckLink device enumeration and
`ComPtr`) implemented and written to disk; **not yet built or tested.**
WU-15 (scheduled playback, file source to SDI out) is next once WU-14 is
confirmed green.

**This session's own first job, as flagged going into it, was research, not
implementation.** Read `HANDOFF.md`, `INVARIANTS.md`, `DECISIONS.md`,
`CORRECTIONS.md` and `WORK-UNITS.md` in full (`SESSION-PROTOCOL.md`'s
required order), then re-read `docs/architecture.md` section 7 (the DeckLink
SDK sketch) and section 8's module layout for `src/io/`. Then, per this
session's own brief and `HANDOFF.md`'s own flag from session 14 — WU-14 is
"genuinely new ground, not a fill-in-a-parametrisation gap" — read the real
Blackmagic DeckLink SDK 16.0 headers under `~/src/Blackmagic DeckLink SDK
16.0` rather than working from architecture.md's summary alone:
`Mac/include/DeckLinkAPI.h`, `DeckLinkAPIDiscovery.h`, `DeckLinkAPITypes.h`,
`DeckLinkAPIConfiguration.h`, `DeckLinkAPIVersion.h` and
`DeckLinkAPIDispatch.cpp`, plus several of the SDK's own C++ samples
(`CapturePreview/com_ptr.h`, `DeviceStatus/com_ptr.h`,
`DeviceList/main.cpp` + `platform.cpp`, `FileCapture/DeckLinkDeviceDiscovery
.cpp` + `DeviceManager.cpp`, `DeviceStatus/DeckLinkDeviceListModel.cpp`) to
see the real interface shapes and the idiom actually used to call them, not
just the declarations. What that reading found and the choices it forced
are recorded in full in `DECISIONS.md` ADR-031 — summary below.

**Design choices this session had to make — now ADR-031 in
`DECISIONS.md`:** `IDeckLink` itself is minimal (`GetModelName`/
`GetDisplayName` only, both `CFStringRef`) and lives in
`DeckLinkAPIDiscovery.h`, not `DeckLinkAPI.h`; `IDeckLinkInput`/
`IDeckLinkOutput`/`IDeckLinkProfileAttributes` are all obtained via
`QueryInterface`, never exposed directly. `CreateDeckLinkIteratorInstance()`
and `IDeckLinkIterator::Next()` both hand the caller an *already-owned*
reference (ordinary COM factory/enumerator convention), which the SDK's own
`com_ptr<T>` sample — read in full this session, and the direct model for
this project's own `ComPtr` — does not itself always handle correctly (its
raw-pointer constructor always `AddRef`s, which over-retains a factory
result by one reference per call; verified directly against
`Samples/FileCapture/DeckLinkDeviceDiscovery.cpp`'s own
`CreateDeckLinkDiscoveryInstance()` usage). This project's own
`src/io/com_ptr.hpp` matches the SDK sample's public shape closely
(renamed to this codebase's own `PascalCase` convention) but adds one new
method, `adopt(T*)`, specifically to close that leak for factory-style
direct returns — `releaseAndGetAddressOf()` (already in the SDK's own
sample, unchanged here) already has correct owning-transfer semantics for
the `Next()`-style out-parameter case, so no second new method was needed
there. `src/io/decklink_device.hpp`/`.cpp` (new — this project's first
`src/io/` header) declare `DeviceInfo` and `enumerateDeckLinkDevices()`,
converting `CFStringRef` names to `std::string` immediately and reading
`IDeckLinkProfileAttributes::GetInt(BMDDeckLinkVideoIOSupport, ...)`'s
capture/playback bits — all without opening a stream anywhere, which is
this unit's own scope boundary against WU-15. `BLACKMAGIC_SDK_DIR` is a new
CMake cache variable (not a hardcoded path), matching `SCATTER_TILE_LOG2`'s
existing pattern in this same file; the new `scatter-decklink` target and
`test_decklink_device` are gated on `APPLE` and the SDK actually being found
there, and skip cleanly with a `STATUS` message otherwise — the Linux cloud
sandbox's existing `scatter-core` matrix is unaffected. Full reasoning,
including the exact SDK citations, is in ADR-031.

**Tests:** `tests/test_decklink_device.cpp` (new) is written but **has never
been run.** It checks, against real hardware: at least one device
enumerates; every device has non-empty model/display names; at least one
device reports both capture and playback support via
`IDeckLinkProfileAttributes` *and* a live `QueryInterface` succeeds for both
`IID_IDeckLinkInput` and `IID_IDeckLinkOutput`; `QueryInterface`'s COM
identity guarantee holds through `ComPtr`'s converting constructor; repeated
enumeration returns a stable device count. Nothing in it opens a stream.

**Build:** unverified. No AppleClang/Xcode toolchain and no Blackmagic SDK
exist in the Linux cloud sandbox this session's own drafting happened in,
and the device bridge's own shell tool is a sandboxed Linux VM with neither
either — `HANDOFF.md`, going into this session, already flagged this as the
expected shape for WU-14 specifically, not a gap to route around the way
every unit since WU-06 has. This is the first session since WU-05 whose own
code has not been run through the Linux Clang 18/GCC 13/ASan/UBSan matrix at
all before being written to disk, because that matrix itself cannot see
this unit's own files (`CMakeLists.txt`'s `BLACKMAGIC_SDK_DIR` guard skips
them there by design).

## Where we are

WU-14 adds three new files under `src/io/` (`com_ptr.hpp`,
`decklink_device.hpp`, `decklink_device.cpp`) and one new test
(`tests/test_decklink_device.cpp`), plus a `scatter-decklink` CMake target
and `BLACKMAGIC_SDK_DIR` cache variable in `CMakeLists.txt`. See
`DECISIONS.md` ADR-031 for the full design and `WORK-UNITS.md`'s WU-14 entry
(now `wip`, with **Files:**/**Accept:** filled in) for the accept criteria.

**Corrections this session:** none. Nothing found while reading the real SDK
headers contradicted an earlier claim in `DECISIONS.md`, `INVARIANTS.md` or
`CORRECTIONS.md` — architecture.md 7's own claims (COM-style interfaces,
`CreateDeckLinkIteratorInstance()` as the entry point, the 4K Mini's own
full-duplex `IDeckLink`) all held up against the real headers; they were
underspecified, not wrong, the same relationship every ADR since ADR-020 has
had to architecture.md's own gaps.

## Delivery mechanics — read before doing anything else this session

This is the part of the loop that is genuinely different for WU-14, not a
repeat of sessions 6 through 14's own delivery note:

1. **Nothing was implemented and verified in a disposable Linux cloud
   sandbox first**, unlike every unit since WU-06. There is no SDK and no
   AppleClang there for this unit to be checked against, so `com_ptr.hpp`,
   `decklink_device.hpp`/`.cpp`, `tests/test_decklink_device.cpp` and the
   `CMakeLists.txt` changes were reasoned through against the real SDK
   headers (ADR-031's own citations) and written straight to this machine
   via the device bridge, unbuilt.
2. **This session did not run `close.sh`, and did not tag anything.** WU-14
   stays `wip` in `WORK-UNITS.md` until you build and run it yourself.
3. **What to run, at your own terminal (not through the device bridge's own
   shell — that is a sandboxed Linux VM with no Xcode/AppleClang either):**

   ```
   cmake -B build -DBLACKMAGIC_SDK_DIR="/Users/stephenneal/src/Blackmagic DeckLink SDK 16.0"
   cmake --build build
   ./build/test_decklink_device
   ```

   If that configures and builds clean and `test_decklink_device` passes,
   the rest of the suite should be unaffected (nothing in `scatter-core` or
   any existing test changed), but it costs little to also run the full
   `ctest` you'd normally run at session close, to confirm this unit's own
   `CMakeLists.txt` changes didn't disturb anything else's configuration.
4. **`./tools/close.sh` may need updating to pass `-DBLACKMAGIC_SDK_DIR`.**
   I have not been shown `close.sh`'s own contents this session (rule 1:
   never edit a file not shown in the current session), so I have not
   touched it and cannot say whether it already passes CMake cache
   variables through or would need a line added. If it does not build with
   the SDK by default, either add `-DBLACKMAGIC_SDK_DIR=...` to it yourself,
   or paste its contents into a future session's own context so that can be
   done here instead.
5. **If it does not build clean:** the most likely single point of failure,
   flagged directly in `CMakeLists.txt`'s own comment and in ADR-031, is
   whether a trailing `-w` on `DeckLinkAPIDispatch.cpp`'s own compile command
   actually overrides the target-level `-Werror` set (ADR-017) the way this
   session assumed — paste the actual compiler error back and this can be
   fixed directly, whether that's the cause or something else entirely
   (e.g. a header path difference between this SDK release and what this
   session read, or a interface signature this session mis-transcribed).

## Next work unit

Once WU-14 is confirmed green (built, `test_decklink_device` passing,
`WORK-UNITS.md` updated from `wip` to `green` and tagged `wu-14-green`),
`WORK-UNITS.md`'s own ordering names WU-15 — scheduled playback, file source
to SDI out — next. Its own accept criterion ("one hour on a broadcast
monitor, no dropped frames") is exactly the kind of real capture/playback
smoke test this session's own brief warned might not fit "one session, one
unit" — worth scoping and, if needed, splitting (the same way WU-12 split)
*before* writing any implementation code for it, not discovered mid-unit.

## Open questions

Unchanged from session 14: Q1 (tile size), Q2 (4K Mini program outputs), Q3
(macOS/Desktop Video version), Q4 (lattice edge damping, C-008(a)) — all
still open, none blocking, and nothing this session touched any of them.

New from this session, both flagged directly above rather than buried here:
whether `DeckLinkAPIDispatch.cpp`'s own warnings are actually suppressed by
the trailing `-w` this session's `CMakeLists.txt` change relies on
(unverified — see "Delivery mechanics" above); and whether `close.sh` needs
a `-DBLACKMAGIC_SDK_DIR` line added (unverified — `close.sh`'s own contents
were not shown this session).

## Blocked / red

Not red — `wip`, pending a build this session could not itself run. See
"Delivery mechanics" above for exactly what to do next.

## Environment check

Confirmed this session (by you, at the real terminal, before this session's
own work started): the UltraStudio 4K Mini enumerates. WU-14's own accept
criteria need exactly this and nothing more — `enumerateDeckLinkDevices()`,
`GetModelName`/`GetDisplayName`, and `IDeckLinkProfileAttributes` capability
queries, no stream opened anywhere. **Still not separately confirmed:**
Desktop Video Setup showing both input and output active, or a
capture/playback round trip in Media Express (architecture.md 10's own
Phase 0 checklist). Not needed for WU-14 as scoped, but flagged here since
WU-15 (scheduled playback) is next and will need it — worth confirming
before that session starts, the same way this session's own confirmation
was worth doing before WU-14 started.

## Append to DECISIONS.md

Nothing this update — ADR-031 was appended in full earlier this session; see
`DECISIONS.md`.

## Append to CORRECTIONS.md

Nothing this update — see "Corrections this session" above.
