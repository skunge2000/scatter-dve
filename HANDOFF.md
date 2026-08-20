# Handoff
Overwritten at the end of every session. This is the first thing to read.

---

**Session:** 33 (WU-28 — scoped only, per Steve's own explicit instruction
this session's job was to scope, not build. No code written. WU-22c
confirmed `green` — Steve's own close-out completed between sessions,
`WORK-UNITS.md`'s stale `wip` status line fixed directly this session.)
**Tag:** `wu-22c-green` — unchanged this session, confirmed still current.
No new tag this session: nothing was built or tested, only three docs
(`WORK-UNITS.md`, `DECISIONS.md`, this file) were changed.

**Before doing anything else in the next session:** run `git tag`, `git
log --oneline -10` and `git status --short` directly against
`~/src/scatter-dve` via the device bridge, the same as every session
before this one — do not trust this file's own account of tag/commit state
without checking it against the real repository first. This session found
`WORK-UNITS.md` one session stale on WU-22c's own status line (Flagged item
1, below) even though the real repository was already fully caught up —
worth specifically re-checking whether that kind of drift has recurred
before assuming any status line here is still accurate.

## Flagged now

**1. `WORK-UNITS.md`'s own WU-21i entry may still be stale — not checked
or touched this session (out of scope for WU-28 scoping).** As of Session
31's own `HANDOFF.md`, it read `wip` and "not yet built or run at Steve's
own real terminal," despite `wu-21i-green` existing and containing the
right files, still needing Steve's own by-eye acceptance detail (which
letter keys were tried, whether `Q` exits cleanly) before the status line
could be honestly fixed. Now five sessions old if still unfixed. Worth
doing directly whenever a session next touches `WORK-UNITS.md` for an
unrelated reason, the same way this session fixed WU-22c's own stale line
on sight rather than carrying it forward again.

**2. WU-22c's own `WORK-UNITS.md` status line was stale this session
(fixed).** `wu-22c-green` exists in the real repository and `git status -sb`
reads `## main...origin/main` with no ahead/behind marker — Steve's own
build/run/verify/commit/tag/push (Session 32's own "Steve's own next
steps") completed in full between sessions, with no issues to report. But
`WORK-UNITS.md`'s own WU-22c entry still read `wip` at the start of this
session — the exact same pattern WU-21i's entry (item 1 above) already
demonstrated can persist for several sessions if nobody happens to touch
that file for an unrelated reason. Fixed directly this session (see "This
session in full" below); worth this project generally getting into the
habit of checking every `wip`-marked entry's own tag against `git tag`
whenever a session already has the device bridge open for something else,
rather than only noticing when a later session happens to need that exact
entry.

**3. WU-28 itself: scoped only, no code written, deliberately — this
session's own explicit job per Steve's own instruction.** Full scoping
conversation and design recorded in `DECISIONS.md` ADR-059; the resulting
split, `WU-28a` (storage: `core/types.hpp`, `core/splat.hpp`/`.cpp`) and
`WU-28b` (resolve: `core/resolve.hpp`/`.cpp`, `core/pipeline.cpp`),
recorded in `WORK-UNITS.md` replacing the old single `WU-28` backlog line.
Neither sub-unit has been started. **Both stay entirely inside
`scatter-core`** (no Blackmagic SDK, no Metal/Cocoa in any touched file) —
per Steve's own answer during this session's scoping conversation, WU-28a
is buildable, runnable and testable directly in this cloud sandbox, unlike
every DeckLink/Cocoa-touching unit before it. The natural next session's
first job is WU-28a, and — a first for this project — that session should
actually build and run it here rather than reasoning it through and
handing it to Steve via the device bridge.

**4. `.git/index.lock` pattern — not encountered this session (no local
commits attempted; this session only edited files via `SendUserFile` +
`device_commit_files`, same delivery path as WU-22c's own text-heavy
files).** Still the same known, non-blocking behavior documented across
Sessions 29-32: `device_bash` git commands that only read (`status`, `log`,
`diff`, `show`, `tag`) succeed and print correct output even with a stale
lock file present; `device_bash` itself can never remove it. **Steve: run
`rm -f ~/src/scatter-dve/.git/index.lock` before your own `git add`/`git
commit` below** — routine, not a one-off fix.

**5. `device_stage_files` HTTP 403 `untrusted_device` — not encountered
this session.** If it recurs: it means the Mac slept mid-session and
invalidated the device's trusted-sign-in state, not an account problem —
ask Steve to re-enable access in the Claude desktop app (no fresh sign-in
needed), then retry. Noted only so a future session does not have to
re-diagnose it from scratch again.

## This session in full

Session opened by requesting device-bridge access to `~/src/scatter-dve`
and `~/src/Blackmagic DeckLink SDK 16.0` (approved), reading
`SESSION-PROTOCOL.md`, `HANDOFF.md`, `WORK-UNITS.md`, `DECISIONS.md`,
`CORRECTIONS.md` and `INVARIANTS.md` in full, then verifying Session 32's
own account of tag/commit state directly against the real repository
(`git tag`, `git log --oneline -5`, `git status --short`) before trusting
any of it. Found: `wu-22c-green` now exists (it did not at the end of
Session 32) and `git status -sb` reads clean and in sync with
`origin/main` — Steve's own full WU-22c close-out completed exactly as
Session 32's own "Steve's own next steps" specified, no discrepancies.
Separately found `WORK-UNITS.md`'s own WU-22c status line still read `wip`
— fixed directly (Flagged item 2, above).

WU-28 was scoped directly with Steve, per his own explicit instruction not
to assume a design beyond `WU-28`'s own backlog entry and ADR-053/054, via
direct questions grounded in `core/types.hpp`, `core/splat.hpp`/`.cpp`,
`core/resolve.hpp`/`.cpp` and `core/pipeline.cpp`, all read in full before
asking anything. First question (design depth) was answered with a request
to research the real Quantel Mirage patent (US 4,563,703) before choosing
— done via two `WebFetch` calls against Google Patents; findings (the
patent's own "Z" is a coverage-weight fraction, not a depth value; its only
overlap mechanism is the two-layer opaque/transparent flap case already
built as `compositeLayered()`; no patent precedent at all for more than two
surfaces or a surface folding over itself) relayed back to Steve, then the
design-depth question re-asked informed by them. Full reasoning, every
answer, and the resulting design — tag-keyed bounded k-buffer (k=4),
same-tag accumulation staying exactly today's order-independent
`accumulateCorner()` arithmetic, `z` used only once at resolve time to sort
the small number of occupied slots (never during accumulation, specifically
to avoid an I6 determinism risk this session found: `Frag::z`'s 16-bit
quantisation makes exact ties between same-surface fragments routine, not
rare), a hard k-slot cap during accumulation with an explicit, accepted
non-order-independence caveat for the rare >k-distinct-tags-in-one-cell
case, and two resolve-time outcomes (opaque front-wins, user-controlled
blend) left for WU-28b's own build session to design in full — all
recorded in `DECISIONS.md` ADR-059.

Sizing the resulting design against `SESSION-PROTOCOL.md`'s own "3 source
files plus test, ~400 lines" cap found it did not fit one unit (a new
per-cell record type in `core/types.hpp`; tag-routed accumulation with
eviction in `core/splat.hpp`/`.cpp`; a new depth-sort-and-composite resolve
step plus `PipelineParams` additions in `core/resolve.hpp`/`.cpp`; and
`resolveOneTile()` wiring in `core/pipeline.cpp` — four source files before
even counting tests). Split into `WU-28a` (storage/accumulation) and
`WU-28b` (resolve/composite), the same seam `core/splat.cpp`/`core/resolve.cpp`
already have between them since WU-09/WU-10. `WORK-UNITS.md`'s own single
`WU-28` line replaced with both sub-units' full `Files:`/`Accept:` entries,
matching this project's own established convention (WU-16a/b, WU-19a/b,
WU-20a/b as the closest precedents for a storage/consumer-shaped split).

