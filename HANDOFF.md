# Handoff
Overwritten at the end of every session. This is the first thing to read.

---

**Session:** 36 (scoping only, no code — WU-28c/WU-28d named and scoped in
outline, gated on WU-26; `CORRECTIONS.md` C-020 and `DECISIONS.md` ADR-062
recorded. Triggered by Steve's own direct question against the real
hardware: "should WU-28b have shown occlusion, it didn't.")
**Tag:** none this session — scoping-only sessions don't tag, same as
Session 33's own ADR-059 commit.

**Before doing anything else in the next session:** run `git tag`, `git
log --oneline -10` and `git status --short` directly against
`~/src/scatter-dve` via the device bridge, the same as every session
before this one — do not trust this file's own account of tag/commit state
without checking it against the real repository first.

## Flagged now

**1. `WORK-UNITS.md`'s own WU-21i entry is still stale — not touched this
session, now eight sessions old.** Unchanged since Session 35's own note.

**2. WU-28b is still `wip`, not `green`, as of this session's own opening
check — `HEAD` at `2b9eea4`, no `wu-28b-green` tag, tree clean,
`origin/main` in sync.** Steve's own real-terminal build/run/`ctest`/
`close.sh 28b` is unaffected by this session's own findings and remains
the right next action — WU-28b's own committed code is correct for
exactly what its `Accept:` line says (synthetic multi-tag slot sets; I6
across the new threaded code path). Nothing this session found is a defect
in that commit. See "Steve's own next steps."

**3. The real finding this session exists to record: WU-28a's and
WU-28b's own k-buffer mechanism is not reachable by any real content in
this codebase today, and won't show occlusion for the live sphere demo no
matter what mode is set.** Full trace in `CORRECTIONS.md` C-020 and
`DECISIONS.md` ADR-062 — short version: (a) `tests/
test_decklink_live_sphere.cpp` never sets `PipelineParams::kBufferMode`
away from `Off`, so the k-buffer code path isn't even reached; (b) even
fixing that alone would not help, because `core/binner.cpp` stamps one
single `PipelineParams::tag` value onto every fragment a
`generateFragments()` call produces, so the sphere's self-folding front
and back always carry the same tag and collide into the same k-buffer
slot — Opaque/Blend resolve would still produce pixel-identical output to
the pre-WU-28 plain path for this exact content. ADR-059 itself named "a
sphere's own front and back" as the motivating k=2 case; this session
found that neither sub-unit actually closes it. Not a defect in WU-28a's
or WU-28b's own `Files:`/`Accept:` — a real gap neither unit's own scope
included, now named as WU-28c/WU-28d in `WORK-UNITS.md`.

