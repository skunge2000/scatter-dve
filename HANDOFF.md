# Handoff
Overwritten at the end of every session. This is the first thing to read.

---

**Session:** 47 (WU-23b1 build — Weston 3-field de-interlace filter core,
`video::Deinterlacer`: real code written, a real data-flow error in the
immediately preceding session's own scoping found and corrected, built
and verified green across this project's own full portable-unit matrix).

**Tag:** `wu-23a2b-green` is still the newest real tag on the repository
(confirmed this session, see below). This session's own work is **not
yet tagged** — real code, built and tested in the cloud sandbox, but not
yet built/tested at Steve's own real terminal, which
`SESSION-PROTOCOL.md`'s own "sandbox edits are not delivered until
pushed"/anti-drift rule 8 requires before `wu-23b1-green` exists. See
"Steve's own next steps" below.

## Before doing anything else in the next session

Run `git tag --sort=creatordate`, `git log --oneline -10`, `git rev-parse
HEAD origin/main` and `git status --short` directly against
`~/src/scatter-dve` (via the device bridge, or at a real terminal) — do
not trust this file's own account without checking it against the real
repository first.

## This session in full

Opened with a continuation prompt whose job was to build WU-23b1
(`video::Deinterlacer`, the Weston 3-field de-interlace filter core),
scoped but not built by the immediately preceding session
(`DECISIONS.md` ADR-078).

**Repository state, confirmed directly before reading anything else:**
`git tag --sort=creatordate` (newest: `wu-23a2b-green`), `git log
--oneline -10` (`HEAD` = `13b7d13`, "WU-23b scoping: Weston 3-field
de-interlace source confirmed, video::Deinterlacer design, split into
WU-23b1/WU-23b2 (ADR-078)"), `git rev-parse HEAD origin/main` (both
`13b7d13ea66e769877c145437d9f47a73902bd25`), `git status --short` (empty,
clean tree) — all run directly against `~/src/scatter-dve` via the device
bridge, matching the continuation prompt's own expected state exactly.

**Fetched and re-read `libavfilter/vf_w3fdif.c` directly** (not from
ADR-078's own paraphrase — `SESSION-PROTOCOL.md` rule 6), per the
continuation prompt's own explicit instruction. Coefficients, scale
(2^15), the reflect-by-±2 edge convention, uniform plane treatment and
the frame-rate-mode choice all checked out exactly as ADR-078 stated.

**Found a real error in ADR-078's own design, not just a paraphrase
looseness: the data-flow half was wrong and could not implement the real
algorithm.** ADR-078 described `video::Deinterlacer` as fed one
already-field-split, half-height Raster444 per call (a single field
parity's own temporal sequence). Tracing `deinterlace_plane_slice()` and
`filter_frame()` line by line showed the real filter's high-pass term
reads `cur`'s own frame a *second* time, at the *other* parity's rows —
meaning `cur` must be a full-height weave frame (both parities really
present), not a field-native one — and that in frame-rate mode `adj` is
unconditionally `prev`, never `next` (confirmed by tracing three real
pushes through `filter_frame()`'s own shift, including its
duplicate-first-frame stream-start convention). Full account:
`CORRECTIONS.md` C-025. Not a coefficient or edge-handling error — those
parts of ADR-078 were right.

**Froze the corrected interface and wrote the code, `DECISIONS.md`
ADR-079:** `class Deinterlacer` (`video/deinterlace.hpp`/`.cpp`),
constructed with a fixed `FieldParity` (which stream it serves) and a
`DeinterlaceCoefficients` (simple/complex), `bool push(const Raster444&
weaveFrame, Raster444& outFrame)` — `bool` + caller-owned out-parameter,
matching this codebase's own `runFrameFile()`/`extractField()`/
`interleaveFields()`/`runFrame()` convention rather than introducing
`std::optional<Raster444>` as a return type (`Raster444` has no default
constructor; `std::optional<Raster444>` is still used internally for the
three history slots, the same tool `core/ring_buffer.hpp`'s `RingBuffer`
already uses for a different reason). Coefficient sum properties (unity
low-pass gain, zero high-pass gain) encoded as `static_assert`s in
`video/deinterlace.cpp` itself, re-verifying the real source's own values
every time this file compiles. Descale is round-half-up on a signed
64-bit sum (I4/I6), narrowed to `Sample` by plain conversion — wrapping,
not clamping, `video/chroma.hpp`'s own already-documented precedent for a
negative-lobe integer filter, honouring I2.

**Wrote `tests/test_deinterlace.cpp`:** a separately-written reference
reconstruction function (per-pixel, vector-based, its own copy of the
reflect/round-half-up arithmetic, never calling into
`video/deinterlace.cpp`) checked against `Deinterlacer`'s own real output
for a 6-frame marked sequence, both coefficient sets, both field
parities, at a small hand-tractable geometry; two explicit hand-computed
edge-row values (cross-checked with a standalone Python script during
this session, not just derived once and trusted) for both coefficient
sets; and a 720×576 (this project's own 625i50 SD standard — per Steve's
own explicit stay-in-SD-domain scope decision, not 1080p) sanity sequence
against the same independent reference. 120 checks, all passing.

**Registered in `CMakeLists.txt`:** `src/video/deinterlace.cpp` added to
`scatter-core`'s source list; `scatter_test(test_deinterlace)` added
alongside `test_field_pipeline`.

**Built and tested in this session's own Linux cloud sandbox, the full
matrix:** GCC 13.3.0 and Clang 18.1.3 (both confirmed present, exact
versions the continuation prompt named), Release and Debug,
`SCATTER_TILE_LOG2` 4 and 5 (8 configurations), plus GCC 13
`-fsanitize=address,undefined -fno-sanitize-recover=all` at both tile
sizes (2 more) — **10 configurations, all green, zero warnings under the
full `-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror`
set** (one real `-Wsign-conversion` error found and fixed along the way:
`std::array::operator[]` needs an unsigned index; `coefs.low[j]`/
`coefs.high[j]` needed `std::size_t(j)`, not bare `int j`). `ctest`
showed 26/26 passing in every configuration (24 pre-existing plus
`test_testpat` plus this unit's own new `test_deinterlace`) — no
AArch64 cross-compile needed, this unit touches no platform-specific
surface, the same scope prior portable units used.

**Edited `WORK-UNITS.md`'s WU-23b1 entry** (edited-as-scope-firms-up, not
append-only) to describe the corrected data flow directly, so a future
reader does not need to cross-reference C-025 just to know what the class
actually does; its own `Accept:` line's wording is unaffected in
substance (see C-025's own closing paragraph). Status now "built this
session, unverified" pending a real terminal build/`ctest` run.

Wrote `video/deinterlace.hpp`, `video/deinterlace.cpp`,
`tests/test_deinterlace.cpp`, `CMakeLists.txt`, `WORK-UNITS.md`,
`DECISIONS.md` and `CORRECTIONS.md` to the real repository via the device
bridge, then re-staged all seven from the device and diffed against this
session's own edited copies before writing this sentence —
`SESSION-PROTOCOL.md`'s own rule 8.

## Where we are

**WU-23b1 is built, not yet green.** Real code, 10/10 cloud-sandbox
configurations passing including sanitizers, but `SESSION-PROTOCOL.md`'s
own bar for `wu-23b1-green` is a real build/`ctest` run at Steve's own
terminal (this project's own standing convention — the Mac is the
build-configuration `docs/architecture.md` section 6 actually targets,
the cloud sandbox is this project's own pre-flight check, not a
replacement for it). `DECISIONS.md` now runs through ADR-079;
`INVARIANTS.md` unchanged through I11; `CORRECTIONS.md` now runs through
C-025.

## Next work unit

**WU-23b2** (live-capture wiring: a new owned `video::Deinterlacer`
member in `io/decklink_capture_consumer.cpp`, plus the output-side
re-interlace decimate) is the natural next pick once WU-23b1 is confirmed
green at the real terminal — genuinely depends on WU-23b1's own actual
interface, now built (`push(const Raster444&, Raster444&)`, both field
parities need their own instance since each instance serves one fixed
anchor parity). Not scoped with `Files:`/`Accept:` yet — that unit's own
scoping session should target 576i25/625i50 as its own real-hardware
verification standard, not 1080i (flagged again here per the immediately
preceding session's own note, not assumed to carry forward silently), and
should resolve the stream-start question ADR-078 left open for it
(though note: `video::Deinterlacer`'s own internal stream-start behaviour
— the duplicate-first-frame convention — is now fully resolved and built,
per ADR-079; WU-23b2's own open question is what the *caller*
(`CaptureConsumer`) does before `Deinterlacer` has produced its first
real output, a different question). Everything named in Session 43's own
"Next work unit" section (WU-28d, WU-27, WU-33, WU-35, WU-37) is
unchanged and still pickable if the interlace thread is set aside
instead.

**Steve's own explicit stay-in-SD-domain scope decision (WU-24/WU-25
skipped until he says otherwise) is unchanged and still in force** —
carried forward from the immediately preceding session, not touched this
session, reiterated here per that session's own request not to let it be
lost silently.

## Open questions

Unchanged from Session 42/43's own list (`kCaptureRingCapacity`, Q3, Q4,
Task A1, Task D6, ADR-070's open question, WU-35's `compositeLayered()`
question), plus WU-23b2's own stream-start question (ADR-078, refined
above) — this session did not touch any of them.

## Blocked / red

Nothing red. Nothing newly blocked. WU-23b1 is built but not yet tagged
(see "Tag" above) — not a blocker, the ordinary state of a unit awaiting
its own real-terminal build.

## Environment check

Built and tested in this session's own Linux cloud sandbox only — GCC
13.3.0, Clang 18.1.3, both present and used, exact versions the
continuation prompt named. Real-terminal build (the M1 Max, AppleClang)
not yet done this session; see "Steve's own next steps" below. The
standing condition from prior sessions (C-024: `tools/close.sh` cannot
currently succeed for any unit, on Steve's real terminal, because of the
PSU/two-device-architecture mismatch — `DECISIONS.md` ADR-034/035/037,
`CORRECTIONS.md` C-024) is unchanged: this unit has no DeckLink
dependency of its own and does not even link `scatter-decklink`, but
`test_decklink_device`'s own `test_at_least_one_device_is_full_duplex`
check still runs on every close-out regardless, so the manual-tag path
below is required, not `./tools/close.sh`.

## Append to DECISIONS.md

**ADR-079** — already appended in full this session (WU-23b1 build:
`video::Deinterlacer`'s exact interface frozen; corrected data-flow
design carried forward from `CORRECTIONS.md` C-025). See `DECISIONS.md`.

## Append to CORRECTIONS.md

**C-025** — already appended in full this session (ADR-078's own
`video::Deinterlacer` data-flow description did not match the real
source and could not implement the algorithm; corrected, full account in
`DECISIONS.md` ADR-079). See `CORRECTIONS.md`.

## Closed out this session

**WU-23b1 build.** `video/deinterlace.hpp`/`.cpp` (new),
`tests/test_deinterlace.cpp` (new), `CMakeLists.txt` (registration).
`DECISIONS.md` ADR-079, `CORRECTIONS.md` C-025, `WORK-UNITS.md`'s WU-23b1
entry corrected and marked "built, unverified". 10/10 cloud-sandbox
configurations green (GCC 13.3.0 + Clang 18.1.3 × Release/Debug × tile
2^4/2^5, plus GCC 13 ASan/UBSan × both tile sizes), 26/26 tests passing
in every configuration, zero warnings. Not yet tagged — needs a real
terminal build/`ctest` run first, per `SESSION-PROTOCOL.md`'s own
"sandbox edits are not delivered until pushed" discipline extended to
"cloud-sandbox green is not real-terminal green" for the same reason.

## Steve's own next steps

**Do not run `./tools/close.sh` — see `CORRECTIONS.md` C-024, unchanged.**
This unit has no DeckLink dependency of its own, but
`test_decklink_device`'s own duplex-check exception still runs on every
close-out and `close.sh` treats any `ctest` failure as blocking.

Build and test manually at your own real terminal:

```
cd ~/src/scatter-dve
cmake --build build
ctest --test-dir build --output-on-failure
```

Confirm nothing **other than** `test_decklink_device`'s own
`test_at_least_one_device_is_full_duplex` check fails (everything else,
including the new `test_deinterlace`, should pass — it did in all 10 of
this session's own cloud-sandbox configurations). If so, commit, tag and
push by hand:

```
cd ~/src/scatter-dve
git add src/video/deinterlace.hpp src/video/deinterlace.cpp tests/test_deinterlace.cpp CMakeLists.txt WORK-UNITS.md DECISIONS.md CORRECTIONS.md
git commit -m "WU-23b1: Weston 3-field de-interlace filter core, video::Deinterlacer (ADR-079); corrects ADR-078's own data-flow design (C-025)"
git tag -a wu-23b1-green -m "WU-23b1: video::Deinterlacer filter core green"
git push origin main
git push origin --tags
```

(File paths above match this session's own `git status --short` at the
time of writing — re-confirm against a real `git status --short` before
running the `git add` line. **This session's own first draft of this
block used `video/deinterlace.hpp`/`.cpp` — missing the `src/` prefix —
despite this file's own "This session in full" section, and the real
`git status --short` output this session captured directly, both already
showing the correct `src/video/...` paths; Steve caught it when `git add`
failed with "pathspec did not match any files." See `CORRECTIONS.md`
C-026.**)
