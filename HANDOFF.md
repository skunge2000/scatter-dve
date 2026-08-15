# Handoff
Overwritten at the end of every session. This is the first thing to read.

---

**Session:** 13
**Tag:** none yet this session. Last confirmed: `wu-12a-green`. WU-12b is
implemented and verified in the cloud sandbox but **not yet run through
`./tools/close.sh 12b`** on the M1 Max with AppleClang — that step needs a
human at the real terminal (see "Next" below). Do not tag `wu-12b-green`
until that comes back clean.
**Phase:** 2 — Shapes. WU-12b — priority-tag opaque mode — implemented and
cloud-verified this session; `core/resolve.hpp`/`.cpp` only, per
`DECISIONS.md` ADR-028's own scope split and ADR-029 (new this session),
which freezes `compositeLayered()`'s name, signature and behaviour. Once
`close.sh 12b` confirms green, WU-12 as a whole is done — both of US
4,563,703 FIG. 5's modes reproduced (WU-12a's transparent default, WU-12b's
opaque-with-priority-tag) — and the WU-12a/WU-12b split in `WORK-UNITS.md`
does not need reconciling back into one entry (matching how WU-03's own
stale status line was corrected in place rather than erased, WU-04's own
session — same precedent, a different kind of non-reconciliation).

**Tests:** All thirteen green in a Linux cloud sandbox — the twelve carried
over unchanged from WU-01 through WU-12a plus `test_layered_composite`, new
this session (28 checks: four direct `AccumCell` unit-test cases against
`compositeLayered()` alone — tag-mismatch sum path, and the opaque path's
full-alpha/zero-alpha/partial-alpha edges, each checked by exact equality,
not tolerance — plus two pipeline-level checks reusing WU-12a's own
page-turn-flap-over-page-behind construction, duplicated locally per
SESSION-PROTOCOL.md rule 2, checked exactly against an independent local
re-derivation of the read-replace-write formula). 336976 checks total
across all thirteen executables (Clang 18, Release, tile 2^5).