**4. The fix depends on WU-26 (Normals from lattice), still unscoped.**
`core/lattice.hpp`'s own header comment already reserves dz/du/dz/dv and
the cross-product normal for WU-26 by name — the front/back facing signal
WU-28c needs is exactly that normal's view-facing sign. A same-session
finite-difference shortcut around this was considered and rejected (see
ADR-062) — it would duplicate, not reuse, work `core/lattice.hpp` already
promises to a specific future unit. WU-26 is therefore now a hard
prerequisite, not just "next in the backlog" — see `WORK-UNITS.md`'s own
WU-26 entry, updated with a pointer this session (no `Files:`/`Accept:`
invented for it here — that stays a future session's own first job).

**5. `.git/index.lock` pattern — not encountered as blocking this session
(read-only `git` checks via `device_bash` only).** Same known,
non-blocking behavior documented every session since Session 29. **Steve:
run `rm -f ~/src/scatter-dve/.git/index.lock` before your own `git
add`/`git commit` below** if it's present — routine, not new.

## This session in full

Session opened by Steve's own direct question against the real repository
and the real hardware, not a scoping request in the abstract: "should
WU-28b have shown occlusion, it didn't." Verified real repository state
first, per standing discipline: `HEAD` at `2b9eea4` ("WU-28b: k-buffer
resolve..."), no `wu-28b-green` tag, `git status -sb` reading `##
main...origin/main` with no ahead/behind marker — WU-28b's own code is
committed, correctly, just not yet Steve's-own-real-terminal-closed.

Traced the symptom against the real files rather than reasoning from
memory of what WU-28a/WU-28b built: staged and read
`tests/test_decklink_live_sphere.cpp` (confirmed `kBufferMode` and `tag`
are never set on its own `PipelineParams`, and confirmed its own lattice
genuinely self-folds, `kAngleSpanH = 2*pi`), `src/core/binner.hpp`/`.cpp`
(confirmed `tag` is one scalar function parameter stamped onto every
`Frag` a call produces), `src/core/splat.cpp` (confirmed
`routeIntoKBuffer()` keys slots by tag, same-tag reuses the same slot —
ADR-059's own deliberate, correct choice for I6, not a bug), and
`src/core/lattice.hpp`/`shapes.hpp` (confirmed the sphere's own
parametrisation and confirmed `core/lattice.hpp`'s own header comment
already earmarks dz/du/dz/dv and the cross-product normal for WU-26 by
name). Two independent, stacking causes found, both recorded in full in
`CORRECTIONS.md` C-020 and `DECISIONS.md` ADR-062: the live demo never
enables the k-buffer path at all, and even enabling it would not help
against this specific content, because front and back of one self-folding
lattice always share one tag today.

Scoped the fix as two new units, gated on WU-26, kept separate from each
other for the same core/DeckLink split reason ADR-059 itself kept WU-28a
and WU-28b separate from any Cocoa/Blackmagic-touching work: **WU-28c**
(`core/binner.hpp`/`.cpp` only, core-only, sandbox-buildable once WU-26
exists to build against — computes a per-fragment facing tag from WU-26's
own normal) and **WU-28d** (`tests/test_decklink_live_sphere.cpp` only,
DeckLink-linked, reasoned-through/handed-off only, never sandbox-built —
turns `kBufferMode` on for the live demo once WU-28c's tags exist).
Neither unit's `Files:`/`Accept:` in `WORK-UNITS.md` is written as final;
both are explicitly provisional pending WU-26, the same discipline
WU-28b's own blend formula followed (designed against real code once its
prerequisite existed, not invented at scoping time).

**No code written this session — scoping and correction only,** the same
shape Session 33's own ADR-059 scoping session took. Does not reopen
ADR-059, ADR-060, or ADR-061 — WU-28a's and WU-28b's own `Files:`/`Accept:`
are both still correct for exactly what they say they cover, unchanged and
undisputed by this session.

Delivered four changed files to the real repository via `SendUserFile` +
`device_commit_files`, to `/Users/stephenneal/src/scatter-dve/...`:
`WORK-UNITS.md`, `DECISIONS.md`, `CORRECTIONS.md`, this file.
**Delivery confirmation (this session's own device_bash checks after
writing every file):** `wc -l` on each of the four files matched this
session's own sandbox copies exactly; `git status --short` showed exactly
these four files changed, nothing else; byte-for-byte `diff` against this
session's own sandbox copies, re-staged from the device afterward, showed
no differences on any of the four.

## Where we are

Phase 6 (Scale up) unchanged: WU-22a/b/c all `green`. Phase 7 (Starlight)
now reads: **WU-26 `todo`** (now a named hard prerequisite for WU-28c, not
yet scoped), WU-27 `todo`, WU-28a `green`, **WU-28b `wip`** (still awaiting
Steve's own real-terminal close-out — unaffected by this session),
**WU-28c `todo`** (new, gated on WU-26), **WU-28d `todo`** (new, gated on
WU-28c), WU-29 `todo`. `DECISIONS.md` runs through ADR-062.
`CORRECTIONS.md` runs through C-020.

## Next work unit

Two things, in order: (1) Steve's own real-terminal close-out of WU-28b —
unaffected by this session, still the right next action, see "Steve's own
next steps" below; (2) after that, or independently, WU-26 (Normals from
lattice) needs its own real scoping session — a future session's first job
there is `Files:`/`Accept:` scoping, same discipline as every unscoped
unit, now with WU-28c specifically depending on its outcome (the normal
needs to be something WU-28c's own per-fragment tagging can consume, i.e.
available per source sample during `generateFragments()`, not only at
control-vertex resolution — worth that future session's own attention
alongside whatever WU-27's Blinn-Phong shading needs from the same
normal).

## Open questions

Unchanged from Session 35: `kCaptureRingCapacity`'s value of 8 (WU-20a/20b,
ADR-046), the cold-start green-frame artifact (WU-21d), Q3 (macOS/Desktop
Video version), and Q4 (lattice edge damping, C-008(a)) all remain open,
none touched this session.

## Blocked / red

Nothing red. WU-28c is blocked on WU-26 (not yet scoped); WU-28d is
blocked on WU-28c. Neither is a broken state — both are honestly `todo`,
correctly sequenced. WU-28b remains `wip`, not blocked, simply awaiting
Steve's own close-out.

## Environment check

Unchanged from Session 35: **UltraStudio Monitor 3G** (output,
HDMI-mirrored) and **UltraStudio Recorder 3G** (input) both last confirmed
working in Session 29's own real-hardware runs. **UltraStudio 4K Mini**
remains on hold pending a PSU replacement. `origin` remains configured and
in sync as of this session's own opening check.

## Append to DECISIONS.md

ADR-062 (real-content gap scoping: why WU-28a/WU-28b cannot show occlusion
for real self-folding content, WU-28c/WU-28d named and provisionally
scoped, the WU-26 dependency and why a same-session shortcut around it was
rejected) — appended in full this session; see `DECISIONS.md`. Does not
reopen ADR-059, ADR-060, or ADR-061.

## Append to CORRECTIONS.md

C-020 (ADR-059 named "a sphere's own front and back" as WU-28's own
motivating case; neither WU-28a nor WU-28b actually closes it, because the
k-buffer keys slots by a tag that's one scalar per call, not per fragment;
general lesson — check a unit's own motivating real-world symptom against
the real symptom, not only against its own synthetic `Accept:` data) —
appended in full this session; see `CORRECTIONS.md`.

## Closed out this session

Nothing — scoping-only, no tag, same as Session 33's own ADR-059 commit.

## Steve's own next steps

**First, close out WU-28b for real — this session's own findings do not
change anything about this step, it's still correct:**

```
rm -f ~/src/scatter-dve/.git/index.lock
cd ~/src/scatter-dve
git status
cd ~/src/scatter-dve
cmake --build build
cd build
ctest --output-on-failure
```

If that's green:

```
cd ~/src/scatter-dve
./tools/close.sh 28b
```

If `close.sh` prints `WU-28b closed green.`, it already pushed. If it
prints `WARNING: push failed`, push explicitly:

```
git push origin main
git push origin --tags
```

**Then, separately, review and commit this session's own scoping docs** —
doc-only, no build/test needed (nothing here touches source):

```
cd ~/src/scatter-dve
git diff -- WORK-UNITS.md DECISIONS.md CORRECTIONS.md HANDOFF.md
git add WORK-UNITS.md DECISIONS.md CORRECTIONS.md HANDOFF.md
git commit -m "WU-28 scoping: self-fold facing-tag gap, WU-28c/WU-28d named, gated on WU-26 (ADR-062, C-020)"
git push origin main
```

(No tag for this commit — scoping-only sessions don't tag, the same as
Session 33's own `a18a419` scoping commit for ADR-059.)

Once both are done, `git log --oneline -3` should show this scoping commit
at `HEAD`, the WU-28b close commit just below it, and `wu-28b-green` in
`git tag`.
