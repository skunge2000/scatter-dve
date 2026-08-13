# Decisions
Append only. Numbered. A decision is settled unless explicitly superseded by a
later numbered entry. Do not reopen; propose a superseding ADR instead.

---

**ADR-001 — Forward scatter, not inverse gather.**
Every contemporary DVE and every GPU does inverse mapping: for each output
pixel, sample the source. Mirage does the opposite, per US 4,563,703. Forward
scatter is chosen because it is the only formulation that handles surfaces which
fold, tear, self-intersect or shatter, and because accumulation transparency and
the signature granulation and explosion effects fall out of it naturally.
Costs accepted: random writes, magnification holes, a normalisation pass.

**ADR-002 — Tile binning for bandwidth; four-bank split retained for
serialisation.**
These are distinct problems and only the first is solved by caching. Tile
binning converts scattered writes into streaming and keeps the accumulator in
cache. Quantel's four-bank decomposition is retained because a single
accumulator still requires four sequential read-modify-writes per fragment,
serialising on store-to-load latency; four banks give four independent chains
that pipeline. Fixed address offsets on write, common addressing and summation
on read, exactly as the patent.

**ADR-003 — Integer accumulators, int64 for colour.**
Chosen for determinism first and headroom second. See I4, I6.

**ADR-004 — No legalisation. Clamp only to v210 protocol limits.**
See I2.

**ADR-005 — 4:2:2 v210 only for I/O; 4:4:4 internally.**
4:4:4 SDI is not used in practice, so it is out of scope as a transport.
Internally, chroma is upsampled to 4:4:4 once at the front and downsampled once
at the back. Warping 4:2:2 directly is rejected: scatter computes an
independent destination per sample, so chroma lands at non-integer positions
relative to luma and reconstruction produces colour fringing that follows the
geometry. Output chroma must be low-passed before decimation because the warp
synthesises high-frequency chroma the source never had.

**ADR-006 — Host is the M1 Max MacBook Pro with UltraStudio 4K Mini. CPU
first; Metal deferred.**
The 4K Mini is Thunderbolt 3, full duplex, one clock, with composite and
component analogue inputs and 12G headroom. The M1 Max offers 8 performance
cores and roughly 400 GB/s memory bandwidth, which removes the binning
bandwidth concern entirely. Metal compute with imageblocks and integer atomics
maps onto this design almost unchanged and is the fallback if the splat
overruns budget at 1080p50 — not before.

**ADR-007 — Development starts at 576i25 / 576p25.**
10.4 Mpx/s versus 103.7 for 1080p50, a factor of ten of headroom, and it is the
standard Mirage actually worked in, enabling direct comparison with archive
footage.

**ADR-008 — `src/core/` and `src/video/` carry no platform dependencies.**
*Superseded by ADR-013.* Original rationale was an independent Linux
correctness oracle on a second machine. The module split is retained; the
rationale is not.

**ADR-009 — Phase 1 transparency is pure weighted accumulation. k-buffer
deferred.**
Accumulation is authentic — it is the patent's default behaviour — and requires
no depth buffer, no sorting and no per-pixel layer storage. The k-buffer is a
quality refinement for correct layered compositing and comes after the pipeline
works end to end.

**ADR-010 — Genlock is out of scope for the proof of concept.**
Free-running. Input timing derives from the source, output from the device
clock, so an unlocked source drifts against the output; the drift is logged and
tolerated. The 4K Mini has a sync input, so one BNC of black burst resolves it
whenever it matters. Playing from file there is nothing to drift against.

**ADR-011 — Diagnostic coverage view goes to the Mac display or the spare
UltraStudio Monitor 3G, not a second SDI output.**
The 4K Mini's "2 × program out" are near-certainly mirrored copies of one frame
buffer rather than independent channels. Supersedes an earlier assumption that
they could carry independent signals. See C-003.

**ADR-012 — Session-bounded development with repository-held state.**
See `SESSION-PROTOCOL.md`. The repository is the only authoritative record; the
assistant's recall is not.

**ADR-013 — Single machine. Supersedes ADR-008.**
Development, testing and hardware runs all happen on the M1 Max MacBook Pro
with the UltraStudio 4K Mini attached. No second build host, no cross-machine
verification, no remote git host required. The `src/core/` and `src/video/`
split is retained, but for a simpler reason than ADR-008 gave: those modules
link no Blackmagic SDK, so the bulk of the test suite runs with no hardware
connected and no driver installed.

**ADR-014 — Trunk-only git. No per-work-unit branches.**
Solo developer, one machine. Work commits directly to `main`; a green work unit
is marked with an annotated tag `wu-NN-green`. Abandoning a bad session is
`git reset --hard <last green tag>`, which is simpler than branch bookkeeping
and achieves the same thing.

**ADR-015 — Determinism oracle is the single-threaded build, not a second
machine.**
I6 survives ADR-013 intact and matters more without a second platform to diff
against. The reference is `--threads 1`: any multi-threaded run must produce
byte-identical output to it on the same input. This is checked in-suite rather
than by hand.

**ADR-016 — Hand-rolled test harness, not `assert()` and not a framework.**
Release builds define `NDEBUG`, which compiles `assert()` to nothing, so an
assert-based suite passes silently in exactly the configuration that ships.
`tests/harness.hpp` provides `CHECK` and `CHECK_ONCE`, always evaluated,
counted, and reported with a non-zero exit code. No external dependency:
nothing needs one yet and a dependency is a thing that breaks between sessions.

