# Handoff
Overwritten at the end of every session. This is the first thing to read.

---

**Session:** 35 (WU-28b — k-buffer resolve, built and tested, per Session
33's own scoping and Session 34's WU-28a. Per Steve's own standing answer,
WU-28b stays entirely inside `scatter-core`, so this session actually
built and ran it directly in this project's cloud sandbox, the same way
Session 34 did for WU-28a.)
**Tag:** none this session. WU-28b is `wip`, not `green` — it was built
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
session (out of scope for WU-28b), now seven sessions old.** Unchanged
since Session 34's own note. Worth doing directly whenever a session next
touches `WORK-UNITS.md` for an unrelated reason and is prepared to ask
Steve the deferred by-eye acceptance question (which letter keys were
tried, whether `Q` exits cleanly) rather than answer it on his behalf.

**2. WU-28a's own status line in `WORK-UNITS.md` was found stale this
session — read `wip` despite `wu-28a-green` already existing in the real
repository — and corrected, doc-only, before WU-28b's own work started.**
Session 34's own close left the status paragraph text at `wip` because it
was written before Steve's real-terminal close-out happened; that
close-out (commit `5ba1086`, tag `wu-28a-green`, `origin/main` in sync)
landed afterward without a further doc edit. This session verified the
real state directly (`git tag`/`git log --oneline -10`/`git status -sb`)
before trusting either the old `HANDOFF.md` text or the old `WORK-UNITS.md`
text, per the same anti-drift discipline every session follows, then fixed
the status line since this session was already touching the file. Did not
touch any of WU-28a's own source files (`core/types.hpp`, `core/splat.hpp`/
`.cpp`, `tests/test_kbuffer_storage.cpp`) — read only, per this session's
own standing scope.

**3. WU-28b is `wip`, not `green` — Steve's own real-terminal build/run/
commit/tag/push is this unit's own remaining step.** See "Steve's own next
steps" below for the exact commands. The cloud sandbox's own build/test
run (`DECISIONS.md` ADR-061) is real compiler/test evidence — GCC 13.3 and
Clang 18.1 on Linux x86_64, Release and Debug, tile 2^4 and 2^5, plus
ASan+UBSan and TSan, all 22 portable `ctest` targets green in every
configuration, no sanitizer reports — but is evidence, not a substitute:
the sandbox's toolchain does not match Steve's own (AppleClang, ARM64),
and the sandbox has no git identity to commit/tag/push with regardless.

**4. Sizing ran well over `SESSION-PROTOCOL.md`'s own "~400 lines" figure
— 576 total after one trimming pass from an initial 638 (243 across
`src/core/resolve.hpp`/`.cpp`, `src/core/pipeline.cpp`, `CMakeLists.txt`;
333 in the new `tests/test_kbuffer_resolve.cpp`).** Trimmed comment
density only — no test case, no Opaque/Blend coverage, no design-rationale
content cut to make the number smaller, and the touched-file set is
already exactly `WORK-UNITS.md`'s own `Files:` list for this unit, the
minimum ADR-059's own PASS-2/resolve split leaves for one coherent
"resolve/composite" unit. Flagged plainly, per this session's own
instruction to stop and report rather than force a further split or cut
coverage. See `DECISIONS.md` ADR-061 for the full breakdown.

**5. A real, but minor, compiler issue this session's own sandbox build
caught: GCC 13's `-Werror=array-bounds` misfired on a `std::sort` call
over a 4-element `std::array` with a runtime sub-range.** Not a design or
reasoning error — Clang 18 raised nothing on the same code, and the
`std::sort` call itself was correct — so no `CORRECTIONS.md` entry; see
`DECISIONS.md` ADR-061 for the full explanation. Fixed by replacing the
call with a hand-written insertion sort, documented in `resolve.cpp`'s own
comment as a known compiler quirk at this array size. Worth a future
session remembering if another small fixed-capacity `std::array` sort ever
shows up on this same GCC line.

**6. One untracked `Testing/` directory found at the repository root
during this session's own opening verification
(`Testing/Temporary/CTestCostData.txt`, `Testing/Temporary/LastTest.log`)
— not part of the tracked tree, did not block the clean-tree check, not
removed (`device_bash` cannot delete files).** Almost certainly `ctest`
was run from the repository root instead of `build/` at some point —
harmless, but **Steve: run `rm -rf ~/src/scatter-dve/Testing` at your own
terminal** whenever convenient; not urgent, not blocking anything below.