Delivered three files, all written locally in the sandbox first, then
staged to the real repository via `SendUserFile` + `device_commit_files`
(the same delivery path WU-22c's own text-heavy files used, to avoid
shell-escaping risk against the markdown's own many backticks/quotes):
`WORK-UNITS.md` (WU-22c's stale status line fixed; `WU-28`'s single line
replaced by the `WU-28a`/`WU-28b` split), `DECISIONS.md` (ADR-059 appended
in full), and this file. No source code, no `CMakeLists.txt` change — this
session wrote no code, per its own explicit scope.

**Delivery confirmation (this session's own device_bash checks after
writing every file):** `wc -l` on each of the three files matched this
session's own sandbox copies exactly; `git status --short` at
`~/src/scatter-dve` after delivery showed exactly the three files listed
above as modified, nothing else; `git diff --stat` confirmed each file's
own diff was append-only (`DECISIONS.md`, this file) or a targeted
in-place replacement of the two entries named above (`WORK-UNITS.md`), not
a wider rewrite. No `.git/index.lock` issue encountered (Flagged item 4 —
this session made no commits of its own).

## Where we are

Phase 6 (Scale up) now reads: WU-22a `green` (`wu-22a-green`), WU-22b
`green` (`wu-22b-green`), WU-22c `green` (`wu-22c-green`, status line fixed
this session — all three of Phase 6's own DeckLink/Metal/Cocoa live-capture
units are now genuinely closed out). Phase 7 (Starlight) now reads: WU-26
`todo`, WU-27 `todo`, WU-28a `todo` (scoped this session, not started),
WU-28b `todo` (scoped this session, not started), WU-29 `todo`.
`DECISIONS.md` runs through ADR-059. `CORRECTIONS.md` is unchanged this
session (still through C-018) — nothing this session surfaced was a
codebase-logic defect; the I6/tied-`z` risk ADR-059 records was caught and
designed around during scoping, before any code existed to be wrong.

## Next work unit

**WU-28a** — the storage/accumulation half of this session's own scoped
design (`core/types.hpp`, `core/splat.hpp`, `core/splat.cpp`,
`tests/test_kbuffer_storage.cpp`; see `WORK-UNITS.md`'s own entry and
`DECISIONS.md` ADR-059 for the full design). Unlike every WU-28-adjacent
unit before it, and unlike every DeckLink/Cocoa-touching unit this project
has ever scoped, **this one stays entirely inside `scatter-core`** — the
next session should actually build, run and test it directly in this
sandbox rather than reasoning it through and handing it to Steve via the
device bridge. Fixing `WU-21i`'s own stale status line (Flagged item 1)
remains a small, unrelated open item worth doing opportunistically.

## Open questions

Unchanged from Session 32: `kCaptureRingCapacity`'s value of 8 (WU-20a/20b,
ADR-046), the cold-start green-frame artifact (WU-21d), Q3 (macOS/Desktop
Video version), and Q4 (lattice edge damping, C-008(a)) all remain open,
none touched this session. **Front/back occlusion/transparency (WU-28) is
no longer an open question in the same unscoped sense** — it is now a
concrete, recorded design (ADR-059) split into two `todo` sub-units, not
resolved code.

