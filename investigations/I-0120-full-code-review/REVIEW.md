# I-0120 — La revue du code entier

Base: `main` `831023b8` (2026-11-02 21:26 Paris; the code of `493e58aa`, I-0119 R3a and R3b, the last merge whose start check
passed). Read-only: nothing corrected, built or run. Brief and lots: [REVIEW-BRIEF](REVIEW-BRIEF.md); the skill's raw pass:
[SKILL-PASS](SKILL-PASS.md).

## Method

1. The session's `code-review` skill at xhigh (21:28): the branch had no diff, so it reviewed the latest native merge
   (`493e58aa`, 15 files): 13 findings (S1 to S13).
2. Eight fresh Opus 5.5 reviewers at xhigh, in parallel, one lot each (A to H: 244 files, 5.0 MB, every production source of
   `native/` but `third_party/`), with one brief (correctness, engine reads, the mandate, optimise-do-not-stack); each finding
   CONFIRMED (the failure follows from the code) or PLAUSIBLE (it depends on a runtime condition).
3. The thread re-read in the code both high findings and most medium ones before recording them (« Thread » column: kept,
   downgraded or refuted, with the reason); a « — » there means not re-read (B4, C3, C5, D4, G5, G6 among the medium ones, and
   most low ones), so the reviewer's status stands alone. None was refuted; E2 was downgraded.
4. A mandate sweep by the thread (D3D11 in the production path).

## Summary

All eight lots were read in full (A to H: 244 files; lot D skimmed only `light_producers.cpp`'s snapshot oracle, which no
production code calls); the reviewers took 16 to 22 minutes and 454k to 587k tokens each. With the skill's 13 and one of the
thread's own (T1), the review holds **83 findings (82 distinct: G4 is A5): 2 high, 28 medium, 53 low**, and **no high is
CONFIRMED**:

- **High (2, both PLAUSIBLE, both a crash under a thread interleaving the code does not exclude):** G1, a menu model freed by
  its retire hooks while the offscreen collect calls engine code on its drawables; C1, the engine's texture registry lookup
  (D24490) called from the admission workers, unguarded, while loader threads insert and erase in that unlocked hash. Neither
  has been seen in a run; each has a cheap structural fix (a busy mark the retire hooks wait on; a guarded native walk or an
  SEH guard).
