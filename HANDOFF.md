# Handoff
Overwritten at the end of every session. This is the first thing to read.

---

**Session:** 30 (WU-22a only — WU-22b explicitly not started, see below)
**Tag:** `wu-22a-green` — confirmed at Steve's own real terminal this
session. `cmake --build build` clean, `ctest --test-dir build
--output-on-failure` 27/28 (the sole failure `test_decklink_device`'s
`foundDuplexDevice` check, ADR-035's own already-accepted exception,
unrelated to this unit), `test_coverage_capture` itself green.
`./tools/close.sh 22a` correctly refused to auto-tag (it has no knowledge
of ADR-035); the manual `git tag -a wu-22a-green ...` step every
DeckLink-touching unit since ADR-035 has used was run instead, and
verified directly against the real repository: `git show wu-22a-green
--stat` lists exactly the seven files this unit's own commit touched
(`CMakeLists.txt`, `DECISIONS.md`, `HANDOFF.md`, `WORK-UNITS.md`,
`src/core/pipeline.cpp`, `src/core/resolve.hpp`,
`tests/test_coverage_capture.cpp`). `git push origin HEAD --tags` reported
no `origin` remote configured — the commit and tag are local-only, same as
`close.sh` itself would have silently accepted. Also re-confirmed
`wu-21i-green` still exists and still contains all four of its own files
— Session 29's own flagged commit gap remains genuinely resolved.

**Before doing anything else in the next session:** run `git tag`, `git
log --oneline -10` and `git status --short` directly against
`~/src/scatter-dve` via the device bridge, the same as every session
before this one — do not trust this file's own account of tag state
without checking it against the real repository first.

## Flagged now

**1. `WORK-UNITS.md`'s own WU-21i entry is stale.** It still reads `wip`
and "not yet built or run at Steve's own real terminal," but the
`wu-21i-green` tag now exists and contains the right files (confirmed
above). This session did not rewrite that entry's status line, because it
only has the tag's existence to go on, not the actual by-eye acceptance
detail (which letter keys were tried, whether `Q` exits cleanly, etc.)
that an honest status line needs — that detail belongs to whoever was at
the terminal when it was tagged. **Next session (or Steve directly) should
fix WU-21i's status line in `WORK-UNITS.md` and this file's own tag
history once that detail is available.**

**2. `device_stage_files` (the device-bridge tool that re-reads files back
from `~/src/scatter-dve` for confirmation) failed all session with `HTTP
403 "untrusted_device"` — the device's sign-in is stale. `device_bash` and
`device_commit_files` (writing files *to* the device) both kept working
throughout, so nothing was blocked outright, but every file this session
wrote back was verified by `shasum -a 256` comparison via `device_bash`
instead of the normal re-stage-and-diff. **Steve: please sign in again in
the Claude desktop app on this device** — a sign-in banner should already
be showing there — so the next session's file staging works normally
again.

**3. (Resolved this session.)** A stale `.git/index.lock` appeared in
`~/src/scatter-dve` after two consecutive `git status --short` calls via
the device bridge; `device_bash` cannot delete files on a mounted folder,
so it needed Steve's own `rm -f ~/src/scatter-dve/.git/index.lock` at his
real terminal, which he ran before committing — confirmed gone afterward.
No action needed; noted here only so a lock file reappearing next session
reads as a new occurrence, not a repeat of this one.

## This session in full

Session opened by requesting device-bridge access to `~/src/scatter-dve`
and `~/src/Blackmagic DeckLink SDK 16.0` (approved), reading all six
project docs in full from the staged copies, and then checking Session
29's own flagged commit gap directly against the real repository rather
than trusting `HANDOFF.md`'s account of it — found already resolved (see
Tag, above).

Asked which work unit was next; after two clarifying questions about what
"weight," "coverage," and "det J" actually mean, Steve chose: **weight
only to begin with, rendered in a window on the Mac** — i.e. WU-22
(diagnostic coverage view), split per this project's own established a/b
discipline into WU-22a (the weight-capture plumbing itself, portable, no
Mac dependency) and WU-22b (the Metal window that displays it, Mac-only,
not started).

**WU-22a** adds `PipelineParams::weightOut` (`src/core/resolve.hpp`): a
non-owning, caller-owned, opt-in pointer field, `nullptr` by default,
mirroring `PipelineParams::pool`'s own shape (WU-19a/ADR-044) rather than
threading a second output raster through every `runFrame()`/
`runFrameBytes()`/`runFrameFile()` signature. When set, it must point to
`destWidth * destHeight` tight-packed `WeightAccum` values; `core/
pipeline.cpp`'s `resolveOneTile()` writes each destination cell's raw
`AccumCell::w` (captured before `composite()`'s own clamp to
`[0, kWeightUnity]`) into it, immediately after that cell's ordinary
`dest.Y/Cb/Cr` writes. See `DECISIONS.md` ADR-056 for the full design
rationale.

