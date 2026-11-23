# Wave 1 final control review (Fable 5.1, high effort, D-0188): log and findings

Thread « Revue finale Fable V1 », branch `claude/charming-mclean-efca04` from `main` cbf0fad0, 2026-10-18 from 19:10 Paris.
Brief: [FABLE-REVIEW-PREP](FABLE-REVIEW-PREP.md). Thomas (2026-10-18 19:45 Paris) added one instruction: check the native
architecture against D-0157 explicitly (the D3D12 renderer must not merely reproduce D3D11's historical operations and
constraints; name the engine responsibilities that should be owned or restructured natively to exploit D3D12: resources,
commands, synchronisation, GPU lifetime, parallelism; separate real architectural limits from fidelity gaps; no further
redesign now: the corrections wave 1 needs, and the optimisations for the next workstream). Principle (Thomas, 19:47):
« Reproduire fidèlement le résultat de D3D11, oui. Reproduire ses limitations architecturales, non. »

## 1. Diagnoses (OBSERVED unless marked)

### 1.1 The darker, duller native picture at Crucifixion Woods (m33)

- Presented pictures Native2161 / Reference2160 (reflections off, same pose), mean sRGB per region: sky 1.00, far trees
  0.84-0.89, lake 0.87, character 0.73, near bark 1.12, near ground 1.00-1.12 (native / original).
- The fog records are equal word for word in both modes (`native_fog_set` 2161 against `reference_fog_set` 2160, key
  [133000000, 0], map position (-241.06, -236.62, -311.369)); the light-set records are equal too; the native light-key list
  lacks the set [133000000, 200] the original lists (7 entries against 6): the m33 MSB gives light set 200 to 706 of its
  1,422 objects (`UnkT07` in SoulsFormats 1.0.1 = the object's LightSetID; map pieces use 0/5/6/31/20/11/220/30/32/300/301,
  the collisions 100).
- **The lit HDR matches.** Reference2173 (bonfire, `yebis-tonemap-capture`, `yebis-capture-late`) against Native2174
  (`world-pictures`, frame 3600, same pose) with `Compare-SceneHdr.ps1`: whole 1.07, sky 1.00, far 0.96-0.99, near 1.05,
  character 1.02, ground 1.00; only the bonfire's flames region differs (1.35, effects). So the scene lighting is the
  original's; the gap is after it.
- **The post chain differs at m33.** The original's YEBIS frame runs `tech_MadColorGradingLutT1_S1` after the tone map
  (Reference2173 draws 42/43) and its tone-map constants carry exposure 2.3 (`reference-tonemap-tone.raw`); the native
  globals say `exposure 1.52, filmic_contrast 0.3, grading 0` and the grading pass never runs (`native_grading_frame
  frames: 1`). The native YEBIS keys (view context +0x3FD8/+0x4040 slots) are [133000000, 0] for every group.
  The native scene holder applied by D52B70 is `stage+0x500` with tokens [133000000, 0] (`native_drawparam_holder`).
- Next: the original's context keys (new row `reference_light_recording.gparam_keys`, light-observe.flag), then the writer
  of the holder's per-category keys.

- **Cause found (Reference2177/2180, Native2174, the recovered m33 gparam files).** The original's view context keys are
  [133000000, 0] for all 13 slots too (`reference_light_recording.gparam_keys`, main view), and its scene holders are the
  native's (own holder unset, slot 500 tokens [133000000, 0]): the key is not the difference. The records are: every m33
  file that carries a Tone Map group (m33_00_0000, 0010, 0200, 0210) sets Exposure 2.3 for id 0; the live layers at the
  Iota save are m33_00_0010 and m33_00_0110 (0100/0110 are partial overlays without Tone Map, Bloom or grading groups);
  default.gparam's Tone Map id 0 is Exposure 1.52, FilmicContrast 0.3, which is exactly what the native fed
  (`native_yebis_globals`). `Banks::GroupEntries` looked only at the area's *bound* file (MapOverride_0000 at m33, bound
  last), then the area's own file by name (m33_00_0000, not loaded at this save), then mapbase, then default: it never
  consulted the live kind-7 layers that the light, fog and sun lookups already use (`Layer()`, the m45 rule). Fix in
  light_producers.cpp: the layers first, latest first, the first whose group holds the id; then the former chain. The same
  lookup serves the grading (hence no LUT natively), the glare, shafts, DoF and SSAO records.
