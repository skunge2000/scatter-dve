# Handoff
Overwritten at the end of every session. This is the first thing to read.

---

**Session:** 70 (`WU-35a2` — wiring `manualTransp` into
`tests/test_decklink_live_sphere.cpp`'s own operator controls; a real
live-pipeline wiring gap found and flagged, not fixed).

**Tag:** the newest real commit at this session's own start was
`41fe0a3` (`WU-35a1`), confirmed directly before touching anything:
`git fetch origin`, `git log --oneline -5`, `git tag -l | grep 35a`,
`git rev-parse HEAD origin/main` (both `41fe0a34806137317c4fad37da97d5fe01bab88d`),
`git describe --tags --exact-match HEAD` (`wu-35a1-green`, exact match —
`WU-35a1` genuinely is what shipped last session), `git status --short`
(clean). All matched this session's own continuation prompt exactly, no
discrepancy to flag this time. `.git/index.lock` checked directly too:
absent at session open.

## Before doing anything else in the next session

Run `git fetch origin`, `git log --oneline -5`, `git tag -l | grep 35a`,
`git rev-parse HEAD origin/main`, `git describe --tags --exact-match HEAD`
and `git status --short` directly against `~/src/scatter-dve` — do not
trust this file's own account without checking it against the real
repository first. Also check `.git/index.lock` directly (present again at
this session's own close, see "Environment check" below — the same
recurring quirk every session since Session 55 has hit) and whether it
actually blocks git (`git status --short`, not just its presence) before
concluding either way.

## This session in full

**Read, in order, before touching any code:** `SESSION-PROTOCOL.md`,
`HANDOFF.md` (Session 69's own entries — the version this file replaces),
`INVARIANTS.md`, `DECISIONS.md` ADR-087/ADR-088, `WORK-UNITS.md`'s
`WU-28d` (superseded), `WU-21i`, `WU-35a`, `WU-35a1` (`green`) and
`WU-35a2` entries in full, and `docs/sources/WU-SM-02.md` §4.0 and
fixture 30. All matched this session's own continuation prompt.

**Job: `WU-35a2`** — wire `PipelineParams::manualTransp` (`WU-35a1`,
`wu-35a1-green`) into a live operator control in
`tests/test_decklink_live_sphere.cpp`, two new letter keys, `WU-21i`'s own
increment/decrement scheme. Re-read the real, current file before picking
the key pair, per this unit's own standing instruction (its own note was
written last session and might have gone stale) — it had not:
`X`/`x`/`Y`/`y`/`Z`/`z` are still `WU-21i`'s own sphere controls, and
`A`/`B`/`C`/`D` are still consumed only as arrow-key escape-sequence
terminator bytes (`ESC [ A`/`B`/`C`/`D`), not bound as standalone letter
keys themselves — a bare `A`/`B`/`C`/`D` keypress is `Key::Unknown` today,
same as any other unclaimed letter. Picked `T`/`t` (mnemonic —
`WU-SM-02.md` §4.0 itself names the coefficient `T`), confirmed unclaimed,
wired everywhere `X`/`x`/`Y`/`y`/`Z`/`z` already were (the `Key` enum,
`readKey()`, `applyKey()`, `mapCoverageWindowKey()`,
`IncrementalKeyParser::mapLetter()`, `CoverageInputContext`, both stderr
help-text lines, and the flag-off loop's own separate inline switch — see
this file's own header comment for why that loop keeps a second copy
rather than calling `applyKey()`). Step size `kWeightUnity / 16` (2048) —
chosen so fixture 30's own three checkpoints (`T=0`/`0.5`/`1`) land
exactly on whole numbers of keypresses (0/8/16), no rounding drift. Range
clamped to `[0, kWeightUnity]` both directions — not a new design
decision needing its own ADR: `core/resolve.hpp`'s own doc comment on
`manualTransp` already states that range is the field's documented
contract. Also set `params.kBufferMode = scatter::KBufferResolveMode::Blend`
in the same file — required, not an extra: this test left `kBufferMode`
at its default `Off` before this session (`WU-28d`, which would have set
it, was superseded before ever being built), and `Opaque` mode ignores
`manualTransp` outright per `compositeKBuffer()`'s own doc comment — so
without this line the coefficient would have had no fold to run through
at all, regardless of the two new keys. Every change confined to
`tests/test_decklink_live_sphere.cpp`, matching `WU-35a2`'s own `Files:`
line — confirmed afterward with `git status --short`: exactly this one
source file plus the standing doc files (`WORK-UNITS.md`,
`CORRECTIONS.md`, this file).

