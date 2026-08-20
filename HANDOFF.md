# Handoff
Overwritten at the end of every session. This is the first thing to read.

---

**Session:** 34 (WU-28a — built and tested, per Session 33's own scoping.
Per Steve's own standing answer, WU-28a stays entirely inside
`scatter-core`, so this session actually built and ran it directly in this
project's cloud sandbox, rather than reasoning it through and handing it
off via the device bridge — a first for any WU-28-adjacent unit and, per
Session 33's own account, a first for this project generally.)
**Tag:** none this session. WU-28a is `wip`, not `green` — it was built
and tested in the cloud sandbox, but `SESSION-PROTOCOL.md`'s own `green`
status is reserved for Steve's own real-terminal build/run/commit/tag/push,
which has not happened yet.

**Before doing anything else in the next session:** run `git tag`, `git
log --oneline -10` and `git status --short` directly against
`~/src/scatter-dve` via the device bridge, the same as every session
before this one — do not trust this file's own account of tag/commit state
without checking it against the real repository first.

## Flagged now

**1. `WORK-UNITS.md`'s own WU-21i entry is still stale — not touched this
session (out of scope for WU-28a).** As of Session 31's own `HANDOFF.md`,
it read `wip` and "not yet built or run at Steve's own real terminal,"
despite `wu-21i-green` existing and containing the right files, still
needing Steve's own by-eye acceptance detail (which letter keys were
tried, whether `Q` exits cleanly) before the status line could be honestly
fixed. Now six sessions old if still unfixed. Worth doing directly
whenever a session next touches `WORK-UNITS.md` for an unrelated reason —
this session touched `WORK-UNITS.md` for WU-28a's own status line but did
not check WU-21i's, since that would have meant asking Steve the same
by-eye acceptance question this file has deferred for six sessions
running, not something to answer on his behalf.

**2. WU-28a is `wip`, not `green` — Steve's own real-terminal build/run/
commit/tag/push is this unit's own remaining step.** See "Steve's own next
steps" below for the exact commands. The cloud sandbox's own build/test
run (DECISIONS.md ADR-060) is real compiler/test evidence — GCC 13.3 on
Linux x86_64, the full portable `ctest` suite green, no regressions — but
is evidence, not a substitute: the sandbox's toolchain does not match
Steve's own (AppleClang, ARM64), and the sandbox has no git identity to
commit/tag/push with regardless.

**3. A real bug this session's own sandbox build caught, not design
review: a rawWeight-0 corner touch (harmless in the plain
`accumulateCorner()` path) was phantom-claiming k-buffer slots before the
fix.** Fixed within `splatTileKBuffer()`'s own file scope this session —
see DECISIONS.md ADR-060 and CORRECTIONS.md C-019. Worth a future session
remembering the general lesson C-019 draws (a new consumer of
`splatCorners()`'s shared `sink` contract needs its own review of that
contract's full behaviour, not just the part an existing caller happens to
rely on) if `core/binner.cpp`/`core/splat.cpp` ever gain a third consumer.

**4. `.git/index.lock` pattern — not encountered this session (no local
commits attempted; this session only wrote files via `SendUserFile` +
`device_commit_files`, same delivery path as Session 33's own text-heavy
files, extended this session to source files too).** Still the same known,
non-blocking behavior documented across Sessions 29-33: `device_bash` git
commands that only read (`status`, `log`, `diff`, `show`, `tag`) succeed
and print correct output even with a stale lock file present;
`device_bash` itself can never remove it. **Steve: run `rm -f
~/src/scatter-dve/.git/index.lock` before your own `git add`/`git commit`
below** — routine, not a one-off fix.

**5. `device_stage_files`/`device_commit_files` HTTP 403 `untrusted_device`
— not encountered this session.** If it recurs: it means the Mac slept
mid-session and invalidated the device's trusted-sign-in state, not an
account problem — ask Steve to re-enable access in the Claude desktop app
(no fresh sign-in needed), then retry. Noted only so a future session does
not have to re-diagnose it from scratch again.

## This session in full

Session opened by requesting device-bridge access to `~/src/scatter-dve`
only (the Blackmagic SDK folder was not requested — WU-28a touches only
`src/core/`, nothing DeckLink- or Metal/Cocoa-related, per Session 33's own
scoping). Read `SESSION-PROTOCOL.md`, `HANDOFF.md`, `WORK-UNITS.md`,
`DECISIONS.md`, `CORRECTIONS.md` and `INVARIANTS.md` in full, then verified
Session 33's own account of repository state directly (`git tag`, `git log
--oneline -10`, `git status --short`, `git status -sb`) before trusting any
of it: `HEAD` at `a18a419` ("WU-28 scoping..."), clean tree, `git status
-sb` reading `## main...origin/main` with no ahead/behind marker — exact
match, no drift to report. `core/types.hpp`, `core/splat.hpp`/`.cpp` then
re-read in full against ADR-059's own design before writing anything, per
this session's own standing instruction — nothing had drifted since
Session 33.

