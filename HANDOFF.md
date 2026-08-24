# Handoff
Overwritten at the end of every session. This is the first thing to read.

---

**Session:** 54 (next sequential after Session 53's own `HANDOFF.md`; no
evidence of an intervening session — no later tag, no later commit, no
other `HANDOFF.md` account. WU-38, Phase 9 kickoff: accept ADR-085,
finalise I3/I4, open Phase 9. Documentation-only, per its own continuation
prompt — no production code touched).

**Tag:** `wu-34b-green` was the newest real tag at this session's own
start (confirmed directly: `git tag --sort=creatordate`, `git log
--oneline -10` showing `HEAD` = `37b236e` = `origin/main`, matching
Session 53's own WU-34b commit message exactly — Steve had already run
his own real-terminal close-out for WU-34b, including the manual tag and
push, before this session started). `git status --short` at this
session's own start read exactly one line: `?? docs/proposals/` — see
below. **This session's own changes are not yet committed, tagged or
pushed** — that is Steve's own next step, below.

## Before doing anything else in the next session

Run `git tag --sort=creatordate`, `git log --oneline -10`, `git rev-parse
HEAD origin/main` and `git status --short` directly against
`~/src/scatter-dve` — do not trust this file's own account without
checking it against the real repository first.

## A real discrepancy from this session's own opening prompt, caught before writing anything

The prompt that opened this session assumed `docs/proposals/ADR-085-draft-RGB-native.md`
existed **and was already committed**. Checked directly, first, per this
session's own standing instruction to verify real repository state rather
than trust the prompt's framing: the file existed on disk, but `git
status --short` showed `?? docs/proposals/` (untracked) and `git log
--all -- docs/proposals/` returned nothing — no commit had ever added it.
Most likely explanation, cross-checked against Session 53's own account
below and the draft's own §8 text: it was written in Session 53,
immediately after that session's own WU-34b/ADR-084 closed out, but
deliberately kept out of that session's own close-out (WU-34b is
Session 53's whole recorded "Closed out this session" list) and never
integrated into the state files, since it was not yet Steve-accepted —
consistent with `SESSION-PROTOCOL.md` rule 3 (never reopen an ADR;
propose a superseding one) treating `docs/proposals/` as a holding area
distinct from `DECISIONS.md`. Not a defect in Session 53's own delivered
work (WU-34b/ADR-084 is real, green, committed, tagged, pushed, per its
own tag `wu-34b-green` and this session's own `git log` check above) —
only a gap in what this session's opening prompt assumed about the
draft's own commit state. Resolved by not inventing a commit reference
that does not exist: `DECISIONS.md`'s new ADR-085 entry states plainly
that the draft was never committed and cites `git status --short`/`git
log --all` directly instead of a commit hash; this session's own
close-out commits the draft for the first time, alongside every other
file this session changed, in one commit (see Steve's own next steps,
below). No `CORRECTIONS.md` entry follows — nothing wrong was ever written
to a state file; this session's own job was exactly to write the state
files correctly the first time, which is what happened.

## This session in full

Opened with a continuation prompt whose own job was: accept
`docs/proposals/ADR-085-draft-RGB-native.md` into `DECISIONS.md`/
`INVARIANTS.md`/`WORK-UNITS.md`, re-derive I4 for real rather than assume
either way, open Phase 9, and — budget permitting — scope (not build)
`WU-39` with a real grep. Confirmed real repository state directly first
(git tag/log/status, above), then read `SESSION-PROTOCOL.md`, the five
state files in full, `docs/proposals/ADR-085-draft-RGB-native.md` in full,
`INVARIANTS.md` I2/I3/I4 specifically, `DECISIONS.md` ADR-003/ADR-005/
ADR-084 in full, and `docs/architecture.md` §2/§3, per `SESSION-PROTOCOL.md`
rule 6 and this session's own opening prompt.

**Job 1 — `DECISIONS.md` ADR-085 appended, accepted as written except I4.**
The draft's own content (motivation, what does/doesn't change, I3
supersession, scope-by-inspection, hard-cutover decision, work breakdown)
carried over with the "PROPOSED" framing stripped and stated plainly as
Steve's own direct acceptance this session — see the discrepancy note
above for why no prior commit is cited. `docs/proposals/ADR-085-draft-RGB-native.md`
itself is edited in place (status line only) to point at `DECISIONS.md`
ADR-085 as the now-settled entry, so a future reader does not mistake it
for still-open.

**Job 2 — `INVARIANTS.md` I3/I4 finalised.** I3 replaced with the draft's
own RGB text (terminology footnote on "pedestal"/C-004 carried forward
unchanged, since nothing about it is superseded). I4 **re-derived for
real, directly against `src/core/types.hpp`**, not assumed either way: the
draft's own premise — that today's bound was derived asymmetrically, "one
full-range channel (Y) plus two channels offset around a mid-point
(Cb/Cr)" — does not hold up against the real accumulator code.
`AccumCell`'s three `ColourAccum` (`int64`) fields all share one constant,
`kMaxFragContribution = 65535 * kWeightMax`, with no per-channel
distinction anywhere; the bound comes from each field's own storage-type
range alone, and I2 already lets any channel — chroma included — reach
the full range under filter ringing or fold-edge overshoot. So the bound
was already channel-agnostic before this ADR, and moving to three
full-range channels (RGB) changes nothing: **int64 accumulators remain
provably sufficient, headroom unchanged** (≈2.147 × 10⁹ full-weight
fragments per cell, same as today, on every channel). Full derivation is
in `DECISIONS.md` ADR-085 itself, not just the conclusion, per this
session's own brief. This came out resolved rather than ambiguous, so
nothing here needed raising with Steve directly — the brief's own
escalation trigger ("if int64 is no longer provably sufficient, or
reveals any other real ambiguity") did not fire.

**Job 3 — `WORK-UNITS.md` Phase 9 opened.** New `## Phase 9 — RGB-native
internal colour (hard cutover)` heading, inserted after Phase 8 and before
the `## Cross-cutting documentation units` section (keeping phase numbers
in ascending order, matching how every other phase heading in this file
already sits). `WU-38` (this unit) stubbed `green`; `WU-39` through
`WU-44` stubbed `todo` from ADR-085's own §6 breakdown, at the same level
of detail `WU-34c`/`WU-35`/`WU-37` already use for pre-scoped-but-not-yet-
built units (files likely touched, accept criterion sketched, real
dependencies named — not full implementation detail). `WU-39` additionally
carries this session's own real, repository-wide grep (below) rather than
an estimate.