**7. `.git/index.lock` pattern — not encountered this session (no local
commits attempted; this session only wrote files via `SendUserFile` +
`device_commit_files`).** Still the same known, non-blocking behavior
documented across Sessions 29-34: `device_bash` git commands that only
read (`status`, `log`, `diff`, `show`, `tag`) succeed and print correct
output even with a stale lock file present; `device_bash` itself can never
remove it. **Steve: run `rm -f ~/src/scatter-dve/.git/index.lock` before
your own `git add`/`git commit` below** — routine, not a one-off fix.

**8. `device_stage_files`/`device_commit_files` HTTP 403 `untrusted_device`
— not encountered this session.** If it recurs: it means the Mac slept
mid-session and invalidated the device's trusted-sign-in state, not an
account problem — ask Steve to re-enable access in the Claude desktop app
(no fresh sign-in needed), then retry. Noted only so a future session does
not have to re-diagnose it from scratch again.

## This session in full

Session opened by requesting device-bridge access to `~/src/scatter-dve`
only (the Blackmagic SDK folder was not requested — WU-28b touches only
`src/core/`, nothing DeckLink- or Metal/Cocoa-related, per ADR-059's own
scoping). Read `SESSION-PROTOCOL.md`, `HANDOFF.md`, `WORK-UNITS.md`,
`DECISIONS.md`, `CORRECTIONS.md` and `INVARIANTS.md` in full, paying
specific attention to `HANDOFF.md`'s own Session-34 account, `WORK-UNITS.md`'s
WU-28b entry, and `DECISIONS.md` ADR-059/060 — then verified real
repository state directly (`git tag`, `git log --oneline -10`, `git status
--short`, `git status -sb`) before trusting any of it, per this project's
own standing anti-drift rule and this session's own explicit instruction
not to build WU-28b against an unconfirmed WU-28a: `wu-28a-green` present
in `git tag`; `HEAD` at `5ba1086` ("WU-28a: k-buffer storage..."), the same
commit; `git status -sb` reading `## main...origin/main`, no ahead/behind
marker. Confirmed, not assumed — WU-28a really is real-terminal `green`,
even though `HANDOFF.md`'s and `WORK-UNITS.md`'s own text still said
otherwise (Flagged items 2 above). `core/types.hpp`, `core/splat.hpp`/
`.cpp`, `core/resolve.hpp`/`.cpp`, `core/pipeline.cpp` then re-read in full
against ADR-059/060's own design before writing anything — nothing had
drifted since Session 34.

Built WU-28b exactly per `WORK-UNITS.md`'s own `Files:`/`Accept:` entry:
`src/core/resolve.hpp`/`.cpp` (new `KBufferResolveMode` enum and
`compositeKBuffer()`, alongside — not replacing — `composite()`/
`compositeLayered()`; new, additive `PipelineParams::kBufferMode` field,
default `Off`, zero-cost-when-absent, the same shape `pool`/`weightOut`
already established), `src/core/pipeline.cpp` (`resolveOneTile()` wired to
the new k-buffer resolve path in both the `threads<=1` oracle branch and
the threaded PASS-2 path, explicitly not writing `weightOut` along that
path), `tests/test_kbuffer_resolve.cpp` (new), `CMakeLists.txt`
(`test_kbuffer_resolve` target added). Two design questions ADR-059 left
open, decided and recorded in `DECISIONS.md` ADR-061: one `KBufferResolveMode`
field rather than two separate opacity/blend fields; Blend mode as a
direct generalization of `compositeLayered()`'s own two-layer
read-replace-write mechanism to up to `kBufferK` occupied slots, sorted by
`firstSeenZ` with a smallest-tag tie-break, cross-checked directly against
`compositeLayered()` for the two-slot case. A real but minor compiler issue
(GCC 13's `-Warray-bounds` false positive on a small fixed-capacity
`std::array` sort) caught by the sandbox's own build, fixed with a
hand-written insertion sort — see ADR-061 and Flagged item 5; not a
`CORRECTIONS.md`-worthy design error.

