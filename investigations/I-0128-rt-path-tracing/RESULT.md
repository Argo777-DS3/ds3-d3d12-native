# I-0128 result: Le path tracing complet (R8)

## Conclusion

2026-11-07 to 2026-11-08 (Paris), the R pilot; R8d its last tranche. The plan: [R8-PLAN](R8-PLAN.md) (no study thread report for R8; ER has no path
tracing to copy). D-0257 (Thomas): the light sets' fill lights kept on the GI frames again (below). R8a (the traced passes' GPU times) and R8b (several bounces at the PT level) written, reviewed and measured: one
bounce more takes the PT level's GI from 3 to 5 % under the long-path reference to within 0.1 % of it, two vertices the default;
it costs about 7 ms of GPU for the GI and 7 ms for the reflections a vertex at 1920x1080 DLAA. R8c (the hits' local lights from a
world grid instead of the whole list) takes 3 to 4 ms off the frame at Firelink, 0.8 to 1.9 ms at the High Wall, the picture the same
(by the code's argument and the GI's mean); a vertex still costs about 7 ms a pass. R8d decorrelates ReSTIR GI for RR (the temporal
neighbour permuted, a share of the pixels shaded from their initial sample): unbiased over the picture, now about as steady as the one
ray but not steadier, for 5.2 ms more at 1920x1080, so it stays opt-in; PathFrom, compiled into every programme, cost the RT level
0.7 ms: it is now only in the PT level's builds. ReSTIR DI was not written: R8-PLAN §2's condition (a picture of R8b showing a
local-light noise RR leaves) was not examined for it; no such noise was reported, so it stays to look for (R11's tour).

## R8a and R8b (2026-11-07)

- **Written** (`1ae717f2`): R8a, `rt-gpu-measure.flag` (native/renderer/gpu_zones.h's GpuZoneTimer: a timestamp pair a zone a frame
  slot, resolved at the list's close, read at the slot's completion; `native_rt_gpu` rows every 600 frames; [Get-R8Gpu.ps1](Get-R8Gpu.ps1)).
  The zones: the main view's frame (its first list's start to its last list's close), the scene's build, the traced sun's draws, the
  cache (its update and resolve together), the reflections, the water, the local lights, the GI (its ReSTIR stages or NRD
  together), RR's guides, DLSS (its evaluation). R8b, rt_reflect.hlsl's `PathFrom` (R8-PLAN §3): at the PT level the reflections' and
  the GI's paths go on past their first hit, B vertices before the cache (`rt-bounces.txt`, `rt-bounces-N.flag` for the runs, 1 to 4,
  default 2); `rt-pt-no-cache.flag` the reference (8 vertices, no cache). Run-R7aGi.ps1 took R8's modes and `-Measure`.
- **Step review** (fresh Opus, high, `8b33bda5..1ae717f2`): no High or Medium. Answered: L1 the B = 1 and RT-level hits in R7's exact
  expression; L2 a zone timed only where its traced pass ran (the sun's zone around its traced draws, a refused reflection or lights
  pass's zone dropped by `Cancel`, the water's begun only when it runs); L4 each written pair resolved alone; L6 the rows say 1 vertex
  off the PT level; L8 the HLSL limit tied to `kRtPathVerticesMost` by a comment; L9 the comments. Recorded: L3 the cache's and the GI's
  stages timed as one zone each (below); L5 the last vertex's cache term weighted as R7's (`albedo × HitAmbient`, no glossy), so B = 1
  against the reference measures the cache's error and the hits' missing glossy ambient together; L7 PathFrom compiled into the
  programmes whatever B: its register cost at the RT level not measured against an R7 build (the timer did not exist there).
- **The answers' step review** (fresh Opus, high, `1ae717f2..43416104`): no High or Medium, the claims checked. Answered: L1 the
  water's line braced; L3 the cache's zone ended only when the cache ran, the GI's when its inputs were there, DLSS's when it evaluated
  (else cancelled); L4 the rows count the frames the PT level traced (`pt_frames`: the level from RR's frame, not the startup's).
  Recorded: L2 the water's zone counts its failure frame (one a session, the pass then off).
- **The final review's answers' step review** (fresh Opus, high, `da7354a1..59984543`): no High or Medium. Answered: L1 the diffuse
  horizon end's cache term divided by the diffuse draw's probability (it is reached only on that draw now); L3 the comment on the
  zone helpers' gate (a safety net). Recorded: L2 the cache update's vertices' salts (R7b's, 0x400 apart) overlap from 768 lights
  (160 today): to space when R8c touches HitLight's callers.
