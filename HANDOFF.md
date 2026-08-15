# Handoff
Overwritten at the end of every session. This is the first thing to read.

---

**Session:** 16
**Tag:** `wu-15a-green`. Steve ran `./tools/close.sh 15a`: 15/16, the single
already-understood `test_decklink_device` duplex exception ADR-035
predicted (Monitor 3G is playback-only) — `close.sh` correctly refused to
tag on its own, since it has no way to know that specific failure is an
accepted exception. Steve tagged `wu-15a-green` by hand, accepting that
exception himself. WU-15a is `green`.
**Phase:** 3 — SDI output. WU-14 (device enumeration, `ComPtr`) is `green`.
WU-15a (scheduled playback of one looped, file-sourced, warped frame) is
`green` (`wu-15a-green`), confirmed working end to end against real
hardware.

**This was a long session: a real hardware incident, a real (if ultimately
false-alarm) investigation, and a genuine end-of-session hardware-target
decision.** Summary in chronological order; `DECISIONS.md` ADR-032 through
ADR-037 and `CORRECTIONS.md` C-013 are the frozen record — this section is
the narrative, not a replacement for reading them.

**1. Research, then WU-15a implementation (as usual).** Read
`HANDOFF.md`/`INVARIANTS.md`/`DECISIONS.md`/`CORRECTIONS.md`/
`WORK-UNITS.md` in order, re-read architecture.md 7/10, then the real
Blackmagic DeckLink SDK 16.0 headers and its own `FilePlayback`/
`SignalGenerator` samples, before writing `src/io/decklink_output.hpp`/
`.cpp` and `tests/test_decklink_output.cpp` — the WU-15a/WU-15b split and
every design choice are ADR-032. Written via the device bridge, unbuilt
(no SDK, no AppleClang in the cloud sandbox).