- **Witnessed (Native2183 against Reference2156, the bonfire, same pose).** With the layered lookup the native picture
  equals the original's region by region (mean sRGB native / original): whole 1.01, sky 1.00, far trees 1.00 and 1.00,
  near bark 1.00, ground 1.00 left, character 1.00, centre 0.99; the bonfire's flames region 1.08 (effects, as at every
  zone). `native_yebis_sources` names m33_00_0010 for Tone Map, Bloom, LightShaft and ColorGrading, m33_00_0110 for
  Setting Info; the grading pass runs (`native_grading_frame`). The gap of D-0188 is closed; the lake pose and the
  other zones are re-checked in the final side by side.
### 1.2 The Shrine Handmaid's chair (o404854), white natively

- MTD `M[ARSN]_e` = `M[ARSN]` with `g_AlphaRef (128, 0)` (dump of `mtd_AllMaterialBnd`, `Dump-AllMtd.ps1`): a cutout
  material (the chair's back is a lattice). The native row took recipe 1 (no alpha test): `ClassifyModelMaterial` returns
  4 only when `policy.alpha_ref` is nonzero, so the policy read 0 for this material. The white is the texture's masked
  texels drawn opaque. Cause of `alpha_ref 0`: open (the SPX declaration rule or the authored value); the named-draw row now
  prints b3 and the policy (`ps_b2_b3_b4`, `policy`).
- **Re-read (Native2185 row, Native2188 / Reference2189 pan).** The row's policy is `alpha_ref 128, class 4`, pixel variant 1
  (`[A]`), `cbMtdParam` word 20 = 0.50196 (FC_AlphaRef): the alpha test is active and the lattice's holes are open. What is
  white is the wood itself: the diffuse `m40_00_chair_01_a` (1024², BC1 sRGB, the FLVER's own path resolved through the
  engine's registry), specular `Reflectance_055_055_055_r`, normal `m40_00_chair_01_n`, PS b4 colour words (1,1,1,1). The
  original's draw of the same row is not captured by name. **Open**: next, read the resident texture the native sampled (a
  readback of the record's resource, or the TPF entry of the map's texture bank) and the original's SRVs for this draw.

### 1.3 Thomas's play of 13:24 (hitches)

- `%LOCALAPPDATA%/DS3-DX12/play-20261018-132357.jsonl`, `native_present_pacing` / `native_frame_times`: the slowest frames
  coincide with pipeline-state creation on the frame path (`pso_created` 13 -> 88: 5.36 s at the world's first frame; -> 112:
  1.61 s; -> 149: 1.29 s; -> 138: 0.97 s) and with the loading phases (draws 0, 0.4-0.8 s). Steady play holds 60 frames/s
  (p99 18-22 ms) between those. Cause: PSOs are created at a material family's first draw (design review R9 open).
- **Measured (the `native_present_pacing` windows; means over the run's windows, loads included):**

  | Run | windows | fps mean | slowest frame max / mean per window | gpu_wait ms mean / max | collect / prepare / record / present ms | PSOs created on the frame path |
  |---|---|---|---|---|---|---|
  | Thomas's play 13:24 (m33, 1920x1080) | 65 | 50.2 | 5,360 / 339 | 0.05 / 0.1 | 8.28 / 3.05 / 2.49 / 0.22 | 163 |
  | Native2159 lake (1280x720) | 36 | 53.3 | 3,487 / 294 | 0.40 / 0.6 | 4.32 / 2.39 / 9.13 / 0.47 | 59 |
  | Native2195 lake, reflection view | 29 | 45.9 | 3,541 / 374 | 0.43 / 0.8 | 7.38 / 2.43 / 7.41 / 0.44 | 59 |

  Reading: the GPU never waits on the CPU's fences in play (gpu_wait under 1 ms: the two frame slots are enough), the CPU
  frame is the collect (the engine-thread world walk and preparation, 4-8 ms) plus the record; the long frames are the
  pipeline-state creations (163 in a play of 20 minutes, up to 5.4 s at the first world frame) and the loading phases. No
  costly synchronisation hides in steady play; the costs are architectural (§5 axes 1-3, §6.2).

### 1.4 The loading screen's edges at 1920x1080 windowed

- The window client is 1920x1061 (`native_window`): Windows clamps the 1080-high window to the work area. The native
  renders and maps the Scaleform movie viewport to the client (`native_ui_movie_viewport` 1920x1061, scale 1), so the
  16:9 loading plate is fitted with side margins. The original keeps the configured 1920x1080 as the buffer and GX target
  and DXGI scales it into the client (MENU-TESTS-PREP §4: the engine's size S is the configured one; the native takes the
  client once at start). The general rule is ROADMAP item 3's; see §3.
- **Corrected (Native2187, 1920x1080), as a general rule.** The renderer starts at the engine's configured size S
  (`image+0x46601E8`, whatever the resolution or mode) whenever S is valid and differs from the client rectangle, and the
  flip-discard swap chain scales that extent into the client (`native_output_extent` configured [1920,1080] client
  [1920,1061]; `native_ui_movie_viewport` 1920x1080). The in-game and menu plates fill the window (`title-1080.png`,
  `ingame-1080-2.png`). The pilot scales its measured 1280x720 coordinates to the client rectangle (`Pilot-TitleContinue`),
  so runs at any window size can be driven; `Run-ZoneWalk -LoadShots` shoots the loading screen.

### 1.5 The Crucifixion Woods cross (Native2157 / Reference2156)

- Not an MSB object within 15 m of the bonfire (only vegetation objects o331204-o331208 there; the objects' table has
  1,422 entries); most likely a map piece. The native walk refuses nothing by bounds at m33 (`native_main_walk_outcomes`:
  385 objects, 184 models, 56 zero-fade, 9 missing bindings, 2 unsupported).
- **Cause and correction (Reference2175/2186 `reference_draw_entity`, Native2187).** The cross is the object `o000499`: the
  original emits it in the main pass with bounds all zero (its E+0xB80 box was never written; MENUS-MAP 62 section 2, "the
  constructor does not initialise B80"), and the native walk culled it by that zero box. Rule added to the admission: an entity
  whose eight bounds words are zero is not culled by them (the shadow walk keeps its box test: a zero box casts in no cascade).
  The cross is drawn natively at the bonfire (`title-1080.png`, right of the fire, the red hollow on it).


### 1.6 Characters refused as zero fade at m33 (found by the cross hunt, Native2178)

- The terrain piece `m33_00_00_00_000705` (the one whose bounds enclose the cross's place) is drawn entire natively (five
  materials, `native_named_draw_sources` complete, G-buffer DOL family), as 000713: the cross is not a map piece.
- The native walk refuses whole character families at the bonfire as `zero_fade` (`native_character_candidate`, Native2178):
  c1441 x8, c2271 x6, c1370 x3, c1360 x2, c2020, c2150, c1446; admitted: c2270 (one), the objects. The m33 MSB places
  c1370 (cross-bearing hollows) and c2271 (small crabs) around the bonfire and in the lake: the cross and the small object
  of the lake are most likely these refusals.
- `zero_fade` = `SelectModelFade` finds `blend x weight <= 0` for the geometry passes; the weight is the LOD controller's
  geometry weight E+0x170 (MENUS-MAP 62 section 3: moved by D1CCE0 toward 1 while the distance to the manager's camera,
  times the manager's scale, is under the LodParam draw distance). The original draws them at the same place, so either
  the manager's camera or scale differ natively, or the native reads a weight the original does not use for these
  entities. Row `lod_step` (both modes, light-observe.flag) compares the step's inputs; row `native_zero_fade` prints the
  fade words of each refused model.
- The two globals 0x1446402E0/E4 are constants (the Ghidra listing shows only reads): D52B70 applies the scene holder's
  per-category keys when the holder's tokens are unset (-1, -1) and otherwise writes every slot from the holder's single
  key +0x188. Natively the applied holder is `stage+0x500` with tokens (133000000, 0), so every YEBIS slot takes id 0.
  Row `reference_scene_holders` (shadow-observe.flag) reads the original's holders.

### 1.7 The lake's planar reflection (WT-6/WT-7; Native2159 / Reference2158)

- With ReflectionQuality DISABLE both match (Native2161 / Reference2160); with the normal settings the original shows the
  trees and sky reflected in the lake and the native a black t54 (WATER-TESS-WAVE1-READY 3.3: WT-6/WT-7 were the open rungs).
- **Implemented on this branch** (sheet 1.5 and 3.3, the first version; opt-in while witnessed, on by default since D-0189 with
  `no-water-reflection.flag` as the opt-out): the main collect
  takes the plane of the nearest water mesh (policy bit 4; the owner's box centre, n = +Y; row `native_water_plane`), walks
  the world again through the mirrored camera F.pose.M (the Y flip keeps the winding and the main raster state) with the
  gather mask low byte 0x10 (INFERRED to be DrawByReflectCam; the walk falls back on the main union 0x07 when it admits no
  model, the row `native_reflection_view` says which) and the view's own six planes (the engine's culler is the main
  camera's), drops the water rows (group 0x39), lights the scene as the main view (no sun shadow, no AO: the menu-view
  path), and the frame job draws it before the frame through `RenderNativeOffscreen` into an RGBA16F target of half the
  output extent, the lit HDR copied (no composition), bound as t54 by `AdmitNativeWaterSlots`. The plane goes to every
  instance's `userClipPlane` (identity context) and to `cbSceneParam` row 4. Known departures of this version: no sun shadow
  and no kept draws in the reflection view (prepared each frame); the plane is the box-centre height (the MSB
  `ReflectPlaneHeight` default and the original's scalar `t` are not read: WL3/WL11).
- **Witnessed (Native2195 against Reference2158, the lake teleport, `lake-walk2.png`, `lake-sheet.png`).** Rows: the plane
  y = -261.6 from `m33_00_00_00_000012` (one candidate, 27.8 m); the walk with the low byte 0x10 admits 248 objects / 180
  models / 328 rows, lit, 640x360, the mirrored eye 6.7 m under the real one; no failure. The lake shows the trees and the
  light of the far shore mirrored as the original's; the native reflection reads a little brighter and sharper than the
  original's (no sun shadow in the view, and the original's surface blurs its sample with the normal map's distortion, which
  the same shader does natively: the brightness difference is the lighting of the view, INFERRED). The small crabs of the
  lake (c2271) stay the known zero-fade residual (1.6). Cost at the lake: collect 7.4 ms against 4.3 ms without the view
  (`native_present_pacing` means, Native2195 / Native2159), prepare unchanged (2.4 ms), record 7.4 against 9.1 ms (sampling noise).
  Native2194 (the first attempt) faulted on the debug layer's clear-value check of the lit HDR (the view cleared it to
  0,0,0,1 while its creation value is zero): the clear is the creation value now.
- **After the pre-merge review (1.8): Native2196 against Reference2158 (`lake2-walk2.png`).** With the mirrored culler the walk
  admits 159 objects / 70 models / 137 rows (180 models before: the main frustum's set), no fault; the lake's reflection now
  reads as the original's (the trees mirrored at the far shore, the surface no brighter than the original's). The crabs stay
  the 1.6 residual.

### 1.8 The D-0176 pre-merge review of this branch (Opus 5.5, 2026-10-18 21:30-21:50) and what it changed

Eight findings, all applied (commit "Pre-merge review (D-0176) applied"):

1. The reflection's clip plane was written in world space while the vertex stages compute `dot(SV_Position, plane)` in clip
   space (DECALS-PAINT-READY's clip-distance line): now `q = VP^-1 . plane` goes to cbSceneParam row 4 and the instances take
   the mirrored view-projection as their clip context (the producer inverts it), the world plane as the vector.
2. The reflection walk ran with `inside = 1` on the main camera's culler: map pieces were never frustum-tested by the mirrored
   view and the tree pruning followed the main frustum. The walk now classifies through a copy of the culler holding the
   mirrored planes (+0x160, the inside sign read as the shadow walk does) and the main box mirrored across the plane.
3. t54 could bind the reflection target before any view had drawn it, and kept binding it after the scene went away: the
   record is handed out only while a scene is published and a frame job has drawn the target (black otherwise).
4. The reflection collect's point-light pass reached `CollectNativeLocalShadows`, which resets the engine's local-shadow
   container for a camera that fails its check: it returns early under the reflection collect.
5. The dominant-light row cache (keyed by the banks object and the four keys) could serve stale rows after a kind-7 layer or
   area binding changed with the same keys: `Banks::BindingGeneration()` counts every binding change and keys the cache.
6. Folding "all rotation images present" into `ao_frame` also turned the sun shadow off when a later rotation image was
   missing: `ao_frame` is as before and only the AO plan the key carries says "no AO" then.
7. `GroupEntries`' layered lookup labelled its source with the layer file's name; a layer not named `mAA_BB_NNNN` would have
   been refused by the grading resolver where "area" was accepted: such layers keep the "area" label.
8. Smaller: `catch(...)` on the two `noexcept` reflection collects, a reflection extent over 4096 skipped (the offscreen bound),
   `%g` in the `lod_step` and `reference_draw_entity` rows (a huge float and `sprintf_s` terminate the process), the D1CCE0
   LOD hook installed only with `light-observe.flag`, `all-groups.flag` honoured by the reflection walk.

Verified correct by the review: the mirror math and its acceptance by `BuildSceneView`, the frustum extraction, the HDR
path's resource states and the early return's guards, the shared state under its mutex, the flag-off path (t54 black, no
clip override), the output extent rule, the zero-bounds rule, the F1/dominant/try-catch/Yebis/measure_run fixes.

## 2. Corrections made on this branch

- `game_bootstrap.cpp`: the colour-group reader no longer indexes `colours[0]` on an empty vector (reviewer F1).
- The Crucifixion Woods cross: zero-bounds entities are not culled by their box (1.5).
- The output extent: the engine's size S at any resolution, scaled into the client (1.4).
- The m33 picture: the bank group lookup consults the area's live kind-7 layers first (1.1).
- The water reflection view, opt-in (1.7).
- The per-receiver tallies on the engine's parallel pose and task paths (`ObservePartPoseUpdate`, the public pose update,
  `LogChrEzTask`: a global mutex and a map per call) run only with `pose-observe.flag`.
- Witness rows: `native_named_draw_sources` prints PS b2/b3/b4 and the policy; `reference_light_recording` prints the view
  context's 13 gparam key slots.

## 3. Open items and decisions for Thomas

Open after this review (none blocks the merge; each has its next step):

- **The Handmaid chair o404854** (1.2): white natively with the alpha test active; next the resident texture the native sampled
  and the original's SRVs/constants for that draw (a named draw capture on the reference side).
- **The water reflection** (1.7): **on by default since D-0189** (Thomas, 21:58, after the lake witnesses Native2195/2196
  against Reference2158); `no-water-reflection.flag` opts out. To correct next: the mirrored view's missing sun shadow; its
  collect cost joins the optimisation axes (1.7 departures).
- **D2 character paint** (blood on struck NPCs): coded earlier, no live witness yet (needs Thomas striking an NPC natively).
- **A resolution change in game**: the native refuses a window extent change since its start (pre-existing); the optimisation
  workstream owns it with the swap-chain restructuring (§6.2).
- **Zero-fade characters at m33** (1.6): pooled instances at the origin in both modes, not a defect; the scene holder tokens
  (133000000, 0) and the YEBIS slot keys are documented, the picture now matches (1.1).
- **Hitches** (1.3): diagnosed (pipeline states created on the frame path, loading phases, witness readbacks); axis 1 of §5.

Decisions: the reflection on by default (D-0189, taken); for Thomas, the order of the §5 axes for the optimisation
workstream (proposed: 1, 2, 3 first).

## 4. Code review (native/) findings, verified or to verify

Three Sonnet 5.5 readers (renderer/; game_bootstrap.cpp 1-9000; 9000-end and the other frame/*.cpp; content/ and the
production producers of world/ still running) read every file in full; I keep here what I verified or what is anchored to
quoted code. Line numbers are those of `main` cbf0fad0 unless noted.

### 4.1 Corrected on this branch (wave 1)

| Where | Defect | Correction |
|---|---|---|
| game_bootstrap.cpp ~7951 | `colours[0]` read on an empty vector when a colour group started and the material's colour array was absent (a crash on the engine thread) | guarded (`!colours.empty()`) |
| game_bootstrap.cpp ~9996 | `std::copy(keys,...)` swallowed by a line comment: the dominant-light cache never hit, `DominantAt` ran twice per collect and per menu view | the statement restored on its own line |
| game_bootstrap.cpp ~11644 | one row whose `PrepareNativeModelDraw` throws (a pipeline the device refuses, a binding out of range) faulted the render worker and `Refuse`d the process | caught per row, counted (`native_model_draw_prepare_failed`), skipped, as the decal rows |
| game_bootstrap.cpp ~13192, ~13600 | the menu offscreen views' 20th/200th draw and the first movie's frames 240/2400 were read back and written as RGBA files in every run, play included | only outside a measure run |
| game_bootstrap.cpp 2796, 4439, 2689 | per-receiver tallies with a global mutex and a `std::map` on the engine's parallel pose path and the EzTask path, on by default | only with `pose-observe.flag` |
| scene_pass.cpp 2292 | the plan key's `ao_frame` did not test the 16 rotation images that `AoStage::Record` requires: a frame whose images arrive later than its key would make the executor throw (`the frame's passes differ from its plan`) and fault the session | the key tests them |

### 4.2 Verified, left for the next workstream (not wave-1 blockers)

- **Renderer, GPU drains inside a frame** (scene_pass.cpp 2542, 2566, 2584, 2617, 2622, 2628, 1281, 1311, 1324): a DoF or
  motion-blur level-count change (the lens), a glare technique change, a LUT view change or the AO mask views flipping drain
  the queue mid-frame; with two frames in flight that is a bubble of a frame or more. Root: shared shader-visible heaps
  rewritten in place. Remedy: per-slot descriptor ranges (the effect stage already does it) and retirement by ticket.
- **PSO creation on the frame path** (model_draw.cpp 481 at prepare; effect_stage.cpp 146/436/456 and scene_pass.cpp 1172 on
  the render worker; primitive_pass.cpp 277 under the UI cache lock): the cause of the play hitches of §1.3. Remedy:
  `ID3D12PipelineLibrary` on disk plus creation at material admission (design review R9); the UI cache lock must not be held
  across `CreateGraphicsPipelineState` (the pool deleters take it from the worker's slot release: a stall by design).
- **Witness diagnostics on by default** outside measure runs (scene_pass.h 281: back-buffer readback and CPU hash every 120th
  frame, velocity readback every 30th; game_bootstrap.cpp 15837 debug layer on): the launcher's measure.flag turns them off,
  the witness runs pay them (their frame-rate rows are not play numbers).
- **Effect stage** (effect_stage.cpp 198, 284): descriptor-region exhaustion drops GPU particle work with a counter only, and
  scene_pass.cpp 2509 swallows a mid-record exception (state tracking may be left half-advanced). Remedy: grow per slot, fault
  on exception.
- **Frame pacer** (frame_pacer.cpp 32-37): hand-paced at 1/60 s on refresh rates that are not multiples of 60 (144, 165 Hz:
  judder), no tearing/VRR flag, unthrottled when occluded; `RefreshRateOf` queries the monitor every 120th present. Wave 2
  (D-0169) owns this.
- **UI path** (game_ui_session.cpp 100 and the owners): `std::terminate` on any Scaleform family not yet witnessed (blend modes
  outside the seen set, fill kinds, texture formats other than R8/RGBA8/BGRA8/BC1-3/7). A menu or DLC screen using one ends the
  game. Remedy: skip and count where no state can corrupt.
- **UI texture store** (primitive_textures.cpp 47/56, game_ui_images.cpp 75-104): the whole image set deep-copied and the atlas
  re-uploaded per glyph update; `PrimitiveTextureWrite` already carries the dirty rectangle.
- **gparam banks** (game_bootstrap.cpp 14776-14808): every admit/retire rebuilds all banks under `gparam_lock`, which the
  collect takes each frame; build outside the lock, swap under it.
- **Paint queue** (game_bootstrap.cpp 9889-9948): an entry of an unknown type keeps the queue from draining (the erase never
  runs; the 4096 cap then blocks every later paint), types other than 0 are dropped without `consumed()`, a paint whose
  texture is not yet resident is consumed and lost. To verify live with Thomas's D2 witness.
- **Worker prepare reads live engine memory** (game_bootstrap.cpp 11796, 11837, 11918: the YEBIS globals block 0x4961F70
  for the glare, DoF and motion-blur plans) while the engine thread's next collect rewrites it: a torn or one-frame-newer
  block. Remedy: the collect's own snapshot (`yebis.globals`) carried in the scene. **Corrected on this branch** (see 4.1 once built).
- Smaller: `Refuse` (process exit) on transient per-frame data (non-finite bone or placement, an unreadable list node,
  `bad_alloc` in the walk) instead of skipping the model; `RetireWhenSubmittedWorkCompletes` reads an unguarded global
  queue pointer; `Close` does not reset the lighting/env/SSS objects (a reopened device would use the old device's);
  change detection by raw pointer for the AO rotation and sun dither tables (ABA); `sprintf_s` with unbounded `%s` of
  exception texts at six sites; the first-walk `std::vector<bool>::at` and the XOR entity/model key of the shadow walk.

### 4.3 Known departures from the original (kept, documented)

The readers' tables are consolidated here; each is a fidelity gap, not an architectural limit: forward rows without IBL,
SSAO or local lights (t30/t31 black cube, t33 white, t44 zero, empty light buffers); spot lights counted but not lit;
local-light shadows opt-in and without consumer; env regions without the view bucket/frustum test and capped at 64; the
temporal dither phase fixed (one of 16); tessellation flat without `native-tess.flag`; glow mode A never drawn; light
shafts only with the DoF branch; shadow record always id 0 of the area; characters' sun cascades from a fixed 2 m box;
no depth occlusion history; FaceGen marks not drawn; post-transition intent never consumed; AA types 1-3 (FXAA) not
native; colour grading only for 16x256 strips of map records; vignette rectangle checked at 1280x720 only; glare shape 1
only; motion blur refused outside mode 2; forward blend modes other than 0/2 not drawn; forward light-grid words zero;
POM light direction zero; parallax-corrected cube boxes only when both box flags are off; gamma fixed at 2.2 (+0x94
zeroed); wind bit only once the effect system is published; SFX requests before the effect system are not replayed; effect
draws with a geometry shader or a sampler other than slot 9 dropped; the YEBIS host's 3D LUT creator returns E_FAIL (opt-in
host only); character light-probe blending omitted; the output extent fixed at start (no resize, no full screen).

### 4.4 Content and producers (the fourth reader), verified items

- **A material policy failure drops every mesh of the material silently**: `PrepareModelMaterialPolicy` returns false for
  kinds 5, 8 (`g_DepthBias`), 13 and above 41 and for any unsupported parameter (game_bootstrap.cpp ~6287); the prepare then
  gates the mesh with code 9 and no row names it. Remedy: a partial policy keeping kinds 6, 7, 16, 19, 20 and the flags, and
  one row per family.
- **"Unsupported" generated-parameter outcomes are discarded without a row** (`ReadModelGeneratedParameters`: register below
  0x27, a duplicate GXMD record, a provider descriptor of type 0, a component past the row, an unknown row type); the SAT
  `snp_*` floats and damaged-normal UVs of such a material stay unproduced.
- **`ProduceMtdParams`** (producers.cpp 226-249) is a name table with invented defaults (colours 1, everything else 0;
  `g_DiffuseColor`, `g_ScatterFactor`, `g_IOR`, the Falloff/Flowmap/Ghost rows have no rule) and its `unknown` result is
  ignored by every caller: a variable the shader uses is then refused or zero-filled, never authored. Remedy: map by the
  builtin table's vector index and scalar offset.
- **Zone-load cost, O(N squared)**: each texture admission decodes its whole TPF container to extract one entry
  (game_bootstrap.cpp 12374-12378, two more full copies in texture_container.cpp 23/110); each gparam admission or retirement
  re-parses every gparam file with a DCX inflate under `gparam_lock` (`Banks::Load`, light_producers.cpp 1212-1230).
- **Parser strictness ends the process** (every `Refuse` is `ExitNative(0xD3126606)`): a DDS legacy format outside the listed
  set (DXT2/4, L8, A8, R5G6B5), a DX10 format without a `PixelBytes` entry (64, 67, 68, 69), a duplicate SPX parameter name,
  a non-finite MTD float, an odd TPF float-struct size. For visual-only assets the right outcome is skip, log, placeholder.
- Smaller: two copies of the same SIMD cosine table disagree by one hex digit (decal_constants.cpp 35 against
  model_common_textures.cpp 18; the correctly rounded value is `0xAD49CBA5`, the term is below 1e-10); the TPF float-struct
  length field is read as bytes where SoulsFormats reads a float count (masked in production by the engine's own constant);
  `model_textures.cpp` 36 maps an unknown texture kind to 0 and 132 aliases slots at or above 32; `model_vertex_layout.h` 35
  does not force the instance step rate to 0 for per-vertex elements; unguarded divisions in yebis_tonemap.cpp 43 and
  decal_constants.cpp 168/430.
- Not in the production module (dead for the backend, kept as hosts or tests): shader_bank, material_programs,
  material_admission, cpu_ingress, adapter, controller, host; `effect_producers.cpp`; the Snapshots half of
  light_producers.cpp (about 900 lines); `gx_replay.cpp`; `ProduceMatDyn`, `ProduceGrass*`, `ProduceModelParam` in
  producers.cpp. No test covers `ProduceModelPose`, `ValidateModelPaletteIndices`, the sampler bank decoder, the texture
  tiling producers, `ComputePaintProjector`, the generated/damage/wet parameter readers, `NativeLegacyDdsByteCount`, nor an
  Int2 `g_AlphaRef` through the policy.

## 5. Optimisation axes (ranked, not implemented; D-0167/D-0169, for the optimisation review)

Evidence: Thomas's play journal (section 1.3), the m45 profiles of STATE, the readers' line anchors (section 4).

1. **Pipeline states ahead of the frame**: create at material admission (the collect knows the pass rows then), keep an
   `ID3D12PipelineLibrary` on disk between runs, prewarm the effect, light-batch and UI sets. Removes the 1-5 s first-frame and
   streaming hitches of every play session (pso_created 13 -> 88 -> 163 in the 13:24 journal).
2. **No GPU drain inside a frame**: per-slot descriptor ranges for the stage inputs (glare, DoF, motion blur, grading LUT, AO
   masks, sun tables) and retirement by fence ticket instead of `Drain`; the `Close`/`Resize` drains stay.
3. **Barrier batching and the frame graph**: one `ResourceBarrier` call per pass boundary (the executor already batches; the
   hand path makes 135 single-barrier calls), redundant RT->SRV->RT pairs removed, the graph shadow's full recompile per frame
   reduced to the first frames of a key then a sampled check.
4. **Copies**: the full-resolution ping-pong copies (DoF back-copy, motion blur, AA, the decal GB0/GB1/depth copies, the water
   depth copy, the scene copies) replaced by pointer swaps and region copies; upload ranges not zeroed twice.
5. **Collect and prepare on the engine thread**: the pass decisions cached per model record (11 passes x models x fades x
   meshes re-evaluated per collect), `ProduceNativeSelectedInstances` cached per (group key, bank epoch), the shader-repository
   rescans replaced by one name index per registry generation with negative caching, the `.rdata` DXBC scans indexed once,
   the pose update's lock held only for the publish (five allocations and three palette copies per update today).
6. **Loading**: the TPF container parsed once per container, the gparam banks built outside the lock and swapped, the
   material/layout builds hashed by stem, texture ingress rows read in one block.
7. **UI**: dirty-rectangle uploads on a persistent atlas instead of whole-set copies per glyph update; programmes interned
   by id; per-frame validation once; the state filtering the G-buffer path already has.
8. **Threading**: a copy queue for texture streaming (today ingress lists ride the direct queue ahead of the frame), secondary
   command lists for the cascades, the G-buffer halves and the UI, a deeper render job slot.
9. **Instrumentation**: per-pass timestamp queries on the pass names the graph already carries (no GPU timing exists today), so
   the next review tunes from measured GPU cost.
10. **Smaller**: per-frame allocations on the render thread (`shared_ptr` copies per draw, `std::function` per completion),
    the dominant-light and palette caches hashed, the effect capture bookkeeping gated by a flag, the Bink waits, the 88 MB
    executable read at startup replaced by a bank-only read.

## 6. Architecture against D-0157 (Thomas, 2026-10-18 19:45-19:58: reproduce D3D11's result, not its architectural limits)

ER references for this reading (added 2026-10-19 at Thomas's request): the method is [study716](ER-ENGINE-D3D12-DEEP-STUDY-716.md) and the [D3D12 design review](D3D12-DESIGN-REVIEW.md) (E1-E12 -> R1-R16); ER gives no studied witness yet for the effect system, the job/streaming system or a bindless binding model (ROADMAP wave 2 lists what is missing).

### 6.1 What the native backend already owns (natively, as D3D12 should)

- The device, queues, swap chain (flip model, three buffers, waitable), two frame slots with fenced retirement, the upload
  arena per slot, the descriptor heaps per view and per stage, the frame graph that plans and applies the barriers of a
  keyed frame, the render worker that records off the engine thread, the kept draws (persistent tables per slot).
- The scene selection (the engine's own CPU walk and culler), the pass rows, the material programmes from the engine's banks,
  the constants producers (the engine's own builders for lights, fog, instances), the lighting, SSS, env regions, decals,
  paint, water rows, the YEBIS family as native passes, the UI and movie paths.

### 6.2 Where the structure still copies D3D11 one to one (architectural, to restructure; none blocks wave 1)

| Responsibility | Today | Native D3D12 shape |
|---|---|---|
| Material binding model | each draw owns a 284-descriptor range (14 CBV + 128 SRV per stage, bounded CBVs to keep D3D11's read-past-the-end-returns-zero), copied one descriptor at a time at prepare (model_draw.cpp 18, 267-281, 521-531) | a per-material table shared by its draws, root CBVs per draw, `CopyDescriptors` with ranges; the SM5.0 registers fix the slot numbers, not the per-draw copies |
| Stage inputs | one shared shader-visible heap per stage, rewritten in place, hence the drains (4.2) | per-slot ranges, retirement by ticket |
| Effects (FFX) | a statement-replay layer: the GX draw side's D3D11 calls captured and re-encoded (Dispatch/Copy/ClearUav/DrawIndirect, D3D11 buffer description words, hidden UAV counters emulating append/consume, state banks decoded back) (effect_stage.h 34-42, 122-128) | the only translation layer left in production; acceptable for wave 1 (D-0185/D-0186 chose it over transcribing thousands of lines), to be replaced by native particle encoders in the remaster |
| Pipeline states | created at first use on the frame path, no library, no async | section 5 item 1 |
| Queues | one direct queue; ingress copies ahead of the frame on it | copy queue with a fence; compute for the particle simulation |
| Command recording | one list per frame on one worker; a depth-1 job slot | secondary lists per pass family; a deeper queue |
| Presentation extent | the engine's size S taken once at start (this review, 1.4); `Resize` exists and has no caller (a resolution change in game is refused); 60 Hz hand pacing | S followed on every change (the engine's resize path driving `Resize`), the pacer per refresh rate (D-0169) |
| Engine responsibilities not yet owned | the scene gparam keys of the view context (1.1: the native now reads the area's live layers itself), the planar reflection view (1.7: native, opt-in, without the engine's plane state), the output mode (full screen), the shader cache | engine-side state the native must own or drive; not D3D11 limits |

### 6.3 Real architectural limits against fidelity gaps

- Architectural (D3D12 use): the per-draw descriptor copies, the shared stage heaps with drains, the frame-path PSO
  creation, the single queue and single list, the fixed extent, the pacer. Each costs frame time or robustness, none
  changes the picture.
- Fidelity (the picture): the section 1 gaps and the 4.3 departures; each has a precise cause in the engine data or a pass
  not yet native, and none needs a renderer redesign.
- Robustness (silent defects): the process-exit `Refuse` on transient data and strict parsers, the UI `std::terminate` on
  unseen families, the silent policy/parameter drops, the effect-stage silent skips. These are the audit's silent defects:
  the remedy is the same everywhere: skip the asset or row, count it, name it in one row.
