# I-0081 result: the stutter while moving (second tranche)

Complete (2026-10-21). The residual of I-0079 (about 11 frames over 34 ms in 80 s of the Firelink walk against the original's
1 to 3) is mostly the game's own work at an area crossing, met once per walk cycle, plus a native share that this tranche
reduced: two registry locks no longer make the engine thread wait on the loading threads. Interleaved walks give, against main,
40-56 frames over 25 ms instead of 58-71 and 22-36 frames held two refreshes on screen instead of 34-51; the sprint the same.
The measurement points were not comparable: the game's own step, measured at the same engine point in both renderers, already
has the original's 1-3 frames over 34 ms. Thomas's lead (the development instrumentation) is refuted by measurement. The video
memory does not leak over zone travels; the 2.2 GB at Firelink is the walk's working set, which the former main reached too.

## Method

The I-0079 path and tools (Eta's save at the Firelink shrine bonfire, walk Z 4 s / S 4 s, sprint, camera turn, 80 s of motion
after a 40 s settle, measure mode without the debug layer, checked in each run's `baseline.json`; `Run-StutterSeries.ps1`,
`Get-Stutter.ps1`). Added: the mode `NativeNoK` (the collect timers off); [Get-StutterSummary.ps1](Get-StutterSummary.ps1), which
also reads the game step at the world update gate (`game_frame_times`, the same engine point under both renderers, absent from
the original's probe runs) and the frames the display held two refreshes or more (`native_present_refreshes`, the swap chain's
`SyncRefreshCount` between consecutive presented frames); [Run-ZoneLoop.ps1](Run-ZoneLoop.ps1) (the Shebang zone save, each
character loaded from the title, `Pilot-ZoneTour.ps1 -Loops` with a screenshot before every click). A profile run needs its
end mark (`name:Native:walk:2300-8000`): without it the sampler never reaches its 75,000 samples and writes nothing (i81-01).
The validation runs use a build folder without the pipeline library (below).

## Observations

