# Handoff
Overwritten at the end of every session. This is the first thing to read.

---

**Session:** 69 (`WU-35a1` — `compositeKBuffer()`'s `Blend`-mode
between-sheet step gains `manualTransp`, DECISIONS.md ADR-088; `WU-35a2`
scoped but not built, DeckLink-SDK-linked).

**Tag:** the newest real commit at this session's own start was `147bb51`
(confirmed directly: `git fetch origin`, `git log --oneline -5`,
`git tag -l | tail -5`, `git rev-parse HEAD origin/main` — both
`147bb513e73cf20c52ebbdc321a41347d11f1ba4` — `git status --short` read
empty, clean tree). Deliberately untagged, sitting directly on top of
`wu-45-green` (`git describe --tags` on it: `wu-45-green-1-g147bb51`, one
commit past the tag, not an exact match — expected, per this session's own
continuation prompt, since that commit was a documentation-only note
(`DECISIONS.md` ADR-087, `WORK-UNITS.md` WU-35/WU-35a) never meant to be
tagged on its own). One discrepancy surfaced before proceeding: the
continuation prompt described that commit's message as *starting with*
"Docs/planning only: ADR-087 (WU-SM-02 Task A1 actioned...", but the real
message opens with unrelated content (a WU-25 pause note, WU-23b1/b2a/b2b
label corrections, C-034) before reaching the ADR-087/WU-35a material
partway through. Everything else about the expected state matched exactly
(HEAD/origin/main, untagged-on-top-of-wu-45-green, clean tree), so this
was flagged to Steve directly rather than assumed away; Steve confirmed
proceeding, treating it as an imprecise paraphrase rather than a
wrong-commit problem. State (a), genuinely confirmed (with that one
flagged, resolved exception), so the session proceeded.

## Before doing anything else in the next session

Run `git fetch origin`, `git log --oneline -5`, `git tag -l | tail -5`,
`git rev-parse HEAD origin/main`, `git describe --tags --exact-match HEAD`
and `git status --short` directly against `~/src/scatter-dve` — do not
trust this file's own account without checking it against the real
repository first. Also check `.git/index.lock` directly (this session's
own recurring quirk, again present at close — see "Environment check"
below) and whether it actually blocks git (`git add --dry-run -A`) before
concluding either way.

## This session in full

