# docs/sources — immutable research artefacts

Research documents dropped in verbatim, with their own numbering and
confidence-tier scheme (`[A]`/`[B]`/`[C]`/`[P]`) intact. Not edited here even
when a later document corrects an earlier one — the correction lives in the
later document's own text (each carries its own "corrections carried in
from" section), the same append-only discipline `DECISIONS.md`/
`CORRECTIONS.md` use for this repository's own state.

Do not promote a proposed `ADR-SM-nnn` into `DECISIONS.md` by editing these
files. Add a new `ADR-0nn` entry to `DECISIONS.md`, with its own provenance
line pointing back here, and add a row to the mapping table below.

| Document | Status | Scope |
|---|---|---|
| `WU-SM-01.md` | Draft 0.3 | Mirage shape, UI and storage model — five primary Quantel documents (S1–S5) across three machine generations, plus S6 identified but not yet held |
| `WU-SM-02.md` | Draft 0.1 | Surface arbitration and the front/back source pair — gates `ADR-SM-017`; informs WU-26/WU-27/WU-28 |

## ADR-SM → ADR mapping

Only the subset that binds scatter-dve's own implementation is promoted.
Most `ADR-SM-nnn` entries are claims about the historical machine, not
decisions about this repository's code, and stay `Proposed`/`Resolved` in
the source document only — WU-32 (`WORK-UNITS.md`) promoted the nine items
below and no others. An `ADR-SM` id not in this table has not been
promoted; check the source document's own §9/ledger for its status there.

| Source ADR | Source doc | This repository | Note |
|---|---|---|---|
| ADR-SM-003 | WU-SM-01 §3.9 | `DECISIONS.md` ADR-066 | "Address map + validity mask" — promoted with mask *width* left open (WU-SM-02 Task A5 unresolved); do not read this as "soft stencil, settled" |
| ADR-SM-009 | WU-SM-01 §5.6.1 | `DECISIONS.md` ADR-067 | WORLD-rooted axis tree; documentation only, no scatter-dve axis/effect model exists yet to change |
| ADR-SM-014 | WU-SM-01 §3.9.2 | `DECISIONS.md` ADR-068 | Shade pre-projection; resolve-time shading unavailable |
| ADR-SM-015 | WU-SM-01 §4.6.1–4.6.3 | `DECISIONS.md` ADR-069 | Phong, not Blinn-Phong — renames `WORK-UNITS.md` WU-27 |
| ADR-SM-011 | WU-SM-01 §3.9.1 | `DECISIONS.md` ADR-070 | Coarse-grid facet shading, filtering ladder, grid shift |
| ADR-SM-012 | WU-SM-01 §4.5.1 | `DECISIONS.md` ADR-071 | Material owned by `(light, zone)`, not surface |
| ADR-SM-016, ADR-SM-020, ADR-SM-022 | WU-SM-02 §3.4, §4, §4.0 | `DECISIONS.md` ADR-072 | Accumulate within sheet; transparency-coefficient `T` between sheets — explicitly does **not** describe the already-shipped WU-28a/WU-28b k-buffer |
| ADR-SM-018 | WU-SM-01 §3.9.4.3 | `DECISIONS.md` ADR-073 | Front/back video source pair, selected by facing |
| ADR-SM-017 | WU-SM-02 §3.2–3.4, §9 | `DECISIONS.md` ADR-074 | Arbitration mechanism (M1/M2/M3) not settled; swappable-interface requirement |

Not promoted this session (remain `Proposed` in `WU-SM-01.md` §9 only):
`ADR-SM-001`, `002`, `004`–`008`, `010`, `013`, `019`, `021` (021's
substance — shading pre-projection — is covered by `ADR-068`/`ADR-SM-014`
directly; `019`'s three-level depth distinction is recorded in
`WU-SM-02.md` §2 itself and needs no separate scatter-dve promotion since
this repository's own lattice format already only ever stored the first of
the three levels). A future unit that actually needs one of these — the
project-file-as-command-log model, the sparse masked state-delta model, the
spin-as-destination semantics, and so on — should promote it then, with its
own provenance line, rather than this unit reaching ahead of its own scope.