1. **The slow frames come once per walk cycle.** In every walk run (I-0079's included) the frames over 34 ms are about 592 frames
   apart (the cycle Z 4 s + S 4 s with the pilot's start-up), at the same point of the path, where the journal shows an area
   crossing: NPC models initialised, the GPARAM banks of area 141000000 and environment regions admitted. The streamed
   creations churn with it: about 833 placed and 6 committed creations per 600 frames while the live placed bytes stay at
   1,024-1,029 MB (i79-23); the committed ones are the resources over the 8 MB placement bound (model storages of 10.3-10.7 MB,
   textures of 11.2 MB).
2. **What fills those frames** (the sampler, i81-07, 13 frames over 34 ms, 203 engine-thread samples; the whole run 50,568 for
   about 4,500 frames). DS3's task join 23C1DA4 is not an idle wait: the joining thread runs the frame's tasks itself (71 of
   its 84 samples have native frames above it: the collect, the UI and render step, the effect update). Per slow frame against
   the run's average: the collect 4.4 samples against 2.8, DS3's own step outside the join and the pacer 6.2 against 2.5 (the
   game's crossing work, which the original does too), the pacer's spin 2.2 against 3.0. Inside the collect, the models
   entering at the crossing: `AcquireNativeModelPrograms` 17 samples in the 13 frames against none listed over the run.
3. **The measurement points** (table below). The native counts are taken at the native frame end, the original's at its present
   pacer D5CB0 (I-0079). At the engine point both share, the native game step has the original's 1-3 frames over 34 ms: the
   presented intervals of 34-37 ms are game steps of 25-34 ms shown one refresh late. The original's game step at that point
   was not measured (its probe runs install no other hook); its frames over 25 ms at its pacer are 11.
4. **Thomas's lead, the instrumentation.** The collect timers off moved the counts within the run-to-run spread (i81-03: 61
   over 25 ms against 71 and 71). Over a whole profiled run (i81-07) no journal write and no timer is among the engine
   thread's or the render worker's listed samples (`Log` 6 and `WriteFile` 12 samples on one loading thread of 252,840; the
   timers 0.5 % of the recording pool's samples). Not a cause.
5. **Video memory** (i81-19, measure mode, five zone loads from the title: Undead Settlement m31, Dreg Heap m50, Ringed City m51,
   Irithyll m37, Ariandel m45). Local memory 1.67-1.80 GB in a zone, 1.57-1.67 GB back at the title, no upward trend; 19-20
   placed heaps throughout, two released at each zone change. At Firelink the walk climbs to 2.17-2.23 GB and stays level in
   every native build (i81-16/17/18); the former main with committed resources (I-0079 run 03) also climbed during the walk,
   1.30 GB before it to 2.18 GB at its end. I-0079's « 2.2 against 1.5 GB » compared the new build after the walk with the old
   one in the middle of it: corrected here. Ten loads were asked and five happened: after a quit the next zone's notices were
   sometimes not up when clicked (screenshots), every other zone.
6. **The validation layer and the pipeline library.** A debug-layer run on a build folder whose pipeline library was written by
   measure runs stopped at its third frame: `ID3D12PipelineLibrary::StorePipeline: A pipeline with name ... already exists`
   (i81-20), an error under the debug layer, so the run faults. It belongs to the pipeline cache (I-0072), not to this tranche;
   the validation runs here use a folder without the library (i81-26, i81-29). Open lead for the DX12 audit.

## Corrections kept (D-0195: ER for each)

- **Static buffers created with no registry lock** (`AdmitModelStorage`, `14_scene_step_and_storage.inc`): the buffer (placed,
  or committed over 8 MB) and its copy are created with no `model_storage_lock` held, the insertion alone exclusive; the
  admission is counted from its closed check, and Close closes the registry, waits for the count, then drains. Before, the
  creation ran under the exclusive lock that the collect's model selection takes for every model. ER: its texture and buffer
  holders are created on the loading threads and published to the frame by a swap (I-0073 §1); the textures were already
  created outside their registry lock natively (`TextureIngress::AdmitOwned`).
- **The layout registry** (`model_layout_lock`, `01_state.inc`, `18_resource_admission.inc`): a `std::shared_mutex`, the lookups
  shared (the collect's record checks and the admission workers take it per mesh), and `BuildModelLayout` finds an equal layout
  through an index keyed by its identity instead of scanning every layout under the lock (about 64 builds a second while
  walking), its journal row written after the lock. The other pure lookups of the storage registry are shared too.
  ER: no analogue traced for this registry (the layouts are DS3's own resolver cache).

## Tried and removed

Preparing a new layout's programme choice on the loading thread (`WarmNativeModelPrograms`, after ER's programme objects built
when the shader archives finish loading, I-0072 ER-PIPELINE-STUDY 9). It took `AcquireNativeModelPrograms` off the engine
thread and halved the collect in the crossing frames (i81-10: 2.2 samples a frame instead of 4.4), but the interleaved walks
were worse than without it (75 and 82 frames over 25 ms, 42 and 45 held refreshes, against 49 and 40, 27 and 22): removed,
with the step review's guard against an unsettled loose-programme absence that it needed. The reason was not isolated
(INFERRED: the loading thread's work at the crossing delays the game's own loading tasks the engine thread joins).

## Measurements (80 s of motion, measure mode)

Frame end >17/25/34 ms, the game step at the world update gate >17/25/34 ms, frames held two refreshes or more.

| Run | Build | Motion | frame end | game step | held 2+ |
|---|---|---|---|---|---|
| i79-02, i79-13 | original (pacer) | walk | 625/11/1, 623/11/3 | - | - |
| i79-23, i81-02, i81-13, i81-16 | main | walk | 898/71/11, 1057/71/11, 1037/66/10, 864/58/9 | 724/58/1, 763/65/3, 718/56/3, 746/52/1 | 34, 51, 39, 34 |
| i81-03 | main, timers off | walk | 1013/61/9 | 721/57/2 | 40 |
| i81-05, i81-06, i81-14, i81-17 | i81b (locks) | walk | 936/54/10, 917/48/10, 920/49/9, 1021/40/7 | 700/45/2, 720/41/1, 671/38/2, 684/38/0 | 31, 28, 27, 22 |
| i81-08, i81-09, i81-15, i81-18 | i81d (+ programme warm) | walk | 924/70/11, 867/75/7, 999/82/10, 933/75/9 | 733/68/2, 717/71/1, 722/65/1, 709/63/1 | 49, 39, 45, 42 |
| i81-21, i81-22 | i81e (locks, shared lookups) | walk | 958/48/10, 894/56/8 | 692/46/3, 700/42/1 | 28, 30 |
| i81-27, i81-28 | i81f (final) | walk | 866/53/11, 983/52/9 | 696/42/3, 749/41/2 | 29, 36 |
| i79-18 | original (pacer) | sprint | 688/11/3 | - | - |
| i79-21 | I-0079 e (≈ main) | sprint | 1091/66/11 | 775/48/2 | 40 |
| i81-23 | i81e | sprint | 915/58/10 | 716/50/1 | 25 |
| i81-25 | original (pacer) | turn | 81/0/0 | - | - |
| i79-19, i81-24 | I-0079 d, i81e | turn | 782/2/1, 296/3/2 | 46/2/1, 27/3/2 | 2, 2 |

Validation: i81-26 (i81e under the debug layer, the walk) 0 error over 8,559 frames; i81-29 (i81f) 0 error over 8,362 frames.
Unit controls pass (`Build-GameBootstrap.ps1`, builds i81b to i81f).

Optimisation pass (WORKFLOW « Optimise, do not stack »): the kept changes remove work (a scan per layout build) and waits;
nothing is read or decoded twice and no diagnostic was added beyond one row for a storage dropped at Close. The engine-thread
phases in measure mode over the walk (`native_present_pacing`, frames 3,600-8,300) are unchanged: collect 3.91 / 3.88 ms on main
(i81-13, i81-16) against 3.91 / 3.95 on i81f (i81-27, i81-28), prepare 1.95 against 1.95-1.97, record 1.27-1.28 against
1.28-1.30; the gain is in the waits behind the loading threads at the crossings, not in the phases' own cost.

## Reviews (Opus 5.5, D-0198)

Step review (after i81d): no High. M1 (exclusive lookups of the storage registry left on the engine thread's path) fixed: shared;
M2 (the warm could keep a programme choice whose loose programme was linked just after) guarded, then removed with the warm;
M3 (D24430 called from the loading thread) removed with the warm; Lows: the rebuild-collect double build (removed with the
warm), the programme lock held during the preparation and the profile counters (pre-existing, unchanged), the silent index
miss at a layout's destruction and the late admission at Close (recorded below). Final review: no High. M1 (a shared hold
during the creation still holds new readers back once an exclusive request queues, SRW lock) fixed: no registry lock during the
creation, the admission count waited by Close; M2 (`Run-ZoneLoop.ps1`'s trace budget) fixed with the measured zone cycle and a
refusal past 900 s; L3 (a Close between the creation and the insertion ended the process) fixed: the admission is dropped
and logged; L4 (a failed second insertion of a layout) left: an allocation failure there ends the process through `Refuse`
before the dangling entry can be read; L5 (the records) updated.

## Uncertainty / open

- Run-to-run spread is large (main 58-71 frames over 25 ms); four interleaved walks per build carry the decision, Firelink only.
- The crossing frame stays one refresh late in most cycles: DS3's own step there exceeds a refresh (observation 2). Bringing
  it under would need the engine thread's frame shortened elsewhere (the collect's base cost, the engine thread toward 120
  frames/s, I-0078's third tranche) or a deeper presentation queue; both are other workstreams.
- The original's game step at the shared engine point is not measured (a probe with the world update gate only would give it).
- The pipeline library under the debug layer (observation 6), for the DX12 audit.
- `Pilot-ZoneTour.ps1` misses every other zone after a quit (observation 5).

## Artifacts

Runs `evidence/I-0066/artifacts/i81-01..29-*`; builds `evidence/I-0081/artifacts/i81{a..f}-build` (i81a = main's module, a copy
of I-0079's `i79f-build`), series logs `series-1..6.log`; profiles `i81-04`, `i81-07`, `i81-10` (`sample-profile-slow-report.txt`).