Built WU-28a exactly per `WORK-UNITS.md`'s own `Files:`/`Accept:` entry:
`src/core/types.hpp` (new `KSlot` record — `tag`, `firstSeenZ`, one
`AccumCell`; `kBufferK = 4`), `src/core/splat.hpp`/`.cpp` (new, additive
`TileKBufferAccum`/`splatTileKBuffer()`/`sumBanksKBuffer()`, alongside —
not replacing — `TileAccum`/`splatTile()`/`sumBanks()`), `tests/
test_kbuffer_storage.cpp` (new), `CMakeLists.txt` (`test_kbuffer_storage`
target added). Two implementation choices made within ADR-059's own
already-fixed design, and one real bug the sandbox's own build/test run
caught, all recorded in DECISIONS.md ADR-060 and CORRECTIONS.md C-019: the
k-buffer storage is deliberately NOT split across `kBanks` independent
banks the way `TileAccum` is (a genuinely separate design question ADR-059
never specifies, not a free extension of the plain path's own scheme); and
a `rawWeight == 0` corner touch — routine for an exact-grid-position
fragment, harmless in the plain path — was phantom-claiming k-buffer slots
before `splatTileKBuffer()` was fixed to skip zero-weight corners, caught
by `test_kbuffer_storage.cpp`'s own first failing `ctest` run in the cloud
sandbox, not by design review beforehand.

Per Steve's own standing answer (WU-28a stays entirely inside
`scatter-core`), this unit was genuinely built and tested in this
project's cloud sandbox this session, not just reasoned through: cloned
the real `skunge2000/scatter-dve` origin, applied the new/changed files,
configured and built the `scatter-core`/`test_kbuffer_storage` targets
with the sandbox's own CMake/GCC 13.3 toolchain (Linux x86_64), and ran
the resulting binaries directly. First build surfaced the zero-weight-
corner bug above (two failing checks); fixed, rebuilt, reran. Full
portable `ctest` suite (21 targets — every `scatter-core` test that builds
without the Blackmagic SDK or Metal/Cocoa) then ran green, including every
pre-existing test this unit did not touch — no regression.
`test_kbuffer_storage` itself: 1082 checks passing.

Delivered all five files to the real repository via `SendUserFile` +
`device_commit_files`, to `/Users/stephenneal/src/scatter-dve/...` (the
real device paths from `get_device_info`'s own `connectedFolders`, not the
`device_bash` mount path). **Delivery confirmation (this session's own
device_bash checks after writing every file):** `wc -l` on each of the
five files matched this session's own sandbox copies exactly; `git status
--short` showed exactly the four modified files (`CMakeLists.txt`,
`src/core/splat.cpp`, `src/core/splat.hpp`, `src/core/types.hpp`) plus the
one new file (`tests/test_kbuffer_storage.cpp`), nothing else; `git diff
--stat` matched the sandbox's own diff stat exactly (175 insertions across
the four modified files). Additionally re-staged all five files from the
device afterward and ran a byte-for-byte `diff` against this session's own
sandbox copies directly (not just `wc -l`/`git diff --stat`) — all five
identical, no output from any comparison.

`WORK-UNITS.md`'s own WU-28a status line updated from `todo` to `wip`,
with a status paragraph recording the sandbox build/test evidence and that
Steve's own real-terminal run is still this unit's own path to `green` —
delivered via the same `SendUserFile`/`device_commit_files` path as the
source files above (not yet separately confirmed in this write-up; see
"Steve's own next steps" for the diff Steve should review, which includes
this file alongside the source changes).

## Where we are

Phase 6 (Scale up) unchanged: WU-22a/b/c all `green`. Phase 7 (Starlight)
now reads: WU-26 `todo`, WU-27 `todo`, **WU-28a `wip`** (built and tested
in the cloud sandbox this session; Steve's own real-terminal run still
needed for `green`), WU-28b `todo` (unstarted, untouched this session —
still consumes WU-28a's own per-cell occupied-slot set once that exists at
`green`), WU-29 `todo`. `DECISIONS.md` runs through ADR-060.
`CORRECTIONS.md` runs through C-019.

## Next work unit

Steve's own real-terminal close-out of WU-28a (build, run `ctest`, commit,
tag `wu-28a-green`, push) — see "Steve's own next steps" below for the
exact commands. After that lands, **WU-28b** (resolve/composite:
`core/resolve.hpp`/`.cpp`, `core/pipeline.cpp`, `tests/
test_kbuffer_resolve.cpp`; see `WORK-UNITS.md`'s own entry and
`DECISIONS.md` ADR-059 for the full design) is the natural next unit to
build — also entirely inside `scatter-core`, so also buildable/testable
directly in this project's cloud sandbox the same way this session built
WU-28a, per Steve's own standing answer. Fixing `WU-21i`'s own stale
status line (Flagged item 1) remains a small, unrelated open item worth
doing opportunistically.

## Open questions

Unchanged from Session 33: `kCaptureRingCapacity`'s value of 8 (WU-20a/20b,
ADR-046), the cold-start green-frame artifact (WU-21d), Q3 (macOS/Desktop
Video version), and Q4 (lattice edge damping, C-008(a)) all remain open,
none touched this session.

## Blocked / red

Nothing red. Nothing blocked. WU-28a is `wip` (sandbox-green, not yet
Steve's-real-terminal-green) — not blocked on anything, its own close-out
is simply Steve's next action. WU-28b remains `todo`, fully scoped, not
started.

## Environment check

Unchanged from Session 33: **UltraStudio Monitor 3G** (output,
HDMI-mirrored) and **UltraStudio Recorder 3G** (input) both last confirmed
working in Session 29's own real-hardware runs. **UltraStudio 4K Mini**
remains on hold pending a PSU replacement. `origin`
(`https://github.com/skunge2000/scatter-dve.git`) remains configured and
in sync as of this session's own opening check — no push happened from
this session's own device_bash (it never runs `git commit`/`git push`
itself); Steve's own commit/push below will be the first change to that
state since Session 33's own doc-only commit.

