# Handoff
Overwritten at the end of every session. This is the first thing to read.

---

**Session:** 38 (WU-21d, Cold-start black fill for `LiveFramePlayback`'s own
pool — scoped and built this session, per `SESSION-PROTOCOL.md`'s normal
shape, though "built" covers only the portable half; see below). Also:
`WORK-UNITS.md`'s own WU-21i stale `wip` status line corrected doc-only
(confirmed `wu-21i-green` genuinely exists via `git tag`), and WU-17's own
entry corrected doc-only (its own body text claimed "Steve tagged
`wu-17-green` by hand"; no such tag exists in the real repository, confirmed
via `git tag` — the code itself is done, only the tag is missing).

**Tag:** no new tag exists yet. `wu-21d-green` is Steve's own next action,
once his own real-terminal build/`ctest`/by-eye run confirms the
DeckLink-linked half (untestable in this session's own cloud sandbox — see
below) is genuinely green. See "Steve's own next steps" below.

## Before doing anything else in the next session

Run `git tag`, `git log --oneline -10`, `git status --short` and `git status
-sb` directly against `~/src/scatter-dve` via the device bridge, the same as
every session before this one — do not trust this file's own account of
tag/commit state without checking it against the real repository first.

## This session in full

Opened by requesting device-bridge folder access to `~/src/scatter-dve`
only (core-only scoping, not the Blackmagic SDK folder), then reading
`SESSION-PROTOCOL.md`, `HANDOFF.md`, `WORK-UNITS.md`, `DECISIONS.md`,
`CORRECTIONS.md`, `INVARIANTS.md` in full, then verifying real repository
state directly via the device bridge, per standing discipline, before
trusting `HANDOFF.md`'s own account: `git tag` listed both `wu-26-green` and
`wu-28b-green`; `git log --oneline -10` showed `HEAD` at `4381823` ("WU-26:
normals from lattice (ADR-063); C-021 tag-before-commit correction");
`git merge-base --is-ancestor` confirmed both tags as ancestors of `HEAD`
(`wu-26-green` in fact dereferences to `HEAD` itself — `git rev-parse
wu-26-green` returning a different-looking hash is the annotated tag
object's own SHA, not the commit it points at; `git log wu-26-green`/`git
show -s wu-26-green` both resolve to `4381823`); `git status --short` empty;
`git status -sb` read `## main...origin/main` with no ahead/behind marker.
Session 37's own C-021 fix had genuinely landed — no drift found this time,
unlike several recent prior sessions. Full detail in `DECISIONS.md` ADR-064's
own opening section.

With WU-26/WU-28b both confirmed genuinely closed, moved to WU-21d, per this
session's own brief. Read WU-21d's own full `WORK-UNITS.md` entry, then
`src/io/decklink_live_output.hpp`/`.cpp` (where `LiveFramePlayback` actually
lives — not `decklink_live_playback.hpp`, which does not exist; the real
file layout was checked directly, not assumed) in full, then `DECISIONS.md`
ADR-050's own same-session addendum (the original cold-start-green finding
this unit is scoped against) in full.

**Confirmed DeckLink-linked before assuming either way, per this session's
own brief.** `CMakeLists.txt`'s own `scatter-decklink` static library target
lists `src/io/decklink_live_output.cpp` among its sources, guarded by
`if(APPLE AND BLACKMAGIC_SDK_DIR AND EXISTS
"${BLACKMAGIC_SDK_DIR}/Mac/include/DeckLinkAPI.h")` — confirmed directly by
reading the file. This session's own cloud-sandbox `cmake -B build` output
stated plainly "BLACKMAGIC_SDK_DIR not set (or not Apple, or SDK not found
there) — skipping scatter-decklink" — `decklink_live_output.cpp` is not
compiled by this sandbox's own CMake configuration at all, confirmed by its
own absence from every build log this session produced.

**Design decision, recorded in full in `DECISIONS.md` ADR-064: push the
pure, portable part (building black v210 bytes) into `video/v210.hpp`/`.cpp`
as a new `packBlackFrame()`, and keep `decklink_live_output.cpp`'s own
change to the minimum SDK-facing glue** — three lines of substance in
`startWith()`'s pool-creation loop (build the black bytes once, fill each
buffer with them right after `CreateVideoFrame()`, fail `create()` cleanly
if a fill fails). This is a materially more testable design than putting the
black-fill logic directly in the DeckLink-linked file would have been: it
follows the project's own established `scatter-core`/`scatter-decklink`
separation (the same shape `CaptureConsumer` calling into `runFrameBytes()`
already uses, ADR-048) and means the actual byte-construction logic is
genuinely built and tested in this session's own cloud sandbox, not only
reasoned through by hand — even though the unit as a whole remains not
core-only, and `decklink_live_output.cpp` itself stays reasoned-through-only,
unbuilt and unrun in this sandbox, the same as every DeckLink-touching unit
before it since WU-14/ADR-031.

