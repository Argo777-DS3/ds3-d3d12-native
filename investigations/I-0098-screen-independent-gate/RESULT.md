# I-0098 result - Seuil indépendant de l'écran (the screen-independent gate)

## Conclusion

The replayed path gate (I-0083) now judges a native tree on any screen, one base per screen (D-0209): two measure runs on each
required screen (the 1920x1080 ASUS VA24E at 60 Hz, secondary, and the 2560x1440 MO27Q28G OLED at 279.96 Hz, primary), each
judged by its own base; the validation run and the memory leg once. Its limits are relative to the run's frame period P, its
display count comes from PresentMon, and a merge needs a pass on every required base (`scripts/Test-Project.ps1`).

- **Relative limits (H1 confirmed).** P is the median, over the 120-frame windows ending in the motion's segments, of the mean
  of each window's frames within 1.5 times its median: 16.667-16.678 ms under DS3's 60 frames/s cap on both screens. Frames
  over 1.5 P and 2.04 P (25 and 34 ms at 60 frames/s) from every frame's time (`frame-dump.flag`, now 0.001 ms): on the 16
  frame-dump runs `i97-t2c-*` at 60 Hz the relative counts equal the fixed ones, 64 of 64 (frame end and game step). P itself
  is a metric (period_ms), since relative counts alone would pass a uniformly slower run.
- **The display count (H2 refuted for DXGI).** DXGI's frame statistics do not measure the screen with two screens (`i98-c1`):
  on the 60 Hz secondary screen the native chain's PresentRefreshCount kept the OLED's rate (4.67 refreshes a frame) and its
  samples almost stopped (7 rows a run instead of about 60); on the OLED frames « held » 0 or 3 refreshes at 60 frames/s. The
  module's per-sample holds (`frame-dump.flag`, `display_refreshes` rows' `holds`) stay as a diagnostic. The gate counts
  instead what PresentMon sees: each displayed frame's interval since the previous display change beyond the screen's cadence
  (c refreshes, c = the refreshes a frame period spans rounded up with a tenth of slack), in frame periods (`pm_late`; one per
  repeat at 60 Hz). NVIDIA FrameView's PresentMon does not run outside its service (it exits at once, silent, elevated or not);
  Intel's PresentMon 2.6.0 (signed by Intel, `C:/Users/User/Tools/PresentMon/`, installed by Thomas) runs unelevated since
  Thomas joined the Performance Log Users group (2026-10-24 06:20). Its frames are placed on the marks' clock by the
  performance counter (`presentmon-clock.txt`): its `--date_time` was two hours ahead of local time.
- **The presentation model (H3).** Under PresentMon (`i98-c2-*`, one run each): on the OLED both renderers run in Hardware
  Composed: Independent Flip and the screen follows 60 frames/s (16.67 ms a frame: no 4/5 cadence); the native shows 9.5 late
  periods against the original's 34.9. On the 60 Hz secondary screen both are composed by Windows (Composed: Flip): over the
  motion's segments the native drops 247 of 7,793 frames (3.2 %; its present intervals spread over 15-19 ms) against the
  original's 26 of 7,797 (0.33 %), and shows 259 late periods against 28. A defect of the native on a composed 60 Hz screen,
  not of the measure (K-0479). (An earlier count here, 99 of 3,000 against 13 and a later display, came from a slice of
  frames 4,000 to 7,000 of the whole capture; the final review recounted over the motion: the drops hold, the latency claim
  did not, and it is withdrawn.) The original's windowed chain is still DXGI swap effect 0 to the game
  (`original_display_probe`, statistics unsupported) whatever Windows does with it.
- **Placement.** The pilots place the game's window on a named screen (`DS3_PILOT_SCREEN`, `Pilot-Game.ps1 -Action place`,
  `Invoke-ReplayRun -Screen`, `Run-Endurance -Screen`); the standstill picture and its client rectangle are of that screen
  (8 31 1280 720 on both); the module's frame rows carry the screen's rate (60 or 280 after the move: the renderer re-reads it
  every 120 presents).