- **Medium (28: 18 CONFIRMED in the code, three of them with a runtime trigger still open; 10 PLAUSIBLE).** The confirmed ones that touch play directly: B1 (one paint target made or
  retired, on the first hit on each new enemy, throws away every model record of the scene: a hitch), D1 (an effect frame
  dropped on a resize or the DLSS latch loses its releases and first-buffer bytes: GPU buffers leaked until Close, a particle
  buffer later created with zero seeds), D2 (an effect whose texture is not resident is drawn with a null view: a stand-in,
  D-0197), E1 (a reflection context kept for every output size the session has had: VRAM growth on each resolution change),
  C2 (the water's Fresnel stale for ~8.5 s after a Reflection option change), A3 (a texture the decoder refuses closes the
  game instead of being skipped), A1 (the launcher's controller accepts only the default Steam path: a release blocker, not a
  problem on Thomas's PC).
- **Diagnostics active in play without a flag** are the largest family (H1, A5 = G4, B2, B3, B7, C3, C8, D5, T1, and the
  unbounded journal rows on the loading threads in lot A's notes): each is small, together they put strings, maps, atomics,
  guarded reads and synchronous `WriteFile`s on the game thread, the pool and the loaders every frame, against « optimise, do
  not stack ». The play launcher's measure.flag turns the render worker's readbacks off (E2 downgraded) but turns the K-split
  profile on (T1).

Five patterns recur across lots, which a fix should treat as families rather than one finding at a time:

1. **Flagless diagnostics in hot paths** (above).
2. **« Skip the object » applied unevenly**: texture decode (A3), instance and admission reads (B6, lot A's notes), the UI's
   own caps (G5, G6), the asset parsers (lot C's notes) still `Refuse` on game data.
3. **Engine code or engine memory touched from module threads under thread assumptions written only in comments**: C1, G1,
   D3, the raw reads of the cloth copy (A6) and of `effect_system`'s transcribed bodies (lot D's notes).
4. **Invalidation too wide or caches frozen at their first success**: B1, B5 (one global epoch for an input few records read);
   C4, C6, F5 (a first set kept for the session against Native2202's late links).
5. **Stale comments and dead code from earlier waves** (B10, E5-E8, F5-F8, G8, G10, D6, A8, S13): misleading, not harmful.

No D3D11 device, bridge or fallback was found in the production path (mandate sweep below; lot H confirmed the Reference
blocks' gating). The worker, presentation and close sequencing (lot A), the holder rule and retirement by ticket (lot E), the
post stages (lot F), the texture ingress lifetimes and the DDS, TPF and GPARAM parsers' bounds (lot G) were found correct.

## Priorities (the thread's proposal; corrections only with Thomas's agreement)

| Order | Findings | Why first |
|---|---|---|
| 1 | G1, C1 | The only two crash candidates; cheap structural fixes; a thread witness first if the pilot prefers evidence (the in-world scene CPU update's thread against the menu thread for G1). |
| 2 | B1, D1, D2, E1, F1 | Wrong or degraded play: a hitch per struck enemy, leaked GPU buffers and zero-seeded particles after a resize, black effect particles (a stand-in), VRAM growth, and F1's compaction size race (PLAUSIBLE, corruption if it fires). |
| 3 | The flagless diagnostics (H1, A5, B2, B3, B7, C3, C8, D5, T1) | One mechanical pass behind their flags (Sonnet work under the pilot's review); measurable per-frame CPU. |
| 4 | A3, B6, G5, G6, C2, C4, E3, A2 | The skip rule, the stale water Fresnel, the frozen lighting set, the resize refusal, the movie black stand-in. |
| 5 | The rest (low) | With the code they touch, when a workstream passes by. |
| — | A1 | Before any release beyond Thomas's PC. |

## Findings

By lot, each reviewer's finding with the thread's verdict (« Thread »: re-read in the code, downgraded, or why not re-read). Paths without a folder are under `native/frame/game_bootstrap/` for `.inc` files and `native/renderer/` for renderer files.

### Lot A — Module, lifecycle and threading (30 files; 1 050 s, 587k tokens)

| # | Sev (reviewer → thread) | Status | File:line | Finding | Scenario | Fix | Thread |
|---|---|---|---|---|---|---|---|
| A1 | medium | CONFIRMED | `native/frame/game_bootstrap_ctl.cpp:95-96` | The controller accepts only `C:/Program Files (x86)/Steam/steamapps/common/DARK SOULS III/Game/DarkSoulsIII.exe`, compared with the throwing `std::filesystem::equivalent`. | DS3 in another Steam library: `equivalent` throws out of `wmain` (terminate) or returns 5; the launcher can never inject. | Check the image by name and its pinned size or version, or a path the launcher passes; the `error_code` overload. | Re-read: as stated. No effect on Thomas's PC (the path matches); a release blocker. |
| A2 | medium | PLAUSIBLE | `20_startup_phases.inc:394-402` | `NativeUiTextureImport` binds SYSTEX_BLACK for a menu offscreen proxy or a movie frame texture with no native target yet (its row is `native_ui_offscreen_placeholder`): a stand-in (D-0197). | A movie frame imported before `NativeMovieStepInit`, or a proxy never built: that movie or portrait shows black. | Return 0 (the original's optional missing source) and count a skip, or defer the import. | Re-read: the black record is bound as stated (comment 1250: « only for an import before it »); whether the UI image is rebuilt once the target exists not settled. |
| A3 | medium | CONFIRMED (code), PLAUSIBLE (data) | `14_scene_step_and_storage.inc:524-540, 594-597` | Any locate or decode failure of an encoded texture (entry not found, a DDS the decoder refuses, a 2D/cube mismatch) calls `Refuse`: the game closes instead of skipping the texture (FINISH-LINE line 1). | A texture outside the decoder's checks (modded, or a vanilla file not yet met) loaded at a zone or menu ends the session. | `NoteNativeSkip` and no record (the models wait for residency); `Refuse` only for a fault outside the guarded range. | Re-read: every path refuses as stated; no such refusal among the ~370 native runs at the witness zones. |
| A4 | medium | PLAUSIBLE | `01_state.inc:236-238` vs `:344, 413, 446, 451`; `02_frame_lifecycle.inc:179, 201`; `backend.cpp:503-504` | The renderer is leaked so nothing drains at DLL detach, but plain statics owning D3D12 resources and descriptor leases (`textures`, `model_storage`, `cloth_streams`, `model_streams`, `native_scene_copy`, `native_light_grid`, `native_common_images`, `native_stage_textures`, `face_images`, …) are destroyed by the CRT at DLL_PROCESS_DETACH; each lease deleter takes the pool's lock. | A thread killed by ExitProcess while holding the pool's or the driver's lock: the static destructor blocks and the process never exits. | Hold them through leaked objects like the renderer, or empty them only in `Close`. | Not re-traced end to end; no exit hang recorded (night endurance `full-1028-night` clean). |
| A5 | medium | CONFIRMED | `22_setup.inc:256-264, 527-539`; `16_offscreen_and_movies.inc:174-183` | The RCA pass-through witnesses (1172, 1173, 1175-1177: body-scale update and retire, deferred enqueue and flush, title dialog and step, window close, location step, sound event, part mesh mask, menu item draw) are in the native `guards` list but not in `witness_only`: every native play installs them. | `ObserveBodyScaleUpdate` on every D126B0 call: a static mutex, two guarded reads and an `unordered_map` insert never pruned; the others add detours and rows. | Add them to `witness_only` (update-observe.flag). | Re-read the loop (`22_setup.inc:610`) and `ObserveBodyScaleUpdate`: as stated. |
| A6 | medium | CONFIRMED | `17_render_step.inc:142-154` | Every cloth upload zero-fills a vector, copies the whole cloth copy, FNV-hashes it byte by byte and uploads it; the hash is useless once `simulated` (sticky). | Every frame with cloth on screen: two copies, a byte-serial hash and an upload on DS3's cloth thread. | Skip the hash once simulated; no zero-fill. | Re-read: as stated; the copy itself is a raw `memcpy` from an engine pointer (outside the guarded convention). Cost not measured. |
| A7 | low | CONFIRMED | `03_engine_cpu_owners.inc:1015, 1089, 1120` | After `CloseNativeModelStreams`, a physics stream update or init throws and refuses; the cloth path returns quietly. | A spurious 0xD3126606 refusal at a test run's close. | Return without work after close. | — |
| A8 | low | CONFIRMED | `03_engine_cpu_owners.inc:699-701, 746-749`; `01_state.inc:160, 168` | `native_post_transition_intent` and `native_scene_auxiliary_view` are written under `scene_view_cpu_lock` and never read. | Dead state, a lock a frame. | Remove or wire. | — |
| A9 | low | CONFIRMED | `14_scene_step_and_storage.inc:406` | `model_geometry_refusal` never erased when a storage goes. | A reused wrapper address reports an old refusal; the map grows. | Erase it in `DestroyModelStorage`. | — |

High: none. Notes: several hooks still refuse on bad game data (`AdmitModelStorage`, `InitializeNativeModelStreams`, `NativeDynamicMeshBufferSet`, `AdmitEncodedTextureFamily`, `NativeUiTextureImport`); unbounded production rows (every texture request and residency, storage admission, geometry join, decal request, env-region record) each a synchronous `WriteFile` under `journal_lock` on DS3's loading threads; flagless periodic rows (`world_update_probe`, the thread census); raw reads of engine buffers in the cloth path; the worker, presentation and close sequencing found correct.

### Lot B — Scene collection and the model's frame (14 files; 1 044 s, 546k tokens)

| # | Sev (reviewer → thread) | Status | File:line | Finding | Scenario | Fix | Thread |
|---|---|---|---|---|---|---|---|
| B1 | medium | CONFIRMED | `05_selected_scene.inc:351` (bumped at `12_scene_collect.inc:1577, 1673`) | `paint` (native_paint_epoch) is in `NativeRecordEpochs::SameStable`, which every model record compares: making or retiring one paint target throws away every record of the scene. | The first hit on each new enemy makes a target, a struck character that dies or despawns retires one: that collect, every visible model returns RecordEpochs (fresh admission) and `FinishNativeModelRecords` erases every record out of view: a hitch per event. The comment at `01_state.inc:114` meant only the receivers' meshes. | Compare `paint` only for records whose meshes bind t41/t42, or key it per receiver. | Re-read `08_model_programmes.inc:1470, 1565`: as stated. The hitch's size not measured. |
| B2 | medium | CONFIRMED | `13b_prepare_model_frame.inc:228-233` | Every prepared row of a model with `asm_part>=0` (draw entities 1000 and skinned map objects 1001 included) builds an `AsmTally` (a `NativeFamilyKey` string, two map updates), whose only reader stops after 40 rows. | Thousands of string allocations and map lookups a frame on the prepare workers, forever; kept hits too. | Build it only while the summary can log, and for `asm_part<1000`. | — |
| B3 | medium | CONFIRMED | `05_selected_scene.inc:1036-1054` | The 1142 witness `native_instance_sh` has no flag: an `InterlockedIncrement64` per instance record and a row every 3 000th call. | At m45 (~1 700 models) a journal row every one or two collects, for the whole session. | Behind light-observe.flag. | — |
| B4 | medium | PLAUSIBLE | `05_selected_scene.inc:73-114` | `UpdateNativeModelPose` holds the exclusive `model_pose_lock` for the whole update (compose, palette, a non-finite scan that only feeds a log row, the copy, `Log`). | If DS3 updates poses from several job threads, they serialise, and the collect's shared lookups wait. | Compose and scan outside the lock. | — |
| B5 | low | PLAUSIBLE | `13_model_frame_inputs.inc:165-176`; `13b_prepare_model_frame.inc:139-148` | The global kept-draw epoch includes blended per-frame values (velocity scale, snow rate): any change clears every kept draw. | A gparam blend across a region boundary rebuilds every row each frame of the blend. | Compare them only for the rows that read them. | — |
| B6 | low | PLAUSIBLE | `05_selected_scene.inc:884-890` | `ProduceNativeSelectedInstances` refuses when the group or model fields cannot be read (pool workers). | An entity freed under the walk closes the game instead of skipping it. | Skip and count. | — |
| B7 | low | CONFIRMED | `13b_prepare_model_frame.inc:1303-1314` | The C1 witness `native_pass_rows` has no flag: every 60th frame, maps over every row and a journal write. | Once a second, the whole session. | Behind a witness flag. | — |
| B8 | low | CONFIRMED | `12_scene_collect.inc:2152-2153` | The wet shader catalogue (43 × 512 bytes of an executable table) is read and copied every main collect. | A 22 KB guarded copy and 43 `wstring` a frame for data fixed at start. | Read once. | — |
| B9 | low | CONFIRMED | `game_scene_geometry.cpp:100-103` | The two-copy stability check ignores `vertex_offset2`/`stride2` and sets no refusal code on mismatch. | A second-stream change undetected (latent). | Compare them; set code 14. | — |
| B10 | low | CONFIRMED | `renderer/model_frame.h:16-20, 139-143, 213, 220` | Stale comments (« no point/spot lights yet », « nothing drawn in wave 1 »). | Misleads. | Rewrite. | — |
| B11 | low | CONFIRMED | `game_model_selection.cpp:98-100` | `if(table)` always true. | Dead branch. | Drop it. | — |

High: none. Notes: always-on witnesses in hot loops (modulo rows, per-row atomics and QPC); global epochs for inputs only some records read (B1, B5); admission and instance paths still refuse on transient reads (the skip rule applied unevenly); the engine reads in the lot bounded and guarded; the effects side worker joined before the admission, no race.

### Lot C — Model programmes, admission and content (37 files; 1 304 s, 565k tokens; GhidraMCP unreachable: the decompiles read from `decompiled/`)

| # | Sev (reviewer → thread) | Status | File:line | Finding | Scenario | Fix | Thread |
|---|---|---|---|---|---|---|---|
| C1 | high | PLAUSIBLE | `08_model_programmes.inc:161` (and :945) | `NativeTextureHolders` (and the lightmap witness) call the engine's D24490 directly, with no SEH guard, from the admission workers and the engine thread; D24490 walks the texture registry hash (2356180) without a lock, and its mutators take none either (D4EE10 inserts; D4F4B0 and D4F7A0 erase and `delete`). | Zone streaming (when fresh admissions call D24490) while a loader thread inserts or releases a texture: a worker follows a freed node or a reallocated bucket array, a crash. | Walk `[144792F58]+0x60` natively with guarded reads (as `ScanLoadedProgramme` does for the shader repositories), or keep a path-to-holder snapshot updated in the `ObserveTextureRegistry` wrappers. | Re-read: the direct call is as stated. I-0071 RESULT M1 (OBSERVED in the decompile) accepted the insert race « as with the engine thread alone »; the workers widen the window, and the erase side was not part of that judgement. No such crash recorded. |
| C2 | medium | CONFIRMED | `09_model_admission.inc:786` | The water block's FC_WaterFresnel reads the Reflection quality byte `[[GXRS+0x5360]+0x6C]`, which neither the record's material bytes nor `SameStable` track. | The player changes Reflection quality beside water: the old Fresnel terms stay until that model's revalidation (512 collects, ~8.5 s at 60 frames/s), then pop. | Put the option byte (or the options' generation) in `SameStable`, or keep the GX07 water meshes unrecorded. | Re-read: as stated. |
| C3 | medium | CONFIRMED | `10_options_and_passes.inc:212-213, 311-327` | Every frame, each gated row of a model with `asm_part>=0` (every draw entity 1000 and skinned item 1001) pushes to `asm_gates` and inserts a `NativeFamilyKey` string into a map whose only reader stopped after 40 rows; the capped witnesses increment one shared static per refused row from every worker. | At m45 thousands of string builds, map inserts and contended atomics a frame for a finished diagnostic (the same pattern as B2). | Only while the summary logs (or light-observe.flag); a relaxed load before each capped increment. | — |
| C4 | medium | CONFIRMED (code), PLAUSIBLE (runtime) | `06_pass_programmes.inc:150` (`02_frame_lifecycle.inc:390`) | `AcquireNativeLightingPrograms` keeps its first complete set for ever: an optional member missing then (GXLightOrganize, the sun-shadow mask programmes, the composite, SSS, env map) stays missing; the caller's retry while `organize` is empty always gets the same object. | A repository node linked a frame after the five required ones (Native2202 saw late links): no forward-light grid, or white t32/t33, for the session. | While an optional slot is empty, re-resolve it (bounded, as the glow and AO acquirers do). | Re-read: « the first complete set wins », as stated. |
| C5 | medium | PLAUSIBLE | `08_model_programmes.inc:343` (with :74, :341) | A group neither a registered effect nor composable is cached « not owned », but `ComposeNativeShaderGroup` then walks both repositories again on every call and caches no failure. | One such material in view: two full repository walks per mesh in every main and reflection collect. | Cache a failed composition per group while `shader_registry_changes` holds. | — |
| C6 | low | PLAUSIBLE | `08_model_programmes.inc:362` (and :78) | A layout's kept `Programs` is returned whenever the effect and material match, bypassing `NativeRecipeSet::Current`'s re-ask of absent loose programmes; `composed_effects` never rebuilt. | A loose programme linked after the first build (GXFlver_Common_Velo, `_Phantom_Fwd`) never taken until the FLVER reloads: those meshes get no motion vectors for DLSS and motion blur. | Re-check the absent names (or `shader_registry_changes`) before returning the kept set. | — |
| C7 | low | CONFIRMED | `native/content/decal_constants.cpp:35` | `kCosC1[3]` is `0xad49cbc5`; the game's word at 1439F2DBC is `0xAD49CBA5` (`timing/evidence/I-0007/static/velocity-threshold/constant-values.json:203`; `model_common_textures.cpp:18` has it right). | DecalCos not bit-identical to 141778230 (an error of ~1e-10); already in I-0066 FABLE-REVIEW. | `0xad49cba5`. | — |
| C8 | low | CONFIRMED | `06_pass_programmes.inc:29`; `08_model_programmes.inc:10-14` | `WitnessLightingProgrammes` and the `effect_scan` block run without a flag: on the first composition, a walk of all six repositories and every effect's programme list. | A one-time hitch in play. | light-observe.flag. | — |
| C9 | low | CONFIRMED | `06_pass_programmes.inc:391-401` | `AcquireNativeGlarePrograms` (up to three calls a frame) rebuilds a technique map of SHA strings before its cached lookup. | Allocations every frame for a result that changes with the technique set. | Remember the last plan's list. | — |
| C10 | low | CONFIRMED | `09_model_admission.inc:547` | The reflection walk copies every reflected model's whole `NativeModelRecord` each reflection collect only to check it. | A steady per-frame cost near water. | Check in place under the shard lock. | — |

High: C1 (PLAUSIBLE). Notes: engine code called from the admission workers on structures the loader threads mutate without a lock (D24490, D24430 at :301), outside the module's guarded-walk convention; caches that freeze their first success (the lighting set's optional members, `composed_effects`, the kept `Programs`, the null per glare technique set) against Native2202's late-link lesson; the asset parsers refuse on bad data (a duplicate or non-finite MTD parameter, a duplicate SPX name, invalid FLVER UV records); `AdmitShader` (`18_resource_admission.inc:28`) still runs `D3DReflect` on every shader only for a version word the module's own decoder validates.

### Lot D — Lighting and effects (18 files; 1 084 s, 577k tokens; light_producers.cpp 60-620 and 664-926 skimmed: the snapshot oracle, unreachable from production)

| # | Sev (reviewer → thread) | Status | File:line | Finding | Scenario | Fix | Thread |
|---|---|---|---|---|---|---|---|
| D1 | medium | CONFIRMED | `21_effects.inc:654, 714`; `13b_prepare_model_frame.inc:66-70, 1321`; `scene_pass_frame.cpp:535, 661` | The effect frame takes the pending released serials and marks first-described buffers « sent » at collect; when `PrepareNativeModelFrame` returns {} (no prepared meshes, a render-extent change, the DLSS latch) or Encode throws before `Objects()`, neither Encode nor Retain sees it and the builder's GiveBack does not run (`finished_`). | Released records keep their GPU buffers and counter slots until Close (a leak per dropped frame); a buffer first described in a dropped frame is later created without its initial bytes (a GPU particle's seed and constants zero). | Do the bookkeeping when the renderer consumes the frame, or give back whenever a scene's effects are dropped. | Re-read: `frame->effects` is set only at 13b:1321, after the early return at :66-70; Retain is called only for a frame that reached the renderer (scene_pass_frame.cpp:535). As stated. |
| D2 | medium | CONFIRMED (code), PLAUSIBLE (frequency) | `21_effects.inc:723-732, 812`; `effect_stage.cpp:229, 313-315` | A statement whose FFX texture or SYSTEX_WHITE is not resident, or whose scene copy or depth has no resource, is still drawn with a null SRV (zeros): a stand-in (D-0197), only counted. | A newly spawned effect's texture still admitting, or the scene copy unusable: multiply-blended particles draw black, distortion quads black rectangles. | Drop and count the statement. | Re-read `Resolve` and `shader_view`: as stated. |
| D3 | medium | PLAUSIBLE | `effect_draw.cpp:448-451` (`17_render_step.inc:89, 105`) | While Execute runs on the collect side worker, [GXRS+0x4DD0] points to the native settings shadow; the 61950 and 1AC840 hooks call the original bodies first, which gate their SH and probe update on that member. | A character update or a probe-record set on another thread during the effect record takes the device-present branch (probe sampling through root+0x41E0, which already faulted natively: Native899): a crash or wrong SH rows. | Stop publishing the shadow in the root, or skip the originals while it is published. | Not re-traced; depends on engine threads unshown (ENGINE-STEP-THREAD-MAP §5(c)). |
| D4 | medium | PLAUSIBLE | `12_scene_collect.inc:576-580` → `07_scene_lighting.inc:845-872`; `light_producers.cpp:1249-1356` | Every collect rebuilds every forward model's cbLight and cbFog from scratch (NativeGparamRecords, Banks::Sun, Banks::Fog, ~80 .rdata reads a Sun call), though they depend only on the key block and the bank generation. | Milliseconds a frame with hundreds of forward-row models (not measured). | Cache by (key block, banks, BindingGeneration) as NativeInstanceLightRows does. | — |
| D5 | low | CONFIRMED | `21_effects.inc:929` | Every frame, a guarded Read and a `std::map` update per recorded command, read only by the ffx-record.flag witness. | A diagnostic with its flag off. | Under `witness`. | — |
| D6 | low | CONFIRMED | `light_producers.cpp:765-897, 1358-1373, 1413, 1583, 1605-1614`; `producers.cpp:125, 257`; `cb_reads.cpp:175`; `light_tiles.cpp:72` | The snapshot oracle and several Banks entries are compiled into the module but reachable only from unit tests; mapoffset.txt, envregions.txt and sssprofiles.txt are never published natively, so their parses and the MapOffset fallbacks cannot run. | Dead code beside its replacement. | Move to test-only units; remove the fallback. | — |
| D7 | low | CONFIRMED, latent | `world/host_input.h:49-58, 68` | Bytes(), Shared(), Lines() and List() release IO_LOCK and then dereference the entry. | A concurrent Put or Remove frees the vector being read; today every write runs under gparam_lock on one thread. | Take the shared_ptr under the lock. | — |

High: none. Notes: `effect_system.cpp` and the transcribed bodies dereference engine memory raw (safe only while each step matches the original's null checks); `ID3D11ShaderReflection` for DXBC reflection (not a device; `ID3D12ShaderReflection` would drop the type); `effect_draw` writes engine-global state from the side worker each Execute; per-frame gparam rebuilds where a generation-keyed cache exists.

### Lot E — Renderer core (28 files; 1 165 s, 573k tokens)

| # | Sev (reviewer → thread) | Status | File:line | Finding | Scenario | Fix | Thread |
|---|---|---|---|---|---|---|---|
| E1 | medium | CONFIRMED | `scene_pass_views.cpp:25` (`scene_pass.cpp:94-114`) | `offscreen_sets_` keeps one full view context (6 G-buffer targets, depth, lighting, env/SSS, readback) per distinct menu-view extent until Close; the water reflection is one, its extent following the output (output/2, `12_scene_collect.inc:1730`). | With reflections on, every output-size change makes a new context and keeps the old (tens of MB at 1080p), at residency priority HIGH: VRAM grows for the session. | Retire by ticket a context unused for some frames, or let a new reflection extent replace the old. | Re-read line 25: keyed by {width, height}, as stated. |
| E2 | medium → low | CONFIRMED | `scene_pass.h:243`; `20_startup_phases.inc:93`; `scene_pass_frame.cpp:320-321` | The development witnesses (velocity readback and decode every 30th world frame, back-buffer readback and FNV hash every 120th) are on unless measure.flag. | CPU spikes on the render worker and READBACK buffers held. | Opt-in through their own flag. | Downgraded: the play launcher writes measure.flag (`tools/Launch-DS3-DX12.ps1:66`), so Thomas's play has them off; they run in the test runs and the start check (by design, opt-out). See T1 for measure.flag's other effect. |
| E3 | medium | PLAUSIBLE | `backend.cpp:921-924`; `scene_pass.cpp:414-416` (via 438, 482, 512) | `FramePresenter::Resize` throws on any ResizeBuffers failure, the renderer faults and the toggle hook refuses (`02_frame_lifecycle.inc:628`), while FollowPresentation treats the same status as recoverable (`TryResizeBuffers`). | E_ACCESSDENIED after a focus loss (i101) or through Streamline's proxy (i111) on ResizePresentation, TogglePresentationFullscreen or ApplyDisplayMode: rendering stops and the module refuses. | TryResizeBuffers' rule on those paths; throw only on a device loss. | Not re-traced; whether DXGI returns a non-loss failure there is open. |
| E4 | low | CONFIRMED | `scene_pass_views.cpp:131, 136`; `scene_pass_velocity.cpp:305-306, 330, 360, 371` | A failed witness readback map or file write throws from a slot completion; the next Begin faults the renderer. | A full disk with a picture requested ends rendering (rt-view turns itself off instead). | Log and drop. | — |
| E5 | low | CONFIRMED | `scene_pass.cpp:82-84` | `ViewContext::readback` and `footprint` allocated for every context at every generation, never read. | ~4 B/px of dead memory per context. | Remove. | — |
| E6 | low | CONFIRMED | `scene_pass.cpp:117, 276` | `native_depth_view_` set true before any DepthTarget caller: its D32S8 branch dead. | Neutralised flag. | Drop it. | — |
| E7 | low | CONFIRMED | `frame_plan.cpp:20, 98, 104`; `frame_graph.cpp:98`; `scene_pass_frame.cpp:28` | MaterialColorFramePlan, PlanFramePlacement (tests only), RenderNativeModels (no caller); TakePlanPassesPeak read by tests but its CAS updated every compile. | Dead code, a per-compile atomic loop. | Move to tests or remove. | — |
| E8 | low | CONFIRMED | `scene_pass.h:566-568, 683-685`; `scene_pass_frame.cpp:85-86, 246, 406, 1102-1103` | Comments contradict the code (the atlases « inert », the AO « nothing drawn in wave 1 », CloseRtView's callers). | Misleads. | Correct. | — |

High: none. Notes: in-place rewrites safe only because every target-generation or extent change follows a WaitIdle or Drain; the graph shadow allocates a string per declaration and pass every frame; holder rule L2 and RetireAfterCurrentRecording applied consistently (no use-after-free found).

### Lot F — Ray tracing, post stages, YEBIS, DLSS and shaders (53 files; 1 224 s, 561k tokens)

| # | Sev (reviewer → thread) | Status | File:line | Finding | Scenario | Fix | Thread |
|---|---|---|---|---|---|---|---|
| F1 | medium | PLAUSIBLE | `rt_scene.cpp:146` (with :104, :112-119, :153-155, :162) | `Advance` returns a destroyed model's postbuild slot to the free list once its `weak_ptr` expires, though its committed build list may not be submitted yet; `Build` pops LIFO, so the next build takes that slot. | A storage released between its BLAS build commit and the next submission: both lists in one ExecuteCommandLists (which the code itself treats as able to overlap, :377-378); readback[s] may hold A's compacted size, B's compacted buffer sized from it (only `size>built_bytes` checked): a COMPACT copy past its placed buffer, corruption or device removal. | Free the slot when the build list completed (the CompletionToken), not on expiry. | Re-read `Advance` (:141-148): the slot goes back on expiry, the M2 comment assumes queue order between separately committed lists. |
| F2 | low | PLAUSIBLE | `rt_scene.h:66-67` (`rt_shadow.hlsl:53` via `rt_material.hlsli:75-84`; `14_scene_step_and_storage.inc:260-262`) | The traced shadow's pixel shader reads a storage buffer whose contract state lacks PIXEL_SHADER_RESOURCE. | In the call carrying the storage's ingress list: a GPU-based-validation error, harmless on current hardware. | Add PIXEL_SHADER_RESOURCE to the read state. | — |
| F3 | low | CONFIRMED, latent | `rt_scene.cpp:286` | `RtSkinner::Encode` marks a skinned BLAS failed for good on an empty palette (transient). | Unreachable today (`12_scene_collect.inc:833` refuses first). | A per-frame skip. | — |
| F4 | low | CONFIRMED | `grading_stage.cpp:128-132` | The LUT 2D-to-3D copy and its two barriers run every graded frame though the volume changes only with (lut, generation). | Repeated GPU work. | Copy only on change. | — |
| F5 | low | CONFIRMED | `yebis_stage.h:12-19` (`06_pass_programmes.inc:221-236, 291`) | `scene_copy` and `dof_table` programmes resolved and pinned, never read; their absence or pin mismatch nulls the set and delays or disables the grading. | The grading off for a session on a pin mismatch of an unused programme. | Resolve only `lut_copy`. | — |
| F6 | low | CONFIRMED | `ao_stage.h:18-20` | « inert in wave 1 »: the AO draws every lit frame. | Misleads. | Correct. | — |
| F7 | low | CONFIRMED | `tonemap_stage.h:21-22, 43-44` | « t1 black until the glare family exists »: the glare is native. | Misleads. | Correct. | — |
| F8 | low | CONFIRMED | `rt_scene.h:4-5` | « Behind rt-scene.flag; nothing here draws »: RtSunShadow draws in production. | Misleads. | Correct. | — |

High: none. Notes: the RT ordering model (separate ExecuteCommandLists calls never overlap, lists inside one may) honoured everywhere but F1; root SRVs into the material table and instance rows unbounded, correct by construction only; buffer states rely on decay and promotion (F2 the exception); the post stages showed no crash, barrier or lifetime fault.

### Lot G — UI, movies, primitives and texture ingress (50 files; 1 150 s, 486k tokens)

| # | Sev (reviewer → thread) | Status | File:line | Finding | Scenario | Fix | Thread |
|---|---|---|---|---|---|---|---|
| G1 | high | PLAUSIBLE | `16_offscreen_and_movies.inc:393` (retire hooks :285-326) | `CollectNativeOffscreenScenes` passes a menu model's part drawables to `AdmitNativeSelectedObject`, which calls engine code on them; the retire hooks (D66280, D68EB0) only erase bookkeeping under `native_offscreen_lock` and then let the original free the model: nothing makes them wait for a collection in progress. | In play, a Status or portrait view armed; the player closes the menu while the scene CPU update thread is in the loop (:371-393): engine code on a freed drawable, a crash. The code's own comments say two threads can collect (:333-336) and a view can retire during selection (:437). | A per-holder busy mark under `native_offscreen_lock` and a condition variable the retire hooks wait on before the original. | Re-read: the race window is as stated; the menu thread equals the presentation thread (MENU-OFFSCREEN-MODELS « Native1170 »), but whether the in-world scene CPU update runs on another thread is not witnessed; no such crash recorded. The only high of the review. |
| G2 | medium | CONFIRMED | `game_ui_bindings.cpp:70` (`primitive_pass.cpp:209-212`) | Every frame, each CPU-created UI image a draw binds (the glyph atlas, gradients) is copied whole into the frame packet and compared byte for byte with the pass's cache. | Any frame with text: the 1-4 MB glyph atlas memcpy'd on the UI worker and compared on the render worker. | Bind the resident TextureIngress record as `external`. | Re-read: `textures.push_back(origin)` copies when `resident` fails; the cache compares `mips` in full. |
| G3 | medium | CONFIRMED | `game_ui_images.cpp:104` | `Publish` uploads every created or edited UI image to TextureIngress, a record never bound (only `resident`/`upload_fence` read); the primitive pass uploads the bytes again. | Two GPU uploads a glyph-atlas edit, one never sampled; an unused GPU copy of every CPU UI image. | Sample this record (G2) or drop the upload. | Re-read the upload (:104-108); its readers not all re-traced. |
| G4 | medium | CONFIRMED | `16_offscreen_and_movies.inc:174` | = A5 (the 1172-1177 RCA witnesses installed in play without a flag). | | | Same finding as A5, found independently. |
| G5 | medium | PLAUSIBLE | `game_ui_mesh_owner.cpp:131` | `AllocateItem` ends the game at 4096 live UI mesh items; the native allocation never reports memory pressure, so the engine's mesh cache never evicts by budget. | A long session with many menus and text fields reaches 4096 items: the game closes. | Return the allocation failure so the cache's LRU runs, or skip and count. | — |
| G6 | medium | PLAUSIBLE | `primitive_textures.cpp:12, 73` | The UI texture set refuses past 1024 images, and `NativeUiTextureImport` then refuses (`20_startup_phases.inc:434`), byteless imports counted too. | More than 1024 live UI images: the game closes on the next import. | Do not count external entries; skip and count. | — |
| G7 | low | CONFIRMED | `primitive_constants.cpp:106`; `game_ui_shaders.cpp:50, 106-107, 129-130`; `game_ui_session.cpp:442, 1056` | Per UI draw: constants packed twice, `UniformVariables` twice, a 4 KB zero-fill per uniform write, a 5 KB shadow copied twice, each draw validated twice. | ~8-10 interface cache lookups (bytecode compares under a global mutex) per draw per frame. | Return the packed bindings once; validate once. | — |
| G8 | low | CONFIRMED | `texture_tiling.cpp:96, 112, 122`; `primitive_textures.cpp:94, 99`; `primitive_constants.cpp:61, 149`; `primitive_programs.cpp:157` | Dead code (BuildTextureTiling, TextureScrollOwner::Apply/Acquire, the ProduceTexturedPrimitiveWork chain, PrimitiveUniformSource::Write, PrimitiveTextureStore::Withdraw, DecodeCpuTextureContainer tests only). | Earlier attempts beside their replacements. | Remove or move to tests. | — |
| G9 | low | CONFIRMED, latent | `game_ui_image_owner.cpp:406`; `game_ui_session.cpp:140` | `ConstructManager` may return null and `ConsumeCpuPending` dereferences it; `ImageServicesAvailable` throws through engine frames. | A callback after the owner closed, or no R8 sampling. | Test and `Fail` named. | — |
| G10 | low | CONFIRMED | `16_offscreen_and_movies.inc:937` | The « 1159: ADEF50 … Transcription » comment sits above `NativeGxDeviceSync`, not `NativeLoadGameSummaryPortraits` (`17_render_step.inc:7`). | Misleads. | Move it. | — |

High: G1 (PLAUSIBLE). Notes: the module's own limits end the game instead of skipping (4096 mesh items, 1024 UI images, 4096 instances or commands, Scaleform contract checks); UI frame packets copy immutable data every frame (image bytes, mesh vertices and indices, programme bytecode); `20_startup_phases.inc:394-402` (A2) seen independently; the offscreen and movie paths rely on « same thread » assumptions written only in comments; texture ingress lifetimes and the DDS, TPF and GPARAM parsers' bounds held up.

### Lot H — Diagnostics, witnesses and profilers (14 files read in full; 984 s, 454k tokens)

| # | Sev (reviewer → thread) | Status | File:line | Finding | Scenario | Fix | Thread |
|---|---|---|---|---|---|---|---|
| H1 | medium → medium | CONFIRMED | `04_updates_and_test_tools.inc:716-722, 817-848` | The world update gate (8FA3E0) and the characters' schedule (8DD2E0) hooks are installed in every native run (`22_setup.inc:482-483`, not in `witness_only`) and carry investigation rows with no flag: `frame_pacing` (1118), `player_skin_colour` and `player_part_model_masks` (1097, 1102), `world_update_gate`, `native_chr_manager_schedule`. | Every play session: about ten guarded engine reads a frame on the game thread and a journal row every few seconds (the 300- and 600-call windows, each key change), for nothing; `frame_pacing` duplicates `RecordGameStep`. | Keep `RecordGameStep`, the test tools and the gate's step; put the rows behind `update_witnesses` (update-observe.flag). | Re-read: the rows are unconditional (lines 820-848); the cost is small (rows every 5 to 10 s at 60 frames/s, not every second), but it is a diagnostic active without its flag. |
| H2 | low | CONFIRMED | `04_updates_and_test_tools.inc:513, 531, 546` (called at 715) | The teleport, phantom and event-flag witness files are probed in every mode, play included, unlike the F4-F7 tools behind test-tools.flag. | Three `GetFileAttributesW` every 15th schedule call; a stray `witness-flag.txt` beside the module would set an event flag in a play save. | Gate the three appliers behind a flag read once. | Re-read the call site (715): unconditional. |
| H3 | low | PLAUSIBLE | `04_updates_and_test_tools.inc:252-257` | `camera_laws` keeps one `params_seen` and one `wall_object`: two follow cameras updated in turn would log a row a call and restart the wall snap. | Needs two 140515790 objects updated alternately (judged unseen by the I-0114 final review L3). | Key both per object. | — |
| H4 | low | PLAUSIBLE | `04c_scene_witnesses.inc:550` | `lod_step` prints `record+0x70` and `manager+0x110` as floats where `PopinLodChange` (:477-485) reads the same fields as integers. | light-observe rows show denormals for the LOD and extent. | Read them as int32 and uint32. | — |
| H5 | low | PLAUSIBLE | `04b_pass_witnesses.inc:1206-1211` | `ModelDrawObserved` credits a model with the process-wide indexed-draw difference across its call. | Effect chunks on other threads add their draws to the model's count (Reference witness only). | A thread_local counter. | — |
| H6 | low | CONFIRMED | `04_updates_and_test_tools.inc:772-773` | `ObservePartPoseUpdate`'s per-receiver map has no bound (its siblings cap at 4096). | pose-observe.flag over a long run: unbounded growth under a global mutex. | Cap it. | — |

High: none (D3D11 devices and hooks confined to the Reference blocks, `22_setup.inc:305-419, 680-748`). Notes: `sprintf_s` into fixed buffers with `%f`/`%g` of engine floats and no invalid-parameter handler (an overflow fast-fails the process; the tightest in production is `camera_laws_params`, 384 bytes); witnesses read through `Read`/`GuardedCopy` consistently; `KeepLodReference` (a production replacement in this lot) rests on D-0222's claim that 6E770 only writes that value, not re-verified.

### The thread's own finding

| # | Sev | Status | File:line | Finding | Scenario | Fix |
|---|---|---|---|---|---|---|
| T1 | low | CONFIRMED | `20_startup_phases.inc:93`; `tools/Launch-DS3-DX12.ps1:66` | measure.flag, which the play launcher writes to turn the witnesses off, also turns the K-split profile on (`kprofile::enabled=measure_run && !collect_no_profile`) and the UI profiling: every play session pays the profile's QPC scope switches, counters and its periodic rows. | Thomas's play: a small constant CPU cost and journal rows from a measuring tool. | The launcher also writes no-kprofile.flag, or the profile gets its own opt-in flag. |

## The skill's pass, verified

| # | Sev | Status (thread) | Note |
|---|---|---|---|
| S1 | low | CONFIRMED, latent | Encode gives every slice of a set the storage's buffer; the slices the collect never reaches (no programme set, no policy, failed bounds) keep zero index and position fields. Today only `rt_view` reads `s.position` and `s.geometric`, and its comparison is gated on `s.material`; the alpha slices write their own triangle data. Matters as soon as R3c or a later pass shades from RtSurface. |
| S2 | low | CONFIRMED (code), PLAUSIBLE (data) | `out.material` depends only on the diffuse texture (`12_scene_collect.inc:1053`); a slice with no readable UV0 (counted in `no_uv`) samples one texel. Whether DS3 has a TEXCOORD0 format outside SINT16, float2 and half2 is not settled. |
| S3 | low | PLAUSIBLE | Already I-0119 step review L6 « TO CHECK »: the scroll row on float-UV slices. |
| S4 | low | CONFIRMED | `rt_material.hlsli:148` takes albedo 1 × FC_DifMapMulCol when the diffuse view is none while `RtRecordMaterial` stays set (`rt_scene.cpp:490`); only when the bindless heap refuses a view. |
| S5 | low | CONFIRMED (code), PLAUSIBLE (runtime) | With `named_registry` 0 the material path leaves every role out for good (`12_scene_collect.inc:1016`), the alpha path marks the slice pending (`:1199`). Whether the registry reads 0 at a walk that builds a set is not settled. |
| S6 | medium | CONFIRMED | A specular, bump or emissive role pending (not resident or its `_l` stand-in) sets `pending` and pushes into `kept` before the material decision; a slice with no `g_DiffuseTexture` (the map blends) then drops the views (`:1055`) but keeps `pending` and the textures: such a set is made again every 30 walks while that texture stays a stand-in, for records it never writes. CPU cost bounded by the walk's budget; most relevant to R3c (the map blends). |
| S7 | low | CONFIRMED | `rt_scene.cpp:529`: a skinned instance with no set still takes a positions view and writes its bases. |
| S8 | low | CONFIRMED, diagnostic | `rt-view.flag` only; kept knowingly by I-0119's final review (L5). |
| S9 | low | PLAUSIBLE, diagnostic | `rt-view.flag` only, its sampled frame. |
| S10 | low | PLAUSIBLE | Needs the same texture record bound at two registers with different tiling rows. |
| S11 | low | CONFIRMED | Three copies of the vertex-element lookup (optimise-do-not-stack); I-0119 step review L4 kept it. |
| S12 | low | CONFIRMED, latent | `RT_FAMILIES` 256 equals `kRtViewFamilies` today; nothing ties them. |
| S13 | low | CONFIRMED | Dead parameter and an always-true guard. |

## Mandate sweep (the thread)

`D3D11CreateDevice`, `ID3D11Device` and `LoadLibraryW(L"d3d11.dll")` appear only in `04b_pass_witnesses.inc` (the Reference
run's observers: the original renderer's device, never the native's); `ID3D11ShaderReflection` in `18_resource_admission.inc` and
`21_effects.inc` is d3dcompiler's DXBC reflection (reading the game's programmes), not a rendering dependency; `21_effects.inc:507`
and `04c_scene_witnesses.inc` read D3D11 objects only in Reference runs (their comments say so; their gating is lot H's).
