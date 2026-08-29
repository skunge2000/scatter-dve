# HANDOFF — Session 74

## Where we are

**Verified repo state at session start, not trusted from the incoming
prompt — this time it matched.** `HEAD` and `origin/main` both `1dd5309`,
tagged `wu-46-green`, working tree clean, `wu-35a4-green` a real annotated
tag two commits back (`00aa6a6`), no stray `.git/index.lock` at session
start. No correction needed this time (contrast Session 73's own C-039).

**Job (`WU-47`, built and `green` this session):** wired `WU-46`'s
`generateFragmentsFieldRowsTagByFacing()` (`core/binner.hpp`/`.cpp`,
already `green`) into `core/pipeline.cpp`'s `runFrameField()`/
`resolveOneParity()`, per `DECISIONS.md` ADR-090's own already-settled
design direction (not reopened, per this project's own anti-drift rule 3):

- `core/resolve.hpp`'s `runFrameField()` doc comment narrowed from "both
  `kBufferMode` must be `Off` and `weightOut` must be `nullptr`" to
  `weightOut` alone. `PipelineParams::frontTag`/`backTag`'s own doc
  comment updated too — it previously said these fields were "not
  consulted by `runFrameField()`"; checked directly, that sentence would
  now be false, so it was rewritten to describe the new gate instead of
  left stale.
- `core/pipeline.cpp`'s `resolveOneParity()` (inside `runFrameField()`)
  gained the same gated branch `ADR-089` already put on the other two
  PASS-1 call sites — `kBufferMode != Off && frontTag != backTag` selects
  `generateFragmentsFieldRowsTagByFacing()` over the plain
  `generateFragmentsFieldRows()` — and now constructs its own
  `TileKBufferAccum`/`KSlot` scratch whenever `kBufferMode != Off`
  (mirroring `runFrame()`'s own threads<=1 branch, same variable names),
  instead of always passing `nullptr` to `resolveOneTile()`. This file's
  own header comment (the paragraph that used to say the third PASS-1 call
  site was "deliberately NOT given the same branch") was stale after this
  change and has been rewritten to describe what `WU-46`/`WU-47` actually
  did, not just what `WU-35a4` left undone.
- New tests in `tests/test_kbuffer_resolve.cpp`, a new "Part F" section —
  chosen over `tests/test_field_pipeline.cpp` after reading both files in
  full: Part F reuses Part C/E's own real self-folding sphere fixture and
  `frontTag`/`backTag`/`manualTransp` plumbing directly (same file,
  nothing to duplicate across files against `SESSION-PROTOCOL.md` rule 2),
  while `test_field_pipeline.cpp`'s own two existing tests are a different
  concern (general field-mode correctness via affine warps, no k-buffer
  concepts at all) and are left untouched, unmodified, satisfying this
  unit's own "nothing regresses" Accept: criterion by construction rather
  than by a new test. Three new tests, the field-mode counterparts of
  `WU-35a4`'s own Part E(b)/(c)/(d):
  `test_kbuffer_pipeline_field_mode_default_tags_are_unaffected_by_manual_transp()`,
  `test_kbuffer_pipeline_field_mode_tag_by_facing_manual_transp_changes_real_output()`,
  `test_kbuffer_pipeline_field_mode_tag_by_facing_differs_from_single_tag_default()`.

`weightOut`'s own precondition is untouched, still `nullptr` — the
genuinely harder question ADR-090 explicitly left for a future unit, not
this one. `ADR-077`/`ADR-090` narrowed, not reopened.

## Build/test verification (this session, cloud sandbox)

Built and tested independently of `HANDOFF.md`'s own prior claims, from a
fresh checkout of `1dd5309` (Session 73's own tagged `wu-46-green`) —
baseline before any change: 28/28 `ctest` targets green, GCC 13.3.0
Release tile 2^5, confirming Session 73's own report was correct rather
than assumed. After this session's three changed files, four
configurations, all green:

- GCC 13.3.0 Release, tile 2^5 — 28/28 `ctest` targets green.
- Clang 18.1.3 Release, tile 2^5 — 28/28 green.
- GCC 13.3.0 Debug, tile 2^4 — 28/28 green.
- GCC 13.3.0 Debug+ASan+UBSan, tile 2^5 — 28/28 green; `nm -D` confirms
  genuine instrumentation linkage (28 `asan`/14 `ubsan` hits); `ldd`
  confirms `libasan.so.8`/`libubsan.so.1` actually linked.

Zero compiler warnings in any configuration. `test_kbuffer_resolve`
itself: 498499 checks passing (up from a pre-unit baseline of 436190,
rebuilt and confirmed at `1dd5309` before any change), including all
three new Part F tests — the check-count delta confirms they are real,
non-vacuous assertions, not silently skipped. `test_field_pipeline` itself
unmodified and still green (39698-checks-style file-level confirmation
not re-run for that file specifically; its own ctest pass/fail above is
the direct check). Brace/paren balance checked as a secondary sanity
check on all three changed files against their pre-edit `HEAD` baseline —
all balanced (open == close in both versions) — the build-then-test
result above is the authoritative check regardless.

All three changed files (`src/core/resolve.hpp`, `src/core/pipeline.cpp`,
`tests/test_kbuffer_resolve.cpp`) written back to the real repository via
the device bridge, then re-staged and diffed byte-for-byte against the
cloud sandbox's own working copy — identical. As a further check beyond
the diff alone, the real repository's own just-written copy was
independently re-tarred, re-staged, and rebuilt from scratch in the cloud
sandbox (GCC Release): 28/28 green again. `WORK-UNITS.md`'s own `WU-47`
entry updated in place (title tag `todo` → `green`, `Files:`/`Accept:`
sections de-provisionalized now the test-file choice is made and
confirmed, new `Status:` paragraph) via a Python read-modify-write script
run through the device-bridge shell, each replacement asserted unique
before being applied — no cmake/ninja needed for a doc-only edit, so this
one did not need the cloud-sandbox round trip the three source/test files
did.

Not yet built, run, tagged or pushed at Steve's own real terminal.

## What's next (Steve's own to run)

1. Review the diff — `git status --short` should show exactly four
   modified files (`src/core/resolve.hpp`, `src/core/pipeline.cpp`,
   `tests/test_kbuffer_resolve.cpp`, `WORK-UNITS.md`), nothing else:
   ```
   cd ~/src/scatter-dve
   git status --short
   git diff -- src/core/resolve.hpp src/core/pipeline.cpp tests/test_kbuffer_resolve.cpp WORK-UNITS.md
   ```
2. Commit, then let `tools/close.sh` build, test, tag, and push in one
   step — it refuses on a dirty tree, so the commit must happen first
   (`C-038`: this block is never handed over with the tag command alone,
   commit shown explicitly immediately before):
   ```
   git add src/core/resolve.hpp src/core/pipeline.cpp tests/test_kbuffer_resolve.cpp WORK-UNITS.md
   git commit -m "WU-47: wire generateFragmentsFieldRowsTagByFacing() (WU-46) into runFrameField()/resolveOneParity(), relaxing its kBufferMode precondition -- DECISIONS.md ADR-090"
   ./tools/close.sh 47
   ```
   `close.sh` builds (default Release config, whatever `BLACKMAGIC_SDK_DIR`
   is already cached in `build/` from earlier sessions — this unit is
   core-only and does not depend on it either way), runs the full `ctest`
   suite, tags `wu-47-green` only if everything passes, and pushes
   `HEAD --tags` to `origin` automatically on success. If it reports
   `WARNING: push failed; commit is local only.` (this session's own
   device-bridge shell cannot push — no stored credentials there — but
   `close.sh` runs at your own real terminal, where `git push` has worked
   in every prior session), push by hand:
   ```
   git push origin main
   git push origin --tags
   ```
3. This session's own cloud-sandbox verification covers `scatter-core`
   only (GCC/Clang, Release/Debug, ASan+UBSan) — the parts that sandbox
   cannot reach (`test_decklink_live_sphere`, `test_decklink_device`) still
   need a real build/run at your own terminal, same limitation every
   session has named; this unit does not touch anything DeckLink-linked,
   so no new real-hardware confirmation is expected to be needed for it
   specifically.
4. A stale `.git/index.lock` (0 bytes) was found and deleted this session
   (delete permission requested and granted for this folder) — the same
   recurring device-bridge quirk `SESSION-PROTOCOL.md`'s own opening
   checklist now names explicitly. If a future session's own `git`
   commands mysteriously refuse to run, check there first before assuming
   anything worse.

## What's broken / flagged, not fixed (out of this session's own scope)

**`docs/wu-audit-2026-08.md` line 113 is still stale — re-checked directly
this session, not fixed, same standing instruction Sessions 72/73 already
followed.** It still reads: "The real-content gap (single tag per call)
was already found and logged as C-020/ADR-062 before this sweep, and
closed by WU-28c. Nothing new." Still incorrect: `WU-28c` only ever built
the TagByFacing functions; the whole-frame/row-range gap was not actually
closed until `WU-35a4` wired them into `core/pipeline.cpp`, and field
mode's own equivalent gap was not closed until this session's own `WU-47`
(building on `WU-46`). Left unfixed — touching an audit doc is outside
this session's own scope (`core/resolve.hpp`, `core/pipeline.cpp`,
`tests/test_kbuffer_resolve.cpp`, `WORK-UNITS.md`), same restraint the
prior two sessions already applied.

## Untouched, deliberately

`INVARIANTS.md` (not touched, not read for a change — no invariant this
session's own work bears on). `ADR-059/062/065/077/085/086/087/088/089/090`
(not reopened; this session's own work narrows one clause of `ADR-077`'s
own precondition exactly the way `ADR-090` already said it could,
mechanically, not a new decision). `CORRECTIONS.md`/`DECISIONS.md` (no new
entry — nothing wrong was found this session, and the design decision
`WU-47` implements was already made and recorded, at Session 73, as
`ADR-090`; nothing to append). `HANDOFF.md`'s own prior sessions'
entries and `WU-35a1`/`WU-35a4`/`WU-46`'s own `WORK-UNITS.md` entries
(left exactly as written). `src/core/binner.hpp`/`.cpp` (read, not
edited — `WU-46`'s own file, already `green`, this unit only calls what
it built). `docs/wu-audit-2026-08.md` (flagged above, not fixed).
