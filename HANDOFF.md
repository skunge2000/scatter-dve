# Handoff
Overwritten at the end of every session. This is the first thing to read.

---

**Session:** 37 (WU-26, Normals from lattice — scoped and built together,
one session, per `SESSION-PROTOCOL.md`'s normal shape; unlike ADR-059/
ADR-062, WU-26's own scoping did not need splitting from its build, since
`core/lattice.hpp`'s and `core/jacobian.hpp`'s own header comments already
named the design in full. Also: WU-28b's own stale `wip` status line
corrected doc-only, after confirming Steve's own real-terminal close-out —
`wu-28b-green` — directly via the device bridge. **Also: a same-session
mistake, found and fixed — see "Flagged now" item 1 and `CORRECTIONS.md`
C-021.**)
**Tag:** `wu-26-green` exists but, as of this file being written, points at
the *wrong* commit (see C-021 below) — do not trust it without checking
`git log --oneline -3` shows it on a commit that actually contains WU-26's
own file changes, not `5ba60b3`. "Steve's own next steps" below fixes this;
if you are reading this file in a later session and that fix has not yet
run, treat WU-26 as still not properly closed regardless of what `git tag`
alone shows.

**Before doing anything else in the next session:** run `git tag`, `git
log --oneline -10` and `git status --short` directly against
`~/src/scatter-dve` via the device bridge, the same as every session before
this one — do not trust this file's own account of tag/commit state without
checking it against the real repository first.

## Flagged now

