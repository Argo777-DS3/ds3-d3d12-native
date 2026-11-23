# I-0098 - Seuil indépendant de l'écran (the screen-independent gate)

Capability: C3. Owner: Opus 5.5. Thomas (2026-10-24 04:58-05:04 Paris, relayed by the coordinator): the replayed path gate
on an « unlimited » base (« dans l'idéal ce serait sur une base illimitée, il existe plein de configurations différentes et
justement j'ai deux écrans bien différents donc ça peut être notre avantage »): limits relative to the run instead of fixed
milliseconds, the display's repeats against the screen's real rate, one base per screen never mixed, the gate passed on both.

## Question

Can the replayed path gate (I-0083, `scripts/Invoke-ReplayGate.ps1`) judge a native tree on any screen, here a 1920x1080 at
60 Hz (ASUS VA24E, the screen every earlier calibration ran on) and a 2560x1440 OLED at 279.96 Hz (MO27Q28G, primary since
2026-10-24), without its 60 Hz assumptions and without relaxing the current limits (frames over 34 ms 22, over 25 ms 36,
display repeats 20, game steps over 34 ms 6)?

## Scope

Included:

- The run's frame period P (the median of its 120-frame windows' mean interval: 16.67 ms under the 60 frames/s cap on any
  screen, the frame rate itself once unlocked) and the frame counts relative to it (1.5 P and 2.04 P, which are 25 and 34 ms
  at 60 frames/s), from every frame's time (`frame-dump.flag` on the gate's measure runs).
- The display count against the screen's cadence: the refreshes a frame holds beyond the ones its period gives (1 at 60 Hz,
  4 or 5 at 279.96 Hz), in frame periods (equal to the current repeats at 60 Hz); the cap's irregular 4/5 cadence at 280 Hz
  kept apart, not counted as a stutter.
- PresentMon (NVIDIA FrameView's 1.9, elevated: Thomas allowed it, 2026-10-23 17:49) on the original and the native on both
  screens: the real presentation model (the original's chain under « optimisations for windowed games ») and a check of the
  module's display count.
- One base per screen in `.project/REPLAY-GATE.json` (thresholds, calibration, last pass, the original's numbers), the runs
  placed on a named screen (the pilots offset to it), the gate passed on every required base; the current thresholds moved to
  the 60 Hz base under their relative names; the 280 Hz base calibrated.

Excluded: unlocking the frame rate (V2), any native change beyond the display count's per-sample dump, the exit fault (I-0099,
next), the stutter's tranche 3.

## Inputs

- I-0083 (the gate, D-0202), I-0085 (the memory leg, D-0204, D-0206), I-0095 §4 and §8, I-0097 (display repeats, D-0208,
  `frame-dump.flag`), D-0207 (the ratchet), ROADMAP (2026-10-24 04:27: relative limits).
- Runs under `evidence/I-0066/artifacts/` (the gate's `i97-*`, the frame-dump series `i97-t2c-*`).

## Hypotheses

- H1: at 60 Hz the relative counts equal the fixed ones (P = 16.67 ms), so the current thresholds carry over unchanged.
- H2: at 279.96 Hz the native's frames hold 4 or 5 refreshes (hand-paced Present(0)), and the frame-end and game-step counts
  are those of the 60 Hz screen within noise; the display count beyond the cadence is small.
- H3: under « optimisations for windowed games » the original's windowed chain is flipped (PresentMon), and may give frame
  statistics.

## Expected evidence

The `i97-t2c-*` frame dumps recounted (H1); two control runs on each screen with PresentMon (H2, H3); then the calibration
of the 280 Hz base and a gate on both screens.

## Exit conditions

- Success: the gate PASSes `main`'s tree on both screens, each judged by its own base, with the 60 Hz limits not relaxed;
  Test-Project requires a pass on every required base.
- Refuted: a relative count cannot reproduce the fixed one at 60 Hz (then the fixed counts stay for 60 Hz and the question
  goes to Thomas).
- Insufficient information: the 280 Hz runs' spread too wide to give a threshold that catches the I-0097 regressions.
- Stagnation: two cycles without a new discriminating measurement.
- Budget exhausted: close the tranche and replan under `.project/WORKFLOW.md`.

## Safety and human interaction

Announced live runs only (D-0116, D-0117, 5 minutes ahead by the PC clock); PresentMon's elevation asks Thomas for one UAC
click. Thomas's save is never restored over; « je joue » pauses everything.