Verified in the cloud sandbox: Clang 18 and GCC 13, Release and Debug,
`SCATTER_TILE_LOG2` 4 and 5 — eight configurations, all green, zero
warnings, checked explicitly in the build logs (not just exit codes) —
under the project's full `-Wall -Wextra -Wpedantic -Wconversion
-Wsign-conversion -Werror` set, plus GCC 13 with `-fsanitize=address,undefined
-fno-sanitize-recover=all` at both tile sizes: clean, no ASan or UBSan
report anywhere. Same practice as every session since WU-06.

**Build:** clean under `-Werror -Wconversion -Wsign-conversion` on Clang 18
and GCC 13 in the cloud sandbox. **Not yet built or tested on AppleClang /
the M1 Max this session** — see "Next" below.

## Where we are

WU-12b adds `compositeLayered()` to the existing `core/resolve.hpp`/`.cpp`
— no new source file for `scatter-core` itself, only its own new test
executable. Given two already-splatted `AccumCell` layers (`lower`,
`upper`), the upper layer's own tag, a caller-configured opaque tag, and a
`Background`, it returns a `CompositedCell`: where the upper tag matches
the opaque tag, `lower` is composited against `bg` first (the "read"),
then `upper`'s own resolved colour is composited over *that* result using
`upper`'s own alpha (the "write", replacing what was read) —
architecture.md 4.7 phase 2's own "read-replace-write" for exactly two
caller-ordered layers, not WU-28's general k-buffer (ADR-009 unchanged).
Where the tags don't match, the two `AccumCell`s are summed component-wise
first (WU-12a's own accumulation-sums identity, exact per I6) and
composited once — WU-12a's own default, reused not reimplemented.

**Design choices this session had to make that ADR-028's own sketch left
open — now ADR-029 in `DECISIONS.md`:** the function's actual name (kept
`compositeLayered`, ADR-028's own placeholder — nothing surfaced while
implementing it that argued for a different name) and full signature
(`CompositedCell compositeLayered(const AccumCell& lower, const AccumCell&
upper, std::uint8_t upperTag, std::uint8_t opaqueTag, const Background& bg
= kDefaultBackground) noexcept`, parameter order and the defaulted `bg`
mirroring `composite()`'s own shape); the implementation approach — two
calls to `composite()` (the "read" step's `CompositedCell` reinterpreted as
a `Background` for the "write" step, `asBackground()`) rather than a
hand-rolled blend, which gets partial upper coverage correct for free at
both edges (zero alpha -> lower alone unaffected; alpha at or above
`kWeightUnity` -> upper's own colour outright) instead of by extra
casework; and confirming (not deciding fresh — ADR-028's own sketch already
described it this way) that the opaque tag is a function parameter, not a
`PipelineParams` field, since nothing in WU-12b's own scope adds an
orchestration entry point that would need one. See ADR-029 for the full
reasoning, including why only the upper layer's tag is read and not the
lower's.

**Corrections this session:** none. No implementation choice made while
writing `core/resolve.cpp` or `tests/test_layered_composite.cpp` turned out
to contradict an earlier claim in `DECISIONS.md`, `INVARIANTS.md` or
`CORRECTIONS.md` — the "reuse `composite()` twice" approach was checked
directly (the exact-equality tests in `tests/test_layered_composite.cpp`'s
own Part A), not merely assumed correct.

**Delivery mechanics, not a design matter:** this session ran remotely, via
the device-bridge tools connecting to this machine, same as sessions 6
through 12. Implementation and the full verification matrix above ran
first in a disposable Linux cloud sandbox, never on this machine directly.
Files were then written to this machine via the bridge, and `git add -A &&
git commit` ran through that same bridge; as in prior sessions it still
cannot clean up its own `index.lock`/`HEAD.lock`/temp-object files
afterward (unlink fails on this mount), so stale ones were moved into
`_to_delete/` rather than removed — safe to `rm -rf _to_delete/` by hand;
it holds accumulated debris from several sessions, not just this one. Git
identity was already set locally on this mount from a prior session
(`Stephen Neal <stephenneal@Stephens-MacBook-Pro.local>`, confirmed against
`git log`/`git config` before committing), so nothing needed
reconfiguring.

## Next

**Run `./tools/close.sh 12b` at the real terminal on the M1 Max.** This
session could not — `close.sh` needs AppleClang, unreachable from a cloud
session, same reason every prior session's own `close.sh` run has been by
hand. If it comes back green: update `WORK-UNITS.md`'s WU-12b status line
from `wip` to `green` (matching WU-12a's own close-out) and tag
`wu-12b-green`; at that point WU-12 as a whole is done and the next session
should move on to WU-13 (`WORK-UNITS.md`'s own ordering — keyframed
lattices, temporal interpolation/morph). If it comes back red: the next
session isolates the failure, fixes it within the smallest possible file
scope, re-verifies the full matrix, ships the fix as its own commit, and
asks for `close.sh 12b` again — same process every prior AppleClang
divergence (e.g. WU-11's own C-012) has used.

## Open questions

Unchanged from WU-10/WU-11/WU-12a: Q1 (tile size), Q2 (4K Mini program
outputs), Q3 (macOS/Desktop Video version) — all still open, none blocking.
Q4 (`core/lattice.cpp`'s `jacobian()` edge damping, C-008(a)) — still open,
still not urgent; this session's own work is entirely within
`core/resolve.*` and adds no new evidence either way.

No new open question from this session beyond what ADR-029 already
resolved — see "Design choices" above.

## Blocked / red

Nothing red. WU-12b is `wip`, cloud-verified, awaiting `close.sh 12b` on
the M1 Max before it can be marked `green` and tagged.

## Environment check still outstanding

Unchanged from session 2 — Desktop Video / UltraStudio 4K Mini smoke test,
independent of WU-12b and costs no session time.

## Append to DECISIONS.md

ADR-029 was appended in full earlier this session; see `DECISIONS.md`. Not
reopened or amended now.

## Append to CORRECTIONS.md

Nothing this update — see "Corrections this session" above; nothing to
log.
