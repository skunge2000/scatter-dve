# Session protocol

This project is built across many short sessions with an assistant that has no
memory between them and a bounded context window. The repository is the only
authoritative record. Nothing is "understood" unless it is written down here.

## The five state files

Read in this order at the start of every session. Together they are under
2000 words, which is the point.

| File | Mutability | Purpose |
|---|---|---|
| `HANDOFF.md` | Overwritten each session | Where we are, what's next, what's broken |
| `INVARIANTS.md` | Frozen | Rules no session may violate without an ADR |
| `DECISIONS.md` | Append only | Numbered ADRs. Settled questions stay settled |
| `CORRECTIONS.md` | Append only | Errors already made. Do not repeat them |
| `WORK-UNITS.md` | Edited as scope firms up | The backlog, with acceptance criteria |

`docs/architecture.md` is the full design. It is reference material, not
session-opening material — quote from it when needed, don't paste it.

## Session open

Run `./tools/context.sh [files...]` and paste the output. It emits the state
files, the current git tag, the test status, and any source files named.

`WORK-UNITS.md` lists the files each work unit touches, so the invocation is
mechanical:

```
./tools/context.sh src/video/v210.cpp src/video/v210.hpp tests/test_v210.cpp
```

## Session close

Every file the assistant creates or changes must be written into the real
repository on the Mac (`~/src/scatter-dve`) via the device bridge — never left
sitting only in the assistant's own sandbox copy. The sandbox and the Mac are
two separate filesystems; editing one does not touch the other. A fix that
exists only in the sandbox is not a fix, it's a rehearsal of one, and the
build the assistant did not run will still show the old error. After writing
each changed file back, re-stage it from the device and diff it against what
was intended before saying the write happened — don't infer success from the
write call returning without error alone.

The assistant ends every session by emitting, as a single block:

1. Files created or changed, complete — never a diff fragment or "add this
   near the top" — and confirmed written to the real repository via the
   device bridge, per the paragraph above, not just described as changed.
2. Test results expected after the change.
3. Lines to append to `DECISIONS.md`, if any decision was made.
4. Lines to append to `CORRECTIONS.md`, if anything earlier was wrong.
5. A replacement `HANDOFF.md`.

Then: commit, run the tests, tag `wu-NN-green` if green. If red, `HANDOFF.md`
records the failing test verbatim and the next session starts there.

## Work unit sizing

One session, one work unit. A work unit must:

- Touch at most 3 source files plus its test.
- Add at most ~400 lines.
- End with a runnable test that passes.
- Be independently useful — no unit leaves the tree unbuildable.

If a unit cannot meet this, split it in `WORK-UNITS.md` before starting.

## Anti-drift rules

Binding on the assistant:

1. **Never edit a file you have not been shown in the current session.** Ask
   for it. Guessing at its contents produces plausible, wrong code.
2. **Never rename or refactor across module boundaries.** Names in
   `INVARIANTS.md`, `WORK-UNITS.md` and existing headers are fixed.
3. **Never reopen an ADR.** If it looks wrong, say so and propose a superseding
   ADR — do not quietly implement the alternative.
4. **Never relax an invariant to make a test pass.** The test is right.
5. **Emit whole files.** Partial edits across a session boundary are the single
   biggest source of corrupted state.
6. **Do not rely on recall.** Anything not in the pasted context did not
   happen. Say "I don't have that file" rather than reconstructing it.
7. **Log corrections.** Discovering an earlier error is a normal outcome, not a
   failure. It goes in `CORRECTIONS.md` in the same session.
8. **Sandbox edits are not delivered until pushed.** The assistant's own
   working copy (wherever it read and edited the file) and `~/src/scatter-dve`
   on the Mac are different filesystems. Never tell Steve a bug is fixed, a
   file is updated, or he can rebuild, until the changed file has actually
   been written back into the real repository via the device bridge and
   re-read from there to confirm — not merely edited in the sandbox and
   assumed to have gone somewhere useful.

## Build configuration

Everything runs on the MacBook Pro with the UltraStudio 4K Mini attached.

Two CMake targets, no options to remember:

- **`scatter-core`** — `src/core/` and `src/video/`. Links no Blackmagic SDK.
  Most of the test suite runs against this, with no hardware connected.
- **`scatter`** — the application. Adds `src/io/`, `src/diag/`, `src/app/` and
  links DeckLink.

The determinism oracle is `--threads 1` (ADR-015). Any multi-threaded run must
produce byte-identical output to it for the same input, and the suite checks
this rather than leaving it to be verified by hand. That property is what makes
a subtle splat bug findable at all, so do not give it up.