**Job 4 — `WU-39` scoped with a real grep, not built.** `grep -rlw
'Frag'`/`'AccumCell'` across `src/` and `tests/`, each match's context
checked directly (not trusted from the grep alone) — one false positive
excluded (`src/io/com_ptr.hpp` only mentions "AccumCell" in a
naming-convention comment). Real result: 10 production files (2
comment-only) plus 12 test files reference `Frag` and/or `AccumCell`
directly — see `WORK-UNITS.md`'s own `WU-39` entry for the full list.
This is a real, different, more narrowly-scoped count than ADR-085's own
"21 of 35" figure, which counts a broader query (every file touching *any*
`Y`/`Cb`/`Cr`-shaped data, including `Raster444`'s own same-named planes,
a separate struct `WU-39` does not touch) — both figures are recorded,
attributed to what each actually measures, in `DECISIONS.md` ADR-085 and
`WORK-UNITS.md` `WU-39` respectively, rather than treating one as simply
wrong. This scoping came back small and clear (22 files, all with
well-understood mechanical rename semantics) but was **not built** this
session regardless, per this session's own opening instruction: `WU-39`
is left `todo` for its own dedicated session, the same restraint WU-34b's
own session used when it deferred WU-34c.

**No production source file touched.** `src/`, `tests/` and
`CMakeLists.txt` are all untouched this session — confirmed directly,
below.

## Where we are

**WU-38 is written and delivered — documentation only, matching the shape
WU-32/WU-36 already established for this kind of unit:**

| Compiler | Build type | `SCATTER_TILE_LOG2` | Result |
|---|---|---|---|
| GCC 13.3.0 | Release | 4 | 28/28 tests pass |
| GCC 13.3.0 | Release | 5 | 28/28 tests pass |
| GCC 13.3.0 | Debug | 4 | 28/28 tests pass |
| GCC 13.3.0 | Debug | 5 | 28/28 tests pass |

Run in a fresh `git clone` of `skunge2000/scatter-dve` at `HEAD` `37b236e`
(confirmed matching the real repository before any edit) in this
session's own Linux cloud sandbox — actually run, not assumed from "no
source file touched" alone, matching WU-32/WU-36's own precedent
(`docs/wu-audit-2026-08.md`'s four-configuration matrix). This is the
expected, trivial result for a documentation-only unit; the value is in
having actually run it rather than inferred it.

All five changed/added files (`DECISIONS.md`, `INVARIANTS.md`,
`WORK-UNITS.md`, `docs/proposals/ADR-085-draft-RGB-native.md`, this
`HANDOFF.md`) are written to the real repository via the device bridge,
re-staged and diff-confirmed byte for byte against what this session
intended — see the confirmation note below — but **not yet committed,
tagged or pushed**. That is Steve's own next step, after his own
real-terminal build/test confirms green (expected trivially, since no
production file changed).

## Next work unit

**`WU-39`** (`core/types.hpp`'s `Frag`/`AccumCell` `Y`/`Cb`/`Cr` → `R`/`G`/`B`
rename) is Phase 9's own next pick — scoped this session with a real
22-file grep (`WORK-UNITS.md`), not built. Its own dependency is nothing
upstream; every later Phase 9 unit (`WU-40`–`WU-44`) depends on it
landing first. **Reminder for whoever picks it up:** per ADR-085 §5 and
this phase's own standing exception, `WU-39` is not expected to leave the
build green at the end of the unit — report red honestly if that is what
happens; this is accepted, not a failure. Everything named in earlier
sessions' own "Next work unit" sections outside Phase 9 (WU-28d, WU-29,
WU-33, WU-35, WU-37) is unchanged and still pickable — WU-37 (specular
model LUTs) remains blocked exactly as before; this session did not touch
any of them.

