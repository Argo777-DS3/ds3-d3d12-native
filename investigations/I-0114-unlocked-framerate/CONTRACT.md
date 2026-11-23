# I-0114 - Le framerate débloqué

Capability: C3 · [ROADMAP](../../.project/ROADMAP.md) between R1 and R2 (D-0240: the cadence by the modern loop, then the unlocked
frame rate, method B) · the RT pilot, opened 2026-10-30 evening on the branch `claude/exciting-visvesvaraya-0537da`.

## Question

What must the native module do so that Dark Souls III's logic runs correct at any frame time (30, 60, 120, 240 frames/s and
« Unlimited », D-0240: Thomas's own setting 120), with the display as the frame's clock under V-Sync (the modern loop of
[CADENCE-REFLEX-READY](../I-0111-dlss-reflex/CADENCE-REFLEX-READY.md), whose wait only takes the clock once the logic asks for more
frames than the display shows: [I-0111 RESULT](../I-0111-dlss-reflex/RESULT.md) « Tranche 2 ») and Reflex as an option on that wait?

## Scope

Included, in tranches, each merged once written and reviewed (D-0227), every change behind a flag until Thomas accepts it in play:

1. **The lever and the clock**: DS3's requested rate (`[timing+0x354]` with its enable `+0x358`, K-0019) from an option (a flag
   first); `k = dt / dt60` read once a frame in `BeginFrame` from `[timing+0x268]`, exactly 1 at 60; the display clock of I-0111
   tranche 2 engaged whenever DS3's period is not longer than the display's frame; a call counter per site (a diagnostic flag) and
   one announced diagnostic run (UNLOCK-READY §4: calls a frame, the open units).
2. **The corrections, by visible risk** ([UNLOCK-READY](../I-0112-unlocked-framerate-impact/UNLOCK-READY.md) §3-§4, the sites of
   [UNLOCK-SITES](../I-0112-unlocked-framerate-impact/UNLOCK-SITES.md) and [RECUR2](../I-0112-unlocked-framerate-impact/RECUR2.md)):
   locomotion, the terrain response, turning, animation and transitions, camera and target tracking, physics, effects and post,
   interface, the rest; each with the general laws (`c·k`, `r^k`, `1 − (1 − g)^k`), bit for bit the original at 60.
3. **Reflex** (R1 tranche 2: Off, On, On + Boost, the PCL markers) on the frame-start wait, an option, never the only clock.
4. **The rate in the game's options** and the sweep against the original at 60 (UNLOCK-READY §5).

Excluded: the logic interpolated at a fixed 60 (method A, not chosen, D-0240); copying LTF's or TRTF's code (studied, D-0240);
a patched or killed limiter beyond its requested rate and enable (MODERN-CADENCE-STUDY §1, the fields DS3 reads itself).

## Inputs

- [UNLOCK-READY](../I-0112-unlocked-framerate-impact/UNLOCK-READY.md), [UNLOCK-SITES](../I-0112-unlocked-framerate-impact/UNLOCK-SITES.md),
  [RECUR2](../I-0112-unlocked-framerate-impact/RECUR2.md), I-0112 [RESULT](../I-0112-unlocked-framerate-impact/RESULT.md); the
  cadence plan [CADENCE-REFLEX-READY](../I-0111-dlss-reflex/CADENCE-REFLEX-READY.md) (§8: DS3's limiter and dt, read statically).
- The native module's hooks (`native/frame/game_bootstrap/22_setup.inc`), `BeginFrame` (`15_cpu_service_and_input.inc`), the
  display clock (`native/renderer/frame_pacer.*`, `scene_pass.cpp`).
- The timing project's runners (`timing/`, historical): validation method only.

## Hypotheses

- H1: with the requested rate above the display's under V-Sync, the frame-latency object blocks every frame and the display paces
  the game; the game's dt follows (DS3 measures it, K-0019).
- H2: the sites of UNLOCK-READY §3 are where the logic depends on the frame rate; corrected with `k`, the mechanics measured at
  120 and 240 match the original at 60 within UNLOCK-READY §5's bounds.

## Expected evidence

Unit controls of each law (`k = 1` bit for bit; two calls at `k/2` equal one at `k`); the diagnostic run's call counts; announced
live sweeps on the ASUS (and the OLED for the high rates, Thomas present) against the original at 60, mechanic by mechanic; the
one-minute start check after each merge (D-0229).

## Exit conditions

- Success: the logic correct at 30, 60, 120, 240 and Unlimited on the mechanics of UNLOCK-READY §5, the display the clock under
  V-Sync, Reflex an option, the rate in the options, reviewed and merged, Thomas accepting it in play.
- Refuted: a mechanic that cannot be corrected from the module without patching protected code: stop, record, ask Thomas.
- Insufficient information: a site whose call rate or unit the diagnostic run cannot settle.
- Stagnation: two failed local fixes of the same mechanic open an RCA.
- Budget exhausted: close the tranche and replan under `.project/WORKFLOW.md`.

## Safety and human interaction

Live runs under WORKFLOW « Live game rule » (announced, offline, hash-checked restore); the OLED only with Thomas present; nothing
installed in the game folder; Thomas's saves never touched. A rate above 60 is offered to Thomas only once its mechanics are
corrected.