A new test file, `tests/test_coverage_capture.cpp` (registered in
`CMakeLists.txt`), checks four things: capture is side-effect-free
(identical composited output whether `weightOut` is null or supplied);
weight reads exactly zero at genuinely uncovered cells and exactly
`kWeightUnity` at fully, evenly covered interior cells; captured values
match bit-for-bit against an independent recomputation via the public
`generateFragments()`/`splatTile()`/`sumBanks()` path (not
`resolveOneTile()` itself); and results are identical across thread
counts (I6). Writing test 2 surfaced a real, already-known codebase
property — `CORRECTIONS.md` C-008(a)'s edge-derivative damping (ADR-022's
edge-replication clamp) — showing up as doubled weight at the lattice's
own `u=0`/`v=0` edge; root-caused via direct `Lattice::jacobian()` calls
in a scratch program, then fixed by narrowing the "near-unity" check to
interior rows/columns and tightening it to exact equality. Judged, per
the ADR-048/C-006 precedent, as routine iteration rather than a new
lesson, so no new `CORRECTIONS.md` entry was logged for it.

Verified across the project's full matrix — Clang 18 and GCC 13, Release
and Debug, tile `2^4` and `2^5` (8 configs), plus GCC ASan/UBSan — all
green, 14221/14221 checks passing in `test_coverage_capture` itself and
no regressions elsewhere. Delivered to the real repository (`src/core/
resolve.hpp`, `src/core/pipeline.cpp`, `tests/test_coverage_capture.cpp`,
`CMakeLists.txt`) via the device bridge and confirmed written correctly
by sha256 comparison (see Flagged item 2 above for why sha256 rather than
the normal re-stage-and-diff). `DECISIONS.md` ADR-056 and the
`WORK-UNITS.md` WU-22a/WU-22b entries were delivered the same way.

Steve then built and tested it himself at his real terminal: `cmake
--build build` (`ninja: no work to do` — already configured from Session
29), `ctest --test-dir build --output-on-failure` 27/28, the sole failure
`test_decklink_device`'s `foundDuplexDevice` check (ADR-035's own
already-accepted exception), `test_coverage_capture` itself green.
Committed (`d58641a`), then `./tools/close.sh 22a` correctly refused to
auto-tag (no knowledge of ADR-035), so the manual `git tag -a
wu-22a-green ...` step every DeckLink-touching unit since ADR-035 has
used was run instead — confirmed via `git show wu-22a-green --stat`
listing exactly the seven files this unit's own commit touched. No
`origin` remote configured, so the tag is local-only. **WU-22a is
genuinely `green`.**

**WU-22b (the Metal window itself) was deliberately not started.** It is
this project's first Metal/Cocoa dependency of any kind — nothing else in
`src/` touches Apple's UI frameworks — and deserves its own real scoping
session rather than being drafted sight-unseen in a Linux sandbox that
cannot compile, let alone run, anything Metal-related.

## Where we are

Phase 5 (Live capture) is otherwise unchanged from Session 29's own
account. Phase 6 (Scale up) now has its first real entry: WU-22a `green`
(`wu-22a-green`), WU-22b `todo` (unscoped, not started). `DECISIONS.md`
runs through ADR-056. `CORRECTIONS.md` is unchanged this session (still
through C-018) — see above for why the C-008(a) finding did not get a new
entry.

## Next work unit

Not decided here, deliberately — Steve's own call at the start of the
next session. WU-22b (the Metal window) is the natural continuation if
Steve wants to see the weight capture actually rendered — but it needs a
real scoping conversation first (target frame rate for the display
refresh, whether it shares the Mac's main event loop with anything else,
a colour ramp for the weight visualisation, window size/resizability),
not an assumed design, since it is this project's first Metal/Cocoa
dependency of any kind. WU-21d (cold-start black-fill fix) and starting
to scope WU-28 (k-buffer) remain open candidates too, exactly as Session
29 left them, if Steve would rather pick up one of those instead.

## Open questions

Unchanged from Session 29: `kCaptureRingCapacity`'s value of 8
(WU-20a/20b, ADR-046), the cold-start green-frame artifact (WU-21d), and
front/back occlusion/transparency (WU-28) all remain open. Q3 (macOS/
Desktop Video version), Q4 (lattice edge damping, C-008(a)) remain open
from earlier sessions — Q4 in particular is now directly exercised (not
just theorised about) by WU-22a's own test 2, see above.

## Blocked / red

Nothing red. WU-22a is genuinely `green`, confirmed at Steve's own real
terminal.

## Environment check

Unchanged: **UltraStudio Monitor 3G** (output, HDMI-mirrored) and
**UltraStudio Recorder 3G** (input) both last confirmed working in
Session 29's own real-hardware runs (not touched this session — WU-22a is
portable, no DeckLink dependency). **UltraStudio 4K Mini** remains on hold
pending a PSU replacement.

## Append to DECISIONS.md

ADR-056 (WU-22a design: the WU-22a/WU-22b split, `PipelineParams::
weightOut`'s own rationale, the C-008(a) finding, the full verification
matrix, the `untrusted_device` staging failure and its sha256 workaround,
the stale `.git/index.lock`) — appended in full this session; see
`DECISIONS.md`. Does not reopen any earlier ADR.

## Append to CORRECTIONS.md

None this session — see "This session in full" above for why the
C-008(a) finding during WU-22a test-writing did not warrant a new entry.

## Closed out this session

WU-22a's full loop — sandbox build/verify, delivery via the device
bridge, Steve's own real-terminal build/test, manual tag per the ADR-035
exception pattern, and `git show wu-22a-green --stat` confirmation — all
completed within this same session. Nothing outstanding for WU-22a.