**Scoped `WU-35a` for real, per this session's own explicit first job —
not assumed from `WORK-UNITS.md`'s own carve-out note.** Read
`SESSION-PROTOCOL.md`, this file (Session 68), `INVARIANTS.md`,
`DECISIONS.md` ADR-085 through ADR-087, `WORK-UNITS.md`'s `WU-28d`
(superseded), `WU-35` and `WU-35a` entries, and
`docs/sources/WU-SM-02.md` §3.4/4/4.0/6/7/8 in full before touching any
code. Confirmed directly against the real code: `compositeKBuffer()`
(`core/resolve.hpp`/`.cpp`) is unmoved since `WU-28b`, called from exactly
one site (`core/pipeline.cpp`'s `resolveOneTile()`); its `Blend` mode
folds occupied k-buffer slots farthest-to-nearest, and — checked before
writing any code, not discovered by a failing test — the **first**
(farthest-vs-background) fold step has no real "farther sheet" to
arbitrate against, only the true background, so `WU-35a`'s own note
("`T` applied where that function currently composites farthest-to-nearest
with plain `composite()` alone") could not be implemented literally
without breaking its own `T = 0.5` accept identity (verified by hand: a
uniform application at every step lets the true background leak into a
two-fully-opaque-slot result instead of an exact 50/50 blend). Resolved
as `DECISIONS.md` ADR-088: the base step stays an unconditional plain
`composite()` call; `manualTransp` applies from the second fold step
onward.

**Split `WU-35a` into `WU-35a1`/`WU-35a2`, the same "component before the
thing that wires it" seam this project has used repeatedly** (per this
session's own continuation prompt's expectation) — `WU-35a1`:
`compositeKBuffer()`'s between-sheet blend itself, core-only
(`core/resolve.hpp`/`.cpp`, `core/pipeline.cpp`), sandbox-buildable, built
and tested this session. `WU-35a2`: wiring `manualTransp` into a live
operator control in `tests/test_decklink_live_sphere.cpp`, Blackmagic-SDK-
linked, scoped but **not built** this session (per the standing
constraint: only the sandbox-buildable half, unless there is a good reason
to push further — there wasn't). `WORK-UNITS.md` updated accordingly.

**Built `WU-35a1` in a fresh cloud-sandbox clone of `origin/main` at
`147bb51`** (never in the device-bridge-mounted tree): `compositeKBuffer()`
gains a fourth, trailing, defaulted parameter (`Weight manualTransp = 0`);
`PipelineParams` gains the same field, same default; a new
`blendBetweenSheets()` helper (`core/resolve.cpp`) implements the
WU-SM-02.md §4.0/ADR-SM-020 formula, gated by each slot's own coverage
alpha the same way a plain `composite()` call already is. **Flagged
`[P]`-tier explicitly, in code comments (`core/resolve.hpp`/`.cpp`) and in
`DECISIONS.md` ADR-088/`WORK-UNITS.md` WU-35a1** — `ADR-087` licenses this,
it does not confirm the formula, the sheet-membership test, or (new this
session) the specific two-slot-formula-to-multi-slot-fold generalisation
are what the real Mirage built. `manualTransp = 0` (the default) confirmed
to reproduce `WU-28d`'s own already-scoped accept criterion (self-fold
back half fully occluded) for free, checked first, before anything else,
per the standing constraint — proved algebraically (the fixed-point
rounding collapses exactly at `T = 0`, no fuzz) and by a new test passing
`0` explicitly and cross-checking the existing hand-ordered `composite()`
chain reference. Four more new tests cover `T = 1` (near hemisphere fully
vanishes), `T = 0.5` for two fully-covered slots (matches an unarbitrated
accumulate-then-normalise, fixture 30), `T = 0.5` gated by a half-covered
nearer slot's own coverage (independently hand-derived expected value),
and `PipelineParams::manualTransp`'s own default. `Opaque` mode and
`WU-28a`'s own same-sheet tag-keyed membership test untouched.

**Every changed file written to Steve's own real repository via the
device bridge, then re-staged and diffed to confirm the write landed**
(the four code/test files diffed byte-for-byte against the exact,
already-build-tested sandbox versions — all four `IDENTICAL`; `WORK-UNITS.md`
and `DECISIONS.md` confirmed by direct grep/tail against the real file
after each write) before presenting this session's own close-out.

## Build/test matrix — full twelve configurations (run to confirm, not assumed)

Fresh `git clone` of `https://github.com/skunge2000/scatter-dve.git` into
the cloud sandbox, confirmed at `HEAD = origin/main = 147bb51` before any
build; only the four `WU-35a1` files (`src/core/resolve.hpp`,
`src/core/resolve.cpp`, `src/core/pipeline.cpp`,
`tests/test_kbuffer_resolve.cpp`) were ever edited there — `git status
--short` in the sandbox read exactly those four files throughout.
GCC 13.3.0 and Clang 18.1.3, Release and Debug, tile 2^4 and tile 2^5
(eight), plus GCC + ASan alone and GCC + UBSan alone, each at both tile
sizes (four more) — twelve total.

| Configuration | Build | `ctest` | `test_kbuffer_resolve` |
|---|---|---|---|
| GCC, Release, tile 4 | clean, no warnings | 28/28 pass | PASS, 186957 checks |
| GCC, Debug, tile 4 | clean, no warnings | 28/28 pass | PASS, 186957 checks |
| GCC, Release, tile 5 | clean, no warnings | 28/28 pass | PASS, 186957 checks |
| GCC, Debug, tile 5 | clean, no warnings | 28/28 pass | PASS, 186957 checks |
| Clang, Release, tile 4 | clean, no warnings | 28/28 pass | PASS, 186957 checks |
| Clang, Debug, tile 4 | clean, no warnings | 28/28 pass | PASS, 186957 checks |
| Clang, Release, tile 5 | clean, no warnings | 28/28 pass | PASS, 186957 checks |
| Clang, Debug, tile 5 | clean, no warnings | 28/28 pass | PASS, 186957 checks |
| GCC + ASan only, tile 4 | clean, no warnings | 28/28 pass, no sanitizer trap | PASS, 186957 checks |
| GCC + ASan only, tile 5 | clean, no warnings | 28/28 pass, no sanitizer trap | PASS, 186957 checks |
| GCC + UBSan only, tile 4 | clean, no warnings | 28/28 pass, no sanitizer trap | PASS, 186957 checks |
| GCC + UBSan only, tile 5 | clean, no warnings | 28/28 pass, no sanitizer trap | PASS, 186957 checks |

Twelve rows, zero real warnings, zero sanitizer traps (grepped every
`ctest --output-on-failure` log — zero hits), sanitizer instrumentation
confirmed genuinely present (`nm -D`/`ldd` against `test_kbuffer_resolve`:
28 `asan` hits/`libasan.so.8` linked, 14 `ubsan` hits/`libubsan.so.1`
linked — matching `wu-45-green`'s own prior-session counts exactly).
`test_kbuffer_resolve`'s own 186957 checks are tile-invariant. **28 of 28
tests pass in every configuration** — the cloud sandbox has no Blackmagic
SDK and so never builds `test_decklink_device` at all; that test's own
standing ADR-035 duplex-check exception is unaffected either way, since no
file this unit touches is anywhere near it.

## Flag for Steve, not resolved here

1. **The commit-message discrepancy at session open** (see "Tag" above) —
   resolved by asking directly this session, but worth Steve's own
   awareness: the real `147bb51` commit message opens with unrelated
   content before its ADR-087/WU-35a material. Not a defect, just noted.
2. **`WU-35a1`'s own multi-slot generalisation is unvalidated beyond two
   occupied slots** (`DECISIONS.md` ADR-088's own `[P]`-tier section).
   `WU-SM-02.md` §4.0's formula is written for exactly two sheets; this
   unit extends it to "every fold step but the first" for up to
   `kBufferK` (4) occupied slots, which is a reasonable but unconfirmed
   generalisation with no fixture or accept criterion behind it. The live
   sphere demo's own self-fold only ever produces two occupied slots, so
   this does not block `WU-35a2`, but whoever eventually exercises a
   three-or-more-sheet scene with `manualTransp != 0` should know this
   path is unvalidated.
3. **`WU-35a2`'s own letter-key assignment** is not picked in
   `WORK-UNITS.md` — `X`/`x`/`Y`/`y`/`Z`/`z` and `A`/`B`/`C`/`D` are
   already taken by `WU-21i`'s own scheme in
   `tests/test_decklink_live_sphere.cpp`; the next free pair is that
   unit's own first real scoping decision, deliberately left open in case
   another unit claims letters before it is picked up.

## Where we are

**`WU-35a1` is green: `compositeKBuffer()`'s `Blend` mode now takes a
`[P]`-tier `manualTransp` coefficient, defaulting to `0` (byte-identical
to its pre-`WU-35a1` behaviour).** `WU-35a2` (wiring it into the live
sphere demo's own operator controls) is scoped in full in `WORK-UNITS.md`
but not built — it needs the real DeckLink hardware/SDK this project has
never had access to from the cloud sandbox. `WU-35`'s own remaining scope
(`Auto Transp`, `Ext. Key`, the general swappable M1/M2/hybrid interface,
the Jacobian-derived sheet tolerance) is untouched, still `todo`.

## Next work unit

`WU-35a2` — wire `manualTransp` into `tests/test_decklink_live_sphere.cpp`
via two new letter keys, the same increment/decrement scheme `WU-21i`
already established. Needs Steve's own real hardware to build, run and
by-eye accept (no automated test can observe an SDI monitor). See
`WORK-UNITS.md`'s own `WU-35a2` entry for the full design direction.

## Open questions

The two items in "Flag for Steve" above (multi-slot generalisation,
letter-key assignment). The `video::Raster444`-vs-`video::RasterRGB`
naming question (`HANDOFF.md`'s own long-carried item, `Session 68`'s own
account) is unaffected by this session's work and remains exactly as
documented in `core/pipeline.cpp`'s own WU-41/WU-42 comments: a permanent,
accepted design choice, not a defect, not blocking anything.

## Blocked / red

None. `WU-35a1` is genuinely green in the cloud sandbox, twelve
configurations, zero warnings, zero sanitizer traps. `WU-35a2` is not
blocked or red — it simply has not been started, needing real hardware
this environment does not have. `ctest` was not run on Steve's own real
terminal yet this session — see "Steve's own next steps" below.

## Environment check

Same GCC 13.3.0 / Clang 18.1.3 cloud sandbox as prior sessions
(`gcc-13 (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0`, `Ubuntu clang version
18.1.3 (1ubuntu1)`, `cmake version 3.28.3`, `ninja`) — a fresh
`git clone` of `origin/main` at `147bb51`, confirmed identical to Steve's
own real repository before any build; only the four `WU-35a1` files were
ever edited in the sandbox.

**`.git/index.lock` was checked at both this session's own opening and
closing points in the real repository.** At open: absent (`ls` found
nothing). A `git add --dry-run -A` run as part of the standing
"check whether it actually blocks git" discipline then itself left a
zero-byte `.git/index.lock` behind (git's own dry-run created it and
failed to unlink it afterward — "Operation not permitted" in the
device-bridge shell), the same recurring, already-documented quirk since
Session 55. As before, this did **not** block `git status --short` itself
— checked directly both ways, not assumed either implies the other; that
command ran clean immediately afterward and again at close. All building
and testing happened in a fresh cloud-sandbox clone regardless; the
changed-file writes used the device bridge's own Python-based file-write
path, not `git`, so the lock never blocked this session's own work either.
The close-out block's own first line, `rm -f .git/index.lock`, is expected
to be genuinely needed on Steve's own real terminal, exactly as every
prior session has found.

File writes and re-reads this session used `device_bash` directly
(Python read-modify-write against `src/core/resolve.hpp`,
`src/core/resolve.cpp`, `src/core/pipeline.cpp`,
`tests/test_kbuffer_resolve.cpp`, `WORK-UNITS.md` and `DECISIONS.md`,
replaying the exact same edits made and verified in the cloud sandbox),
confirmed for the four code/test files by staging the real repository's
copies back into the cloud workspace and running a byte-for-byte `diff`
against the sandbox versions that actually passed the twelve-configuration
matrix — all four `IDENTICAL` — and for `WORK-UNITS.md`/`DECISIONS.md` by
direct `grep`/`tail` against the real file after each write, not inferred
from the write calls succeeding alone.

## Append to DECISIONS.md

`ADR-088` — added in full this session. See `DECISIONS.md` itself; not
re-quoted here since the state-file discipline is "read the real file,"
not "trust the handoff's paraphrase of it."

## Append to CORRECTIONS.md

None this session. `WU-35a`'s own carve-out note (Session 68) described
`manualTransp` as applying uniformly across compositeKBuffer()'s whole
fold, which this session found was not quite right (the base step needed
excluding) — but that note was explicitly "not yet scoped in full," never
shipped as a settled claim, and was corrected within this same session,
before landing anywhere as fact, via `DECISIONS.md` ADR-088 and
`WORK-UNITS.md`'s own `WU-35a1` entry rather than by silently
implementing it. Per this project's own standing convention (`Session
68`'s own account of the "ramp/excursion look exempt" hypothesis), a
correction goes to `CORRECTIONS.md` when something already shipped or
misled a real decision on record — an unscoped design-direction note's own
imprecision, caught and fixed in the same session that first tried to
build it, does not meet that bar.

## Closed out this session

**`WU-35a1` (`src/core/resolve.hpp`, `src/core/resolve.cpp`,
`src/core/pipeline.cpp`, `tests/test_kbuffer_resolve.cpp`,
`DECISIONS.md` `ADR-088`, `WORK-UNITS.md`): `compositeKBuffer()`'s
`Blend` mode gains a `[P]`-tier `manualTransp` transparency coefficient,
defaulting to `0` (byte-identical to pre-`WU-35a1` behaviour, reproducing
`WU-28d`'s own accept criterion for free).** Full twelve-configuration
matrix run to confirm empirically: `test_kbuffer_resolve` at 186957 checks
(the five new `WU-35a1` tests included) in all twelve, identical across
every configuration, zero warnings, zero sanitizer traps. **28 of 28 tests pass in the cloud
sandbox** (no Blackmagic SDK there, so `test_decklink_device` is not
built at all — unaffected either way). Not yet committed. Six files
(`src/core/resolve.hpp`, `src/core/resolve.cpp`, `src/core/pipeline.cpp`,
`tests/test_kbuffer_resolve.cpp`, `DECISIONS.md`, `WORK-UNITS.md`), plus
this `HANDOFF.md`. `WU-35a2` scoped in `WORK-UNITS.md` but not built —
needs real DeckLink hardware.

## Steve's own next steps

At your own real terminal, confirm a real build and test run — **expected
to show 28/28 pass in the cloud sandbox's own terms; your own real
terminal may show 29 with `test_decklink_device`'s own standing duplex-
check exception (ADR-035, only present when built against the Blackmagic
SDK) still passing separately, or 28 if not built against the SDK**:

```
cd ~/src/scatter-dve
rm -f .git/index.lock
cmake --build build
ctest --test-dir build --output-on-failure
```

Expect: every test passes, including `test_kbuffer_resolve` at 186957
checks. If anything differs from that, stop and report exactly what,
rather than assuming it is safe to tag past, since this session's own
cloud sandbox found a clean 28/28 across all twelve configurations
checked.

`./tools/close.sh` should work here too (the tree was fully green before
this session and stays fully green after it, in the cloud sandbox's own
terms) — but the manual path below is given regardless, since this
session did not run it and cannot confirm what it would do on your own
real terminal.

```
cd ~/src/scatter-dve
rm -f .git/index.lock
git add src/core/resolve.hpp src/core/resolve.cpp src/core/pipeline.cpp tests/test_kbuffer_resolve.cpp DECISIONS.md WORK-UNITS.md HANDOFF.md
git commit -m "WU-35a1: compositeKBuffer()'s Blend-mode between-sheet step gains a [P]-tier manualTransp transparency coefficient (DECISIONS.md ADR-088, WORK-UNITS.md WU-35a1), carved out of WU-35a (ADR-087) once scoping found compositeKBuffer() unmoved since WU-28b and its Blend-mode fold split into one background-vs-farthest-sheet base step (unaffected by manualTransp) plus a genuine between-sheet step for every nearer slot (WU-SM-02.md section 4.0/ADR-SM-020's formula, gated by each slot's own coverage). manualTransp defaults to 0, collapsing every between-sheet step back to its pre-WU-35a1 plain composite() call exactly -- reproduces WU-28d's own already-scoped accept criterion (self-fold back half fully occluded) for free, proved algebraically and by a new test passing 0 explicitly. Four more new tests: T=1 (near hemisphere fully vanishes), T=0.5 for two fully-covered slots (matches an unarbitrated accumulate-then-normalise, WU-SM-02.md fixture 30), T=0.5 gated by a half-covered nearer slot's own coverage, and PipelineParams::manualTransp's own default. Opaque mode and WU-28a's same-sheet tag-keyed membership test untouched. [P]-tier throughout, per ADR-087: licenses this choice, does not confirm the formula, the sheet-membership test, or this unit's own two-slot-to-multi-slot-fold generalisation are what the real Mirage built. WU-35a2 (wiring manualTransp into tests/test_decklink_live_sphere.cpp's own operator controls) scoped in WORK-UNITS.md but not built -- Blackmagic-SDK-linked, needs real hardware. Full twelve-configuration matrix (GCC 13.3.0/Clang 18.1.3, Release/Debug, tile 4/5, plus GCC+ASan-only and GCC+UBSan-only at both tile sizes): 28 of 28 tests pass in every configuration, zero warnings, zero sanitizer traps, sanitizer instrumentation confirmed genuinely linked (nm -D/ldd, matching wu-45-green's own prior counts exactly). No Blackmagic SDK in the cloud sandbox, so test_decklink_device is not built there at all -- ADR-035's own standing duplex-check exception is unaffected."
git tag -a wu-35a1-green -m "compositeKBuffer()'s Blend mode (core/resolve.hpp/.cpp) gains a [P]-tier manualTransp transparency coefficient, WU-35a1, carved out of WU-35a (DECISIONS.md ADR-087) this session once scoping WU-35a's own carve-out note against the real, current code (SESSION-PROTOCOL.md's own anti-drift rule 1) found compositeKBuffer() unmoved since WU-28b and found that note's own 'apply T at every fold step' description did not survive contact with the real farthest-vs-background base step -- DECISIONS.md ADR-088 records the real design decision made here (the base step stays an unconditional plain composite() call; manualTransp applies only from the second fold step onward, each one a genuine between-sheet decision gated by that slot's own coverage). manualTransp = 0, the default, reproduces WU-28d's own already-scoped accept criterion (self-fold back half fully occluded) for free -- proved algebraically (the fixed-point rounding collapses exactly, no fuzz) and by test. T=1 and T=0.5 (two fully-covered slots) checked directly against WU-SM-02.md fixture 30's own two remaining checkpoints. [P]-tier throughout: ADR-087's own Task A1 finding (GB2158671A read in full, silent on multi-sheet arbitration) licenses this choice, it does not confirm the blend formula, the sheet-membership test, or this unit's own extension of a two-sheet formula to a multi-slot fold are what the real Mirage built -- only the two-occupied-slot case is validated. WU-35a2 (live-demo wiring, tests/test_decklink_live_sphere.cpp) scoped in WORK-UNITS.md but deliberately not built this session -- Blackmagic-SDK-linked, cannot be built or tested in this project's own cloud sandbox. Full twelve-configuration matrix (GCC 13.3.0/Clang 18.1.3, Release/Debug, tile 4/5, plus GCC+ASan-only and GCC+UBSan-only at both tile sizes): 28 of 28 tests pass in every configuration, zero warnings, zero sanitizer traps, sanitizer instrumentation confirmed genuinely linked (nm -D/ldd: 28 asan hits/libasan.so.8, 14 ubsan hits/libubsan.so.1, matching wu-45-green's own prior-session counts exactly). No Blackmagic SDK in the cloud sandbox, so test_decklink_device is not built there at all -- ADR-035's own standing duplex-check exception, unrelated to this unit, is unaffected either way."
git push origin main
git push origin --tags
```

**Tag name is `wu-35a1-green`** — only `WU-35a1` (the core, sandbox-
buildable half) was built and tested this session; `WU-35a2` stays
untagged and `todo` in `WORK-UNITS.md`, exactly the way `WU-28c` was
tagged while `WU-28d` stayed open in the precedent this split follows.

This exact list of 7 paths was checked against a real `git status
--short` run through the device bridge immediately before this block was
written — `.git/index.lock` was present at that same moment (see
"Environment check" above) but, as before, did not block `git status
--short` itself; the command ran clean and read exactly 6 `M` lines
(`DECISIONS.md`, `WORK-UNITS.md`, `src/core/pipeline.cpp`,
`src/core/resolve.cpp`, `src/core/resolve.hpp`,
`tests/test_kbuffer_resolve.cpp`) plus this `HANDOFF.md` itself (written
last, after that check, so it does not appear in that same `git status`
output — add it too, per the file list above). Still worth a quick
`git status --short` yourself before pasting this block, since time has
passed since that check.