**The real finding this session — headline, not a footnote.** Scoping
this properly meant reading `io/decklink_capture_consumer.hpp` directly,
not stopping at `core/resolve.hpp`'s own additive field declaration for
`manualTransp` (which is what `WU-35a2`'s own `Files:` line, written last
session, reasoned from). `CaptureConsumer` stores its constructor's
`PipelineParams` argument as `const PipelineParams m_params`, read by the
consumer thread on every processed frame, for the object's whole
lifetime — copied in once, no update path. `setLattice()` (`WU-21f`) gives
the *lattice* a live-update path for exactly this reason; nothing
equivalent exists for `params`. Consequence, checked directly, not
guessed: **pressing `T`/`t` at Steve's own real terminal will update this
file's own local `manualTransp` variable and the printed help/status
text, but will not change what the running consumer composites** — the
sweep half of `WU-35a2`'s own `Accept:` criterion ("sweeping the control
... should visibly show the far hemisphere increasingly show through")
cannot be satisfied within this unit's own one-file scope as written. The
rest-position half *can* now be observed for the first time, though, and
for a real reason: `kBufferMode` was never set to `Blend` in this file
before this session, so even `T=0`'s "back half occluded" behaviour
(`WU-28d`'s own original criterion) was unreachable until now — this
session's own change does make that much genuinely new to see.
**Deliberately not fixed here** — the fix (a `manualTransp` counterpart
to `setLattice()` on `CaptureConsumer`, touching
`io/decklink_capture_consumer.hpp`/`.cpp`) is out of `WU-35a2`'s own
one-file scope, and this session's own standing instruction is to name a
gap found while working rather than fix it as a side effect. Logged as
`CORRECTIONS.md` C-035 (the earlier `Files:` note's own implicit "additive
field ⇒ one file is enough" reasoning was wrong) and reflected in
`WORK-UNITS.md`'s own updated `WU-35a2` entry.

**Every changed file written to Steve's own real repository via the
device bridge, then re-staged and diffed to confirm the write landed
exactly as intended** — `tests/test_decklink_live_sphere.cpp` re-staged
and its full diff reviewed line by line against the fifteen intended
edits (braces/parens balance-checked too: 66/66, 384/384, no structural
break); `WORK-UNITS.md` and `CORRECTIONS.md` confirmed by direct
`grep`/`git diff --stat` against the real files after each write, not
inferred from the write calls succeeding alone.

## Cannot be built or tested — no sandbox step exists for this unit

Unlike `WU-35a1`, there is no cloud-sandbox build/test step for this
unit at all: `tests/test_decklink_live_sphere.cpp` is Blackmagic-SDK-
linked, and no Blackmagic SDK or AppleClang/Xcode/Cocoa toolchain exists
in the Linux cloud sandbox this session drafted in — the same gap every
DeckLink-touching unit in this project has named. No fresh-clone build
matrix was attempted or claimed. The edit was reasoned through directly
against the real, current file, applied via a scripted read-modify-write
(exact string anchors, each checked to match exactly once before being
replaced — not hand-retyped from truncated tool output), and verified by
re-staging and diffing afterward, not by compiling.

## Flag for Steve, not resolved here

1. **The live-wiring gap itself** (see "The real finding this session"
   above) — the two new keys change local state and printed text only;
   the running `CaptureConsumer` never sees `manualTransp` change after
   construction. A small follow-up touching
   `io/decklink_capture_consumer.hpp`/`.cpp` (a live-update path for
   `manualTransp`, the same shape `setLattice()` already gives the
   lattice) is needed before the sweep half of this unit's own `Accept:`
   criterion can be observed on a real SDI monitor. Not built this
   session — out of `WU-35a2`'s own one-file scope.
2. **`WU-35a1`'s own multi-slot generalisation is still unvalidated
   beyond two occupied slots** (`DECISIONS.md` ADR-088's own `[P]`-tier
   section, carried forward unchanged from Session 69 — this session's
   own work does not touch it either way).

## Where we are

**`WU-35a1` (`compositeKBuffer()`'s `manualTransp` coefficient) is still
green, untouched this session.** `WU-35a2`'s own operator-facing letter
keys (`T`/`t`) are now drafted in `tests/test_decklink_live_sphere.cpp`
and `params.kBufferMode` is now set to `Blend` there for the first time,
making `WU-28d`'s own original "back half occluded at rest" criterion
observable at all — but the keys' own live sweep does not yet reach the
composited output, for the `CaptureConsumer`-const-`params` reason above.
Not `green`, not tagged, cannot be built or tested from this
environment.

## Next work unit

Two real options, Steve's own call once he has read the finding above:

- **A follow-up unit** (call it `WU-35a3` or fold it into a re-scoped
  `WU-35a2`, Steve's own naming choice) that adds a live-update path for
  `manualTransp` to `io/decklink_capture_consumer.hpp`/`.cpp`, mirroring
  `setLattice()` (`WU-21f`) — after which `WU-35a2`'s own `T`/`t` keys
  (already drafted) would need no further change to become fully live.
- Or: build and by-eye-accept `WU-35a2` exactly as drafted, on the
  understanding that only the rest-position half of its own `Accept:`
  criterion can be checked until the follow-up above lands — i.e.
  confirm the folding sphere's back half is occluded at `T`'s rest
  position, and confirm `T`/`t` change the printed status text, without
  expecting the SDI output itself to visibly sweep yet.

`WU-35`'s own remaining rump scope (`Auto Transp`, `Ext. Key`, the
general swappable M1/M2/hybrid interface, the Jacobian-derived sheet
tolerance) is untouched, still `todo`, unaffected either way.

## Open questions

The live-wiring gap above (whether/when to build the `CaptureConsumer`
follow-up) is the only new one this session. `WU-35a1`'s own multi-slot
generalisation (carried forward, "Flag for Steve" item 2) and the
long-carried `video::Raster444`-vs-`video::RasterRGB` naming question
(`HANDOFF.md`'s own Session-68 account) are both unaffected by this
session's work and remain exactly as previously documented.

## Blocked / red

None in the sense of a failing build or test — nothing was built.
`WU-35a2` is not "blocked" either in the sense this project usually means
it (a missing dependency): its own dependency, `WU-35a1`, already landed
green. It is better described as **partially wired**: the operator-facing
half is drafted and handed off; the pipeline-facing half needs a small
follow-up this session did not build, by design (out of scope).

## Environment check

Same device-bridge access as every prior session — no cloud-sandbox
build was attempted this session at all (see "Cannot be built or tested"
above), so no compiler/toolchain versions to record.

**`.git/index.lock` was checked at both this session's own opening and
closing points in the real repository.** At open: absent. By close:
present again — a zero-byte lock file, left behind by a `git diff`/`git
add --dry-run -A` call during this session's own verification pass (the
device-bridge shell's own "Operation not permitted" on unlink, the same
recurring quirk since Session 55). As before, this did **not** block
`git status --short` itself — checked directly both ways, not assumed
either implies the other; that command ran clean throughout, reading
exactly the files this session actually changed each time it was run.
File writes and re-reads this session used `device_bash` directly
(Python read-modify-write with exact-match string anchors against
`tests/test_decklink_live_sphere.cpp`, `WORK-UNITS.md` and
`CORRECTIONS.md`), confirmed by re-staging each changed file and either
reviewing the full `git diff` line by line (`tests/test_decklink_live_sphere.cpp`)
or grepping/diffing the real file directly afterward
(`WORK-UNITS.md`, `CORRECTIONS.md`) — not inferred from the write calls
succeeding alone. The close-out block's own first line, `rm -f
.git/index.lock`, is expected to be genuinely needed on Steve's own real
terminal, exactly as every prior session has found.

## Append to DECISIONS.md

None this session. The range-clamp choice for `manualTransp`'s own two
new keys (`[0, kWeightUnity]` both directions) is not a new design
decision — `core/resolve.hpp`'s own existing doc comment on the field
already states that range is its documented contract, so no ADR was
opened for it, per this unit's own standing instruction to open one only
for a genuine new design question.

## Append to CORRECTIONS.md

`C-035` — added in full this session (see `CORRECTIONS.md` itself, not
re-quoted here per this project's own "read the real file, not the
handoff's paraphrase of it" discipline). In short: `WU-35a2`'s own
`Files:` line reasoned "the field is additive, so one file is enough,"
which does not follow and was wrong — a live control also needs the
*consumer* of that field to expose an update path, which
`io/decklink_capture_consumer.hpp`'s `CaptureConsumer` does not, for
`manualTransp`, today.

## Closed out this session

**`WU-35a2` (`tests/test_decklink_live_sphere.cpp`, `WORK-UNITS.md`,
`CORRECTIONS.md`): two new letter keys, `T`/`t`, drafted for
`PipelineParams::manualTransp`, plus `params.kBufferMode` set to `Blend`
for the first time in this file.** Not built, not tested, not tagged —
cannot be, no sandbox step exists for this unit. A real, load-bearing gap
found and flagged rather than fixed as a side effect: the two keys do not
yet reach the live composited/SDI output, because `CaptureConsumer`
copies `PipelineParams` in once at construction and holds it `const`,
with no live-update path for `manualTransp` the way `setLattice()` gives
the lattice one. Four files touched this session in the real repository
(`tests/test_decklink_live_sphere.cpp`, `WORK-UNITS.md`,
`CORRECTIONS.md`, this `HANDOFF.md`) — confirmed via `git status --short`
immediately before this block was written.

## Steve's own next steps

Your own hardware is back (PSU replaced, UltraStudio 4K Mini reconnected,
duplex check passing again), so a real, complete test run — including
`test_decklink_device` — is possible again for the first time in a while.
Confirm a real build first, at your own real terminal:

```
cd ~/src/scatter-dve
rm -f .git/index.lock
git status --short
cmake --build build
ctest --test-dir build --output-on-failure
```

Expect: everything that was already green stays green (this session
touched no `src/` file), including `test_decklink_device` now that
duplex is passing again. `tests/test_decklink_live_sphere.cpp` itself has
no automated pass/fail — it is the by-eye unit — so `ctest` running it at
all (rather than failing to build) is the only thing to check
automatically; the letter-key behaviour itself needs your own eyes on an
SDI monitor.

By-eye check, with a live source patched into the Recorder 3G's SDI
input: run `test_decklink_live_sphere` interactively and confirm the
folding sphere's back half is now occluded by default (new this
session — `kBufferMode` was never `Blend` in this file before), that
`X`/`x`/`Y`/`y`/`Z`/`z` and cursor rotation still all work exactly as
before, and that `T`/`t` are accepted as keys (no crash, no
"Unknown"-key silence) — **but do not expect the picture to visibly
change when you press them.** That is the flagged gap above, not a bug
in what got built; the printed stderr status line will still say what
`T`/`t` are for, and you can watch the process to confirm the value
itself is changing internally, but the SDI output will not move until
the `CaptureConsumer` follow-up (see "Next work unit" above) is built.

**This unit does not get its own tag from this session — it cannot be
confirmed green from here.** Once you have run the by-eye check above and
decided whether to accept it as drafted (rest-position half only) or wait
for the live-wiring follow-up first, commit and push at your own
discretion:

```
cd ~/src/scatter-dve
rm -f .git/index.lock
git add tests/test_decklink_live_sphere.cpp WORK-UNITS.md CORRECTIONS.md HANDOFF.md
git commit -m "WU-35a2: manualTransp operator controls (T/t) drafted in tests/test_decklink_live_sphere.cpp, plus kBufferMode set to Blend for the first time in this file -- CORRECTIONS.md C-035 records a real gap found while scoping: CaptureConsumer's own const PipelineParams m_params has no live-update path for manualTransp (unlike setLattice() for the lattice), so the sweep half of this unit's own Accept: criterion is not yet reachable; only the rest-position (T=0, back half occluded) half is. Not built or tested -- no Blackmagic SDK/Cocoa toolchain in the cloud sandbox this was drafted in."
```

If `./tools/close.sh` succeeds now that your hardware is back, it will
refuse to tag this unit past its own real test run either way -- but it
has no knowledge of "this unit is a by-eye-only, partially-wired control"
the way it already has no knowledge of the ADR-035 duplex exception, so
treat any tag it offers to create here with the same caution as the
manual path: this unit's own `Accept:` criterion is not fully satisfiable
yet, tag or no tag, until the follow-up above lands. If you tag it
anyway (rest-position-only acceptance, your own call), push explicitly:

```
git push origin main
git push origin --tags
```

This exact file list (four files) was checked against a real `git status
--short` run through the device bridge immediately before this block was
written; `.git/index.lock` was present at that same moment (see
"Environment check" above) but, as before, did not block `git status
--short` itself. Still worth a quick `git status --short` yourself before
pasting this block, since time has passed since that check.