## Open questions

Unchanged from earlier sessions' own list (`kCaptureRingCapacity`, Q3, Q4,
Task A1, Task D6) — this session did not touch any of them. ADR-085 §7's
own open sub-questions are now one item shorter (I4's magnitude bound is
resolved, see `DECISIONS.md`) — the rest (where `ColourStandard`/
`coeffsFor` should live; per-frame boundary-conversion parameterisation;
fixture-value re-derivation strategy) stay open for whoever starts the
relevant Phase 9 unit.

## Blocked / red

Not blocked. Not red. This unit touches no production source file, so the
sandbox matrix stays green trivially — confirmed, not assumed, above.
`ctest` was not run on Steve's own real terminal yet this session — see
"Steve's own next steps" below, though a red result here would be
surprising news given nothing buildable changed.

## Environment check

This session had GCC 13.3.0 in its own cloud sandbox (confirmed via `gcc
--version` directly before building); the four-configuration matrix ran
for real. A fresh `git clone` of `https://github.com/skunge2000/scatter-dve.git`
was used for the sandbox build (not a copy staged through the device
bridge), confirmed matching `HEAD 37b236e` before any edit. The
device-bridge sandbox used for reading/writing files on the real Mac
repository had ordinary read/write access this session; no
`.git/index.lock` stray files encountered. No `git commit` or `git push`
attempted this session — nothing pushed. C-024's standing condition (PSU
out, `tools/close.sh` cannot succeed on Steve's own real terminal for any
unit) checked directly against `CORRECTIONS.md` this session and found
unchanged — `./tools/close.sh` must not be run.

## Append to DECISIONS.md

**ADR-085**, appended this session — see above and the real
`DECISIONS.md` for the full text. Covers the RGB-native acceptance
(motivation, what does/doesn't change, I3 supersession, I4 re-derivation
in full, scope, hard-cutover decision and its standing green-suspension
exception, and the work breakdown now real in `WORK-UNITS.md` Phase 9).

## Append to CORRECTIONS.md

None this session. The draft-vs-commit discrepancy (above) does not
qualify — nothing wrong was ever written to a state file; this session's
own job was to write the state files correctly the first time, and did.

## Closed out this session

**WU-38, Phase 9 kickoff — documentation only, matrix confirmed green in
the cloud sandbox, not yet committed.** `DECISIONS.md` (ADR-085
appended), `INVARIANTS.md` (I3, I4 replaced), `WORK-UNITS.md` (Phase 9
opened: `WU-38` entry, `WU-39`–`WU-44` stubbed), `docs/proposals/ADR-085-draft-RGB-native.md`
(status line updated to ACCEPTED, pointing at `DECISIONS.md`). This
`HANDOFF.md`.

## Steve's own next steps

At your own real terminal, confirm a real, green (modulo the standing
duplex exception) build and test run — expected trivially, since this
session touched no production source file:

```
cd ~/src/scatter-dve
cmake --build build
ctest --test-dir build --output-on-failure
```

Expect `test_decklink_device`'s own
`test_at_least_one_device_is_full_duplex` check to fail regardless (the
standing PSU/two-device exception, `DECISIONS.md` ADR-034/035/037,
`CORRECTIONS.md` C-024) — not this unit's own problem, and this unit
touches nothing DeckLink-related, or indeed nothing buildable, at all.
Every other test should pass unchanged from `wu-34b-green` — if anything
else fails, that is real, surprising feedback for the next session, not a
reason to force a tag past it. **Do not run `./tools/close.sh`** — see
`CORRECTIONS.md` C-024: it treats any `ctest` failure as blocking and
refuses to tag, and the duplex-check exception means it can never succeed
here regardless of which unit is being closed.

**Only once you've confirmed a real, green (modulo the standing duplex
exception) build and test run**, close out with:

```
cd ~/src/scatter-dve
git add DECISIONS.md INVARIANTS.md WORK-UNITS.md HANDOFF.md docs/proposals/ADR-085-draft-RGB-native.md
git commit -m "WU-38: accept ADR-085 (RGB-native internal pipeline, hard cutover), finalize I3/I4, open Phase 9"
git tag -a wu-38-green -m "WU-38: accept ADR-085 (RGB-native internal pipeline, hard cutover), finalize I3/I4, open Phase 9"
git push origin main
git push origin --tags
```

This exact list of five paths was checked against a real `git status
--short` run through the device bridge immediately before this block was
written (`CORRECTIONS.md` C-026's own general lesson) — it should read
exactly: `M DECISIONS.md`, `M INVARIANTS.md`, `M WORK-UNITS.md`,
`M HANDOFF.md`, `A docs/proposals/ADR-085-draft-RGB-native.md` (new to
git, though it already existed on disk before this session), and nothing
else. Still worth a quick `git status --short` yourself before pasting
this block, since time has passed since that check.
