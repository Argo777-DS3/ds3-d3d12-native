# I-0127 RESULT - Les draws d'effets regroupés façon D3D12

## Conclusion

Tranche 1 (2026-11-07, branch `claude/determined-cartwright-0b088e`, the ASUS at 60 Hz): **the effect stage's recording costs
0.13 to 0.18 ms of render-thread time a frame at the heaviest effect scene known; steps A and B take about 10 % of it off
(OBSERVED in two alternated pairs) and halve its D3D12 calls; its GPU time does not move.** The stage is under the study's
« nothing to win » line (about 0.2 ms, [STUDY-EFFECT-BATCHING](STUDY-EFFECT-BATCHING.md) §4): whether step C (runs by
`ExecuteIndirect`) is done is Thomas's choice, asked on 2026-11-07 13:00. **Closed without step C** (Thomas, 2026-11-12, D-0273:
the plan's recommendation accepted; ROADMAP workstream 9): the investigation is complete with tranche 1 (merged `92fe99cd`).

- **Elden Ring first** ([ER-STUDY](ER-STUDY.md), D-0195): ER filters by dirty bits and ranges, not by value; it sets the pipeline
  and rebuilds a dirty table again for an identical value, keeps the root signature filter, and caches only its sampler tables by
  content. The native stage compares values (the draw's state is whole in `NativeEffectDraw`), which ER's mechanism cannot.
- **Step 1, the measure** (`effect-measure.flag`; the module's `native_effect_measure` rows every 600 frames; `EffectFrameMeasure`
  in `effect_stage.h`, `EffectMeasureWindow` in `scene_pass.h`): the stage's render-thread time by QueryPerformanceCounter
  without the 0x29 rows and scene copy its hooks draw (timed apart), the effect block's GPU time by two timestamps a frame slot
  (the stage, its interleaved rows, its copies and the 0x31 layer's composite, read at the slot's release), the draws, the calls
  by kind and those that repeat the list's state.
- **Step A, the state set only when it differs** from what the list holds since the stage's last bind (pipeline, topology,
  targets, sampler table, each root CBV, the vertex and index views; the bind, the 0x29 rows and the GPU-particle statements
  forget it). **Step B, a texture table per distinct five-view tuple of the frame**: written once, bound again by later draws with
  the same views (the views' sources live as long as the frame's records; the depth and copy views are made for the frame); an
  effect record's buffer is still put in the shader-read state by every draw that reads it.

## Evidence

Runs `i127-t1-fire-1-before`, `-2-after`, `i127-t1-fireb-1-after`, `-2-before` (12:40 to 12:55, the slot the coordinator gave;
`Run-I127Measure.ps1`, Alpha's all-pyromancies copy at Firelink, eight casts of Seething Chaos, Flame Fan, Fire Orb and Profaned
Flame in turn, 1280x720 window, `measure.flag`, no debug layer (each `baseline.json`), no video; every run exit 0, no fault,
restored by hash). Builds `evidence/I-0127/build-m1` (step 1, at `7cd8bb5c`) and `build-a1` (steps A and B, at `c0a0ae40`), every
unit test passed in each. Read with `Get-I127Measure.ps1` (600-frame windows; the first window holds the load and is left out):