**2. First real-terminal run, on the UltraStudio 4K Mini: one bug, found
and fixed cleanly.** `bmdModePALp` ("576p25") turned out unsupported —
`DoesSupportVideoMode` returned `S_OK`/`supported=false`, exactly the named
risk ADR-032 had already flagged, with `bmdModePAL` as its own documented
fallback. Switched, rebuilt, and `test_decklink_output` passed both checks
clean (5.34s) — this project's first real signal ever put out over SDI.
Recorded as ADR-033. Steve then corrected my own stated *reason* for the
`bmdModePALp` failure (I'd framed it as 4K-Mini-specific; it's actually
that 576p25 isn't a real deployed broadcast signal at all) — logged as
`CORRECTIONS.md` C-013, which doesn't change the `bmdModePAL` decision
itself, only the reasoning given for it.

**3. Hardware incident: the UltraStudio 4K Mini went unresponsive.**
Immediately after that passing run, the full suite failed on empty device
enumeration — the 4K Mini had dropped off Desktop Video, Media Express, and
Apple's own Thunderbolt System Report entirely, with no Thunderbolt power
passthrough to the Mac. Steve isolated the fault to the unit itself via a
cable/port swap against the spare UltraStudio Monitor 3G (which worked
immediately on the same port/cable). My own assessment, recorded in full in
ADR-034: unlikely to be code-caused (this project's code only calls the
DeckLink SDK's public API, which has no reach into firmware, Thunderbolt
enumeration, or power delivery), but not provable with certainty from
software evidence alone. **Decision: pause 4K Mini work; the UltraStudio
Monitor 3G becomes the active hardware target** for WU-15a's remaining
verification and for WU-15b, until the 4K Mini's status is resolved
(Blackmagic support / a hardware check — Steve's own next steps, outside
this project). Does not supersede ADR-006/013/011. No code change was
needed for the pivot itself — device selection was already generic
(`supportsPlayback`, not model-name-specific).

**4. Monitor 3G run: mechanically clean, one expected/explained
exception.** Full suite: 15/16. `test_decklink_output` passed again
(5.13s), confirming `bmdModePAL` also works on the Monitor 3G.
`test_decklink_device`'s own full-duplex check failed — expected, since the
Monitor 3G is playback-only by design; recorded as ADR-035, which does not
reopen WU-14 (`wu-14-green` stands; that tag recorded a true result against
the 4K Mini at the time).

**5. The warp-visibility investigation — the bulk of this session's own
effort, and ultimately a false alarm.** Steve's first by-eye check reported
a plain, undistorted zone plate, not the cylinder warp the test is supposed
to produce. Chased in stages, each a real check rather than a guess: (a)
the pipeline (`makeZonePlate` -> `buildCylinderLattice` -> `runFrameFile`)
reproduced standalone off-hardware (Linux, g++, no SDK) — clearly warped;
(b) the *actual* file on Steve's own Mac, converted with a throwaway tool
built with his own AppleClang — also clearly warped; (c) temporary checksum
instrumentation added to `decklink_output.cpp`, checking the frame content
at every handoff point including a readback through the SDK's own
`ScheduledFrameCompleted()` callback — identical checksum at every single
stage, and byte-for-byte identical to the independent Linux reproduction
(`0x5fac3fb42c5f7d48`, both places); (d) at Steve's request, swapped to a
sphere warp as a further diagnostic — also confirmed correctly warped, both
off-hardware and on the real Monitor 3G run. **Root cause, found by Steve:**
a 720x576, 4:3-ish SD frame on his 16:9 monitor gets stretched back out
horizontally by the display's own aspect handling, which was making the
cylinder warp's own real, baked-in horizontal compression look deceptively
close to un-warped at a glance. Not a code defect anywhere — nothing this
project claimed was wrong, so no `CORRECTIONS.md` entry; recorded instead
as ADR-036. All diagnostic instrumentation and the sphere swap were
reverted; `decklink_output.cpp`/`.hpp` and `tests/test_decklink_output.cpp`
are back to exactly ADR-032's own cylinder design, with one durable
addition — a comment in the test warning against this exact false-alarm
mode recurring.

**6. Clean rebuild + full suite on the reverted build: confirmed good by
Steve.** WU-15a's `Accept:` criteria — zero dropped/late frames and a clean
stop (established in step 2, unaffected by the investigation), plus the
by-eye warp confirmation (established in step 5) — are both satisfied.
`./tools/close.sh 15a`: 15/16, the same ADR-035 exception, refused to tag
on its own (correct behaviour for the script). Steve tagged `wu-15a-green`
by hand, accepting that exception himself. WU-15a is `green`.

**7. End-of-session hardware decision: the Monitor 3G/Recorder 3G split is
the real going-forward target, not a stopgap.** Steve stated directly that
output will use the UltraStudio Monitor 3G and input will use a separate
UltraStudio Recorder 3G — both already in his hands, not a future plan.
The 4K Mini is on hold pending a PSU replacement (Steve's own diagnosis,
consistent with ADR-034's "unlikely code-caused" assessment) — not
retired, but not the active plan either way. Recorded as ADR-037, which
supersedes ADR-006's specific device choice and ADR-011's "spare" framing
of the Monitor 3G, does not reopen ADR-013 (still one machine, just two
DeckLink devices instead of one), and names three concrete follow-ups for
a future session (WU-14's full-duplex check no longer describes the real
hardware; genlock matters more with two independent-clock devices; future
capture work should target the Recorder 3G by name) without resolving any
of them now.

**Tests:** `test_decklink_output` has passed clean, full-suite, against
both the 4K Mini and the Monitor 3G. `test_decklink_device` passes against
the 4K Mini; against the Monitor 3G alone it fails one check
(`test_at_least_one_device_is_full_duplex`) for the reason ADR-035 records
— expected, not a regression, does not block WU-15a. All fourteen
non-hardware-dependent tests (WU-01 through WU-13) are untouched by
anything this session did.

**Build:** succeeds. No Blackmagic SDK/AppleClang in the cloud sandbox, so
this session's implementation and every fix were written via the device
bridge and built/tested at Steve's own real terminal throughout, per this
project's established pattern since WU-14.

## Where we are

`src/io/decklink_output.hpp`/`.cpp` — `LoopedFramePlayback`, confirmed
working on real hardware (both the 4K Mini and the Monitor 3G). Exactly
ADR-032's design; no diagnostic scaffolding remains.
`tests/test_decklink_output.cpp` — the two checks, `kDisplayMode =
bmdModePAL` (ADR-033), cylinder warp (ADR-032, reaffirmed by ADR-036), now
with a comment warning about the 4:3-on-16:9 false-alarm mode. `CMakeLists.
txt` unchanged since its own commit in step 1. See `DECISIONS.md` ADR-032
through ADR-037, `CORRECTIONS.md` C-013, and `WORK-UNITS.md`'s WU-15a/
WU-15b entries for the full record.

**Corrections this session:** C-013 only (the `bmdModePALp` failure's
stated *reason*, not the `bmdModePAL` decision itself). The hardware
incident (ADR-034), the warp-visibility investigation (ADR-036), and the
end-of-session hardware-target decision (ADR-037) all closed without any
earlier project claim being shown wrong, so none gets a `CORRECTIONS.md`
entry — see each ADR's own closing note for why.

**Delivery mechanics:** this session ran remotely via the device-bridge
tools throughout. Commits this session: `c5605f4` (WU-15a implementation),
`5fd4b4f` (`bmdModePAL` fix, ADR-033/034, C-013, the 4K Mini incident and
Monitor 3G pivot), `8b840e9` (ADR-035, Monitor 3G run results), `0138c54`
(warp investigation opened, off-hardware evidence logged), `ef45a35`
(ADR-036, investigation closed, diagnostics/sphere-swap reverted),
`7410b30` (full-suite confirmation, HANDOFF refresh), plus one further
commit for this update (ADR-037, the `wu-15a-green` tag record, and the
Monitor 3G/Recorder 3G hardware decision) — see `git log` for its actual
hash, made after this file was written. Working tree is clean as of this
handoff. The bridge's own `unlink`-can't-work-on-
this-mount limitation continued to leave stale `index.lock`/`HEAD.lock`/
temp-object files after nearly every commit this session; each was moved
into `_to_delete/` rather than removed, per the established convention —
`_to_delete/` has accumulated a lot this session (including throwaway
diagnostic tools and PGM/PNG dumps used mid-investigation); safe to `rm -rf
_to_delete/` by hand whenever convenient.

## Next work unit

WU-15a is `green` (`wu-15a-green`). Next is Steve's own call: WU-15b (the
one-hour unattended endurance run — no new code, Steve's own hands-on step,
`DECISIONS.md` ADR-032/`WORK-UNITS.md`'s own WU-15b entry; runs against the
Monitor 3G, which is now the project's actual going-forward output device,
not a stopgap — ADR-037) or WU-16 (thread pool, QoS, per-worker bin arenas
— Phase 4), if WU-15b is deferred to run unattended in the background of a
later session rather than blocking the next one.

**Before either, worth a look:** `DECISIONS.md` ADR-037 (written this
session, from Steve's own end-of-session hardware decision) names three
concrete follow-ups that aren't resolved yet and aren't this session's own
job either — read ADR-037's own closing list before picking whichever of
WU-15b/WU-16 comes next, in case one of them is now relevant sooner than
expected:

1. `test_decklink_device.cpp`'s full-duplex check (WU-14) checks a fact
   that will never be true of the real going-forward hardware (two
   separate devices, not one full-duplex unit) — worth deciding whether to
   retire or rescope it, not just keep accepting the same `close.sh`
   exception forever.
2. Genlock (ADR-010) was reasoned about for one device sharing one
   internal clock between input and output; two independent devices (the
   Recorder 3G and Monitor 3G) share no such clock. Worth revisiting once
   the Recorder 3G is actually touched by this project's own code.
3. Future capture-side work should target the Recorder 3G by name, not a
   generic "input device," once scoped with its own `Files:`/`Accept:`.

## Open questions

Unchanged from session 15, none touched this session: Q1 (tile size), Q3
(macOS/Desktop Video version), Q4 (lattice edge damping, C-008(a)).

**Q2 (4K Mini program outputs) is now moot for this project's own
purposes** — the 4K Mini is on hold pending a PSU replacement (ADR-037) and
is no longer the going-forward output device regardless of whether it
recovers; leaving Q2 recorded here for history, not as something blocking
anything.

Resolved this session: `bmdModePALp` unsupported on the 4K Mini, `bmdModePAL`
confirmed working on *both* the 4K Mini and the Monitor 3G (ADR-033,
extended by the Monitor 3G run); `RowBytesForPixelFormat(bmdFormat10BitYUV,
...)` matches `v210::rowBytesMin()` on both devices too. Whether the
warp is actually visible on real hardware — yes, confirmed (ADR-036), after
a real but ultimately false-alarm investigation into a display-side aspect
issue, not a code defect. What caused the 4K Mini's hardware incident —
most likely a failed PSU (Steve's own diagnosis, folded into ADR-037),
consistent with ADR-034's own "unlikely code-caused" assessment.

New this session, still open: the three ADR-037 follow-ups above. Whether
the 4K Mini ever gets its PSU replaced and rejoins the project in some
role is Steve's own call, not tracked as a blocking question here.

## Blocked / red

Nothing red, nothing open. `./tools/close.sh 15a` reported 15/16 (the
already-understood `test_decklink_device` duplex exception, ADR-035) and
refused to tag on its own — correct behaviour for the script, not a sign
anything is wrong. Steve tagged `wu-15a-green` by hand, accepting that
exception himself. Worth keeping in mind for future work units closed
while only the Monitor 3G is attached: `close.sh` will keep refusing to
tag on this same exception every time, and someone will need to tag by
hand each time (or `close.sh` could be taught a documented exception list
— not done, not scoped, just worth naming as a real option if this gets
tedious).

## Environment check

**Changed again at the very end of this session (ADR-037).** Going-forward
target hardware is now a two-device split, both already in Steve's hands:
**UltraStudio Monitor 3G for output** (confirmed working this session,
`bmdModePAL` + `bmdFormat10BitYUV`, playback only — no capture input, which
is fine for its own role) and **UltraStudio Recorder 3G for input** (not
yet touched by any of this project's own code — first contact with it is a
future session's own job). UltraStudio 4K Mini: still unresponsive (Desktop
Video, Media Express, Thunderbolt System Report all show it absent; no
power passthrough), isolated to the unit itself via cable/port swap
(ADR-034), and now diagnosed by Steve as most likely a failed PSU — on hold
pending a replacement, not part of the active hardware plan regardless of
whether it recovers (ADR-037).

## Append to DECISIONS.md

ADR-033, ADR-034, ADR-035, ADR-036, and ADR-037 were all appended in full this
session; see `DECISIONS.md`. None reopens an earlier entry — each records
either a new decision or an investigation's own closure; see each entry's
own closing note for its precise relationship to what came before.

## Append to CORRECTIONS.md

C-013 was appended in full this session; see `CORRECTIONS.md`. Nothing
further — see "Corrections this session" above for why the hardware
incident, the warp investigation, and the hardware-target decision all
closed without one.

---

## What to run at your terminal

Nothing outstanding from this session — WU-15a is tagged `wu-15a-green`
and the working tree is clean. Whenever you're ready to start the next
piece of work (WU-15b's hour-long run, or WU-16), that's the next session's
own job to scope properly, same as always.

If the 4K Mini's PSU gets replaced and you want it back in the picture in
some role, or if anything about the Monitor 3G/Recorder 3G split changes,
let me know whenever it happens — no rush, and it doesn't block anything.