**ADR-017 — Warnings are errors, including `-Wconversion` and
`-Wsign-conversion`.**
The colour path is full of narrowing between 10-bit codes, 16-bit samples,
32-bit weights and 64-bit accumulators. An implicit conversion there is
usually an invariant being bent, so the compiler is configured to refuse it.
Verified achievable at WU-01: the full suite builds clean under this set.

**ADR-018 — v210 short-group padding is black, and deterministic.**
Widths that are not a multiple of 6 leave unused components in the final
16-byte group. These are packed as `kCode10Black` for luma and
`kCode10ChromaZero` for chroma, and ignored on unpack. Determinism is the
requirement: without it `unpack -> pack` is not byte-identical at such widths
and I7 could not be stated for them. Black rather than replication of the edge
pixel because a decoder that renders the full group should show nothing rather
than a smear. Neither 720 nor 1920 exercises this path.

**ADR-019 — Test pattern generation logic is header-only
(tools/testpat.hpp), shared by the tool and its test.**
`tools/make_testpat.cpp` is its own `add_executable`, not part of
`scatter-core`, and `tests/test_testpat.cpp` links only `scatter-core` (via
`scatter_test()`). Neither links the other's translation unit, so
`makeRamp`, `makeZonePlate`, `makeExcursion` and `writeV210` live in a
header both include, rather than being duplicated or exposed only through
the executable. Same interface/implementation split the project already
uses for v210 (WU-02); header-only here specifically because there is no
compiled library boundary between "tool" and "test" for this unit.

**ADR-020 — Chroma resampling filter coefficients, fixed at WU-04.**
`docs/architecture.md` section 5 specified filter *shape* only — "4- or
6-tap" for the 422->444 upsampler, "9- or 11-tap" half-band for the 444->422
downsampler — and left the concrete taps open. WU-18's NEON path needs
something byte-exact to diff against (the same relationship v210.hpp/.cpp
already has to WU-17), so this session picked and froze them:

- Upsample (co-sited-exact, one filtered tap per pair): `out[2i] = in[i]`
  exactly; `out[2i+1] = (-in[i-1] + 9in[i] + 9in[i+1] - in[i+2] + 8) >> 4`.
  Coefficients (-1, 9, 9, -1)/16 — the standard 4-tap half-sample
  interpolator, symmetric, unity DC gain, negative outer lobes.
- Downsample: `out[i] = (3in[2i-5] - 25in[2i-3] + 150in[2i-1] + 256in[2i] +
  150in[2i+1] - 25in[2i+3] + 3in[2i+5] + 256) >> 9`. Coefficients (3, -25,
  150, 256, 150, -25, 3)/512 — a standard truncated half-band low-pass; the
  half-band property means every tap at a nonzero even offset from centre
  is exactly zero, so nominally "11-tap" is 7 nonzero multiplies.
- Edge handling: index clamped to the plane (replicate the boundary
  sample), same choice both directions.
- Rounding: round-half-up via "add half the divisor, then arithmetic
  shift" — the same convention `core/types.hpp`'s `toCode10` uses. C++20's
  guaranteed-arithmetic signed right shift makes this round correctly for
  the negative partial sums the outer lobes produce, not just positive
  ones.
- No clamp on the numeric result beyond what a 16-bit unsigned `Sample` can
  hold. I2 forbids clamping to any legal-range value, and a
  representational-range wrap (well-defined modulo-65536 narrowing, not
  UB) is not that — it is not pulling anything toward a legal range, the
  container is just finite. Worst-case overshoot on a step is 1/16 of the
  step (upsample) or 22/512 (downsample), both worked by direct
  computation in `tests/test_chroma.cpp`; this does not reach the
  representable boundary for chroma content with any reasonable headroom
  around `kChromaZero`, and has not been exercised at the v210 protocol
  limits themselves.

Both filters sum to an exact power of two, so a flat field survives with
zero rounding error in either direction — the property `tests/test_chroma.cpp`
checks first, before the coefficients are checked individually.

**ADR-021 — `file_source.cpp`/`file_sink.cpp` compile into `scatter-core`,
not `scatter`, despite living under `src/io/`.**
`docs/architecture.md` section 8's module-layout sketch places all of
`src/io/` under the `scatter` application target, which links the Blackmagic
DeckLink SDK. Read literally, that would put this unit's raw `.v210`
reader/writer — needed now, at WU-05, five units before DeckLink arrives at
WU-14 — behind an SDK dependency it does not have: `file_source.cpp` and
`file_sink.cpp` use only `<fstream>` and `v210::pack/unpackImage`. Compiled
into `scatter-core` instead, alongside `src/video/`, so
`tests/test_ramp_roundtrip.cpp` — and everything after it that reads or
writes a file — keeps running with no hardware connected and no driver
installed, the same property ADR-013 states for that target. When WU-14
adds `com_ptr.hpp` and the `decklink_*.cpp` files, those alone define the
`scatter` target's actual DeckLink dependency; `file_source.cpp` and
`file_sink.cpp` do not move. Does not reopen ADR-013: the `src/core`/
`src/video` split ADR-013 describes is untouched, this only clarifies which
CMake target a `src/io/` file lands in, which ADR-013 did not itself
address.