**1. Same-session mistake, found and fixed: `wu-26-green` was created
before WU-26's own delivered files were committed.** The device bridge
wrote WU-26's seven files to Steve's working tree but a file write is not a
`git commit`; the manual `git tag -a wu-26-green ...` fallback (used
because `ctest` hit the known `test_decklink_device`/ADR-035 exception,
same as WU-28b) was then given without an intervening `git add`/`git
commit` step, so the tag landed on `5ba60b3` — a commit that does not
contain any of WU-26's own changes. `close.sh` itself would have caught
this (`git status --porcelain` non-empty → refuses with "uncommitted
changes"), but the manual-tag fallback has no such check. Full account in
`CORRECTIONS.md` C-021. Fixed later in this same session (see "This session
in full" below): `wu-26-green` deleted and recreated on the commit that
actually contains WU-26's own changes, both pushed — confirm this landed by
checking `git log --oneline -3` shows `wu-26-green` on a commit whose own
message names WU-26, not on `5ba60b3`.

**2. `WORK-UNITS.md`'s own WU-21i entry is still stale — not touched this
session, now nine sessions old.** Unchanged since Session 35's own note.

**3. `WORK-UNITS.md`'s own WU-28c entry line ("Depends on WU-26 (Normals
from lattice), not yet scoped or built") is now stale — WU-26 is scoped,
built and (once "Steve's own next steps" below is run) genuinely `green`.**
Deliberately not touched: scoping WU-28c in full is that unit's own future
session's job, not this one's (this session's brief was WU-26 only), and a
one-line patch here without doing that unit's own real scoping would risk
exactly the kind of drift `SESSION-PROTOCOL.md`'s anti-drift rules exist to
prevent. Whoever opens WU-28c next should update that line as part of doing
the real work, not before it.

**4. `.git/index.lock` pattern — not encountered as blocking this session
(read-only `git` checks, a fresh `git clone` via the sandbox's own shell,
and Steve's own `git add`/`git commit`/`git tag`/`git push` commands against
`~/src/scatter-dve`, all completing without it blocking anything).** Same
known, non-blocking behavior documented every session since Session 29.
**Steve: run `rm -f ~/src/scatter-dve/.git/index.lock` before your own `git
add`/`git commit`/`git tag` below** if it's present — routine, not new.

## This session in full

Opened by verifying real repository state directly via the device bridge,
per standing discipline, *before* trusting this file's own (Session 36)
account: `git tag` did not list `wu-28b-green` at that point, `HEAD` was at
`5ba60b3` (the WU-28 scoping commit), `git status -sb` read `##
main...origin/main` with no ahead/behind marker. Per this project's own
explicit stop condition for exactly this situation, work paused and the
discrepancy was reported rather than assumed away: Steve's own brief
believed both WU-28 sub-units were already real-terminal `green`, but only
`wu-28a-green` existed; `HANDOFF.md`'s own account was a session behind
Steve's own understanding too (Session 36 had already run, scoping
WU-28c/WU-28d and recording ADR-062/C-020, none of which Steve's own brief
for this session knew about). Steve then closed WU-28b out at his own real
terminal (`test_decklink_device`'s known ADR-035 duplex exception, the
manual `git tag -a wu-28b-green ...` fallback per `SESSION-PROTOCOL.md`,
explicit `git push origin main`/`git push origin --tags`) — confirmed
directly against the real repository afterward: `wu-28b-green` in `git tag`,
`git status -sb` reading `## main...origin/main` with no ahead/behind
marker (pushed, not local-only).

With WU-28b genuinely closed, moved to WU-26 (Normals from lattice),
`WORK-UNITS.md`'s own next-in-line unscoped unit and, since Session 36's
ADR-062, a named hard prerequisite for WU-28c. Read `src/core/shapes/
shapes.hpp` and `src/core/types.hpp` in full first, per this session's own
brief, then `src/core/lattice.hpp`, `src/core/lattice.cpp`, `src/core/
jacobian.hpp` and `src/core/binner.cpp` to ground the design in what
actually exists today rather than what the header comments merely promise:
found that `core/lattice.cpp`'s `jacobian()` already computes the full 3D
`du`/`dv` tangent blends internally (`blend()` sums x, y *and* z) and simply
discards their `.z` components when filling in the returned (2x2-only)
`Jacobian` — so WU-26 needed no new lattice evaluation, only storing what
was already being thrown away. Also found `core/jacobian.hpp`'s own header
comment had already reserved this exact file as WU-26's destination
("architecture.md 4.2 lists three things the Jacobian yields at once...K,
filter footprint, and (WU-26) the surface normal"), and confirmed via
`grep` and `CMakeLists.txt` that `test_jacobian` already links the full
`scatter-core` library (including `src/core/shapes/sphere.cpp`), so a real
`buildSphereLattice()` lattice was available to the test file with no
`CMakeLists.txt` change.

Design decision — the cross-product order (`Tv x Tu`, not the more
conventional `Tu x Tv`) needed by this project's own front/back sign
convention — worked out by hand against `buildSphereLattice()`'s actual
parametrisation (front-most and antipodal self-folded control vertices) and
recorded in full in `DECISIONS.md` ADR-063, along with a useful,
independently-checked internal-consistency fact (`surfaceNormal(j).z`
always equals `-(the existing 2x2 Jacobian determinant)`, an algebraic
property of 3D cross products, not a shortcut this unit takes) and a note
for whoever scopes WU-28c next: `core/binner.cpp`'s `pixelJacobian()` does
not propagate `dzdu`/`dzdv`, so `surfaceNormal()` must be called on
`lattice.jacobian(u, v)`'s own direct output, not on a `pixelJacobian()`-
converted one.

Built and tested in the cloud sandbox, per this project's own standing
discipline for a core-only unit: fresh `git clone` of
`https://github.com/skunge2000/scatter-dve.git` (not a reused prior
sandbox), confirmed matching the real repository's own `git tag`/`git log`/
`git status -sb` (`wu-28b-green` at `HEAD`, `5ba60b3`, clean, in sync)
*before* any file was touched. `cmake -B build -G Ninja && cmake --build
build`: clean, zero warnings, zero errors. Full portable `ctest` suite: 22
of 22 targets passing, no regressions anywhere outside `test_jacobian`
itself. `test_jacobian` alone: 551 checks passing (existing WU-06 checks
extended to `dzdu`/`dzdv`, plus two new WU-26-specific functions, one of
which builds and checks against a real `buildSphereLattice()` lattice, not
synthetic data — see `DECISIONS.md` ADR-063 for why that particular test
needed real shape data rather than the file's usual synthetic fixture).

Delivered seven changed/new-content files to the real repository via
`SendUserFile` + `device_commit_files`, to `/Users/stephenneal/src/
scatter-dve/...`: `src/core/lattice.hpp`, `src/core/lattice.cpp`,
`src/core/jacobian.hpp`, `tests/test_jacobian.cpp`, `WORK-UNITS.md`,
`DECISIONS.md`, this file. **Delivery confirmation (this session's own
device_bash checks after writing every file):** `wc -l` on each of the seven
files matched this session's own sandbox copies exactly; `git status
--short` showed exactly these seven files changed, nothing else;
byte-for-byte `diff` against this session's own sandbox copies, re-staged
from the device afterward, showed no differences on any of the seven.

Steve then ran WU-26's own close-out: `cmake --build build` clean, `ctest`
failing exactly one test (`test_decklink_device`, the known ADR-035
exception — nothing else), matching this session's own cloud-sandbox result
exactly. Instructed to tag manually (`close.sh` refuses to tag past any
failure) — but without first being told to `git add`/`git commit` the seven
delivered files, an omission from this session's own instructions, not
Steve's. `git tag -a wu-26-green ...` therefore tagged `HEAD` (`5ba60b3`)
as it stood — a commit that does not contain WU-26's own changes, which
were still sitting as uncommitted working-tree modifications. `git push
origin main` reporting "Everything up-to-date" (no commit to push, only the
tag) was the signal something was wrong; `git status -sb` confirmed it,
still showing all seven files as `M`. Recorded in full as `CORRECTIONS.md`
C-021.

**Fixed the same session, before handing back to Steve:** delivered an
updated `CORRECTIONS.md` (C-021) and this file via the same `SendUserFile` +
`device_commit_files` route, byte-for-byte-confirmed the same way as the
first seven files, then gave Steve corrected close-out commands — delete
the wrong tag locally and on `origin`, commit the eight now-staged files
(the original seven plus `CORRECTIONS.md`) for real, recreate `wu-26-green`
on the commit that actually contains them, push commit and tag together.
See "Steve's own next steps" below for the exact commands; this file was
written *before* Steve ran them, so "Where we are" below still describes
the pre-fix state — check `git log --oneline -3` against the real
repository before trusting either account.

## Where we are

Phase 6 (Scale up) unchanged: WU-22a/b/c all `green`. Phase 7 (Starlight)
now reads: **WU-26 `wip`** as this file is written — tested green at
Steve's own real terminal (modulo the known ADR-035 exception), but not yet
correctly committed/tagged; will be genuinely `green` once "Steve's own next
steps" below runs — WU-27 `todo`, WU-28a `green`, **WU-28b `green`**
(confirmed this session — Steve's own real-terminal close-out landed),
WU-28c `todo` (gated on WU-26 landing correctly; its own `WORK-UNITS.md`
entry line about WU-26 is now stale — see "Flagged now" item 3), WU-28d
`todo`, WU-29 `todo`. `DECISIONS.md` runs through ADR-063. `CORRECTIONS.md`
runs through C-021.

## Next work unit

Steve's own correction commands below (delete the wrong tag, commit for
real, retag, push) — see "Steve's own next steps". After that: WU-28c
(self-fold facing tag), whose own real scoping session can now proceed in
full — that future session's first job is fixing WU-28c's own stale
"Depends on WU-26... not yet scoped or built" line as part of doing its real
`Files:`/`Accept:` scoping, not before.

## Open questions

Unchanged from Session 35/36: `kCaptureRingCapacity`'s value of 8 (WU-20a/
20b, ADR-046), the cold-start green-frame artifact (WU-21d), Q3 (macOS/
Desktop Video version), and Q4 (lattice edge damping, C-008(a)) all remain
open, none touched this session.

## Blocked / red

Nothing red. WU-28c is blocked on WU-26 landing correctly; WU-28d is
blocked on WU-28c. Neither is a broken state — both are honestly `todo`,
correctly sequenced. WU-26 is `wip`, not broken: its own code is tested and
working (both in the cloud sandbox and, modulo the known ADR-035 exception,
at Steve's own real terminal) — only the commit/tag bookkeeping needs
correcting, per C-021.

## Environment check

Unchanged from Session 35/36: **UltraStudio Monitor 3G** (output,
HDMI-mirrored) and **UltraStudio Recorder 3G** (input) both last confirmed
working in Session 29's own real-hardware runs. **UltraStudio 4K Mini**
remains on hold pending a PSU replacement. `origin` remains configured and
in sync as of this session's own opening and closing checks.

## Append to DECISIONS.md

ADR-063 (WU-26 build: dz/du, dz/dv on the Jacobian, `surfaceNormal()` as
`core/jacobian.hpp`'s third Jacobian-derived quantity, the `Tv x Tu`
cross-product order and its derivation against a real sphere lattice, the
internal-consistency fact relating `surfaceNormal(j).z` to the existing 2x2
determinant, and why it is not used as a shortcut) — appended in full this
session; see `DECISIONS.md`. Does not reopen ADR-059, ADR-060, ADR-061 or
ADR-062.

## Append to CORRECTIONS.md

C-021 (WU-26 close-out: `wu-26-green` was created before the delivered
files were committed, so it initially pointed at a commit lacking WU-26's
own changes; general lesson that the manual-tag fallback, unlike
`close.sh`, has no uncommitted-changes check, so any close-out using it
must spell out `git add`/`git commit` before `git tag`) — appended in full
this session; see `CORRECTIONS.md`.

## Closed out this session

WU-28b, by Steve at his own real terminal (correctly — his working tree was
clean when that tag was created, unaffected by C-021). WU-26 is not yet
correctly closed as this file is written — `wu-26-green` exists but points
at the wrong commit; "Steve's own next steps" below fixes that.

## Steve's own next steps

**Fix `wu-26-green`, then close WU-26 out for real.** This replaces the
tagging step already run — the tag it created needs deleting, not building
on top of.

First, delete the wrong tag, locally and on `origin`:

```
cd ~/src/scatter-dve
git tag -d wu-26-green
git push origin :refs/tags/wu-26-green
```

Then commit the files that are actually sitting in your working tree
(confirm with `git status --short` first — it should list exactly these
eight, the seven WU-26 files plus this session's own `CORRECTIONS.md`
update, nothing else):

```
rm -f ~/src/scatter-dve/.git/index.lock
cd ~/src/scatter-dve
git status --short
git add DECISIONS.md HANDOFF.md WORK-UNITS.md CORRECTIONS.md src/core/jacobian.hpp src/core/lattice.cpp src/core/lattice.hpp tests/test_jacobian.cpp
git commit -m "WU-26: normals from lattice (ADR-063); C-021 tag-before-commit correction"
```

No need to rebuild or re-run `ctest` — the build/test you already ran used
these exact files (they were on disk, just uncommitted), so that result
stands: clean build, one known ADR-035 exception, nothing else. Recreate
the tag on this new commit and push both:

```
git tag -a wu-26-green -m "WU-26: normals from lattice green (test_decklink_device/foundDuplexDevice is ADR-035's known exception)"
git push origin main
git push origin --tags
```

Verify:

```
cd ~/src/scatter-dve
git log --oneline -3
git tag | tail -5
git status -sb
```

`git log --oneline -3` should now show a *new* commit at `HEAD` (your own
WU-26 commit message above, not `5ba60b3`) carrying the `wu-26-green` tag;
`git status -sb` should read `## main...origin/main` with no ahead marker
and no modified files listed at all (the working tree genuinely clean, not
just "no ahead/behind" with `M` lines still present the way it was after
the first, wrong tag).

**If `ctest` fails with anything else** — any test other than
`test_decklink_device`'s own duplex check — stop, don't tag, and paste me
the full `--output-on-failure` output; that would be a real regression this
session's own cloud-sandbox run (Linux x86_64, no DeckLink SDK) could not
have caught, and needs a session to look at it, not a blind tag.

This was compiled and tested in the cloud sandbox only (Linux x86_64, GCC
13.3, no Blackmagic SDK) — it still needs your own real-terminal
build/`ctest` above (AppleClang, ARM64, full DeckLink-linked suite) before
it's genuinely `green`. Run `ctest` from `~/src/scatter-dve/build`
specifically, as above (`cd build` then `ctest`, or `ctest --test-dir
build` from the repo root) — not from the repo root with a bare `ctest`,
which is what left the stray `Testing/` directory at the repo root once
before. If you see a `Testing/` directory at `~/src/scatter-dve` itself
(not inside `build/`), flag it rather than deleting it — `device_bash`
can't delete files, and I'd want to see it before we decide what to do
about it.