- **Not run in the slot**: the High Wall's B = 4 cost mode (cut for time) and `pt3mean`.
- **Final review** (fresh Opus, xhigh, `main...da7354a1`): no High or Medium; the figures recomputed from the rows and the dumps (B = 1
  0.9555, B = 2 0.9994 against the shorter reference launch). Answered: L1 a glossy sample under the surface ends the path with
  nothing (it took the diffuse cache term, over-lighting rough surfaces at grazing angles: the reflections' paths, not bias-checked);
  L3 the vertices' salts 2^16 apart (the light picks overlapped the next vertex's draws from 1,808 lights); L5 the dead water call
  removed; L6 the zone helpers the main view's only; L9 this RESULT (B = 3 not bias-checked, the window's size, the default worded as
  provisional, the builds, status.json, the index row). Recorded: L2 a glossy chain's earlier lobes' spread not carried (the footprint
  under-estimated, the seed answering more: the safe side); L4 the scene's, the GI's exception, RR's latch and the water's failure frames
  still timed (one a session each); L7 the timer not exercised under the debug layer (a hardware unit test to add); L8 the RT level's
  cost with PathFrom compiled in, measurable by this build against main's rt_reflect.hlsl (a follow-up).
- **The optimisation pass** (read, D-0227): no work done twice in the additions; two timestamp paths now exist, I-0127's effect block
  (E's, `effect-measure.flag`) and R8a's zones: the effect block could become a zone of GpuZoneTimer (proposed to E, its workstream
  open); a path vertex's loop over every light is R8c's.
- **The measure** (2026-11-07 16:20-16:47 Paris, `build-r8b1`, the ASUS, offline, restored by hash, no debug layer, DLAA and RR,
  nothing else running but this thread's step reviewer reading files; runs `i128-r8a1b-*`, `i128-r8a2-*`, `runs-r8a1.log`,
  `runs-r8a2.log`; the first run refused at the title's logos, `native_scene_root_frame_foreign`, before the world, run again; the
  windows' frames-weighted means, Get-R8Gpu `-Skip 0`), in milliseconds:

  | Run | frame | GI | reflections | cache | DLSS-RR | scene build |
  |---|---|---|---|---|---|---|
  | Firelink 1920x1080, RT | 16.9 | 5.5 | 3.8 | 1.4 | 3.3 | 0.3 |
  | Firelink 1920x1080, PT B = 1 | 19.1 | 5.6 | 5.7 | 1.5 | 3.4 | 0.3 |
  | Firelink 1920x1080, PT B = 2 | 33.5 | 12.9 | 12.8 | 1.5 | 3.4 | 0.3 |
  | Firelink 1920x1080, PT B = 3 | 46.1 | 19.1 | 19.2 | 1.5 | 3.4 | 0.3 |
  | Firelink 1920x1080, PT B = 4 | 57.1 | 24.7 | 24.7 | 1.5 | 3.3 | 0.3 |
  | Firelink 1920x1080, PT B = 2, ReSTIR GI | 38.1 | 17.3 | 13.0 | 1.5 | 3.4 | 0.3 |
  | High Wall 1920x1080, RT | 16.9 | 5.7 | 4.3 | 1.3 | 3.4 | 0.4 |
  | High Wall 1920x1080, PT B = 1 | 17.7 | 5.7 | 5.0 | 1.4 | 3.3 | 0.4 |
  | High Wall 1920x1080, PT B = 2 | 31.8 | 13.3 | 11.6 | 1.4 | 3.3 | 0.4 |
  | High Wall 1920x1080, PT B = 3 | 41.9 | 18.5 | 16.4 | 1.4 | 3.3 | 0.5 |
  | High Wall 1920x1080, PT B = 2, ReSTIR GI | 40.4 | 21.8 | 11.6 | 1.4 | 3.3 | 0.5 |
  | Firelink 1280x720, RT / PT B = 2 | 8.8 / 16.3 | 2.5 / 5.9 | 2.0 / 5.9 | 0.8 / 0.9 | 1.6 / 1.7 | 0.3 |
  | Firelink 1440x810 (a 1597x899 window: its swap chain 1440x810, a 16:9 size, so not the arbitrary size the plan asked), RT / PT B = 2 | 10.8 / 20.1 | 3.2 / 7.4 | 2.5 / 7.4 | 1.0 / 1.1 | 2.0 / 2.0 | 0.3 |

  The sun's traced draws 0.1 to 0.2 ms, the local lights' pass 0.1 to 0.3 ms, RR's guides 0.1 ms. A time includes the tail of the
  commands no barrier separates from it (R8-PLAN §5). **Each vertex past the first costs about as much as the first leg**, 6 to 7.5 ms
  for the GI and as much for the reflections at 1920x1080, in proportion to the pixels at the other sizes; ReSTIR GI adds 4.4 to 8.5 ms.
  Both zones hold about 160 local lights; at the PT level every vertex loops over all of them (HitLight's choice among the whole list):
  INFERRED the loop's share is large, not separated by this measure (R8c's to measure).
- **The bias check** (16:47-16:57, `build-r8b1`, no timing; runs `i128-r8b1-*` and `i128-r8b1b-m40-ptref`, `runs-r8b1.log`; the camera
  still, 1280x720, the GI's mean against the reference's, Compare-GiMean.py; each run about 40 s in the world, the reference's 8
  vertices slow: 470 frames at Firelink in the second launch, 508 at the High Wall; the walk's -Seconds does not lengthen a still run):

  | View | B = 1 / reference | B = 2 / reference | blocks p5 to p95 at B = 2 | the floor (two reference launches) |
  |---|---|---|---|---|
  | Firelink | 0.9551 | 0.9989 | 0.987 to 1.011 | 0.9995 (0.986 to 1.015, 180 frames) |
  | High Wall | 0.9676 | 1.0006 | 0.996 to 1.007 | not run |

  At one vertex (R7's PT level) the GI is 3 to 5 % under the paths' truth, everywhere (B = 1's blocks 0.93 to 0.97 at Firelink):
  the cache's answer at the first hit and the hits' missing glossy ambient together (L5). At two vertices the difference is within the
  floor. B = 2 the code's provisional default (Thomas's choice below); B = 3 was not bias-checked (`pt3mean` not run, the slot's
  time), so « nothing left to gain » past 2 vertices is INFERRED from B = 2 being within the floor; B = 3 and 4 cost 10 to 13 ms more.
- **The displayed picture** (the bias runs' last presented frames, the same still views; `pictures-r8b1/pt-bounces-display.png`,
  shown to Thomas; mean luma by ffmpeg signalstats, YAVG of 255): Firelink B = 1 57.96, B = 2 58.29, the reference 58.25; the High
  Wall 71.26, 71.10, 71.20 (the bonfire's flicker in the view). The GI's 3 to 5 % reach the displayed frame as +0.6 % at Firelink and
  within the flicker at the High Wall: the direct light dominates these two views. So B = 2's cost (+14 ms at 1920x1080 DLAA) buys a
  difference barely visible there: **Thomas chose A** (2026-11-07 18:02, D-0256: two vertices the default, `rt-bounces.txt` the setting, « comme Cyberpunk »).
- **Not done here**: an interior, where the GI weighs more (R7d's L5, R8d's).

- **Merged** `e53f8187` (2026-11-07 18:05); the launcher rebuilt from `main`, its unit tests passed; start check `start-e53f8187`
  passed (18:30-18:31, restored, no refusal, the rays' self-test passed).

## R8c (2026-11-08): the hits' light grid

- **Written** (`ad807416`, `677c2c2e`; the step review's answers `db120276`; R8-PLAN §6): [rt_light_grid.h](../../native/renderer/rt_light_grid.h), the frame's list binned on the collect
  thread into a world grid about the camera (32 × 16 × 32 cells of 8 m, the origin snapped to the cell), each cell the lights whose
  support's bounding sphere meets it, in the list's order; the words (a header, each cell's first and count, the indices) uploaded right
  after the records in the same range (`UploadRtLights`) and bound at t7 (`RtReflections::Bind`); rt_reflect.hlsl's `HitLight` loops
  over its hit's cell (`LightCell`), the whole list outside the grid, the draws still keyed by the light's index (the same sums and the
  same choice: the zero terms left out). The draft's spot bound replaced by a general one: the sphere about the light through its
  pyramid's apex and four corners (the rows' system solved), so an oblique cone is bounded too; a spot whose q.z > 0 does not imply
  q.w > 0 (the z row not along the w row, or its zero behind the apex), or whose rows are not independent, goes into every cell. Each
  reach × 1.001 + 1 cm, wider than the falloffs' rounding at R and the cell index's at a face. `no-rt-light-grid.flag` the equality
  check's other half; the `native_point_light_batches` rows give the grid's figures (`rt_grid_cells`, `rt_grid_longest`,
  `rt_grid_entries`, `rt_grid_everywhere`, `rt_grid_outside`), `native_rt_level` and `native_rt_gpu` whether it is on; Run-R7aGi's
  `*nog` modes; the unit test (rt_scene_test's LightGrid: 128 lights of every kind at two cameras, points about each and across the grid, every
  light of non-zero falloff at a point in its cell, the cells' order, the bounds, a centred spot's reach).
- **The measure** (2026-11-08 01:07-01:23 Paris, `build-r8c2`, the ASUS, offline, restored by hash, `-Measure`: no debug layer, the PC
  otherwise idle (E's build waited for); runs `i128-r8c1-*`, `runs-r8c1.log`; Get-R8Gpu `-Windows`; the fill lights on in both halves,
  D-0257's default: the baseline is the same build without the grid, `*nog`, not R8a's table). The looks ran in every run (one of the
  three refused in six runs, Pilot-Game's foreground guard; the walk shots show the desktop, so the view is read from the
  world-pictures), after frame 2,100 to 2,700, and the faster half reaches them at an earlier frame: the run means mix different views
  (the final review's M1). So the figures are the window ending at frame 1,800, the load view in both halves of each pair: each run's first timed window
  (the one Get-R8Gpu skips by default), 250 to 411 frames, unequal within a pair. In
  milliseconds, 1920x1080 DLAA and RR, without / with the grid:

  | Run | frame | GI | reflections | cache | water |
  |---|---|---|---|---|---|
  | Firelink, RT | 16.9 / 13.8 | 5.4 / 4.1 | 3.9 / 2.6 | 1.5 / 1.1 | 0.3 / 0.2 |
  | Firelink, PT B = 1 | 19.3 / 15.5 | 5.8 / 4.2 | 5.8 / 4.1 | 1.5 / 1.1 | 0.3 / 0.2 |
  | Firelink, PT B = 2 | 33.4 / 29.5 | 12.8 / 11.2 | 12.8 / 11.1 | 1.6 / 1.1 | 0.3 / 0.2 |
  | High Wall, RT | 17.2 / 16.4 | 5.7 / 5.6 | 4.2 / 3.8 | 1.3 / 1.1 | |
  | High Wall, PT B = 1 | 18.9 / 17.0 | 6.3 / 5.4 | 5.3 / 4.5 | 1.4 / 1.2 | |
  | High Wall, PT B = 2 | 32.4 / 31.1 | 13.4 / 13.0 | 11.7 / 11.1 | 1.4 / 1.2 | |

  The grid's figures (`native_point_light_batches`): at Firelink 163 lights, 1,794 cells filled, the longest cell 34 lights, 3,130
  entries (1.7 lights a filled cell), 31 lights out of the grid; at the High Wall 159 lights, 15,475 of the 16,384 cells filled, the
  longest 8, 17,874 entries, 64 out of the grid: a few lights of very long reach cover nearly the whole grid there (which ones not read).
  **The grid saves 1.3 to 1.7 ms of each of the GI and the reflections at Firelink** (3 to 4 ms of the frame), 0.2 to 0.8 ms at the High
  Wall, where the hits' cells hold a few lights; the High Wall's RT saving (0.8 ms of the frame) is not resolved: at the next window
  the pair reads 16.7 / 16.6 (the answers' last step review's L2). Why the High Wall gains less is not separated (INFERRED: fewer of its hits are lit
  surfaces near lights; or hits beyond the grid's ±128 m and ±64 m, which loop over the whole list). A path vertex past the first still
  costs about 7 ms for the GI and as much for the reflections at 1920x1080: INFERRED its trace and its material take most of it, not
  measured apart; R8-PLAN's RIS presampling left aside, the grid answering the loop's cost.
- **The tree measured** is `677c2c2e` (`build-r8c2`); the step reviews' answers after it (the spot's z-row test made exact, the
  entry budget, the tests) were not run: for the game's spots, whose z row lies along the w row, the old and the new tests accept the
  same lights (the final review's reading), and the runs showed none unbounded. The start check after the merge reads the rows'
  grid figures against these (the final review's L1).
- **The equality** (01:23-01:28, no timing; runs `i128-r8c2-m40-*`, `runs-r8c2.log`; Firelink, 1280x720, the camera still, the GI's
  mean with and without the grid, Compare-GiMean.py at frame 2700): PT B = 2 1.0001 (blocks p5 to p95 0.994 to 1.007, 1,061 and 897
  frames), RT 1.0002 (0.991 to 1.009, 897 frames each). Not the same bits between two launches (the bonfire's flicker, the frames'
  seeds), so the check is a floor's: the floor quoted, R8b's two launches of the reference (0.9995, blocks 0.986 to 1.015), is of
  another mode at 180 frames, likely wider than these modes' at 900 frames (not measured; the final review's L2): the whole-picture
  ratios are within any floor, the blocks' spread not proved to be. The code's argument (the zero terms only left out) is the stronger
  evidence.
- **Step review** (fresh Opus, high, `95f30507..677c2c2e`): no High or Medium; the bounds, the order and draws, the lookup and the
  binding checked sound. Answered: L1 the spot's z-row test made exact (q.z written over q.x, q.y, q.w through the inverse: q.z ≤ 0 on
  the whole backward square cone, the near plane's zero within R / 10⁴ of the apex; a parallel test with a tolerance let a tilted z row
  through at large world coordinates); L4 the grid's frame counter, never read, removed; L5 an entry budget (2^20 indices, 4 MiB):
  past it no grid that frame (`rt_grid_dropped`), the whole list; L6 the unit test at two cameras (one over 1,000 m from the origin),
  points just inside every support's boundary (R (1 − 10⁻⁶), a spot's cone edge and range face), a z row leaning on q.x (bounded),
  tilted past the cone and x and y rows the same (both unbounded), the budget; L7 STATE's rows. Recorded: L2 the legacy points' term
  is exactly zero only if the GPU's sqrt(1) is 1 (D3D allows 1 ULP: a ghost term of 4e-15 at most), and L3 a non-finite F0 gives NaN
  over the whole list and 0 in an empty cell: neither changes the equality's figures (the check is the floor's, not the bits').
- **The answers' step review** (fresh Opus, high, `677c2c2e..db120276`): no High or Medium; the exact z-row test, the inverse, the
  budget path checked. Answered: L1 the frames past the budget counted in the row (`rt_grid_dropped_frames`: the rows are sampled);
  L2 the test's spots whose near plane's zero lies behind the apex by half the accepted depth (bounded, its sliver's lit points listed)
  and by twice it (every cell); L3 the boundary points inset by a millimetre (a float's steps at 1,000 m) and counted by kind, each kind
  required; L4 the slope required below a thousandth of the coefficients (not within the shader's rounding of q.z); L5 the budget's
  edge (64 unbounded lights kept, 65 dropped).
- **Final review** (fresh Opus, xhigh, `main...9f8a55df`): no High; every figure recomputed from the rows and the dumps. Answered:
  M1 the cost table taken from the windows ending at frame 1,800 (the looks ran in every run, after frame 2,100 to 2,700, at different
  frames in the two halves of a pair: the run means mixed views); L1 the tree measured named, the start check to read the grid's
  figures; L2 the equality's floor worded (another mode's, at fewer frames); L3 the cost sentences (the saving, not the loop's cost;
  the vertex's rest INFERRED; the High Wall's other explanation); L4 the test's light count; L5 R8-PLAN's R8c row and its « Seen »;
  L6 the header's length a reminder on the C++ side (a static_assert naming the shader's 8, which it cannot read); L7 the budget known from the boxes' volumes before any cell is counted.
  Recorded: L7's per-frame vectors (about 300 KiB made and zeroed on the collect thread) and the upload on frames whose traced passes
  are off, small, not changed.
- **The final answers' step review** (fresh Opus, high, `9f8a55df..21990b65`): no High or Medium; the table recomputed. Answered: L1
  two cells' rounding; L2 the High Wall's saving worded (0.8 to 1.9 ms of the frame, its RT pair not resolved, the windows' frames);
  L3 the header's assert worded a reminder; L4 the batches row's buffer 768 bytes (its longest text 673); L6 the vectors' size; L7 the
  boundary test's comment (no light left out; not a proof the margins are needed). Recorded: L5 the dropped-frames count printed only
  on the sampled rows (the first 6,000 collects, then on an inert change).
- **Merged** `c5f049c8` (2026-11-08 02:27); the launcher rebuilt from `main`, its unit tests passed; start check `start-c5f049c8`
  passed (02:36-02:38, restored, no refusal, the rays' self-test passed). Its batches rows (the first two collects, 285 world frames)
  are the load's: 44 lights, all out of the grid, no grid unbounded or dropped, as the measured runs' first two rows; the merged
  tree's grid in the world (the final review's L1) not seen by the check: the next R run's rows read it.
- **The optimisation pass** (read, D-0227): one grid a frame, built once on the collect thread and uploaded once with the records,
  read by every traced pass's hits; nothing done twice. Its CPU vectors are made each frame (about 300 KiB), not timed; a frame whose
  traced passes are off still builds and uploads it (131 KiB at least).

## R8d (2026-11-08): ReSTIR GI under RR

- **Written** (`b3ce0991`, the step review's answers `13452b56`, after the slot `e04c5525` and `55240b51`; R8-PLAN §7): ReSTIR GI's samples decorrelated
  in time for RR, each choice made without looking at the samples (the 1/Z form stays unbiased): the temporal neighbour permuted
  (RTXDI's `RTXDI_ApplyPermutationSampling`; the reprojected pixel itself where the permuted one leaves the screen;
  `rt-restir-no-permute.flag`), the history's cap (`rt-restir-history-N.flag`, N 1, 2, 4, 16; else 8), a uniform share of 0.4 of the
  pixels shaded from their initial sample (the surfaces' buffer's third part; the default form after the slot,
  `rt-restir-no-decorrelate.flag`); the `native_rt_restir` row. R7d's L7: the GI's ray written once (`GiRay`, for `gi`, gi_signal and
  ReSTIR's initial sample; one change for `gi`: a hit without its records or material at 1e4 m or more brings nothing, no longer the
  cubes, as the initial sample and D-0197 already had it). `world-burst.flag` (the 31 world frames after 2400 presented alone) and
  [Get-Stability.py](Get-Stability.py) (the consecutive frames' mean |ΔY|, each pixel's standard deviation of Y over the 32 frames,
  R7d's 300-apart figure). R8's final review L8: PathFrom compiled only into the RR frames' reflections and GI (`trace_hit` and `gi`'s
  `_path` builds, `RT_PATHS` 1) and ReSTIR's initial sample, chosen by `RtPathsOf` (the shader's PathVertices condition); NRD's and the
  no-RR reflections' programmes refuse a frame whose paths go further (never met: the PT level's frames are RR's).
- **Step review** (fresh Opus, high, `5810ed19..b3ce0991`): no High or Medium; the unbiasedness of each knob, the bounds, the buffers'
  remake, GiRay against the old paths checked. Answered: L1 GiRay's one change for `gi` worded; L2 the permuted pixel off screen falls
  back to the reprojected one (it lost the history in a border band) and the bijection worded (a still camera's); L3 Get-Stability's
  empty region and its camera claim; L4 the atlas dump never on a burst frame; L5 the burst takes 2400 when the pictures' cap refused it.
- **The slot** (2026-11-08 03:45-04:27 Paris, the ASUS, offline, restored by hash at every run, no fault; `Run-R8dSlot.ps1` and
  `runs-r8d1.log` in `evidence/I-0128/artifacts`; runs `i128-r8d1-*`, `i128-r8d1n-*`, `i128-r8d2-*`, `i128-r8d3-*`; the builds
  `build-r8d2` and the throwaway `build-r8d-nopath` (`b3ce0991` with HitRadiance's PathFrom call removed)).
- **The cost** (block A, the PC entirely free, `-Measure -Still`, Firelink 1920x1080 DLAA+RR, Get-R8Gpu's windows after the first, in
  ms): the RT level with PathFrom compiled in / without: frame 13.77 / 13.06, reflections 2.59 / 2.21, GI 4.06 / 3.74 (2,400 frames each):
  **0.7 ms of the RT level's frame for code it never runs** (register pressure, INFERRED), hence the `_path` builds (the RT level's
  programmes now the throwaway's source; the regained 0.7 ms INFERRED, not measured again). The PT level at 2 vertices: frame 29.45,
  GI 11.10; with ReSTIR GI (permuted, cap 8, no share; one window of 600 frames): frame 34.69, GI 16.37: **ReSTIR GI +5.2 ms** (R8a's
  +4.6 before R8c's grid); the share's own cost (a 16-byte write a pixel, a read where it is drawn) not measured.
- **The bias** (Compare-GiMean.py at frame 3600, about 1,800 frames at Firelink, 1,185 to 1,816 at the High Wall, 1280x720, against
  the one ray `ptone`; `compare-r8d1.txt`): the floor (two `ptone` launches at Firelink) 1.0001, blocks p5 to p95 0.996 to 1.005, the
  largest 1.024. Every form within 0.9997 to 1.0009 over the picture at Firelink and 1.0000 to 1.0001 at the High Wall: **no bias over
  the picture**. Their blocks wider than the floor (p5 to p95 about 0.991 to 1.011 at Firelink, 0.994 to 1.007 at the High Wall; single
  blocks up to 1.07 to 1.19 at Firelink). R7d's local residual stays. One block, (608, 512) by the bonfire, reads 1.092 with the share
  (`ptpd`), 1.149 with the history at 2 (`ptp2`), 1.189 with both at 4 and the share (`ptp4d`), where `ptp` reads 1.004, `ptp4` 1.000,
  `pt7` 1.010 and the floor 0.997 (`blocks-r8d/`; the final review's M2): a local excess of 9 to 19 % or a heavy tail of the reuse's
  noise, one launch each: checked after the final review (below).
- **The stability** (Get-Stability.py, stride 2, the whole picture; `stability-r8d1.txt`; Y of 255):

  | View, form | consecutive |ΔY| | per-pixel std, median | 300 apart |
  |---|---|---|---|
  | Firelink, one ray (two launches) | 0.116, 0.119 | 0.236, 0.217 | 0.85 to 0.94, 0.76 to 0.93 |
  | Firelink, R7d's form (`pt7`) | 0.136 | 0.337 | 1.13 to 1.22 |
  | Firelink, permuted (`ptp`) | 0.127 | 0.281 | 0.85 to 1.00 |
  | Firelink, permuted, cap 4 / cap 2 | 0.126 / 0.121 | 0.272 / 0.257 | 0.66 to 0.79 / 0.78 to 0.94 |
  | Firelink, permuted, share 0.4 (`ptpd`) / with cap 4 | 0.119 / 0.122 | 0.245 / 0.253 | 0.91 to 0.99 / 0.76 to 0.93 |
  | High Wall, one ray | 0.246 | 0.440 | 2.31 to 2.58 |
  | High Wall, `pt7` / `ptp` / cap 4 / `ptpd` | 0.276 / 0.271 / 0.269 / 0.256 | 0.451 / 0.407 / 0.461 / 0.411 | 2.55 to 2.72 / 2.26 to 2.43 / 2.69 to 2.81 / 2.24 to 2.81 |

  R7d's finding reproduced (its form the least steady: Firelink's consecutive figure 0.136 against 0.116 to 0.119, its 300-apart
  drift 1.13 to 1.22 against 0.76 to 0.94). The permutation removes the slow drift; the share brings the frame-to-frame figure to the
  one ray's (Firelink 0.119, High Wall 0.256 against 0.246, one launch there: no floor); the history's cap adds little. The share,
  the cap at 2 and both at 4 are not told apart at Firelink (0.119, 0.121, 0.122; medians 0.245, 0.257, 0.253; one launch each, the
  one ray's two launches 0.003 and 0.019 apart). At the High Wall the GI mean of `ptone` and `ptp` restarted at frame 2415 (1,185
  frames by 3600; why not read), and every run has an 80 to 100 ms frame at 2402 (the 300th frame's witnesses written), inside the
  burst: its pairs are not the largest (the final review's L12); a later burst starts away from a 300th frame. **The best form is
  about as steady as the one ray, not steadier**, and the High Wall's moving grass and fire dominate its figures.
- **An interior: not measured** (the final review's M1). Firelink's view is inside the shrine, the view R7d already had; the planned
  dark interior, Anor Londo's great staircase hall (`m37hall`), was not still: held by the teleport the character plays a run in place, the camera follows it, the GI mean restarts
  every frame (its frames 1 at every dump) and the pictures 300 apart differ by 6 to 108: both figures invalid there, not used. R7d's
  L5 interior stays open.
- **R7d's L12**: the RT level's GI on NRD's frames (`nrdmean`) against RR's (`rtmean`): 0.9997, blocks p5 to p95 0.994 to 1.005, as the
  floor: the seed advances on NRD's frames. Their displayed pictures (mean Y of 255, frames 1800, 2700, 3600): NRD 55.5 to 55.7, RR 56.3
  to 56.5 (`display-luma-r8d1.txt`): **NRD's frames still 0.8 steps darker** with the same GI estimate, so R7c's darker NRD is not the frozen seed (TO CHECK: NRD's
  output against its input, or DLAA against RR). NRD's displayed frames flicker twice as much (consecutive 0.266 against 0.127).
- **The post-slot step review** (fresh Opus, high, `13452b56..55240b51`): no High or Medium; RtPathsOf against PathVertices, every
  caller with a path, the new pipelines and the flag checked. Answered (`3ecf5c29`): L1 the older ReSTIR modes and `ptrgpu` keep their
  form without the share; L2 gi_temporal keeps PathFrom always (ReSTIR at one vertex pays it unused: worded, not split); L3, L4 the
  comments. L5 (a build of `e04c5525` alone drew the GI unbound): that build failed to compile (`build-r8d3`'s first attempt), no figure
  came from it; the slot's build is `build-r8d2` (`13452b56`).
- **The final review** (fresh Opus, xhigh, `main...589878e0`): no High. Answered in the documents (`9631aedb`): M1 the interior not
  measured (R7d's L5 open); M2 the bonfire block; L1 the share's cost, L3 the luma file, L4 ReSTIR DI not examined, L7 the forms not
  told apart, L8 RTXDI's stagnancy mode, L5 the miss's two copies worded, L9 the statuses at the merge, L11 Get-Stability's step, L12
  the burst's restart and slow frame, H3. Recorded: L6 the third part written for every pixel (40 % read; 133 MB at 3840x2160, ReSTIR
  opt-in), L10 no unit test of RtPathsOf or the bits (one source each, the review's scenario a future edit). **M2 and M3 checked**
  (06:50-06:53, `build-r8d4`, the final tree but comments; runs `i128-r8d5-m40-ptone`, `-ptpd`, `runs-r8d5.log`; Firelink still,
  1280x720): the PT level's `_path` programmes and ReSTIR with its share ran 2,876 frames each, no failure (`native_rt_reflect`,
  `native_rt_gi` rows: missed 0, `restir_frames` 2,876); a second `ptpd` launch reads the bonfire block 1.030 against its own `ptone`
  (1.030 against the slot's), the whole picture 1.0001, blocks p5 to p95 0.994 to 1.007: the 1.09 not reproduced, so INFERRED the
  reuse's heavy-tailed noise near the flickering bonfire, a few percent above the floor there; not a reproducible local bias.
- **Decided** (R8-PLAN §7's rule; the CONTRACT's H3, « decorrelated, unbiased and steadier », refuted for « steadier »): ReSTIR GI
  stays opt-in (`rt-restir.flag`): unbiased over the picture, at best as steady as the one ray, with the same local residual, for 5.2 ms
  more at 1920x1080; its default form the permuted one with the share. The PT level keeps the one ray.

- **Merged** `c597b471` (2026-11-08 07:00); the launcher rebuilt from `main`, its unit tests passed; start check `start-c597b471`
  passed (07:06-07:08, restored, no refusal, the rays on).
- **The optimisation pass** (read, D-0227): GiRay removed the GI's ray's second copy; the share's third part written only with a share,
  its buffer made only then; the burst and the knobs behind their flags; the `_path` builds take PathFrom off the RT level. Left: the
  third part written for every pixel (the final review's L6), the permutation's offset hashed by every pixel (one hash, not passed down).

## The fill lights (2026-11-07, Thomas's dark zones)

- **The question**: Thomas (17:51, 18:04): the dark zones' lighting « pas terrible »; « si c'est à cause des fill lights, vaut mieux
  les remettre pour l'instant ». `rt-gi-fill.flag` written (`4c95b00c`: the sets' lights 1 to 3 kept on the GI frames, D-0254's
  removal skipped), built `build-fill1`.
- **The view**: Firelink's unlit lower corridor found by a piloted walk (Thomas's method: a camera turn, a shot, a step, a shot,
  the position from `chr-positions.flag`'s rows; `i128-fill1-explore`): past the Shrine Handmaid along the roots, the teleport
  `243.4 -59.8 589.4 90` (Run-R7aGi's `m40low`); Anor Londo's great staircase hall (`m37hall`).
- **The runs** (19:06-19:27, the ASUS, no measure, offline, restored by hash; `runs-fill1.log`, `explore1.log`): the native today
  (`gi`, `build-fill1`), with the fill lights (`gifill`, rt-gi-fill.flag on that build), both by Run-R7aGi; the original D3D11 on the
  same save, teleport and looks by Run-ZoneWalk `-BootstrapMode Reference` (`orig`: the slot's own script, not a Run-R7aGi mode),
  1280x720. The first shot after the teleport (`walk-1.png` of `i128-fill1-{m40low,m37hall}-{gi,gifill,orig}`, cropped to the window's
  client area; the camera the same in the three; the later shots lost the window's focus): mean luma (ffmpeg signalstats, YAVG of
  255) at Firelink 21.05, 23.53, 21.38; in the hall 43.74, 45.00, 37.85 (the original's camera a little further: that gap is the view's,
  not compared). By eye (`pictures-fill1/fill-lights-sheet.png`,
  shown to Thomas): without them the character and the near walls take only the candles' warm light; with them the original's cool
  fill on the character and the stones comes back. So **it is the fill lights**: the darkness Thomas saw is their colour and relief,
  not the picture's mean (at Firelink within a few steps). The cool fill is by eye; no colour measure over the character's region.
- **Decided** (D-0257): kept by default on the GI frames, `no-rt-gi-fill.flag` the test tool; their double count with the traced
  bounce known, their traced replacement later.

- **Final review** (fresh Opus, xhigh, `main...f4f6d379`; its answers' step review `f4f6d379..a516173e`: no High or Medium, L1 R8-PLAN's wording and L2 STATE's I-0126 row answered): no High; M1 STATE's rows rewritten (the dark zones answered); L1, L2 the
  comments; L3 `rt-gi-fill.flag`, no longer read, dropped from the runner's pattern and Run-R7aGi's modes (its cleanup kept); L4 the
  `native_rt_gi` row says `fill`; L5 this section; L6 R8b's figures were taken without the fill lights (the GI's later comparisons with
  `no-rt-gi-fill` or a fresh baseline); L7 R7-PLAN §1 and I-0126's CONTRACT amended.

- **Merged** `3bfbdfb8` (2026-11-07 20:05); the launcher rebuilt from `main`, its unit tests passed; start check `start-3bfbdfb8` passed
  (20:18-20:19, restored).

## Evidence

- Builds `evidence/I-0128/artifacts/build-r8b1` (the runs'), `build-r8b2` and `build-r8b3` (the step reviews' answers), `build-r8b4`
  (the final review's, the merged tree's); `build-r8a1` compiled while
  its sources changed (its startup test failed): disposable.
- Runs `evidence/I-0066/artifacts/i128-r8a1-*`, `i128-r8a1b-*`, `i128-r8a2-*`, `i128-r8b1-*`, `i128-r8b1b-*`; logs `runs-r8a1.log`,
  `runs-r8a2.log`, `runs-r8b1.log` in `evidence/I-0128/artifacts`.
- R8c: builds `build-r8c1` (failed: windows.h's `far`), `build-r8c2` (the runs'), `build-r8c3` and `build-r8c4` (the reviews' answers); runs
  `i128-r8c1-*` (the measure), `i128-r8c2-m40-*` (the equality), logs `runs-r8c1.log`, `runs-r8c2.log`; the gi-mean dumps disposable
  once the figures above are kept.
- The fill lights: builds `build-fill1` (rt-gi-fill.flag), `build-fill2`, `build-fill3` (the merged tree's); runs `i128-fill1-*`
  (`-explore` the piloted walk, its shots `s00` to `s07`), `runs-fill1.log`, `explore1.log`, `pictures-fill1/`; their raw frames
  (`*.rgba`, `velocity-*`) disposable. The gi-mean dumps and raw frames are disposable once the figures
  above are kept.

- R8d: builds `build-r8d1` (the first, not run), `build-r8d2` (the slot's), `build-r8d-nopath` (the throwaway, a worktree
  `r8d-nopath` at `b3ce0991` with HitRadiance's PathFrom call removed), `build-r8d3` (the `_path` builds), `build-r8d4` (the final
  tree with main merged); `Run-R8dSlot.ps1`, `runs-r8d1.log`, `Analyse-R8d.sh`, `compare-r8d1.txt`, `blocks-r8d/`, `stability-r8d1.txt`,
  `display-luma-r8d1.txt`; runs `i128-r8d1-*`, `i128-r8d1n-*`,
  `i128-r8d2-*`, `i128-r8d3-*` (their bursts, raw pictures and gi-mean dumps disposable once these figures are kept).

## Uncertainty / unresolved questions

- Which lights reach nearly the whole grid at the High Wall (15,475 of 16,384 cells), and why its traced passes gain less (R8c).
- ReSTIR GI's local residual of 1 to 2 % (R7d's, unchanged by R8d; INFERRED the hits' specular, the cache's ramp, the visibility margin).
- NRD's displayed frames 0.8 steps darker than RR's with the same GI estimate (R8d's L12: not the seed; NRD's output or DLAA against RR).
- A still interior view other than Firelink's shrine: Anor Londo's hall by teleport moves the camera (R8d).
- The RT level's 0.7 ms regained by the `_path` builds: INFERRED from the throwaway build, not measured on the merged tree.