Per Steve's own standing answer (WU-28b stays entirely inside
`scatter-core`), this unit was genuinely built and tested in this
project's cloud sandbox this session, not just reasoned through: cloned
the real `skunge2000/scatter-dve` origin fresh (not a reused sandbox from
any prior session), confirmed it matched `wu-28a-green`/`5ba1086` before
any file was touched, applied the new/changed files, and configured/built
across a wider matrix than WU-28a's own single-configuration precedent,
given the new arithmetic and the new concurrent code path at stake: GCC
13.3 and Clang 18.1, Release and Debug, `SCATTER_TILE_LOG2` 4 and 5, plus a
Debug ASan+UBSan build, plus a Release TSan build. All 22 portable `ctest`
targets passed clean in every one of the eight configurations, including
every pre-existing test this unit did not touch (no regression), with no
sanitizer report of any kind in any log. Unlike WU-28a's own test, this
unit's accept criterion required exercising real multi-threading, not
fragment-order permutation:
`test_kbuffer_pipeline_threads_1_matches_threads_8()` runs the full
pipeline (`runFrame()` end to end, `Blend` mode) for a real WU-21g/h
folding-sphere frame at `--threads 1` and compares byte-for-byte,
per-pixel per-channel, against `--threads {2, 3, 8}` — genuinely
satisfied, not simulated.

Delivered all eight changed/new files to the real repository via
`SendUserFile` + `device_commit_files`, to `/Users/stephenneal/src/scatter-dve/...`
(the real device paths from `get_device_info`'s own `connectedFolders`,
not the `device_bash` mount path): `src/core/resolve.hpp`,
`src/core/resolve.cpp`, `src/core/pipeline.cpp`, `CMakeLists.txt`,
`tests/test_kbuffer_resolve.cpp` (new), `WORK-UNITS.md`, `DECISIONS.md`,
this file. **Delivery confirmation (this session's own device_bash checks
after writing every file):** `wc -l` on each of the eight files matched
this session's own sandbox copies exactly; `git status --short` showed
exactly the five modified files (`CMakeLists.txt`, `src/core/resolve.cpp`,
`src/core/resolve.hpp`, `src/core/pipeline.cpp`, plus `WORK-UNITS.md` and
`DECISIONS.md` for the doc updates) plus the one new file
(`tests/test_kbuffer_resolve.cpp`), nothing else unexpected; `git diff
--stat` matched the sandbox's own diff stat exactly for every tracked
file; byte-for-byte `diff` against this session's own sandbox copies,
re-staged from the device afterward, showed no differences on any of the
eight files.

## Where we are

Phase 6 (Scale up) unchanged: WU-22a/b/c all `green`. Phase 7 (Starlight)
now reads: WU-26 `todo`, WU-27 `todo`, **WU-28a `green`** (status line
corrected this session to match confirmed real-terminal state), **WU-28b
`wip`** (built and tested in the cloud sandbox this session; Steve's own
real-terminal run still needed for `green`), WU-29 `todo`. `DECISIONS.md`
runs through ADR-061. `CORRECTIONS.md` unchanged this session, still runs
through C-019.

## Next work unit

Steve's own real-terminal close-out of WU-28b (build, run `ctest`, commit,
tag `wu-28b-green`, push) — see "Steve's own next steps" below for the
exact commands. After that lands, both WU-28 sub-units are fully closed
and Phase 7 continues with **WU-26** (normals from lattice) or **WU-27**
(Blinn-Phong shading), both currently unscoped `todo` entries — a future
session's own first job there is real `Files:`/`Accept:` scoping, same
discipline as every other unit. Fixing `WU-21i`'s own stale status line
(Flagged item 1) remains a small, unrelated open item worth doing
opportunistically.

## Open questions

Unchanged from Session 34: `kCaptureRingCapacity`'s value of 8 (WU-20a/20b,
ADR-046), the cold-start green-frame artifact (WU-21d), Q3 (macOS/Desktop
Video version), and Q4 (lattice edge damping, C-008(a)) all remain open,
none touched this session.

## Blocked / red

Nothing red. Nothing blocked. WU-28b is `wip` (sandbox-green, not yet
Steve's-real-terminal-green) — not blocked on anything, its own close-out
is simply Steve's next action.

## Environment check

Unchanged from Session 34: **UltraStudio Monitor 3G** (output,
HDMI-mirrored) and **UltraStudio Recorder 3G** (input) both last confirmed
working in Session 29's own real-hardware runs. **UltraStudio 4K Mini**
remains on hold pending a PSU replacement. `origin`
(`https://github.com/skunge2000/scatter-dve.git`) remains configured and
in sync as of this session's own opening check — no push happened from
this session's own device_bash (it never runs `git commit`/`git push`
itself); Steve's own commit/push below will be the first change to that
state since his own WU-28a close-out.