**New this session:** the cloud sandbox itself now has a confirmed,
working portable toolchain — CMake 3.28.3, Ninja, GCC 13.3 (also Clang
available), all preinstalled, no network-dependent setup needed. Confirmed
capable of configuring, building and running the full `scatter-core`
target and its entire portable `ctest` suite (21 targets) from a fresh
clone of the real `origin` repository. Worth remembering for any future
`scatter-core`-only unit: this is now a demonstrated, not just claimed,
capability.

## Append to DECISIONS.md

ADR-060 (WU-28a build: the decision to keep `TileKBufferAccum` unbanked,
with reasoning; the zero-weight-corner routing bug this session's own
sandbox build/test run caught and the fix; the sandbox build/test
methodology and its own real limits; sizing) — appended in full this
session; see `DECISIONS.md`. Does not reopen ADR-059 or any already-green
unit.

## Append to CORRECTIONS.md

C-019 (WU-28a's first draft of `splatTileKBuffer()` assumed a corner visit
and a real contribution were the same event — true for the plain path's
own arithmetic, not for k-buffer occupancy tracking; caught by this
session's own first `ctest` run, not design review; fixed within this
unit's own file scope) — appended in full this session; see
`CORRECTIONS.md`.

## Closed out this session

Nothing tagged `green` — WU-28a was built and tested in the cloud sandbox
this session, genuinely (not "reasoned through"), but `SESSION-PROTOCOL.md`
reserves `green`/tagging for Steve's own real-terminal close-out, which is
this unit's own next, and final, step.

## Steve's own next steps

`.git/index.lock` may need clearing first (Flagged item 4):

```
rm -f ~/src/scatter-dve/.git/index.lock
cd ~/src/scatter-dve
git status
git diff -- src/core/types.hpp src/core/splat.hpp src/core/splat.cpp CMakeLists.txt WORK-UNITS.md DECISIONS.md CORRECTIONS.md HANDOFF.md
git status --short
```

`tests/test_kbuffer_storage.cpp` is new, so `git diff` alone will not show
its content — review it directly:

```
cat ~/src/scatter-dve/tests/test_kbuffer_storage.cpp
```

If the diffs and the new file look right, build and test at your own real
terminal (this session's own cloud-sandbox run used GCC 13.3 on Linux
x86_64 — your own AppleClang/ARM64 toolchain is the one that actually
counts for `green`):

```
cd ~/src/scatter-dve
cmake --build build
ctest --output-on-failure
```

If `ctest` is green (all targets, including `test_kbuffer_storage`),
commit, tag, and push:

```
cd ~/src/scatter-dve
git add src/core/types.hpp src/core/splat.hpp src/core/splat.cpp tests/test_kbuffer_storage.cpp CMakeLists.txt WORK-UNITS.md DECISIONS.md CORRECTIONS.md HANDOFF.md
git commit -m "WU-28a: k-buffer storage, tag-keyed depth slots (ADR-059/ADR-060)"
git tag -a wu-28a-green -m "WU-28a green: k-buffer storage/accumulation"
git push origin main
git push origin --tags
```

(The manual-tag path above does not auto-push — see
`SESSION-PROTOCOL.md`'s own Session Close section — so both push commands
above are required, not just the tag.)

If `ctest` is red at your own terminal, the fastest way to get this
session's own log of what to expect is to paste the failing test's own
output back in; nothing in this session's own sandbox run suggests a
platform-specific risk (this unit's own accumulation path is integer-only,
so C-012's own cross-compiler floating-point lesson does not apply here),
but the sandbox's toolchain is evidence, not a guarantee, for yours.

Once done, `git status -sb` should read `## main...origin/main` with no
`[ahead]`/`[behind]` marker, and `git log --oneline -1` should show this
commit at `HEAD`, with `wu-28a-green` in `git tag`.
