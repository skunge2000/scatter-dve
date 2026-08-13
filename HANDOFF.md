# Handoff

Overwritten at the end of every session. This is the first thing to read.

---

**Session:** 0 (design and scaffolding)
**Tag:** none yet — commit this scaffolding as `wu-00-green`
**Phase:** 1 — portable core, file to file, 576p25, single-threaded
**Tests:** none yet
**Build:** none yet

## Where we are

Design complete. Architecture in `docs/architecture.md`, invariants frozen in
`INVARIANTS.md`, twelve decisions recorded, five known errors logged. No code
written.

## Next work unit

**WU-01 — Repo skeleton.**

Deliver: `CMakeLists.txt`, `src/core/types.hpp`, `tests/test_smoke.cpp`.

Scope notes for whoever picks this up:

- `types.hpp` holds the fixed-point aliases and the 16-byte `Frag` struct from
  `docs/architecture.md` §4.3. Static-assert `sizeof(Frag) == 16`.
- Colour type is `uint16_t` offset-binary per I3. Accumulator types
  `int64_t` for colour, `int32_t` for weight, per I4.
- Test harness: plain asserts plus CTest is sufficient. No external dependency
  unless a later unit justifies one.
- Two targets: `scatter-core` (no Blackmagic SDK, most tests run against it)
  and `scatter` (the app, links DeckLink). WU-01 only needs `scatter-core`.
- C++20, `-O3 -mcpu=apple-m1`.

**Session open command:**

```
./tools/context.sh
```

Nothing to pass — there are no source files yet.

## Open questions

- **Q1.** Tile size: 16×16 (32 KB across four banks, fits M1 L1D) versus 32×32
  (128 KB, exactly L1D, less edge replication). Empirical. Make it a
  compile-time constant in WU-01 so WU-09 can benchmark both. Do not decide now.
- **Q2.** Whether the 4K Mini's two program outputs are genuinely mirrored.
  Affects nothing before WU-14. Verify with Desktop Video Setup when convenient.
- **Q3.** macOS version and matching Desktop Video release on the target
  MacBook. Blocks WU-14, nothing earlier. Resolve during Phase 0 environment
  check.

## Blocked / red

Nothing.

## Environment check still outstanding

Phase 0 from `docs/architecture.md` §10: install Desktop Video, approve the
system extension, confirm the UltraStudio 4K Mini enumerates with both input
and output, capture and play a clip in Media Express. Independent of WU-01 —
can be done in parallel and costs no session time.
