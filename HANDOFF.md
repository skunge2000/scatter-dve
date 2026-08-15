# Handoff
Overwritten at the end of every session. This is the first thing to read.

---

**Session:** 19
**Tag:** `wu-15a-green`, still the current recovery point — nothing new
was built this session, so no new tag.
**Phase:** 3 (SDI output) is done in full, as of session 18. This session
did one small piece of forward-looking documentation work ahead of Phase
4/5, at Steve's own request, then handed off to a new chat for WU-16.

**This was a short, decision-only session — no hardware access, no code
touched.** Steve asked to close out `DECISIONS.md` ADR-037's own third
named follow-up: naming the UltraStudio Recorder 3G explicitly, rather
than leaving future capture-side work (WU-20 onward) to describe a
generic "input device" the way `docs/architecture.md`'s own Input
subsection and `WORK-UNITS.md`'s bare WU-20 heading both still did.

**1. `DECISIONS.md` ADR-039, appended.** Names the UltraStudio Recorder 3G
as the input target for all of Phase 5 (WU-20/21/22) — the mirror of what
WU-15b (session 18) already did for the Monitor 3G by exercising it
directly. Does **not** scope WU-20 itself — no `Files:`/`Accept:` lines,
no code — only fixes which physical device that future scoping targets.
Explicitly does not edit `docs/architecture.md`: consistent with this
project's own established convention (no ADR since ADR-020 has ever
edited that document, including ADR-037 itself), `architecture.md`'s
Input subsection stays as originally written, describing the 4K Mini;
`DECISIONS.md`'s own ADRs remain what a session trusts for current
hardware truth. Genlock (ADR-037's own second follow-up) is untouched —
still deferred to whichever session actually writes `decklink_input.cpp`.

**2. `WORK-UNITS.md`'s WU-20 heading, given a short pointer note.** Names
the Recorder 3G, references ADR-039, and reminds whichever session
actually scopes WU-20 to read the real SDK's `IDeckLinkInput`/
capture-callback shape first (ADR-031/032's own established
reading-before-scoping discipline) rather than trust `architecture.md`'s
own unrevised, 4K-Mini-framed Input subsection.

**No corrections this session.** Nothing earlier was found wrong — this
session only closed a follow-up ADR-037 had already named and explicitly
deferred, which is normal, expected work, not an error to log in
`CORRECTIONS.md`.

**Tests / Build:** unchanged — no `src/`, `tests/` or `CMakeLists.txt`
file was touched this session.

## Where we are

Phase 3 (SDI output) remains done in full (WU-14/15a/15b, session 18).
Phase 4 (Threading and NEON) and Phase 5 (Live capture) are both still
`todo` — WU-20 now carries an explicit device name and a pointer to
ADR-039, but is otherwise unscoped. `DECISIONS.md` runs through ADR-039;
`CORRECTIONS.md` is unchanged since C-014 (session 18).

**Corrections this session:** none.

**Delivery mechanics:** ran remotely via the device-bridge tools
throughout, same as every session since WU-14 — touched only
`DECISIONS.md`, `WORK-UNITS.md` and this file; no `src/`, `tests/` or
`CMakeLists.txt` change. One commit this session — see `git log` for its
actual hash, made after this file was written. Working tree is clean as
of this handoff.

## Next work unit

**WU-16** (thread pool, QoS, per-worker bin arenas — Phase 4, `8-thread
output bit-identical to single-threaded`, I6) is next, per Steve's own
choice going into a new chat. Session 18's other two ADR-037 follow-ups —
the `test_decklink_device.cpp` full-duplex check that no longer describes
the real hardware, and genlock for two independent-clock devices — remain
open and unresolved, named here so whichever session eventually picks
Phase 5 back up sees them; neither blocks WU-16, which is pure `src/core/`
work with no DeckLink involvement at all.

## Open questions

Unchanged: Q1 (tile size), Q3 (macOS/Desktop Video version), Q4 (lattice
edge damping, C-008(a)). Q2 (4K Mini program outputs) remains moot per
ADR-037. ADR-037's own follow-ups #1 (full-duplex check) and #2 (genlock)
remain open, unresolved this session — only follow-up #3 (Recorder 3G
naming) was closed.

## Blocked / red

Nothing red, nothing blocked.

## Environment check

Unchanged from session 18 (ADR-037/039): **UltraStudio Monitor 3G** is
the active, confirmed output target (WU-15a/15b). **UltraStudio Recorder
3G** is in hand, now named explicitly (ADR-039) as Phase 5's own input
target, but still untouched by any code. **UltraStudio 4K Mini** remains
on hold pending a PSU replacement, not part of the active plan.

## Append to DECISIONS.md

ADR-039 was appended in full this session; see `DECISIONS.md`. Does not
reopen ADR-006, ADR-013, ADR-034 or ADR-037 — extends ADR-037's own
device-naming decision to the one place it explicitly left open.

## Append to CORRECTIONS.md

Nothing this session.

---

## What to run at your terminal

Nothing outstanding — no code changed, nothing to build or test. WU-16 is
being picked up in a new chat next; this file is the state that session
should open by reading, per `SESSION-PROTOCOL.md`'s own order.
