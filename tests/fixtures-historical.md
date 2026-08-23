# Historical fixtures

The 32 regression fixtures named in `docs/sources/WU-SM-01.md` §8 (1–26) and
`docs/sources/WU-SM-02.md` §8 (27–32). This table is the WU-32 deliverable —
most fixtures are not runnable yet, because the work unit that would build
the code they exercise has not been built. Do not read a `not runnable` row
as a test that exists and passes; it is a planned regression, tracked here
so a future session scoping the owning work unit does not have to
rediscover it from the research documents.

Numbering matches the source documents exactly (WU-SM-02 continues WU-SM-01's
own sequence).

| # | Fixture | Owning WU | Status |
|--:|---|---|---|
| 1 | Rolling cone (`sin θ = 1/n`, n = 2) | *unscoped* | Not runnable — no scatter-dve effect/transform authoring unit exists yet (`ADR-SM-004`/`008`, not promoted) |
| 2 | Corner-to-corner flat rotate (conjugation) | *unscoped* | Not runnable, same reason as #1 |
| 3 | Two-angle non-composition (45° axis) | *unscoped* | Not runnable, same reason as #1 |
| 4 | S1 four-sweep example effect | *unscoped* | Not runnable — no effect/sweep model exists yet; loosely Phase 8 |
| 5 | S2 spin destination (winding + terminal phase) | *unscoped* | Not runnable, same reason as #1 |
| 6 | S2 notch grid (POS = 1 unit) | *unscoped* | Not runnable — control-surface unit, not yet named |
| 7 | S2 perspective (56 units default, 1/d²) | *unscoped* | Not runnable — this project's own current projection model has not been reconciled against S2's historical parameterisation (`ADR-SM-008` not promoted) |
| 8 | Default control tree topology | *unscoped* | Not runnable — no scatter-dve axis-tree implementation exists yet (`DECISIONS.md` ADR-067 is documentation only) |
| 9 | Default lighting state (one light visible) | WU-27 | Not runnable — WU-27 not yet scoped past `DECISIONS.md` ADR-069/070/071 |
| 10 | Filtering ladder (`0`…`+3`) | WU-34 | Not runnable — WU-34 not yet scoped |
| 11 | Grid shift (`Shift 1` moves pattern one cell) | WU-34 | Not runnable — WU-34 not yet scoped |
| 12 | Negative-intensity cancellation | WU-27 | Not runnable |
| 13 | Beam bidirectionality | WU-27 | Not runnable |
| 14 | Parallel-light direction (lights 1/2 vs 3–6) | WU-27 | Not runnable |
| 15 | Point-source limit → parallel light | WU-27 | Not runnable |
| 16 | Zone locking (lights 3/5 zone 1, 4/6 zone 2) | WU-27 | Not runnable |
| 17 | Illumination formula, hand-worked case | WU-27 | Not runnable — this is the fixture that directly exercises `DECISIONS.md` ADR-069; should be among WU-27's first tests once scoped |
| 18 | Distance falloff is linear, not inverse-square | WU-27 | Not runnable |
| 19 | Normal from a three-sample facet, attributed to P | WU-34 | Not runnable. **Design tension, not yet resolved:** this is a different quantity from WU-26's exact analytic `surfaceNormal()` — see `DECISIONS.md` ADR-070's own open note |
| 20 | Splat weights sum to unity across 4 pixels | *existing, informally* | `tests/test_splat.cpp` already covers proportional four-pixel splat weighting for this project's own splat path; re-check at WU-33/WU-34 scoping rather than treating as new |
| 21 | Opaque sphere — no 50/50 blend at the terminator | WU-35 | Not runnable — WU-28a/WU-28b's shipped k-buffer can express `Opaque` mode structurally, but nothing wires real front/back content into it yet (WU-28d, and WU-33 for real content); the transparency-coefficient rule itself is WU-35 |
| 22 | Minification averages; occlusion does not | WU-35 | Not runnable |
| 23 | Visible back faces, from S7 | WU-33 + WU-28d | Not runnable — needs WU-33's real front/back sources; WU-28c's tag mechanism (already `wip`) and WU-28d's demo wiring (`todo`) are the occlusion half |
| 24 | Non-convex self-occlusion (PAGE TURN, SPINY NORMAN) | WU-35 | Not runnable |
| 25 | Front/back source switching, independent freeze | WU-33 | Not runnable — WU-33 not yet scoped |
| 26 | Orthographic specular (no highlight tracking on lateral move) | WU-27 | Not runnable |
| 27 | Same-sheet accumulation at a grazing angle | WU-35 | Not runnable — exercises ADR-072 §4.1's Jacobian-derived tolerance directly |
| 28 | Two sheets at a silhouette (must not blend) | WU-35 | Not runnable — complement of #27; the two together bracket `κ` |
| 29 | Scan-order invariance, four `(u, v)` directions | **WU-32 (this unit)** | **Partially delivered.** `tests/test_scan_order_invariance.cpp` covers the row (`v`) traversal axis only, using the existing `generateFragmentsRowRange()` public API with no production-code change; the column (`u`) axis needs a traversal-direction parameter `core/binner.cpp` does not expose today, which is production code outside this documentation-only unit's scope. See that test file's own header. |
| 30 | Transparency sweep, `T` from 0 to 1 | WU-35 | Not runnable — exercises ADR-072/`ADR-SM-020` end to end |
| 31 | `Opaque` and `Trail` are mutually exclusive | *blocked* | Not runnable — no `Trail` facility exists in scatter-dve yet (no owning WU); depends on both a future Trail unit and WU-35. **Do not silently make the combination work when both eventually exist** — reproducing the historical exclusion is evidence the arbitration mechanism is right, per `docs/sources/WU-SM-02.md` fixture 31 itself |
| 32 | Möbius closure, no self-punch-through | WU-35 | Not runnable — needs a Möbius-strip shape generator (none exists in `src/core/shapes/` today) in addition to WU-35 |
