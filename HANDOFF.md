# Handoff
Overwritten at the end of every session. This is the first thing to read.

---

**Session:** 29 (long session — WU-21c through WU-21i, all in one sitting)
**Tag:** `wu-21c-green` is the last real, confirmed-tagged unit. WU-21d is
a scoped `todo` stub on the roadmap, not started. WU-21e/f/g/h were each
built, run, and given real feedback in turn, then **superseded before
being tagged** as that feedback reshaped the next unit — this is
deliberate, not lost work; see "This session in full" below. **WU-21i is
the current head of that chain, run and confirmed working by Steve — but
not yet safely taggable, see the flagged gap below.**

**Before doing anything else in the next session:** run `git tag`, `git
log --oneline -10` and `git status --short` directly against
`~/src/scatter-dve` via the device bridge, the same as every session
before this one — do not trust this file's own account of tag state
without checking it against the real repository first.

## Flagged now, fix before tagging WU-21i

`git show a1f1a74 --stat` (the commit already made, message "WU-21i:
letter-key manual controls...") shows it contains only `CORRECTIONS.md`,
`DECISIONS.md`, `WORK-UNITS.md` — **`tests/test_decklink_live_sphere.cpp`
itself, the file Steve actually ran and confirmed working, was never
staged into that commit.** `git status --short` on the real repository
confirms it still shows as modified, uncommitted, right now. Steve's own
next terminal commands, before anything else:

```
cd ~/src/scatter-dve
git add tests/test_decklink_live_sphere.cpp
git commit --amend --no-edit
git tag -a wu-21i-green -m "letter-key manual controls (X/x, Y/y, Z/z) confirmed working at the real terminal"
git log --oneline -3
git status --short
```

`--amend --no-edit` folds the file into the existing WU-21i commit rather
than adding a confusing separate one, since that commit's own message
already correctly describes what should be in it. Confirm afterward that
`git show wu-21i-green --stat` lists all four files (the three docs plus
the test file) before treating WU-21i as genuinely closed.

## This session in full

Session 29 opened by confirming `wu-21b-green` and the WU-21b real-
hardware-verification commit were both already in place (session 28's own
one loose end turned out already resolved), then delivered WU-21c
(`LiveFramePlayback`, continuous SDI re-output) as reasoned-through-only,
which Steve then built, ran, and verified against real hardware in the
same session — `ctest` 25/26 (the one known `test_decklink_device`
exception, ADR-035), `test_decklink_live_output` passing all 10 checks,
tagged `wu-21c-green`. That real run surfaced two genuine findings, both
recorded in `DECISIONS.md` ADR-050's own addendum: a second
`kCaptureRingCapacity` data point (still open, ADR-046/049), and a
previously-unanticipated cold-start green-frame artifact in
`LiveFramePlayback`'s own pool, which became WU-21d — a scoped `todo` stub
on the roadmap, not yet started.

Steve then asked when live *warped* (not identity-mapped) video would be
possible, ideally a sphere. Answer, checked against the real code rather
than assumed: immediately, since `buildSphereLattice()` (WU-11, ADR-027)
and the now-lattice-agnostic live pipeline (WU-21b/c) were both already
independently verified — only which lattice `CaptureConsumer` was built
with needed to change. That became a chain of small units, each one
shaped directly by Steve's own real-hardware feedback on the one before
it, every single one actually built and run at his real terminal before
the next started:

- **WU-21e** — first sphere demo. Superseded before ever being built —
  Steve found the geometry cropped vertically and not fully wrapped before
  running it at all.
- **WU-21f** — fixed geometry (equal `angleSpanH`/`angleSpanV`), added
  `CaptureConsumer::setLattice()` (the one real `src/` change in this
  whole chain — thread-safe lattice replacement, `m_latticeMutex`, a
  snapshot copy taken before touching the capture frame's own buffer) and
  a first two-axis oscillating rotation. Built and run — "interesting,"
  but the wrap still read as only 120–180 degrees, stopping short of the
  poles.
- **WU-21g** — full pole-to-pole (`angleSpanV == pi`) / seamless 360-degree
  (`angleSpanH == 2*pi`) wrap; one continuous rotation axis, one
  oscillating. Built and run — **"hugely better."** Surfaced, as
  predicted, visible front/back overlap where the wrap folds (no
  occlusion sorting yet) — recorded as a note on `WORK-UNITS.md`'s own
  WU-28 (k-buffer) entry, covering both opaque front/back switching and
  genuine transparency as Steve asked, rather than a new backlog entry.
- **WU-21h** — replaced automatic rotation with a rudimentary interactive
  UI (cursor keys rotate, shift+cursor reposition, I/O resize, Q quits).
  Built and run — plain cursor rotation worked, shift+cursor did not.
- **WU-21i** — replaced shift+cursor and I/O with six ordinary letter keys
  (`X`/`x`, `Y`/`y`, `Z`/`z`, uppercase increments/lowercase decrements),
  sidestepping the whole terminal-escape-sequence problem rather than
  debugging it. Built and run — confirmed working ("That works — great!").
  **Not yet safely taggable — see the flagged gap above.**

Two real corrections were caught and logged in `CORRECTIONS.md` along the
way, both about claims this session made that didn't hold up:
`C-017` — WU-21f's own "a large rotation could produce negative depth"
justification for a conservative rotation amplitude was wrong; a rotation
about a sphere's own true centre cannot produce `z < 0`, for any
amplitude (`shapes.hpp`'s own invariant, re-derived properly). `C-018` —
WU-21h's own claim that shift+cursor would arrive as `ESC [ 1 ; 2
<letter>` "on both [terminals] by default" was flagged as unverified when
written and turned out wrong on Steve's own real terminal; fixed by
removing the whole class of problem (letter keys) rather than chasing the
right escape sequence.

## Where we are

Phase 5 (Live capture) continues. WU-20a/WU-20b/WU-21a/WU-21b/WU-21c all
`green`. WU-21d is a scoped `todo` stub, not started. WU-21e/f/g/h are
each superseded by the next in the chain, exactly as intended — nothing
from them is lost; each one's own real feedback is what shaped the next.
WU-21i is functionally complete and confirmed working, blocked only on the
commit-content gap flagged above. `DECISIONS.md` runs through ADR-055.
`CORRECTIONS.md` runs through C-018. `WORK-UNITS.md`'s own WU-28 entry
(Phase 7, k-buffer) now carries a note naming both front/back occlusion
and transparency as real, Steve-requested scope for whenever that unit is
picked up.

## Next work unit

Not decided here, deliberately — Steve's own call at the start of the next
session, once WU-21i is confirmed genuinely tagged. Real candidates
already named on the roadmap, in no particular order: WU-21d (the
cold-start black-fill fix, small and well-scoped); continuing to push the
live-sphere demo further (Steve may want more controls, or to try the
endurance/by-eye criteria WU-21c's own `Accept:` text deferred); WU-22
(diagnostic coverage view, next in the plain numeric sequence); or
starting to scope WU-28 (k-buffer) for real, now that there's a concrete
on-screen motivating case for it. A fresh session should read this file
and `WORK-UNITS.md` in full, confirm real git/tag state per the note at
the top of this file, and then ask Steve directly which of these (or
something else) is next, rather than assuming.

## Open questions

`kCaptureRingCapacity`'s own value of 8 (WU-20a/20b, ADR-046) has two real
data points now (WU-21b, WU-21c), both showing `pushed - popped == 8`
exactly — still not conclusively diagnosed. The cold-start green-frame
artifact (WU-21d) is named and scoped, not fixed. Front/back
occlusion/transparency (WU-28) now has a concrete motivating case but is
otherwise unscoped. Whether `readKey()`'s remaining bare-ESC rough edge
(WU-21h/i's own header comment) is worth addressing is unresolved — low
priority, noted, not blocking. Q3 (macOS/Desktop Video version), Q4
(lattice edge damping, C-008(a)) remain open from earlier sessions.

## Blocked / red

Nothing red. WU-21i works; its own commit just needs the fix above before
it can be honestly called done.

## Environment check

Unchanged: **UltraStudio Monitor 3G** (output, HDMI-mirrored) and
**UltraStudio Recorder 3G** (input) both confirmed working throughout this
session's own real-hardware runs. **UltraStudio 4K Mini** remains on hold
pending a PSU replacement.

## Append to DECISIONS.md

ADR-050 (WU-21c design + real-hardware addendum), ADR-051 (WU-21e),
ADR-052 (WU-21f design + real-hardware addendum), ADR-053 (WU-21g),
ADR-054 (WU-21h), ADR-055 (WU-21i) — all appended in full this session; see
`DECISIONS.md`. None reopen anything earlier than ADR-050 except where
explicitly noted (ADR-052's own rotation-amplitude reasoning, corrected by
C-017; ADR-054's own shift-detection mechanism, corrected by C-018).

## Append to CORRECTIONS.md

C-017 (WU-21f's own negative-depth rotation worry, unfounded — see above)
and C-018 (WU-21h's own shift+cursor escape-sequence claim, wrong on real
hardware — see above), both appended in full this session.