**New from this session:** WU-28b's own exact blend formula and the exact
new `PipelineParams` field name(s)/shape for opacity mode and blend control
were deliberately left undecided by ADR-059, for that unit's own build
session to design against the real, already-accumulated slot data WU-28a
will produce — not a gap, a deferred decision by design (the same way
`pool`/`weightOut` were each designed inside their own unit, not invented
at an earlier scoping stage).

## Blocked / red

Nothing red. Nothing blocked. WU-22a/b/c are all genuinely `green`. WU-28a/
WU-28b are `todo`, fully scoped, not yet started — not blocked on
anything, simply not begun.

## Environment check

Unchanged from Session 32: **UltraStudio Monitor 3G** (output,
HDMI-mirrored) and **UltraStudio Recorder 3G** (input) both last confirmed
working in Session 29's own real-hardware runs. **UltraStudio 4K Mini**
remains on hold pending a PSU replacement. Metal/Cocoa confirmed working on
Steve's real Mac hardware/GPU/OS version (Session 31's `coverage_view_demo`
run) and now also exercised live via WU-22c's own real build (Session 32/33
boundary) — Steve's own report during that build was a real behavioral
finding (coverage window not responding to control keys, fixed same-session
per ADR-058's own addendum), not a Metal/Cocoa-itself problem. `origin`
(`https://github.com/skunge2000/scatter-dve.git`) remains configured and in
sync — no push happened *from this session's own device_bash*, since it
never runs `git commit`/`git push` itself; the WU-22c push between sessions
was Steve's own action, already reflected in `git status -sb` above.

## Append to DECISIONS.md

ADR-059 (WU-28 scoping: the full scoping conversation, the Mirage patent
research findings and why they rule out a "port the patent" design, the
tag-keyed k=4 storage design and the I6/tied-`z` risk it was chosen to
avoid, the hard-cap-with-documented-caveat overflow policy Steve chose over
the fully order-independent alternative, and the resulting `WU-28a`/`WU-28b`
split) — appended in full this session; see `DECISIONS.md`. Does not
reopen `compositeLayered()`'s own existing two-layer design (ADR-028/029,
ADR-009's "not a k-buffer" note stays true of that specific mechanism),
`docs/architecture.md`, or any already-`green` unit — see ADR-059's own
closing paragraph.

## Append to CORRECTIONS.md

None this session — the I6/tied-`z` risk ADR-059 records was found and
designed around during scoping, before any code existed for it to be a
defect in; nothing here fits `CORRECTIONS.md`'s own purpose (a lesson about
something already built, believed correct, and then found wrong).

## Closed out this session

Nothing built or tagged — WU-28 was scoped only, per this session's own
explicit, standing instruction not to build it. WU-22c's own close-out
(build/run/commit/tag/push) was closed out by Steve himself, between
sessions, not by this one; this session only fixed the one stale status
line `WORK-UNITS.md` still carried for it.

## Steve's own next steps

This session wrote no code and built nothing — only three docs changed.
Review the diffs, then commit and push them (no tag needed; nothing here
is a work unit reaching `green`). `.git/index.lock` may need clearing
first (Flagged item 4):

```
rm -f ~/src/scatter-dve/.git/index.lock
cd ~/src/scatter-dve
git status
git diff -- WORK-UNITS.md DECISIONS.md HANDOFF.md
```

If the diffs look right:

```
git add WORK-UNITS.md DECISIONS.md HANDOFF.md
git commit -m "WU-28 scoping: tag-keyed k=4 buffer design (ADR-059), split into WU-28a/WU-28b"
git push origin main
```

No `git push origin --tags` needed this time — no tag was created, since
no work unit reached `green` this session.

Once done, `git status -sb` should read `## main...origin/main` with no
`[ahead]`/`[behind]` marker, and `git log --oneline -1` should show this
commit at `HEAD`.
