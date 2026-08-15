# Handoff
Overwritten at the end of every session. This is the first thing to read.

---

**Session:** 15
**Tag:** `wu-14-green` — confirmed. `./tools/close.sh 14` ran clean on the
M1 Max with AppleClang (Release, tile 2^5, the config `close.sh` builds) on
the first attempt — all fifteen tests passed (fourteen carried over
unchanged from WU-01 through WU-13, plus `test_decklink_device`, new this
session), zero warnings under `-Werror -Wconversion -Wsign-conversion`.
**Phase:** 3 — SDI output, under way. WU-14 (DeckLink device enumeration and
`ComPtr`) is done. `WORK-UNITS.md`'s own ordering names WU-15 — scheduled
playback, file source to SDI out — next.

**This session's own first job was research, not implementation**, per its
own brief and `HANDOFF.md`'s own flag from session 14: WU-14 is "genuinely
new ground, not a fill-in-a-parametrisation gap" the way every unit since
WU-11 was. Read `HANDOFF.md`, `INVARIANTS.md`, `DECISIONS.md`,
`CORRECTIONS.md` and `WORK-UNITS.md` in full (`SESSION-PROTOCOL.md`'s
required order), re-read `docs/architecture.md` section 7 (the DeckLink SDK
sketch) and section 8's module layout for `src/io/`, then read the real
Blackmagic DeckLink SDK 16.0 headers under `~/src/Blackmagic DeckLink SDK
16.0` — `Mac/include/DeckLinkAPI.h`, `DeckLinkAPIDiscovery.h`,
`DeckLinkAPITypes.h`, `DeckLinkAPIConfiguration.h`, `DeckLinkAPIVersion.h`,
`DeckLinkAPIDispatch.cpp` — plus several of the SDK's own C++ samples
(`CapturePreview`, `DeviceStatus`, `DeviceList`, `FileCapture`) to see the
real interface shapes and the idiom actually used to call them, before
scoping `WORK-UNITS.md`'s WU-14 **Files:**/**Accept:** lines or writing any
project code. What that reading found and the choices it forced are in
`DECISIONS.md` ADR-031 — summary below.