**Reconfirmed this session:** the cloud sandbox's portable toolchain — now
demonstrated across GCC 13.3, Clang 18.1, CMake 3.28.3 and Ninja, plus
ASan/UBSan/TSan sanitizer builds — remains a genuine, repeatable capability
for any `scatter-core`-only unit, not a one-off from Session 34.

## Append to DECISIONS.md

ADR-061 (WU-28b build: the `KBufferResolveMode` field design and the
Blend-mode formula as a generalization of `compositeLayered()`; the GCC
13 `-Warray-bounds` false positive and its fix; the eight-configuration
sandbox build/test methodology; sizing) — appended in full this session;
see `DECISIONS.md`. Does not reopen ADR-059, ADR-060, or any already-green
unit.

## Append to CORRECTIONS.md

Nothing this session. The GCC `-Warray-bounds` issue (Flagged item 5,
`DECISIONS.md` ADR-061) is a toolchain quirk caught and fixed within this
unit's own build, not a design or reasoning error that misled a decision —
judged not to meet `CORRECTIONS.md`'s own bar ("errors already made during
design"), unlike WU-28a's own C-019.

## Closed out this session

Nothing tagged `green` — WU-28b was built and tested in the cloud sandbox
this session, genuinely (eight compiler/build-type/tile-size/sanitizer
configurations, not just "it compiled once"), but `SESSION-PROTOCOL.md`
reserves `green`/tagging for Steve's own real-terminal close-out, which is
this unit's own next, and final, step.

## Steve's own next steps

`.git/index.lock` may need clearing first (Flagged item 7), and there's a
harmless stray `Testing/` directory worth clearing too (Flagged item 6):

```
rm -f ~/src/scatter-dve/.git/index.lock
rm -rf ~/src/scatter-dve/Testing
cd ~/src/scatter-dve
git status
git diff -- src/core/resolve.hpp src/core/resolve.cpp src/core/pipeline.cpp CMakeLists.txt WORK-UNITS.md DECISIONS.md HANDOFF.md
git status --short
```

`tests/test_kbuffer_resolve.cpp` is new, so `git diff` alone will not show
its content — review it directly:

```
cat ~/src/scatter-dve/tests/test_kbuffer_resolve.cpp
```

If the diffs and the new file look right, build and test at your own real
terminal from the `build/` directory specifically (this session's own
cloud-sandbox run used GCC 13.3/Clang 18.1 on Linux x86_64 — your own
AppleClang/ARM64 toolchain is the one that actually counts for `green`):

```
cd ~/src/scatter-dve
cmake --build build
cd build
ctest --output-on-failure
```

If that's green, stage and commit everything (this project's own
`tools/close.sh` refuses to tag on a dirty tree, so commit first):

```
cd ~/src/scatter-dve
git add src/core/resolve.hpp src/core/resolve.cpp src/core/pipeline.cpp tests/test_kbuffer_resolve.cpp CMakeLists.txt WORK-UNITS.md DECISIONS.md HANDOFF.md
git commit -m "WU-28b: k-buffer resolve, opaque/blend composite (ADR-059/ADR-061)"
```

Then close the unit — `tools/close.sh` reconfigures, rebuilds, reruns
`ctest` itself from `build/`, tags `wu-28b-green` on success, and pushes
both the commit and the tag automatically:

```
cd ~/src/scatter-dve
./tools/close.sh 28b
```

If `close.sh` prints `WU-28b closed green.`, the push already happened —
nothing further needed. If it instead prints a `WARNING: push failed`
line, run the two pushes explicitly:

```
git push origin main
git push origin --tags
```

(This is the same manual-push fallback `SESSION-PROTOCOL.md` calls for
whenever a push doesn't happen automatically — required, not optional,
whenever that warning appears.)

If `ctest` is red at your own terminal (either the manual run above or
inside `close.sh`), the fastest way to get this session's own log of what
to expect is to paste the failing test's own output back in; nothing in
this session's own eight-configuration sandbox run (including
ThreadSanitizer on the new threaded code path) suggests a platform-specific
risk, but the sandbox's toolchain is evidence, not a guarantee, for yours.

Once done, `git status -sb` should read `## main...origin/main` with no
`[ahead]`/`[behind]` marker, and `git log --oneline -1` should show this
commit at `HEAD`, with `wu-28b-green` in `git tag`.
