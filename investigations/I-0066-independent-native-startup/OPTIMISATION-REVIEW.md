# Optimisation review of wave 1 (Fable 5.1, 2026-10-18 onward)

Workstream opened after the Fable final control review ([FABLE-REVIEW](FABLE-REVIEW.md) §3, §5, §6), on Thomas's choice of
Fable 5.1 at high effort (coordinator brief, 2026-10-18 22:51 Paris). Branch `claude/brave-feynman-ad7120` from `main` e89d7844.
Principle (Thomas, 2026-10-18): « Reproduire fidèlement le résultat de D3D11, oui. Reproduire ses limitations architecturales,
non. » Gains are proved by measurements with the instrumentation off (`measure.flag`, no debug layer) and the picture checked side
by side; nothing touches the game clock (D-0167). One Opus 5.5 pre-merge review (D-0176) before the merge into `main`.

Order: the complementary check Thomas asked for (§1), then the §5 axes 1-3 of the review (§2-§4), then the §3 open items (§5).

## 1. Complementary check (Thomas, 2026-10-18 22:59): the darker 3D models at Crucifixion Woods (m33)

Thomas's observation on the D3D11/D3D12 comparisons: the character and the large near objects look slightly darker natively.

### 1.1 Is the gap real? (OBSERVED, from the review's own pictures, same pose, same exposure)

Mean sRGB per region of the presented pictures (desktop screenshots of the 1280x720 window, `Compare-ShotRegions.ps1`
in the scratchpad, regions in window coordinates), native / original:

| Pair (pose) | character | character's back | near rock | tree trunk | far trees | lake | ground |
|---|---|---|---|---|---|---|---|
| Native2183 / Reference2156 (bonfire, resting) walk-1 | 1.00 (lin 1.02) | 1.03 (lin 1.06) | n/a | bark 1.00 | 1.00 | n/a | 1.00 / 1.09 (fire) |
| Native2196 / Reference2158 (lake teleport) walk-2 | **0.92 (lin 0.81)** | **0.84 (lin 0.68)** | **0.93 (lin 0.88)** | 1.04 | 1.03 | 1.07 | 1.01 |
| Native2196 / Reference2158 walk-3 | **0.92 (lin 0.80)** | **0.87 (lin 0.70)** | **0.92 (lin 0.87)** | 1.00 | 0.96 | 1.03 | 1.03 |
| Native2195 / Reference2158 walk-2 | 0.92 | 0.87 | 0.90 | 1.06 | 1.04 | 1.03 | 1.04 |
| Native2161 / Reference2160 (lake, before the tone-map fix) | 0.77 | 0.69 | 0.91 | 1.12 | 0.89 | 0.95 | 1.19 |

So the gap is real and local: at the lake pose the character (its back 0.68-0.70 in linear light) and the near rock (0.87) are
darker natively while the map (trunk, far trees, lake, ground) matches within the fire and water noise; at the bonfire pose the
character matches (1.00-1.03). The review's tone-map correction closed the global part (far trees 0.89 -> 1.00) and left this one.

### 1.2 First divergence (OBSERVED in the traces of the same runs)

- The original's light entries at the lake (`reference_light_entries`, call 3600, Reference2158): 7 sets, keys
  {100, 5, 0, **200**, 31, 10, 11} of area 133000000. The native's (`native_light_set`, Native2196): 7 sets, keys
  {0, 100, 10, 11, **220**, 31, 5}: **220 where the original has 200**, no 200.
- Light set 200 against 0 and 220 (`reference_light_set` records, 64 floats): set 200's sun colour (0.892, 0.907, 0.895) at
  strength 2 and a second light (0.650, 0.673, 0.716) at 1; set 0's sun (1, 0.839, 0.706) at 0.85 and 220's at 0.85: a model
  lit by set 0 or 220 instead of 200 receives roughly half the direct light, which fits the 0.68-0.88 linear ratios measured
  on the character and the rock (INFERRED until the per-model keys say which set each takes).
- The m33 MSB gives light set 200 to 706 of its 1,422 objects (review §1.1); the key block a model draws with is read from
  its draw group (+F8 light a, +100 light b, +108 t, +110/+118/+120 fog) in both modes, so the next question is which models
  carry 200 in the original and what the same models carry natively.