**Design choices this session had to make — now ADR-031 in
`DECISIONS.md`:** `IDeckLink` itself is minimal (`GetModelName`/
`GetDisplayName` only, both `CFStringRef`) and lives in
`DeckLinkAPIDiscovery.h`, not `DeckLinkAPI.h`; `IDeckLinkInput`/
`IDeckLinkOutput`/`IDeckLinkProfileAttributes` are all obtained via
`QueryInterface`, never exposed directly on `IDeckLink`.
`CreateDeckLinkIteratorInstance()` and `IDeckLinkIterator::Next()` both hand
the caller an *already-owned* reference (ordinary COM factory/enumerator
convention) — a distinction the SDK's own `com_ptr<T>` sample (the direct
model for this project's own `ComPtr`) does not itself always handle
correctly, verified directly against one of its own samples' usage.
`src/io/com_ptr.hpp` matches the SDK sample's public shape closely (renamed
to this codebase's own `PascalCase` convention) but adds one new method,
`adopt(T*)`, specifically to close that reference-leak hazard for
factory-style direct returns; `releaseAndGetAddressOf()` (already in the
SDK's own sample, unchanged here) already has correct owning-transfer
semantics for the `Next()`-style out-parameter case. `src/io/
decklink_device.hpp`/`.cpp` (new — this project's first `src/io/` header)
declare `DeviceInfo` and `enumerateDeckLinkDevices()`, converting
`CFStringRef` names to `std::string` immediately and reading
`IDeckLinkProfileAttributes::GetInt(BMDDeckLinkVideoIOSupport, ...)`'s
capture/playback bits — all without opening a stream anywhere, the scope
boundary against WU-15. `BLACKMAGIC_SDK_DIR` is a new CMake cache variable
(not a hardcoded path), matching `SCATTER_TILE_LOG2`'s existing pattern; the
new `scatter-decklink` target and `test_decklink_device` are gated on
`APPLE` and the SDK actually being found there, skipping cleanly with a
`STATUS` message otherwise. Full reasoning, including the exact SDK
citations, is in ADR-031.

**Tests:** fifteen green on the M1 Max — the fourteen carried over unchanged
from WU-01 through WU-13 plus `test_decklink_device.cpp`, new this session
(8 checks; run against the real UltraStudio 4K Mini, not a mock): at least
one device enumerates; every device has non-empty model/display names; at
least one device reports both capture and playback support via
`IDeckLinkProfileAttributes` *and* a live `QueryInterface` succeeds for both
`IID_IDeckLinkInput` and `IID_IDeckLinkOutput`; `QueryInterface`'s COM
identity guarantee holds through `ComPtr`'s converting constructor; repeated
enumeration returns a stable device count. Nothing in it opens a stream —
that is WU-15 onward's own job. No `runFrame()`-level check — this unit
sits entirely in `src/io/`, orthogonal to the `core`/`video` pipeline every
earlier unit's own tests exercise.

Unlike every unit since WU-06, this session's own implementation was **not**
first run through the Linux cloud sandbox's Clang 18/GCC 13/ASan/UBSan
matrix — there is no Blackmagic SDK and no AppleClang/Xcode toolchain there
for this unit's own files to be checked against at all (`CMakeLists.txt`'s
`BLACKMAGIC_SDK_DIR` guard skips them there by design, same as ADR-021
already does for a missing SDK). `com_ptr.hpp`, `decklink_device.hpp`/`.cpp`
and `tests/test_decklink_device.cpp` were reasoned through against the real
SDK headers and written straight to this machine via the device bridge,
unbuilt, then built and verified for the first time at your own terminal —
the loop this unit's own brief asked for, not a fallback.

**Build:** clean under `-Werror -Wconversion -Wsign-conversion` on
AppleClang (M1 Max) — including this project's own new files. The one
vendored SDK file this unit compiles, `DeckLinkAPIDispatch.cpp`, is exempted
from that set via a per-file `-w` (`CMakeLists.txt`'s own
`set_source_files_properties`); confirmed this session to actually work as
intended (the build produced zero warnings), resolving one of the two things
flagged as unverified going into this session's own close.

## Where we are

`src/io/com_ptr.hpp` — `ComPtr<T>`, modeled on the Blackmagic SDK's own
`Samples/*/com_ptr.h`, plus `adopt()` (see ADR-031). `src/io/
decklink_device.hpp`/`.cpp` — `DeviceInfo` and `enumerateDeckLinkDevices()`,
enumeration and capability queries only, no stream opened.
`tests/test_decklink_device.cpp` — the one new test, run against real
hardware. `CMakeLists.txt` — new `BLACKMAGIC_SDK_DIR` cache variable and
`scatter-decklink` target, gated so the Linux cloud sandbox's existing
`scatter-core` matrix is unaffected when the SDK is absent. See
`DECISIONS.md` ADR-031 for the full design and `WORK-UNITS.md`'s WU-14 entry
for the accept criteria and this session's own close-out detail.

**Corrections this session:** none. Nothing found while reading the real SDK
headers, or while building and testing at the real terminal, contradicted an
earlier claim in `DECISIONS.md`, `INVARIANTS.md` or `CORRECTIONS.md` —
architecture.md 7's own claims (COM-style interfaces,
`CreateDeckLinkIteratorInstance()` as the entry point, the 4K Mini's own
full-duplex `IDeckLink`) all held up against both the real headers and the
real hardware; they were underspecified, not wrong, the same relationship
every ADR since ADR-020 has had to architecture.md's own gaps. Both things
this session itself flagged as unverified going into its own close (the
`-w` exemption on `DeckLinkAPIDispatch.cpp`, and whether `close.sh` would
need a `-DBLACKMAGIC_SDK_DIR` line added) resolved cleanly rather than
surfacing a problem — `close.sh` needed no changes at all, since it reuses
the existing `build/` directory's own CMake cache rather than
reconfiguring from scratch, so `BLACKMAGIC_SDK_DIR` (already cached from
this session's first manual configure) carried through automatically.

**Delivery mechanics, not a design matter:** this session ran remotely, via
the device-bridge tools connecting to this machine, same as sessions 6
through 14 — but unlike those, this session's own implementation was not
verified in a disposable Linux cloud sandbox first (see "Tests" above for
why). Files were written to this machine via the bridge; `git add` and
`git commit` ran through that same bridge and, as in every prior session,
could not clean up its own `index.lock`/`HEAD.lock`/temp-object files
afterward (unlink fails on this mount), so stale ones were moved into
`_to_delete/` rather than removed — safe to `rm -rf _to_delete/` by hand; it
now holds further accumulated debris from this session on top of prior
ones. `cmake -B build -DBLACKMAGIC_SDK_DIR=...`, `cmake --build build`,
`ctest --test-dir build` and `./tools/close.sh 14` were all run by hand at
the real terminal, per this unit's own brief — the loop for every WU-14
onward unit that touches `src/io/`'s DeckLink-dependent files, not a
one-time exception.

## Next work unit

`WORK-UNITS.md`'s own ordering names WU-15 — scheduled playback, file source
to SDI out — next. Its own accept criterion ("one hour on a broadcast
monitor, no dropped frames") is exactly the kind of real capture/playback
smoke test this session's own brief warned might not fit "one session, one
unit" — worth scoping, and splitting if needed (the same way WU-12 split),
*before* writing any implementation code for it. WU-15 will also need the
Desktop Video / UltraStudio 4K Mini's own input and output separately
confirmed (Desktop Video Setup showing both active, a capture/playback round
trip in Media Express — architecture.md 10's own Phase 0 checklist) — WU-14
only ever needed enumeration, which is confirmed; WU-15 needs more, and that
is not yet confirmed. Worth doing before that session starts, the same way
this session's own enumeration check was worth doing before WU-14 started.

## Open questions

Unchanged from session 14: Q1 (tile size), Q2 (4K Mini program outputs), Q3
(macOS/Desktop Video version), Q4 (lattice edge damping, C-008(a)) — all
still open, none blocking, and nothing this session touched any of them.

Both questions this session's own first HANDOFF.md draft raised (the `-w`
exemption's effectiveness; whether `close.sh` needed updating) are resolved
— see "Corrections this session" above. No new open question from this
session beyond what ADR-031 already resolved.

## Blocked / red

Nothing. WU-14 closed green.

## Environment check

Confirmed this session: the UltraStudio 4K Mini enumerates, and — new
information from this session's own test run, beyond the bare "it
enumerates" — is full duplex (`bmdDeviceSupportsCapture` and
`bmdDeviceSupportsPlayback` both set, and live `QueryInterface` for both
`IID_IDeckLinkInput` and `IID_IDeckLinkOutput` both succeed). **Still not
separately confirmed:** Desktop Video Setup showing both input and output
active, or a capture/playback round trip in Media Express (architecture.md
10's own Phase 0 checklist) — flagged again under "Next work unit" above,
since WU-15 will need it and WU-14 did not.

## Append to DECISIONS.md

Nothing this update — ADR-031 was appended in full earlier this session; see
`DECISIONS.md`. Not reopened or amended now that the tag is confirmed.

## Append to CORRECTIONS.md

Nothing this update — see "Corrections this session" above; nothing to log,
and the tag is confirmed clean, not reopened or amended now.
