# Handoff
Overwritten at the end of every session. This is the first thing to read.

---

**Session:** 16
**Tag:** none yet — WU-15a is `wip`, not `green`. `wu-14-green` (session 15)
remains the last confirmed tag. WU-15a's own `Accept:` criteria are now all
satisfied (mechanics and by-eye warp confirmation both done); `./tools/
close.sh 15a` and tagging `wu-15a-green` are Steve's own next action, not
run this session.
**Phase:** 3 — SDI output. WU-14 (device enumeration, `ComPtr`) is `green`.
WU-15a (scheduled playback of one looped, file-sourced, warped frame) is
implemented, built, and confirmed working end to end against real hardware
— ready to close, pending only Steve's own `close.sh` run.

**This was a long session with a real hardware incident and a real (if
ultimately false-alarm) investigation in the middle of it.** Summary in
chronological order; `DECISIONS.md` ADR-032 through ADR-036 and
`CORRECTIONS.md` C-013 are the frozen record — this section is the
narrative, not a replacement for reading them.

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
`./tools/close.sh 15a` and tagging are Steve's own next action.

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
through ADR-036, `CORRECTIONS.md` C-013, and `WORK-UNITS.md`'s WU-15a/
WU-15b entries for the full record.

**Corrections this session:** C-013 only (the `bmdModePALp` failure's
stated *reason*, not the `bmdModePAL` decision itself). The hardware
incident (ADR-034) and the warp-visibility investigation (ADR-036) both
closed without any earlier project claim being shown wrong, so neither
gets a `CORRECTIONS.md` entry — see those ADRs' own closing notes for why.

**Delivery mechanics:** this session ran remotely via the device-bridge
tools throughout. Commits this session: `c5605f4` (WU-15a implementation),
`5fd4b4f` (`bmdModePAL` fix, ADR-033/034, C-013, the 4K Mini incident and
Monitor 3G pivot), `8b840e9` (ADR-035, Monitor 3G run results), `0138c54`
(warp investigation opened, off-hardware evidence logged), `ef45a35`
(ADR-036, investigation closed, diagnostics/sphere-swap reverted). Working
tree is clean as of this handoff. The bridge's own `unlink`-can't-work-on-
this-mount limitation continued to leave stale `index.lock`/`HEAD.lock`/
temp-object files after nearly every commit this session; each was moved
into `_to_delete/` rather than removed, per the established convention —
`_to_delete/` has accumulated a lot this session (including throwaway
diagnostic tools and PGM/PNG dumps used mid-investigation); safe to `rm -rf
_to_delete/` by hand whenever convenient.

## Next work unit

**WU-15a:** run `./tools/close.sh 15a` at your own terminal whenever
convenient (this session doesn't run it) and let me know the result — I'll
update `WORK-UNITS.md`'s status line and tag `wu-15a-green` from what you
report back.

**After that,** the choice is WU-15b (the one-hour unattended endurance
run — no new code, Steve's own hands-on step, `DECISIONS.md` ADR-032/
`WORK-UNITS.md`'s own WU-15b entry; can run against the Monitor 3G for now,
same as WU-15a's own verification did, or wait for the 4K Mini) or WU-16
(thread pool, QoS, per-worker bin arenas — Phase 4), if WU-15b is deferred
to run unattended in the background of a later session rather than blocking
the next one.

**Separately, not blocking either of the above:** whenever you've made
progress on the 4K Mini (Blackmagic support, a hardware check), let me know
what you find — I'll fold the outcome into `DECISIONS.md`, either closing
ADR-034's pivot as temporary-and-resolved or recording whatever comes next.

## Open questions

Unchanged from session 15, none touched this session: Q1 (tile size), Q2
(4K Mini program outputs — now also entangled with whether the 4K Mini
itself is usable again), Q3 (macOS/Desktop Video version), Q4 (lattice
edge damping, C-008(a)).

Resolved this session: `bmdModePALp` unsupported on the 4K Mini, `bmdModePAL`
confirmed working on *both* the 4K Mini and the Monitor 3G (ADR-033,
extended by the Monitor 3G run); `RowBytesForPixelFormat(bmdFormat10BitYUV,
...)` matches `v210::rowBytesMin()` on both devices too. Whether the
warp is actually visible on real hardware — yes, confirmed (ADR-036), after
a real but ultimately false-alarm investigation into a display-side aspect
issue, not a code defect.

New this session, still open: is the 4K Mini recoverable at all (Steve's
own hardware diagnosis, not this project's). Nothing else new.

## Blocked / red

Nothing red. The 4K Mini remains hardware-unavailable (see ADR-034) but
that no longer blocks WU-15a, which is now fully verified against the
Monitor 3G instead.

## Environment check

Unchanged from the last update this session: UltraStudio 4K Mini
unresponsive (Desktop Video, Media Express, Thunderbolt System Report all
show it absent; no power passthrough) — isolated to the unit itself, not
the Mac/port/cable/driver. UltraStudio Monitor 3G confirmed working and is
now the verified hardware target for WU-15a (`bmdModePAL` +
`bmdFormat10BitYUV`, playback only, no capture input — expected and
unrelated to WU-15a's own playback-only need).

## Append to DECISIONS.md

ADR-033, ADR-034, ADR-035, and ADR-036 were all appended in full this
session; see `DECISIONS.md`. None reopens an earlier entry — each records
either a new decision or an investigation's own closure; see each entry's
own closing note for its precise relationship to what came before.

## Append to CORRECTIONS.md

C-013 was appended in full this session; see `CORRECTIONS.md`. Nothing
further — see "Corrections this session" above for why the hardware
incident and the warp investigation both closed without one.

---

## What to run at your terminal

Whenever convenient, not urgent:

```
cd ~/src/scatter-dve
./tools/close.sh 15a
```

Let me know the result and I'll update `WORK-UNITS.md` and tag
`wu-15a-green` from what you report back.

If you make progress on the 4K Mini (support ticket, hardware check, a
different cable/dock, anything), let me know what you find whenever it
happens — no rush, and it doesn't block anything above.