- Not the G-buffer (albedo, normals: the bonfire pose matches), not the post chain (the global regions match).

### 1.3 Per-entity keys (OBSERVED, Reference2208 / Native2206 at the lake, `light-observe.flag`)

Rows `native_model_light_key` (the collect's group) and `reference_model_light_key` (the draw state 57A20 reads), logged when
an entity's block changes. Reference2208 (the first 2,000 rows, keyed by model): the lake's rocks **o331204, o331205, o331206,
o331207, o331208** carry light set **200** on some entities and 220 on others (pass id 69, the same model logged at each
switch between its entities); the map pieces m33_00_00_00_331003/331004 carry 5, 0 and 220 by entity; the character parts
(FG_A_7002…) carry 100 in both modes, from -1/-1 at the title. Native2206 (keyed by model, the 2,000 rows spent on the two map
pieces' switches): 5, 0, 220, 100, 31, 10, never 200 (the o3312xx entities are never admitted by the native main walk; their
original rows are at pass id 69, outside the G-buffer passes: not the picture's rock, which is a map piece).

### 1.4 Cause found (OBSERVED, Native2211 / Reference2212, the rows keyed by entity with the instance ambient rows)

The darkening is not the light set index: the character parts carry key 100 in both modes. It is the **instance ambient rows**
(cbInstanceData +0x80..+0xAF, the three rows the model programmes add as ambient light, for draw parameter types 0, 3, 4:
characters and objects, not the lightmapped map pieces' lighting). The native collect reads them from the engine's own lookup
32A80 on `[root]+41E0` (the record's +0x290), as the I-0063 host did; natively the engine's parameter banks never receive a
file (`NativeGparamRecords`, 1173), so the lookup answers its default record (`table+0x700`) whose rows are **zero** for every
entity (149 of 149 at Native2211), while the original's rows are **[0, -0.0973, 0, 0.1527] three times** for every key of the
zone (0, 5, 10, 11, 31, 100, 200, 220; the default record's are [0, 0, 0, 0.25]). In the engine the rows are written by
14BA90 at the record's build from a file: `1495F0(record, record+0x290)`, a pure function of the set's ambient words (record
+0x220..+0x26F: the two ambient colours, their scales and a packed colour): the difference of the two colours in the y row and
their sum in w. At the bonfire pose the character matched within 1-3 % because the ambient term is small against the bonfire's
point light; at the lake, lit by the sun and the sky only, the missing ambient is the 0.61-0.70 linear ratio measured on the
character's back and the 0.86-0.94 on the near rock.

Correction (coded): `NativeInstanceLightRows` builds the key's record from the native banks (`Banks::Sun`, the record layout
`NativeGparamRecords` uses) and runs the engine's 1495F0 on it, cached by (area, id) and the banks' binding generation; the
collect takes these rows for keys a and b (the blend and the strength unchanged); `engine-instance-rows.flag` keeps the engine's
rows (A/B). To witness: the native rows equal the original's at the lake, then the picture side by side (the character and the
rock back to 1.00), then the bonfire pose unchanged.

**Witnessed (Native2213 against Reference2212, same pose):** the native rows equal the original's word for word (two states:
the default record's [0, 0, 0, 0.25] before the area's file is bound, then [-0, -0.097299, -0, 0.1527]); mean sRGB native /
original: character 1.01 (linear 0.97), its back 0.96 (0.87), the near rock 1.00, far trees 1.00, lake 0.99, ground 1.03;
no debug-layer message. **Open:** the tree trunk at the right (980-1260, 40-520) went from 1.00 to **1.22** (43 against 35
sRGB): a map piece whose instance rows were zero before (as the original's effect) and now carry the set's rows. The
original's 5B3D0 writes the rows only for draw parameter types 0, 3 and 4 (`*param_6 == 0 || *param_6 - 3 < 2`, the same
test as the native `type==0||3||4`), and that type is the selected LOD variant (`geometry_fades.intents[i].variant`, the
value `SelectOrdinaryModelLod` takes; 122-148 models a collect carry a non-zero one at the lake), the same on both sides. So
either the native selects another LOD than the original for that map piece (a per-entity row of the variant in both modes
decides), or the trunk's native programme recipe is not the original's lightmapped one (STATE: the lightmap delivery of the
baked map pieces is still open), so that the ambient rows light natively a surface the original lights by its lightmap;
with zero rows the two happened to match in mean. To check before any restriction of the rows. The character's back at
0.96 may be the same term through its second key or the blend; the bonfire pose is to re-check.

**Resolved (Opus, 8.3):** the open trunk was the ownership of the instance rows, not the LOD or the recipe: in the original the
in-world draws take their anim block's own probe rows (q+0x170 owned; the map pieces' rows zero), which natively no one
computed. With the native SH owner and the probe layout byte (8.3) the owned blocks carry the original's rows and the trunk is
back to 0.99-1.03; the light-set rows of this section stay only for the blocks the original lights from its bank (not owned).

## 2. Axis 1: pipeline states off the frame path

### 2.1 The pipeline library on disk (coded, to measure)

`PipelineStore` (backend.h/.cpp, `Pipelines()`): an `ID3D12PipelineLibrary` opened with the device from
`native-pipelines.library` next to the module (a file the driver refuses, another adapter or driver version, is replaced), asked
by a stable 128-bit name (`MixPipelineName`: the programmes' content hashes and the fixed-function state, never an address):
`LoadGraphicsPipeline` / `LoadComputePipeline` when the library holds the name (the driver's compiled object, no compilation),
`Create…PipelineState` then `StorePipeline` otherwise; `Save` serialises the library when pipelines were stored (a sibling file
moved over the library), from the pacing window on a detached thread and at Close before the device goes;
`no-pipeline-library.flag` keeps direct creation (A/B). Sites on the library: the model draws (`model_draw.cpp`, the key string
without its programme entry addresses, each `CachedProgram` hashed once), the effect stage's draw, particle and compute pipelines
(their key strings), the point-light pipelines (the two programmes' bytes), the primitive pass (its key: programmes, elements,
blend and stencil rows). The stages' fixed pipelines (AO, glare, tone map, …) are created at open, not on the frame path, and
stay as they are. Rows: `native_pipeline_library` (open, file loaded, bytes), `native_frame_audit.pipeline_library` [loaded,
created, stored, saves], `native_pipeline_library_close`; `pso_created` now counts the pipelines created (a library hit is not one).

Expected: the second run of a zone finds its pipelines in the library (no creation on the frame path: the 1-5 s hitches of
§1.3 gone from the second play on); the first run unchanged. The creation ahead of the frame (at material admission) is the
next step, for the first run.

### 2.2 Creation at admission (next)

## 8. Pre-merge review (Opus 5.5, 2026-10-19 00:38-00:47) and what it changed

Five defects, all corrected on this branch (compiled; the control suites and a live witness remain, the game being the other
session's): (1) the effect stage's library names were built from engine object addresses (`NativeEffectProgram::key`, the
layout's address), not stable across runs: now the programmes' bytes, the layout's elements, the bank rows and the target
formats; (2) the graph comparison sampled by the global recording count, which alternates between the main and the reflection
views: now each cached plan counts its own executions; (3) two preparation threads could store the same pipeline name (the
second store refused and reported by the debug layer): `Known` is checked again under the lock before the store; (4) the DoF
table and its uploads were released at once on an extent change: retired by ticket; (5) the ambient-row cache held a raw bank
pointer and missed `SelectVariant` (which changes the file `Sun` reads without a binding change): the bank is held by
`shared_ptr` and `SelectVariant` bumps the binding generation at each variant write. Risks taken: a version in the library's
file name (`native-pipelines.v1.library`, to bump when the root signatures or the fixed states written in code change);
`Save` serialises and copies the names under the lock and writes the files outside it; the glare stages renew their tables on
a frame-input change too (`WriteDrawInputs` writes the fixed and the known frame inputs into the fresh heap); 1495F0 writes
into a 16-byte-aligned buffer; the run scripts' stale-flag list and effect-flag pattern know the three new flags and
`-BuildFolder` is resolved to a full path. The control suites then found two more defects of axis 2 (T15, the DoF stage on WARP, five zones): without a retirement
queue (the stage controls drive the stages with their own queue) `RetireWhenSubmittedWorkCompletes` released a renewed heap at
once under the list still referencing it (now kept for the process's life when no queue is registered, in a list never
destroyed, since the production path always has its queue); and `WriteDrawInputs` viewed the frame inputs from the stage's
markers, which a second zone without a relayout left pointing at the first zone's released scene and depth (the debug layer
crashed in `CreateShaderResourceView`): the frame inputs are now the caller's current resources, and a target generation
change forgets the other recorder's markers (a released target is never viewed again; after a resize the two recorders renew
twice, then settle). Left as documented: retirement at `LastSubmitted` relies on each stage recording
once per list (true today; a second recording in the same list would need the slot's kept list instead); a sampled-out
frame can run up to 63 frames on a cached plan whose uses changed under the same key; the names index is written after the
library (a failed index move leaves an older index: the next store of such a name is refused, nothing worse).

## 7. Sessions sharing the PC (2026-10-18 23:20-23:35)

A second session, « Optimisation V1 Opus » (branch `claude/sleepy-jackson-22f240`), runs the same workstream's load-profile
investigation on this PC at the same time: its build of 23:23 replaced the shared module
(`evidence/I-0066/artifacts/game-bootstrap-build`), so Native2203 ran its module (no `native_model_light_key` rows), and the
two launches of 23:24 collided (controller exit 6: the control mapping already existed; Native2202 lightkeys / Native2202
loadprofile). Remedy on this branch: `Build-GameBootstrap.ps1 -Out` and `-BuildFolder` on `Run-StartupTrace.ps1`,
`Run-ZoneWalk.ps1` and `Run-LiveWitness.ps1` (the module, the controller and the flag and witness files of a session in its own
folder; this session builds in `game-bootstrap-build-fable`); the game itself stays exclusive (one DarkSoulsIII), announced to
the other session by message. Thomas decides whether both sessions continue.

## 3. Axis 2: no GPU drain inside a frame

### 3.1 The drains and what they protect (OBSERVED in the code)

| Drain (`WaitPostLayout`) | What is rewritten in place while an earlier frame may still read it |
|---|---|
| `BindNativeMaskViews` (scene_pass.cpp) | the lighting views heap: slot 10 of every pass block (t32) and the t33 view |
| `ao_.NeedsTables` | the AO stage's SRV heap (every table: depth, normal, rotation images, history targets) |
| `sun_.NeedsTables` | the sun shadow stage's SRV heap (depth, atlas, dither, second mask, mask) |
| `dof_stage_.NeedsLayout`, `glare_family_.NeedsLayout` (twice), `motionblur_stage_.NeedsLayout` | the glare stage's surfaces released (`surfaces_.clear()`) and the `PostDraws` SRV tables rewritten (`SetInput`) |
| `t1_changed()` | the tone-map stage's t1 view (slot 1 of its 4-entry heap) |
| `grading_.NeedsLutView` | the grading stage's LUT view (slot 2 of its 4-entry heap) |

Steady play drains none (Native2196: 0 a frame); each happens at a change (the first AO frame, the lens, a zone's LUT, a
reopened stage), as one bubble of a frame or more with two frames in flight.

### 3.2 Remedy (coded, to measure): versioned heaps and retirement by ticket

Coded: `HoldObject` (backend) holds any COM object for `RetireWhenSubmittedWorkCompletes`; `AoStage::WriteTables` and
`SunShadowStage::WriteTables` write into a fresh heap and retire the former; `GradingStage::Record` (a LUT change) and
`TonemapStage::Record` (a scene or glare view change) rebuild their four views into a fresh heap and retire the former;
`PostDraws::RenewTables` gives the glare stages a fresh table heap at each `Layout`, whose former surfaces are retired by ticket;
the lighting views keep a CPU mirror of their fixed part (`light_views_cpu`), and `BindNativeMaskViews` writes the mask views
into the mirror, copies it into a fresh shader-visible heap and retires the former (the env cube ranges stay per frame slot in
the shader-visible heap). The nine `Drain(WaitPostLayout)` calls of scene_pass.cpp run only under `post-layout-drains.flag`
(A/B; row `native_post_layout_drains`). To witness: `drains_by_site` stays 0 at the first AO frame, a lens change and a zone
change; the picture unchanged; no debug-layer message.

A shader-visible heap is never rewritten while it may be in flight: on a change the stage creates a fresh heap, writes every
view into it from the state it keeps (the sources, its own targets), binds the fresh heap, and hands the old heap (and the
released surfaces) to `RetireWhenSubmittedWorkCompletes`, which releases them after the queue passes the work submitted so far
(the previous frames). The frame being recorded uses the fresh objects only. Heaps are small (4 to 150 descriptors; creating
one costs microseconds) and the changes rare, so no per-slot duplication is needed; the env-region cubes already use per-slot
ranges (`frames_.CurrentIndex()`), which stays. The `Resize` and `Close` drains stay (every view and target changes at once).

## 4. Axis 3: barrier batching and the frame graph

### 4.1 The graph shadow's comparison, sampled once a plan is verified (coded, to measure)

Before: every frame, executed by a cached plan or not, recorded each use (`GraphShadow::Use`, about 100 a frame), compiled
its plan again (`CompileFramePlan`), planned its barriers (`PlanFrameBarriers`) and compared every use state at `End` (Native2196:
450,065 uses over 4,471 frames) on the render worker, inside the frame's record.

After: a cached plan counts the executed frames whose full comparison found no mismatch (`CachedPlan::verified`); once eight
have, the comparison runs on one executed frame in sixty-four (`frames_%64==0`) and the other frames record no use and compile
nothing (`Use` returns after the undeclared check; `End` checks the end states only, which still evicts the plan on a mismatch).
The first frame of a key, the frames without a cached plan and the eight verifying frames are unchanged. Control T-E3b
(`frame_graph_test.cpp`, WARP): 100 frames of one key, the first compared and cached, 99 executed, 10 compared, 90 sampled out,
each executed frame recording the plan's barriers, no mismatch, no eviction. Rows: `ComparedFrames`, `SampledOutFrames`.

First witness (Native2206, the lake, debug layer on, witness rows, axes 1-3 active): `graph_shadow` 13,713 recordings, 0
mismatches, 57,794 uses compared against 450,065 in Native2196 at 4,471 recordings (about 500 a window instead of 24,000);
`graph_barriers` 130 transitions in 55 calls a presented frame, as Native2196's steady windows (126 / 54: the main view and the
reflection view, about 63 transitions a recording); the hand `barrier_calls` 6 a frame; `drains_by_site` 0 everywhere (axis 2);
the pipeline library created and stored 130 pipelines (first run, 1.46 MB written, 21 saves); 47.8-53.9 frames/s against
43.9-49.9 in Native2196, record 11.0 ms against 11.3-12.2 (witness runs: not play numbers; the measure runs decide).

### 4.2 Barrier calls (to measure)

Native2196: 17.55 graph transitions in 7.42 `ResourceBarrier` calls a frame, plus 6.56 hand-written calls a frame on resources
the plan does not declare (82 sites). The hand path (`hand-barriers.flag`) is the A/B only.

## 5. Open items of FABLE-REVIEW §3 (pending)

## 6. Baseline numbers (witness runs, debug layer on; not play numbers)

Medians of the in-game pacing windows (`Get-PacingMedians.ps1`, the first three windows skipped), the lake teleport pose,
1280x720, debug layer on, witness rows (`light-observe.flag` on the later runs):

| Run | windows | fps | slowest ms | collect | prepare | record | gpu wait | drains |
|---|---|---|---|---|---|---|---|---|
| Native2196 (before this branch) | 16 | 51.5 | 33.6 | 5.81 | 3.56 | 11.00 | 0.61 | 0 |
| Native2211 (axes 1-3, observers) | 17 | 49.9 | 36.8 | 5.34 | 3.69 | 11.47 | 0.65 | 0 |
| Native2213 (plus the ambient rows; the library warm: 108 loaded, 0 created) | 17 | 50.9 | 36.0 | 5.48 | 3.66 | 11.56 | 0.64 | 0 |

Reading: no steady-state change at this pose within the witness noise (the axes address the first-frame and streaming
hitches, the mid-frame bubbles and the comparison's CPU, not the steady frame; the per-entity observer rows cost their own
logging). The play numbers (measure.flag, no debug layer, the library warm against `no-pipeline-library.flag`, a zone change
for the drains) remain to be taken.

Native2196 (lake, 1280x720): fps 43.9-47.6, record 11.9-12.2 ms, collect 5.9-7.1 ms, prepare 3.7-4.5 ms, `pso_created` 59,
`graph_barriers` 17.55 transitions in 7.42 calls a frame, hand `barrier_calls` 6.56 a frame (untracked resources),
`drains_by_site` 0 except 0.01 (fixture) a frame.

## 8. Opus session (branch `claude/sleepy-jackson-22f240`, 2026-10-18 23:00 onward; the coordinator's owner of the review)

The Fable session's commits (sections 1-7) were merged into this branch (d788b914) after it stopped on Thomas's word.

### 8.1 The play hitches are the collect, not the pipeline states (OBSERVED)

- Thomas's play of 13:24 (`play-20261018-132357.jsonl`): in the window of the 5.36 s frame the model pipeline creation costs
  0.56 ms a frame (the `model_prepare_programs_validate_pipeline_tables_ms` split) while the engine-thread collect averages
  74.8 ms; 716 textures arrive in that window.
- Native2200 (m33 load, measure, `sample-profile-load.flag`: the sampler from the first world frame): 3.4 s slowest frame; in
  the first 11 s the owner thread spends 3.5 s in `AcquireNativeModelPrograms`, 2.9 s of it in `ds3reads::AnalyzeConservative`
  (D3DDisassemble) and 0.6 s in D3DReflect: the interface and read analyses were rerun for every layout selecting a programme.
- Correction: `AnalyzeNativeProgram` keeps both per programme content (hash, then byte comparison), refusals kept as refusals;
  `FindLoadedProgramme` keeps found programmes until a shader admission or retirement and absences for 250 ms;
  `DecodeCpuTextureContainerEntry` validates the whole TPF but copies one entry (texture admissions read 15.2 GB of
  containers per m33 load, on the loader thread, never the owner thread).
- Measured (measure.flag, no debug layer, no kprofile, 1280x720, the same save): slowest frame of the load 3,396 ms (Native2200)
  -> 670 ms (2201, analyses 208 instead of 12,606) -> 336 ms (2203) -> 357 ms (2204); steady m33 bonfire 60 frames/s
  throughout. The remaining slow frames are the game's own work on the owner thread and the first world frame's pipeline
  states (Native2205 `native_slow_frame` serial 1183: 11 pipelines, prepare 31 ms, record 45.5 ms).
- Measured and dismissed: the gparam publications (19 a load, 1 ms each, none on the owner thread); the in-frame drains (13 in
  Thomas's 20-minute play, all in loads); the lake reflection view in the play configuration (Native2207: 59.4-59.9
  frames/s; the 45.9 of FABLE-REVIEW 1.3 was a witness run).
- Pacing against the original at 1080p, m33 bonfire (new row `game_frame_times`, the same engine point in both modes):
  Reference2208 p50 16.67, p99 16.85-17.09, stdev 0.07-0.11 ms; Native2210 p50 16.67, p99 16.88-17.31, stdev 0.08-0.29 ms
  (one 33 ms frame in eight windows). 1440p and 4K need DSR (a driver setting for Thomas).

### 8.2 The missing m33 crosses o331204-o331208 (cause and correction)

- Reference2208 / Native2210 (1080p, bonfire): the Celtic grave crosses behind the bonfire are absent natively; every one of
  their candidates is `zero_fade` (disp child, geometry weight E+0x130 = 0). Reference2214 / Native2215 (`lod_step` rows for
  the disp controllers, LodParam 33): the geometry weight is 0 in both modes.
- The original's emits (D04B90 for map objects, D06E10 for draw entities) take the geometry weight only for views of engine
  passes 1..0x38 and the track weight for the main view, of type 0x45; the main view gathers the category-0x01 entities
  (E+0x20 bit 0x01). The native main walk weighed every entity as pass 5. Now: category 0x01 in the main walk -> pass 0x45
  rule (the shadow, reflection and menu walks unchanged; the item's own no-geometry flag kept); `fade-geometry-pass.flag` A/B.
- Witnessed (Native2216 against Reference2208): the crosses drawn. Cost (Native2217 against 2218, kprofile): 47 more models
  (the crosses and the pooled characters c1441, c2271, c1370, c1360, c1446, c2150 at the origin with track weight 0.083, which
  the original emits too, `reference_draw_entity` pass 69), collect 4.9 -> 8.5 ms, 59.8 against 60.0 frames/s; the new
  characters are re-admitted every collect (record outcome `streams`). Lever left: records for stream models.

### 8.3 The m33 darkening: the per-instance SH (cause)

- Reference2222 / Native2223 (lake, `light-observe.flag`, new rows `reference_instance_rows` at 5B3D0 and
  `native_instance_rows`): in the original every in-world draw takes the anim block's own rows (`q+0x170` set): characters,
  parts and objects carry per-object probe SH (w 0.05-0.11), map pieces zero rows. Natively no block was owned: the map pieces
  then took the light-set rows of FABLE-REVIEW's correction (w 0.153; the lake trunk 1.22) and the characters too.
- Cause: the blocks' rows are written by 61950 (a new probe record) and 1AC840 (the blend) only while the old renderer's device
  [GXRS+0x4DD0] exists (TONEMAP-SKIN §10): never natively. Correction (coded): the native SH owner runs both bodies without the
  gate (`NativeShBlock`, constants read from the image, light-set rows from the native banks; `no-native-sh.flag` A/B).
  Native2224: the map pieces owned with zero rows as the original; the characters owned but zero rows: 61950 is always called
  with a null record natively (Native2225 `native_sh_record_set`) although the probe volume is registered in both modes
  (Native2227 / Reference2226 `probe_volumes`: 1 volume, 73,700 probes). Next: why the callers pass no record.
- Why no record (Reference2239 / Native2240, `probe_pick` at D3FFB0): the pick runs for the same draws in both modes (1,930 of
  2,000) and finds 1,930 records in the original, 0 natively. Native2243 / Reference2241: the probe volume's box (volume
  +0x90..+0xAC, the 1D650 overlap test) is the empty box natively and (-223.8, -278.9, -685.6)..(363.6, -202.9, -202.0) in the
  original. F3AA0 accumulates that box only when the global byte 144961ABC is set; its only writer is the GX scene set-up
  D573B0 (from D59E90), never run natively; its readers (F3AA0, F47D0, F5840) are CPU only. Correction: the byte is set to 1 in
  `PublishNativeSceneRoot` as the original has it (no device, no GX object; `no-native-sh.flag` A/B).
- Witnessed (Native2244 against Reference2222, the lake): the volume box equals the original's; 9,367 picks of 9,405 find a
  record; owned characters w 0.106 (c2270, the original's 0.106), parts 0.074-0.077, map pieces zero rows as the original's.
  Pictures (three poses each, the poses drift by a few pixels): trunk 1.00-1.06 (1.22 with FABLE 1.4's rows), near rock
  1.00-1.08, character 0.91-0.98, far trees 0.99-1.07. Native2245 (1080p bonfire, the same pose as Reference2208): static
  regions 1.00, character 1.03-1.06, the crosses region 0.98; 60 frames/s, collect 7.0 ms. The light-set rows of FABLE 1.4
  (`NativeInstanceLightRows`) stay for the draws the original lights from its bank (non-owned blocks, keys a/b).

### 8.4 Witness zones side by side and a regression found (2026-10-19 03:37 onward)

- Reference2246 / Native2247 (m40 Firelink, the zone save, 720p): the candles of the shelves are absent natively, present in
  FABLE's Native2190: the 8.2 rule weighed with the track weight every entity carrying category 0x01, including those also in
  the 0x02/0x04 categories, which reach the original through the pass 5..8 / 9..0x38 views and their geometry weight. Rule
  corrected: the track weight only for entities of category 0x01 alone.
- Native2258-2263 (the six witness zones, that rule): m40 still lacked the shelf candles and the player at the bonfire. The A/B
  flags isolated the probe layout byte of 8.3 (Native2270, `no-probe-layout.flag`: both back), not the 8.2 rule. Cause: the
  probe pick 1D650, which the byte lets find its records, also writes the entity's suppress byte (D3FFB0: draw+0x82D = E+0xA5D
  through the thunk +0x230; D3E240: item+0x71A). The original's emits test that byte only for the pass 5..8 views; the native
  main walk weighed every entity through pass 5 and so honoured it for entities the original draws through the main view 0x45.
  A volume box computed from the probe grids with the byte left at 0 (Native2271/2272) found almost no probe (the grids are not
  transformed) and was dropped.
- Final rule (Native2273 m40, Native2274 the lake): the byte stays 1 as the original's; the main walk draws an entity when either
  of its views would: pass 5 for categories 0x02/0x04 (the geometry weight and the suppress bytes), else the 0x45 view for
  category 0x01 (the track weight, no suppress test). Native2273: the candles and the player back; Native2274: 9,405 of 9,405
  picks find a record, character 0.91/0.89, back 0.97/0.94, near rock 1.08/0.98, trunk 0.99/1.03, far trees 0.99/1.02 against
  Reference2222 (two poses).

### 8.5 The optimisation pass before the merge (Thomas, 2026-10-19: optimise, do not stack; WORKFLOW)

Before (Native2275 m33 bonfire 1080p, Native2276 m45; measure, no debug layer, `sample-profile.flag`, the zone saves): collect
7.52 ms and 14.32 ms a frame; the engine thread 46 % and 70 % in the collect. Redundant work found (the record outcomes and the
sampler):
- the record revalidation (a reused model admitted afresh and compared every 32 collects): 52 of the 61 fresh admissions a
  collect at m45, and no mismatch in the 80 native runs before; now every 512 collects (8.5 s per model at 60 frames/s);
- the water reflection walk admitted every model of its view afresh each frame (9.4 % of the engine thread at m33) although
  the main walk had just admitted most of them: it now borrows the main walk's record of the entity (a copy, checked against
  the reflection's own inputs, never stored back; outcomes in `native_reflection_view` `borrows`);
- the record check copied every reused mesh's 0x4F8 material bytes before comparing them: compared in place (`GuardedSame`);
- the instance light rows took a lock per call (3,500 a collect at m45): a per-thread cache.

After (Native2277, Native2278, the same protocol): m33 collect 6.92 ms, the picture equal to Native2245's against Reference2208
region by region (static regions 1.00); m45 collect 13.06 ms, 48.8 -> 51.7 frames/s, frame time p50 20.3 -> 19.1 ms, p99 22-35
-> 21-31 ms by window (most windows 22-24). What remains at m45 is the per-entity work of the 1,750 models of the view each
frame (fade and inputs reads, record checks, instance rows, the pass preparation over eleven passes: 13 % of the engine thread),
plus the main merges since Native2105 (59 frames/s, collect 11.0 ms with 2,833 draws; 2,917 draws now, the 8.2/8.4 rule): an
unchanged-entity reuse and the threading of wave 2 (D-0191: remove, measure, then parallelise).

### 8.6 Pre-merge review of the delta since 212d82f8 (Opus agent, D-0176, 2026-10-19 06:00)

No blocking defect; threading, lock order, the borrowed reuse against a fresh admission, `GuardedSame` and the per-thread
cache checked. Applied: the revalidation turn mixes the entity address (pooled entities at a fixed stride would share a turn
and make a periodic hitch); the main-view weight witness keeps its count test (light-observe only); the symbolizer gives
`GuardedSame` to its caller and names `within:` in its usage. Left as documented: the reflection and offscreen walks still weigh
pass 5, so an entity whose suppress byte the probe pick set (8.4) is refused there (INFERRED: the original's reflect-camera
view is not a 5..8 pass; possibly the player and the candles missing from the water; to check at wave-1 closure with the other
gaps, D-0191); a borrowed check counts its expiry renewals in the main walk's `renewed` statistic; each borrow copies the
whole record (35-69 models a frame; the wave-2 persistence workstream keeps records shared).

Control before the merge (Native2279-2285, the final build, measure; the six witness zones against Reference2246-2256 and the
m33 bonfire): the region ratios equal those of Native2258-2263 within the zones' own motion (m40 with its candles and player as
Native2273; m31 r0c1 1.07 -> 0.98 and r2c3 1.21 -> 1.11, nearer the original); no regression seen on the board. The reflection
borrows: at m45 97 % of the reflected models reuse the main record (122,240 of 126,000 checks); at m33 the lake reflects mostly
map pieces outside the main view (514,803 checks without a record against 23,393 reused), whose admissions are therefore not
redundant but rebuilt each frame for want of a record of their own: the wave-2 persistence workstream.