Built and tested in the cloud sandbox, for the portable half only: fresh
`git clone` of `https://github.com/skunge2000/scatter-dve.git`, confirmed
matching the real repository's own `git tag`/`git log`/`git status -sb`
(`wu-26-green` at `HEAD`, `4381823`, clean, in sync) before any file was
touched. Full 8-configuration matrix — GCC 13.3.0 and Clang 18.1.3, Release
and Debug, `SCATTER_TILE_LOG2` 4 and 5 — all green, zero warnings under this
project's full `-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion
-Werror` set, plus GCC 13 with `-fsanitize=address,undefined
-fno-sanitize-recover=all`: clean, no sanitizer report. `ctest`: 22 of 22
targets passing in every configuration, no regressions anywhere outside
`test_v210` itself. `test_v210` alone: 5117 checks passing (three new
`testPackBlackFrame()` calls at widths 12, 14, 720).

Delivered four changed/new-content source files plus three doc files to the
real repository via `SendUserFile` + `device_commit_files`, to
`/Users/stephenneal/src/scatter-dve/...`: `src/video/v210.hpp`,
`src/video/v210.cpp`, `src/io/decklink_live_output.cpp`,
`tests/test_v210.cpp`, `WORK-UNITS.md`, `DECISIONS.md`, this file.
**Delivery confirmation (this session's own device_bash checks after
writing every file):** `wc -l` on each of the seven files matched this
session's own sandbox copies exactly; `git status --short` showed exactly
these seven files changed, nothing else; byte-for-byte `diff` against this
session's own sandbox copies, re-staged from the device afterward, showed
no differences on any of the seven.

## Where we are

Phase 6 (Scale up) unchanged: WU-22a/b/c all `green`. Phase 7 (Starlight):
WU-26 `green` (confirmed this session, Session 37's own C-021 fix landed),
WU-27 `todo`, WU-28a/WU-28b `green`, WU-28c/WU-28d `todo` (both still gated
on WU-26, which has now genuinely landed — WU-28c's own real scoping session
can proceed in full whenever it is next picked up), WU-29 `todo`. Phase 5
(Live capture): WU-21d now `wip` (this session) — the portable half is
real-terminal-independent green (cloud sandbox, above); the DeckLink-linked
half needs Steve's own real-terminal build/`ctest`/by-eye run before this
line goes `green`. `DECISIONS.md` runs through ADR-064. `CORRECTIONS.md`
unchanged this session, still runs through C-021 — nothing new was wrong
enough to log there.

## Next work unit

**Steve's own real-terminal close-out for WU-21d** — see "Steve's own next
steps" below. After that: WU-28c (self-fold facing tag) is now unblocked in
full and is a reasonable next pick — its own real scoping session's first
job is fixing its own stale "Depends on WU-26... not yet scoped or built"
line as part of doing the real `Files:`/`Accept:` work, not before it.
WU-21d's own original todo entry also named further live-pipeline candidate
territory (the one-hour endurance run, `framesRepeated` rate over a longer
run) that this session deliberately left unscoped — see `DECISIONS.md`
ADR-064's own "Scope, decided now" paragraph — available for a future
session if it becomes a priority, not currently blocking anything.

## Open questions

Unchanged from Session 35/36/37: `kCaptureRingCapacity`'s value of 8
(WU-20a/20b, ADR-046), Q3 (macOS/Desktop Video version), and Q4 (lattice
edge damping, C-008(a)) all remain open, none touched this session. The
cold-start green-frame artifact itself (previously listed here as WU-21d)
is no longer merely an open question — see this session's own work above;
whether it is actually fixed on real hardware is Steve's own next
confirmation, not still an open question about what to do.

## Blocked / red

Nothing red. WU-28c/WU-28d remain honestly `todo`, no longer blocked on
anything (WU-26 landed). WU-21d is `wip`, not broken: its own portable half
is genuinely tested and passing; only the DeckLink-linked half and the
by-eye confirmation are outstanding, the same category every DeckLink-
touching unit's own real-terminal step has always been.

## Environment check

Unchanged from Session 35/36/37: **UltraStudio Monitor 3G** (output,
HDMI-mirrored) and **UltraStudio Recorder 3G** (input) both last confirmed
working in Session 29's own real-hardware runs. **UltraStudio 4K Mini**
remains on hold pending a PSU replacement. `origin` remains configured and
in sync as of this session's own opening and closing checks.

## Append to DECISIONS.md

ADR-064 (WU-21d scoping and build: the DeckLink-linked confirmation,
`scatter::v210::packBlackFrame()`'s design and the portable/DeckLink-linked
split it enables, `decklink_live_output.cpp`'s own minimal integration, and
the explicit scope decision leaving the endurance-run/`framesRepeated`
candidate territory undecided) — appended in full this session; see
`DECISIONS.md`. Does not reopen ADR-050 (extends its own named-not-fixed
candidate into a real implementation) or ADR-010/032/037/048/063.

## Append to CORRECTIONS.md

Nothing this session — no error found that rose to a logged correction.
(Two doc-only status-line corrections were made in place, in `WORK-UNITS.md`
itself, per the same convention Session 35/37 already used for WU-28a/WU-28b's
own stale lines — not `CORRECTIONS.md` entries, since neither was a mistake
made by an assistant session, only stale bookkeeping found while
re-verifying repository state.)

## Closed out this session

Nothing tagged this session — WU-21d's own DeckLink-linked half cannot be
verified without Steve's own real terminal (no Blackmagic SDK, no
AppleClang/Xcode in this cloud sandbox), so no `close.sh`/manual-tag step
was appropriate here, consistent with "the assistant does not run
`close.sh`" and does not tag units it could not itself fully verify.

## Steve's own next steps

**1. Rebuild and test WU-21d's own DeckLink-linked half, with the same
Monitor 3G SDI-out loopback (or a live source, whichever you have patched
in) `test_decklink_live_output.cpp`'s own header comment already documents
— no new physical setup.**

```
rm -f ~/src/scatter-dve/.git/index.lock
cd ~/src/scatter-dve
git status --short
cmake --build build
ctest --test-dir build --output-on-failure
```

Expected: everything green except `test_decklink_device`'s own
`foundDuplexDevice` check — ADR-035's already-accepted exception (the
Monitor 3G is playback-only) — nothing else. `test_v210` should now report
more checks than before (5117 in this session's own cloud-sandbox run,
GCC/Clang on Linux — the real AppleClang count may differ slightly but
should not fail).

**If anything else fails, stop here — don't tag, don't proceed to step 2 —
and paste me the full `--output-on-failure` output.** That would be a real
regression this session's own cloud-sandbox run (which never even compiled
`decklink_live_output.cpp`, no SDK present) could not have caught.

**2. By-eye confirmation — this unit's own only unautomatable `Accept:`
criterion.** Run the live-output test binary directly and watch the Monitor
3G's own HDMI-mirrored output from the very start of the run:

```
cd ~/src/scatter-dve
./build/test_decklink_live_output
```

Expect: **black**, not the strongly saturated green ADR-050's own addendum
recorded, for the first moment or two after the run starts, before real
captured content (or the identity-mapped loopback signal) appears. Worth
reporting either way — if it is still green, that means the fix did not
take effect on real hardware and this needs a fresh look, not a tag.

**3. If both of the above are clean, commit and tag.** This uses the manual
fallback (the known `test_decklink_device` exception `close.sh` cannot
distinguish from a real failure) — `git add`/`git commit` come **before**
`git tag`, so the tag lands on the commit that actually contains these
changes, not on whatever `HEAD` happened to be beforehand (the exact mistake
C-021 recorded last session):

```
cd ~/src/scatter-dve
git add src/video/v210.hpp src/video/v210.cpp src/io/decklink_live_output.cpp tests/test_v210.cpp WORK-UNITS.md DECISIONS.md HANDOFF.md
git commit -m "WU-21d: cold-start black fill for LiveFramePlayback's own pool (ADR-064)"
git tag -a wu-21d-green -m "WU-21d: cold-start black fill green (test_decklink_device/foundDuplexDevice is ADR-035's known exception)"
git push origin main
git push origin --tags
```

**4. Verify it landed correctly:**

```
cd ~/src/scatter-dve
git log --oneline -3
git tag | tail -5
git status -sb
```

`git log --oneline -3` should show your own new commit at `HEAD`, carrying
`wu-21d-green`; `git status -sb` should read `## main...origin/main` with no
ahead marker and no modified files listed at all.

---

**Optional, independent of WU-21d — WU-17's own missing tag.** This
session found `WORK-UNITS.md`'s own WU-17 entry claiming you had already
tagged `wu-17-green` by hand, but no such tag exists in the real repository
(confirmed via `git tag`) — the doc claim was corrected in place this
session (see `WORK-UNITS.md`'s own WU-17 entry). The code itself is already
verified green per that entry's own text (full suite passing at your own
real terminal, `test_v210_neon` itself green). If you want to close this out
for real, independently of WU-21d above, and your working tree is otherwise
clean:

```
cd ~/src/scatter-dve
git status --short
git tag -a wu-17-green -m "WU-17: NEON v210 unpack and pack green (test_decklink_device/foundDuplexDevice is ADR-035's known exception)"
git push origin --tags
```

Only run this if `git status --short` above prints nothing — if it prints
anything, something is uncommitted and tagging now would repeat C-021's own
mistake on a different unit.