- **Bases (`i98-cal`, 2026-10-24 07:12-07:52, judged again as `i98-cal-eval` after the gate stopped requiring DXGI's display
  windows).** 1920x1080@60: period 16.76 ms, frames over 2.04 P 22 to **8** (D-0207's pending recalibration), over 1.5 P 36
  (the formula gave 38: a rise refused), display_late **137** (new, PresentMon: 74-114 in the four runs, the defect above; Thomas accepted this starting point, 2026-10-24 07:19, « Oui d'accord », the original's level being tranche 3's goal; so the contract's « not relaxed » holds for the frame limits, while the display limit changed instrument and starts higher, D-0209),
  game steps over 2.04 P 6 (kept); its former display limit (20, DXGI repeats on a single primary screen) retired, not
  compared. 2560x1440@280 (first calibration, its own reference picture): period 16.76, 9, 40, display_late **18** (10.3-10.9),
  7. Memory leg +1 MB a visit (threshold 40, shared).

- **The final gate (i98-final, 2026-10-24 08:32-09:04, tree 897a5efd389f): PASS on both screens.** 1920x1080@60: period
  16.669 ms, frames over 2.04 P 6.5, over 1.5 P 31.5, display_late 83.25, game steps over 2.04 P 2; 2560x1440@280: 16.668,
  5, 24.5, 10.675, 3.5; pictures within their thresholds; memory +0.8 MB a visit; no fault.

## Evidence

- Relative counts against fixed: `scripts/Get-ReplayMeasure.ps1` on `evidence/I-0066/artifacts/i97-t2c-1-a` to `i97-t2c-8-b`.
- Control runs: `i98-c1-n280`, `i98-c1-n60`, `i98-c1-o280`, `i98-c1-o60` (DXGI rows, no PresentMon), `i98-c2-*` (PresentMon,
  `presentmon.csv`, `presentmon-clock.txt`).
- Calibration: `i98-cal-1920x1080-60-m1..4`, `i98-cal-2560x1440-280-m1..4`, `i98-cal-v1`, `i98-cal-z1` (the exit heap fault
  0xC0000374 after the close, DS3's own, I-0099); gate records `evidence/I-0083/gates/i98-cal.json`, `i98-cal-eval.json`.
- Step review (fresh Opus, 2026-10-24): a two-screen calibration blocked by the first base's notes, a window not placed failing
  instead of being inconclusive, an allocation under the presenting thread's lock, a period inflated by hitches, run names
  shared by two screens of one rate, `-Screens` split late, the pilot screen variable left set, a stale comment: all fixed; a
  late refresh hidden in a multi-frame sample documented.

## Uncertainty / unresolved questions

- The native's drops on the composed 60 Hz screen are not explained. Leads (Thomas, 2026-10-24: tranche 3): its pacer hands the
  60 Hz sync to DXGI (Present(1)) with three buffers and a waitable object, where the original keeps two; its swap chain is made
  on the OLED before the window moves (to test, the chain re-created when the window changes screen, Thomas's choice over
  switching the primary screen).
- The 60 Hz base's display_late spread is wide (74-259 over five runs): its threshold (137) catches only a large regression
  until the defect is fixed; then it comes down (D-0207).
- One original run a screen; unlocked frame rates (V2) are not run yet (P and the cadence then follow the run; without a sync
  interval PresentMon still sees the display changes).

## Artifacts produced or modified

`scripts/Get-ReplayMeasure.ps1`, `scripts/Invoke-ReplayGate.ps1`, `scripts/Invoke-ReplayRun.ps1`, `scripts/Get-Screens.ps1`,
`scripts/Test-Project.ps1`, `.project/REPLAY-GATE.json` (schema 2), `investigations/I-0066-independent-native-startup/Pilot-Game.ps1`,
`Pilot-TitleContinue.ps1`, `investigations/I-0085-night-endurance/Run-Endurance.ps1`, `Run-I98Controls.ps1`; native:
`native/renderer/frame_pacer.h` (DisplayRefreshes holds), `native/renderer/scene_pass.*`, `native/frame/game_bootstrap/01_state.inc`,
`20_startup_phases.inc`, `22_setup.inc`, `native/frame/frame_pacer_test.cpp`.

## Suggested next step

The stutter's tranche 3 (Thomas, 2026-10-24 08:39: before the exit fault), with the composed 60 Hz screen's drops, the screen chosen at launch, the swap chain re-created on a screen change and the three screen modes under PresentMon (ROADMAP); then the exit fault (I-0099 STUDY's plan).
