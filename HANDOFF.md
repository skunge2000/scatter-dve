# Handoff
Overwritten at the end of every session. This is the first thing to read.

---

**Session:** 11 (continued — first `./tools/close.sh 11` attempt was red;
this is the fix)
**Tag:** none yet. WU-11 is still `wip` in `WORK-UNITS.md`. The fix below
is verified in the Linux cloud sandbox only — **`./tools/close.sh 11` needs
to run again on the M1 Max** before this can go green and get tagged.
**Phase:** 2 — Shapes. WU-11 (cylinder and sphere) is the first unit in
it, in progress.

**Tests:** All eleven green in the cloud sandbox again after the fix
below — same eleven as the prior handoff entry
(`test_smoke`/`test_v210`/`test_chroma`/`test_ramp_roundtrip`/
`test_jacobian`/`test_ewa`/`test_binner`/`test_splat`/`test_zoneplate`/
`test_testpat` unchanged, `test_shapes` with one test function's assertions
loosened — see "What went wrong" below). Verified again on Clang 18 and
GCC 13, Release and Debug, tile 2^4 and 2^5 (eight configurations, zero
warnings), plus GCC 13 ASan+UBSan at both tile sizes: clean, no reports.
**Not yet re-run on the M1 Max/AppleClang** — that's the next step.

**Build:** clean under `-Werror -Wconversion -Wsign-conversion` on Clang 18
and GCC 13 in the cloud sandbox. AppleClang status: previously failed at
test level, not build level (see below); unverified since the fix.

## What went wrong

You ran `./tools/close.sh 11` on the M1 Max and it failed one check:

```
FAIL test_shapes.cpp:230  c.x == s.x
FAIL test_shapes (1 of 83385 checks failed)
```

That line was inside
`test_sphere_reduces_to_cylinder_cross_section_at_zero_vertical_span()`,
which compared `buildCylinderLattice()`'s `x`/`z` against
`buildSphereLattice()`'s `x`/`z` (with `angleSpanV == 0`) using `==` —
bit-exact equality — across all 16641 control vertices, at one specific
grid point out of those 16641. This is now `CORRECTIONS.md` C-012: the
reasoning going in (multiplying by an exactly-`1.0` `cos(0)` cannot
introduce rounding) is true of that one step in isolation, but doesn't
extend to asserting two *differently-shaped* floating-point expressions
(`cylinder.cpp`'s one-multiply `radius*sin(theta)` versus `sphere.cpp`'s
two-multiply `radius*sin(phi)*cosPsi`) agree to the literal last bit on
every compiler/platform — FMA contraction, reassociation, and
transcendental rounding can all legally differ between them without
violating IEEE 754. This session tried to reproduce the exact failure in
the Linux sandbox (Clang 18 on x86_64, forcing `-mfma -ffp-contract=fast`
to simulate ARM64's native FMA) and could not — the precise trigger on
AppleClang/ARM64 isn't conclusively identified, and C-012 says why pinning
it down further isn't necessary: the fix is the same regardless of
mechanism.

**Fixed within `tests/test_shapes.cpp`'s own file:** that one test's
`x`/`z` comparisons now use a tight relative tolerance (`1e-12`, about
4000x a double's ~1-ULP noise floor) via the same `relClose()` helper the
Jacobian checks already use, instead of `==`. The `y` comparison in the
same test stays exact — it only ever involves multiplying by an exact
zero, which has no rounding to differ on regardless of platform. No
production code changed: `core/shapes/cylinder.cpp` and `sphere.cpp` are
untouched, both compute the geometrically correct surface (still checked
exactly, via the on-the-sphere/on-the-cylinder algebraic identity, with
its own already-appropriate `1e-9` tolerance in the other two vertex
tests, which never hit this issue). This was a mistaken assumption in this
session's own test assertion, caught by real hardware doing exactly what
`SESSION-PROTOCOL.md`'s cloud-sandbox-first process is for.

Full account, including the failed reproduction attempt and the general
lesson for future units (don't assert `==` between two differently-shaped
floating-point expressions, even when the algebra guarantees equality —
use a tight relative tolerance instead, reserving `==` for values
provably free of rounding), is in `CORRECTIONS.md` C-012.

## Next steps (before WU-11 can close)

1. At the real terminal, on the M1 Max: `./tools/close.sh 11` again. Only
   `tests/test_shapes.cpp` and `CORRECTIONS.md` changed since the last
   attempt (plus this file); `src/core/shapes/*.cpp` are untouched.
2. If green: update `WORK-UNITS.md`'s WU-11 line to `green`, append a
   short confirmation to this file, tag `wu-11-green`.
3. If red again: paste the output back. Given the fix directly targets the
   only prior failure and nothing else in the file makes a similar
   bit-exact cross-expression assertion (checked this session — see
   "Swept for similar risk" below), a repeat failure at the same line
   would be surprising and worth a closer look rather than another
   tolerance bump.

**Swept for similar risk this session:** every other `==`/`CHECK` on a
`double` in `tests/test_shapes.cpp` was re-examined for the same class of
bug. All of them either compare against a value that provably involves no
rounding (front-facing-point spot checks at `theta == 0`/`phi == psi ==
0`, where `sin(0) == 0` and `cos(0) == 1` exactly and every subsequent
operation multiplies by an exact zero or adds an exact zero — safe on any
platform), or already used `relClose()` (the surface-identity checks, the
Jacobian checks). Only the cylinder/sphere cross-check compared two
differently-shaped non-trivial expressions with `==`; that's the one
that broke, and the one that's now fixed.

## Next work unit (after WU-11 closes)

Unchanged from the prior handoff entry: **WU-12 — Page turn**, not scoped
yet. See that entry (preserved in git history, this session's prior commit)
for what the next session should read before scoping it.

## Open questions

Unchanged: Q1 (tile size), Q2 (4K Mini outputs), Q3 (macOS/Desktop Video
version), Q4 (`core/lattice.cpp` edge damping) — all still open, none
blocking.

## Blocked / red

Nothing red now. WU-11 is `wip`, the AppleClang failure from the first
`close.sh 11` attempt is fixed and re-verified in the cloud sandbox,
waiting on a second `close.sh 11` run to confirm on real hardware.

## Environment check still outstanding

Unchanged from session 2 — Desktop Video / UltraStudio 4K Mini smoke test,
independent of WU-11 and costs no session time.

## Append to DECISIONS.md

Nothing this update. ADR-027 (appended earlier this session) is unchanged
by this fix — it never claimed bit-exactness, only the algebraic identity,
which remains true and unaffected.

## Append to CORRECTIONS.md

Nothing this update — C-012 was appended in full earlier this session; see
`CORRECTIONS.md`. Not reopened now.