| Run | Bonfire windows (serials 3000-4800): stage CPU, GPU block, draws | Cast windows (6000 on): stage CPU (window sd), GPU block, draws | Calls a frame (all windows) |
|---|---|---|---|
| fire-1-before | 176.5 µs, 332 µs, 69.8 | 150.7 µs (3.9), 276 µs, 53.3 | 1,200 (of which 630 repeated the list's state) |
| fire-2-after | 158.9 µs, 338 µs, 75.0 | 135.6 µs (4.7), 280 µs, 55.4 | 570 (680 filtered) |
| fireb-1-after | 158.3 µs, 327 µs, 68.9 | 133.2 µs (2.4), 275 µs, 52.7 | 538 (641 filtered) |
| fireb-2-before | 177.8 µs, 352 µs, 69.9 | 142.3 µs (2.9), 278 µs, 54.1 | 1,199 (640 repeated) |

- Windows: the bonfire's are serials 3000 to 4800 (the camera still, the fire in view, before the casts); the casts' are 6000 on,
  without a window holding a frame over 5 ms (fire-1-before's at 6000, 284 µs mean with an 80 ms frame: the series' first casts,
  INFERRED its first pipelines; with it its cast mean is 162.9 µs). « sd » is the spread of that run's windows.
- The stage: -18.6 µs at the bonfire (-10.5 %), well above the runs' spread (176.5 and 177.8 µs before, 158.9 and 158.3 after);
  -12 µs in the casts (-8 %), about the size of the before build's run-to-run spread there (150.7 and 142.3 µs), so the casts'
  gain is the weaker evidence; the hooks' rows (the 0x29 forward rows the stage interleaves) take 80 to 90 µs more, outside this stage.
- Calls a frame after A and B (steady casts): 22 pipelines, 7 topologies, 16 target sets, 22 texture tables, 7 sampler tables,
  234 root CBVs, 56 vertex views, 80 views written (290 before), 57 draws, 6 binds. The root CBVs (five a draw, their ranges in the
  frame's arena) are now most of what is left.
- GPU: unchanged within the runs' spread (±20 µs between runs of one build).
- The arena (the ring's vertices and the constants) is 167 to 175 KB a frame, copied twice (the builder's frame copy, then the
  upload).

## Reviews

- Step review (fresh Opus, high, `main...HEAD` at `d7378335`): nothing high; no filtered draw can carry a state the unfiltered
  recording would not bind (every bind, hook row, GPU-particle statement and pass change forgets the held state; the tuple is
  exact; effect buffers are still transitioned on every draw). Answered: M1 the timestamp heap and readback reset at the
  renderer's Close (a reopened device); L2 the timestamp frequency written and read under the lock; L3 a frame whose preparation
  stops is not taken for the previous one (`recorded`); L4 the texture tables found by hash (no quadratic scan in a dense frame);
  L5 the ring's room checked only for a new table; L6 the comments. Kept as is: `stats_.textures` (the `native_effect_stage`
  row's « textures ») now counts the views written, not the views bound (no reader uses it).

- Final review (fresh Opus, xhigh, `main...HEAD` at `311a27b7`, main already in): nothing high. Answered: M1 the tree to be merged
  built (`build-a3`, at `779f2c3e`, every unit test passed) and run (`i127-t1-debug`, 13:30, the debug layer on: no error, no
  fault, the measure's rows written, the same calls a frame as `build-a1`'s runs); M2 the pictures compared: the still camera
  before the casts (`sweep-1-2`, cropped to the game's 1280x720 client), FLIP's mean before against after 0.031 and 0.032, before
  against before 0.036, after against after 0.034 (the fire's motion; `evidence/I-0127/pictures/`), so no change the noise does
  not hold; the merge of A and B under the « nothing to win » exit said to Thomas; L1 STATE and the index rewritten at the
  checkpoint; L2 the windows and the runs' spread stated; L3 the ER pipeline claim re-read by the pilot; L4 the timestamps' failure
  latched for the device and never thrown from the measure; L5 one reset of the frame's measure, the cut comments.

- Merged into `main` at `92fe99cd` (2026-11-07 13:38); the launcher rebuilt from it (every unit test passed); start check
  `start-92fe99cd` passed (14:02, the ASUS).

## Uncertainty / unresolved questions

- One scene (Firelink, its fire and the pyromancies), one screen (the ASUS at 60 Hz), one render size; the Irithyll bridge and a
  snow zone not measured.
- The pictures: compared at one still view (FLIP, within the fire's own noise); no PIX event list was compared (the draws and
  their order are the same by construction: A and B skip only calls that set the state the list already holds).

## Artifacts produced or modified

- `native/renderer/effect_stage.{h,cpp}`, `scene_pass.{h,cpp}`, `scene_pass_frame.cpp`, `native/frame/game_bootstrap/02_frame_lifecycle.inc`,
  `21_effects.inc`; `investigations/I-0066-independent-native-startup/Run-StartupTrace.ps1` (the flag accepted).
- [Run-I127Measure.ps1](Run-I127Measure.ps1), [Get-I127Measure.ps1](Get-I127Measure.ps1), [I0127Refs.java](I0127Refs.java),
  [ER-STUDY](ER-STUDY.md); `evidence/I-0127/` (builds, series logs, `er-read/`).
