# I-0068 part BE: the native `collect` step (104 ms of 166), its probable causes and numbered reprises (static)

Request (coordinator, 2026-10-11): phase A (main `afcd4115`, [D3D12-PHASES-A-C-PREP](../I-0066-independent-native-startup/D3D12-PHASES-A-C-PREP.md) §8) shows the native frame is dominated by `collect`: 104 ms of about 166; prepare takes 26 and render 23 (19.5 of it GPU wait). Phases B and C do not touch collect. Study in static what collect does natively against the original and ER (walk, copies, allocations, engine reads), name the probable causes, propose numbered reprises with figures, and place them in the master plan. **Division of work (coordinator, 12:39Z):** this note is the native side and the reprises; the original's `E006A0` collect is a separate note of the menus static thread (I-0066), which I read when it reaches main and do not rewrite. Nothing was launched, no native code changed.

Labels: **STATIC** (read in the sources of `main` at `6b2ec2a7`), **OBSERVED** (the retained phase A run `i66-native-1276-m2`, S1 High Wall idle), **INFERRED** (my arithmetic on those counts), **OPEN**. Every millisecond figure in §4 and §6 is an **estimate to be confirmed by reprise C0** (sub-phase timers): the only measured number is the 104 ms of the whole step.

## 0. Answer first

1. **What the step is.** `collect` is the `native_phase_ticks[0]` window around `CollectNativeSelectedScene` and `CollectNativeOffscreenScenes` in `UpdateNativeSceneCpu` (`game_bootstrap.cpp:5551-5554`). In S1 it visits 328 objects and admits 126 models; it builds, for every selected mesh and every **resolved** programme recipe, the complete set of shader constant blocks and texture selections the draw will need, from scratch, every frame. The frame is serial: collect 104 + prepare 26 + render 23 + UI 1.6 leaves about 12 ms for the rest of the engine's frame (166 ms at 6.0 fps).
2. **The main cause is over-production, not slow primitives.** Counting from the S1 log (`program_recipe_pairs` 4,464 resolved recipes, `eligible_meshes` 1,077 prepared rows), admission prepares shader blocks for **all 14 recipes the programme family resolves** (about 4,464 per frame) while the pass walk afterwards keeps **1,077 rows**, so about **three quarters of the work is thrown away**. What is kept is also mostly frame-invariant: the scene and view blocks do not depend on the mesh at all, the instance blocks on the model, the material blocks on the material (§3).
3. **Likely split (INFERRED, to confirm with C0):** about 34,000 `PrepareReadShaderRegister` calls per frame (35-50 ms), about 2,000 to 9,000 texture-binding lookups through the engine's path normaliser (10-15 ms), dynamic constants (5-10 ms), pass evaluation (5-10 ms), the engine's own traversal (3-8 ms), light collectors (2-5 ms), allocation and teardown of the previous scene (5-10 ms). Rules out: the guarded `Read` (about 75,000 per frame, under 1 ms) and the mutexes (about 7,000 uncontended, under 1 ms).
4. **Eleven reprises (§6), in this order:** C0 sub-phase timers; C1 produce blocks only for the rows that exist (no cache, no staleness); C2 share the scene/view blocks per shader; C3 share instance blocks per model; C4 cache the material blocks by material version; C5 cache the texture bindings by registry version; C6 cache the dynamic constants; C7 one **persistent admission record per model** with per-frame deltas (the structural end, and the base of R8a and R8b); C8 cheap clean-ups (witness mutex, string churn); C9 pass-row cache; C10 overlap with render (R13, phase F).
5. **Expected frame, from 166 ms:** C1-C6 about 100 to 115 ms (9-10 fps); C7 about 85 to 97 ms (75 to 90 with C8-C9); with phases B-C on prepare about 60 to 70 ms; with phase D (GPU wait hidden) about 45 to 55 ms; with phase F (collect overlapped with render) bounded by the longer of the two paths, about 30 to 40 ms (§7). Figures are scenario estimates, not promises.
6. **Placement:** a new workstream **K (collect)** in the master plan, **parallel to phases B and C** (it is CPU-only and touches different code than R2-R9), ahead of R8a and R13 so they share its version stamps (§7).

## 1. The frame budget (OBSERVED, phase A)

S1 (Argo, High Wall idle, 1280×720, no debug layer, run M2 `1276`): fps 6.02, **collect 103.6 ms**, prepare 25.9 (validate 1.9, resources 12.2, draws 9.2), render 23.2 (record 0.6, GPU wait 19.4, present 0.3), UI build 1.6. The sum is 154.3 ms; **about 12 ms** of the 166 ms are outside the three phases (the engine's own frame work and the end-of-frame hook). The queue shows 2 `Execute`, 2 `Drain` and 4 CPU waits per frame. The step order is serial: `UpdateNativeSceneCpu` (collect) then `EndFrame` (prepare, render) on the same owner thread (R8B-ENGINE-READS-STATIC §1, INFERRED).

## 2. What collect does natively (STATIC)

In call order inside `UpdateNativeSceneCpu` (`:5426`):

1. **Engine CPU update kept as is** (`:5442-5547`): camera copy, `C1B20`, UI camera, `E6E9E0`, decals `CFA750`, transitions `D52AF0`/`D52B70`, tint, layer `D5E550`, debug requests. Genuine engine code; its cost is part of the 12 ms outside, not of the 104 (the tick window opens at `:5551`).
2. **`CollectNativeSelectedScene`** (`:4631`):
   - per-frame set-up: a fresh `NativeSelectedScene` (`make_shared`), `AcquireNativeCommonImages`, the sampler bank refresh, the 11-float environment rows (`134A20` ×2), the **wet shader catalog** (`ReadModelWetShaderCatalog`, a `vector<wstring>` filled every frame), view constants, `CollectNativeVelocityInputs`;
   - **the walk**: the engine's culler visitor (`234620`) and the world query `decal_world_query_original` (`C1FB0`) call back `AdmitNativeSelectedObject` (`:4166`) for each visited object. S1: **328 objects**, about **170 map-item children** (`SprjFD4ModelItem`) and about **16 character parts** per frame (derived from the cumulative counters of `native_disp_selection_totals`: 93,954 → 135,714 children and 10,352 → 14,192 parts between samples 720 and 960);
   - **per candidate** `AdmitNativeModelCandidate` (`:3430-4140`, 710 lines): fade state, parameter animations (`ReadModelParameterAnimations`), `ReadCurrentModelInputsAt` (reads the mesh table and validates the membership twice), decal choice, wet inputs, group category bytes, pose and storage lookups under `model_pose_lock` and `model_storage_lock`, instance constants (`ProduceNativeSelectedInstances`), then **per mesh** (below);
   - `PrepareNativeSelectedPasses` (`:4401`): 8 engine passes (5, 6, 7, 0x1F, 0x21, 0x22, 0x29, 0x34) × models × fades × meshes, each an `EvaluateOrdinaryModelPass`, a texture-scroll update per material pair and a tiling production;
   - `CollectNativeLightBlocks` (`:3044`) and `CollectNativePointLights` (`:2740`);
   - the new scene replaces `native_selected_scene` under `scene_view_cpu_lock` (`:4706`): **the previous frame's scene (126 models and all their per-mesh vectors) is destroyed there**.
3. **`CollectNativeOffscreenScenes`** (`:6366`): menu views only (none in S1).

**Per mesh, in `AdmitNativeModelCandidate` (`:3593-4122`)**, every frame:

| Block | What it does | Cost class |
|---|---|---|
| material | two locks (`model_material_lock`), about 10 guarded reads of the material (masks, pose policy, special, deformation, index), wet parameters | small |
| layout and programmes | `model_layout_lock`; `AcquireNativeModelPrograms` (`:3230`): `material_lock` map find, **the engine lookup `D24430`** (string registry, `FUN_140D446D0`), `effect_lock`, `programs_lock`; a cache hit returns the stored programmes | one engine string lookup per mesh |
| texture bindings | for each declared binding: a `vector<wstring>` of up to three candidate paths built by `substr`/concatenation, the engine lookup **`D24490`** (`FUN_1402FF080` builds a normalised `wstring`, `FUN_140D4ED20` looks it up, the string is freed) per candidate, `textures.Acquire` (lock + map + `shared_ptr`), `_wcsicmp` scans over the system textures for fallbacks; then the scene bindings (slots 38, 45, 41, 42, 30, 31, 33, 44, 59-62) | allocation and an engine lookup per binding |
| shader textures | `SelectModelShaderTextures` for **every resolved recipe × 2 stages** | 8,928 calls/frame |
| dynamic constants | `dynamic_bytes` vectors, generated/damage/wet/phantom writes, `ProduceMatDynScalars`, the 22E930 block (about 40 guarded reads, `vector<float>`, `vector<array<uint8_t,8>>`, `pow`) | allocation heavy |
| **shader constant blocks** | for **every resolved recipe × 2 stages**: `numeric_blocks` (`cbMtdParam`, register 3, with a fresh `stage_ranges` copy, a `std::list` of fallback buffers and `ProduceMtdParams` per partly covered variable), `scene_blocks` (register 8, `cbSceneParam`), `view_blocks` (register 0, stage 0 only), `instance_blocks` (register 4, once per fade) | **`PrepareReadShaderRegister`**: resource and constant scans with string comparisons, a `FloatMask` vector, `PackShaderConstant`, one `vector<uint8_t>` per block |

The mesh record itself (`NativeSelectedMesh`, `:2280`) carries 126 `ShaderRegisterBlock` slots (28 numeric, 28 scene, 14 view, 56 instance; 32 bytes each), 28 `ShaderTextures`, ten `shared_ptr` leases and several vectors: about 5 to 6 KB constructed, moved into `selected.meshes` and destroyed per mesh per frame.

## 3. Per-frame operation counts (OBSERVED log, INFERRED arithmetic)

From `i66-native-1276-m2/controller.jsonl`, last rows (`native_current_view_selected`, `native_disp_selection_totals`):

| Quantity | Value | Source |
|---|---|---|
| objects visited | 328 | log |
| models admitted | 126 (88 fade intents with a non-zero variant) | log |
| resolved programme recipes (mesh × recipe) | **4,464** | log `program_recipe_pairs` |
| prepared pass rows (mesh × pass × fade) | **1,077** (+5 unavailable) | log `eligible_meshes`, `prepared_meshes` |
| map-item children per frame | about 170 | counters |
| character parts per frame | about 16 | counters |
| meshes | about 560 to 1,100 (4,464 recipes at 4 to 8 per mesh) | INFERRED |
| `PrepareReadShaderRegister` calls | **about 34,000** (4,464 recipes × [2 stages × (numeric + scene + instance blocks at about 1.3 fades) + 1 view]) | INFERRED |
| `SelectModelShaderTextures` calls | 8,928 (4,464 × 2) | INFERRED |
| rows that actually use a recipe | at most 1,077 (about 24 % of the resolved recipes) | log |
| texture-binding lookups | about 2,000 to 9,000 (meshes × 4 to 8 bindings) | INFERRED |
| guarded `Read` calls | about 75,000 (about 60 per mesh + 80 per model) | INFERRED |
| mutex acquisitions | about 7,000 | INFERRED |
| `EvaluateOrdinaryModelPass` calls | up to 8 passes × rows ≈ 9,000 to 17,000 | INFERRED |

The key ratio is **4,464 against 1,077**: the admission cannot know which rows the passes will keep, so it prepares every resolved recipe; the pass walk then discards three quarters.

## 4. Probable causes, ranked (INFERRED; to be confirmed by C0)

| # | Cause | Basis | Estimated share of the 104 ms |
|---|---|---|---|
| K1 | block production for unused recipes (and for frame-invariant blocks, scene/view per mesh instead of per shader) | 34,000 calls, each scanning the shader's resources and constants by string name, allocating a `FloatMask` vector and the output vector; 76 % unused | 35-50 ms |
| K2 | texture-binding resolution through two engine string lookups and path allocations per binding per mesh per frame | `D24490` normalises into a fresh `wstring` and hashes it; `D24430` per mesh; results are stable between frames | 10-15 ms |
| K3 | allocation and teardown: every block is a `vector`, every mesh a 5-6 KB record, the previous scene is destroyed under the lock inside the new collect | tens of thousands of small allocations and frees per frame | 8-15 ms (overlaps K1) |
| K4 | dynamic constants rebuilt per mesh (22E930 values, damage, wet, phantom) with reads and vectors | per mesh, independent of change | 5-10 ms |
| K5 | pass-row evaluation (8 passes × models × fades × meshes) with a scroll `Update` and a tiling production per row | 9,000-17,000 evaluations | 5-10 ms |
| K6 | the engine's walk and visibility calls (`classify`, `D03050`, `64F000`, `D043C0`, the world query) | 328 objects + 170 children + 16 parts | 3-8 ms |
| K7 | light collectors | one pass over the prepared rows, small maps | 2-5 ms |
| K8 | witnesses in the production path: a global mutex and a `map<uintptr_t,string>` per candidate (`CharacterWitness`, `:3438`), `sprintf` rows for the first N samples | 126 candidates | under 1 ms |

Not a cause (STATIC): the guarded `Read` is a `__try` memcpy with a thread-local flag (`:526-534`), a few nanoseconds; the mutexes are taken by the owner thread only and are uncontended.

## 5. Against the original and ER

**Original (what the static reading says; the separate `E006A0` note decides the rest).** The original's per-draw CPU side is the engine's own emit (`D3DA20` for map items, `CFDE70` for character parts, `D079C0` for draw entities) feeding the GX command stream; native replaces the GX statements by its own preparation and must rebuild a typed input per draw. Native keeps the engine's CPU update (`E006A0` body) unchanged, so that part is not a difference. What I need from the original note, listed so it can be answered without rewriting either fiche: (Q1) per frame, how many draws and constant uploads the original's emit produces at S1 (to compare with 1,077 rows and 4,464 recipes); (Q2) whether the original caches per-material constants (the material dynamic object `material+0xD0`, the `22E930` writer) or rebuilds them per draw; (Q3) what the 12 ms outside the three native phases contain.

**ER (architecture only, D-0157, [D3D12-DESIGN-REVIEW](../I-0066-independent-native-startup/D3D12-DESIGN-REVIEW.md) §2).** E10: only dirty descriptor ranges are prepared before each draw; E11: draw inputs come from engine owners, and the collector copies the current parameter bytes into transient space at preparation, from retained structures (not by re-deriving them); E1: the render worker consumes the previous frame's ordered records. The lesson for collect is the same as for R8: per-frame work proportional to what changed. D-0157 authorises the engine-side half (owners publishing versions), which reprises C4-C7 use.

**Update after the original-side note ([COLLECT-ORIGINAL-STATIC](../I-0066-independent-native-startup/COLLECT-ORIGINAL-STATIC.md), part BF).** It answers Q1-Q3: `E006A0` is the scene stage's per-frame update and visits no entity, so the 104 ms is native-only admission work; the original traverses per pass scene on the draw side (parallel job tasks by default, no occlusion for the G-buffer, forward and shadow scenes), and per draw it rewrites **values** into persistent, pre-laid-out records with **no layout analysis** (Q2); the ~12 ms outside the three phases are the engine's own CPU frame and the native hooks outside the brackets (Q3, split by its row C-0b); the number of admitted meshes was never logged (its row C-0). Effects on this note: K4 (dynamic constants) and K5 (pass-row evaluation) are **not excess** compared with the original, which also computes values and evaluates rows per draw, so C6 and C9 are optional and ranked after the block and texture work; a layout cache (K3c in COLLECT-K-PREP) is added as the native form of "layout fixed at creation"; and the study's C7 is bounded by the review's rule that no whole-model record may be reused across frames (the persistent part is digest-keyed and excludes every per-frame field). See COLLECT-K-PREP §8.

## 6. The reprises (numbered C0-C10; estimates to be re-based on C0)

| Id | Reprise | What changes | Estimated gain on collect | Cost | Risk | Witness |
|---|---|---|---|---|---|---|
| **C0** | **Sub-phase timers and counters** inside collect: engine walk, candidate admission split in material/programmes, textures, shader textures, dynamic constants, blocks; pass walk; lights; previous-scene teardown; plus counters (`PrepareReadShaderRegister` calls, texture lookups, allocations through a development `operator new` hook, resolved recipes against rows) in the pacing row | measurement only (the `measure.flag` family) | 0 (confirms or corrects §4) | S | none | CL1: the sub-phase table at S1 |
| **C1** | **Produce what the rows use.** Do not fill `numeric_blocks`, `scene_blocks`, `view_blocks`, `instance_blocks` and `shader_textures` for all resolved recipes at admission; produce a (mesh, recipe, stage) entry when `PrepareNativeSelectedPasses` keeps a row for it (or on first use in prepare, memoised per scene) | `AdmitNativeModelCandidate` loop at `:3764` and `:4059-4116` moves to the row emission (`:4552`) | **-55 % to -75 % of K1**, so -20 to -35 ms; `SelectModelShaderTextures` -75 % | S-M | low: same inputs, same outputs, only for rows that draw; byte-identical blocks | CL2: hash of all produced blocks equal old/new on Native1254 and the five zones |
| **C2** | **Scene and view blocks once per shader interface and frame.** `scene_blocks` (register 8, `cbSceneParam`) and `view_blocks` (register 0, `cbMatrixWVP`) depend on the shader and the frame's camera only; store them in a per-scene table keyed by (interface pointer, stage) and let meshes hold a `shared_ptr<const ShaderRegisterBlock>` | block types become shared pointers | from about 100 to a few hundred calls instead of about 1/3 of the rest | S | low | CL2 |
| **C3** | **Instance blocks once per model, fade and shader.** `instance_blocks` depend on `selected.instances[fade]` and the shader; build once per (model, fade, interface), share across the model's meshes (about 5 to 9 per model) | same | -80 % of the instance share | S | low | CL2 |
| **C4** | **Material blocks cached by version.** `numeric_blocks` (register 3, `cbMtdParam`) depend on the material's authored values, the programme and the animated parameter writes: cache per (programmes object, recipe, stage) with a key of (authored generation, numeric writes digest); recompute only when a write changed | cache on `NativeModelLayout::Programs` | -85 % of the numeric share | M | medium: the animated writes (wet, damage, phantom, animation rows) must be in the key; shadow-compare mode (§8) | CL2, CL3 |
| **C5** | **Texture bindings cached by registry version.** Resolve the material's bindings once per (programmes, texture registry generation) and keep the `NativeTextureRecord` leases; the registry bumps a generation on residency or replacement; scene bindings (slots 30-62) are per scene, not per mesh | `bound.texture_bindings` and `scene_texture_bindings` | -90 % of K2, -9 to -13 ms | M | medium: streaming changes (low-resolution packages `_l`, atlas packages) must bump the generation; shadow-compare | CL4 |
| **C6** | **Dynamic constants cached per material.** `dynamic_bytes`/`dynamic_written` depend on the material, the generated/damage/wet/phantom writes and the draw's wet inputs: cache per (material, parameter-animation version, wet inputs) | per material | -70 % of K4, -4 to -7 ms | S-M | medium: same as C4 | CL2, CL3 |
| **C7** | **Persistent admission record per model** (the structural end of C1-C6): keep the immutable part of `NativeSelectedModel` and its meshes across frames, keyed by (model, storage generation, material set generation, programmes generation, texture generation); per frame only update the delta: placement, fade, group state (R8B E7), pose version, wind, light keys, camera blocks. This is R8 "persistent draws" seen from collect, and the `Level 1` snapshot of [R8B-ENGINE-READS-STATIC](../I-0066-independent-native-startup/R8B-ENGINE-READS-STATIC.md) | `NativeSelectedScene` holds `shared_ptr` to persistent records plus a delta | collect to **25-35 ms**, **12-25 ms with C8 and C9** (floor: walk + deltas); removes K3 teardown | M-L | medium-high: invalidation completeness; permanent shadow-compare F6 | CL2, CL3, CL5 |
| **C8** | **Clean-ups**: drop the `CharacterWitness` mutex and map from the production path (sample every N frames), avoid the per-frame `ReadModelWetShaderCatalog` vector (cache by catalog generation), `reserve` and move discipline on `selected.meshes`, no per-mesh `std::list`/`vector` temporaries in the loops | local | -2 to -5 ms | S | none | CL1 |
| **C9** | **Pass-row cache.** `EvaluateOrdinaryModelPass` results depend on (model state, mesh, pass, fade): reuse across frames when the state version is unchanged (the fade, the category bytes, the material mask) | in C7's record | -3 to -8 ms | S-M | medium | CL2 |
| **C10** | **Overlap with render** (R13, phase F): collect for frame N+1 while the worker records, submits and presents frame N; needs immutable snapshots (C7 gives them) | threading | hides min(collect, prepare+render ≈ 49 ms) | L | high | CL6 |

Order: C0 first (a run), then C1-C3 as one slice (they share the code region and the CL2 witness), then C5, C6 (independent), then C4 (needs the version key design of C7), then C7 as the converging step, with C8 anywhere. C9 and C10 after C7. Every cached path ships behind a flag (`rebuild-collect.flag` returns to the current behaviour) and with a **shadow-compare mode**: every Nth frame also run the old path and compare the produced blocks byte for byte (the `scan-compare` pattern of R8b F6), so a stale cache cannot ship silently.

## 7. Placement in the master plan, and the frame budget

New **workstream K (collect)** in [D3D12-MASTER-PLAN](D3D12-MASTER-PLAN.md) §4: parallel to phases B and C (CPU only; touches the selection code, not the arena, descriptors or PSOs), starting with C0 now, C1-C6 next, C7 converging with R8a (phase C, last slice) and R8b (phase F), C10 with R13. It does not wait for the post-chain families and it makes their later per-frame cost smaller (the shadow, effect and YEBIS notes all assume the selection is cheap).

Frame scenarios from the S1 baseline of 166 ms (estimates; re-base after C0):

| After | collect | prepare | render | frame | fps |
|---|---|---|---|---|---|
| baseline (M2) | 104 | 26 | 23 | 166 | 6.0 |
| C1-C3 | 65-80 | 26 | 23 | 127-142 | 7-8 |
| C1-C6 | 40-55 | 26 | 23 | 102-117 | 8.5-10 |
| C7 (C1-C6 as structure) | 25-35 | 26 | 23 | 86-97 | 10-11.5 |
| C7 + C8 + C9 | 12-25 | 26 | 23 | 73-86 | 11.5-13.5 |
| + phases B-C (prepare divided, R8a) | 12-25 | 8-12 | 20 | 52-69 | 14.5-19 |
| + phase D (GPU wait hidden) | 12-25 | 8-12 | 3-5 | 35-54 | 18.5-28 |
| + phase F (collect overlapped with render) | 12-25 | 8-12 | 3-5 | 20-37 (bound by the longer path + the 12 ms outside) | 27-50 |

The 12 ms outside the three phases is kept in every row. The scenario also assumes the engine's own frame work stays at its present size; the original note (Q3) may change it.

## 8. Live rows (none taken)

| ID | Orig/Nat | Read | Expected |
|---|---|---|---|
| CL1 | Nat | C0 sub-phase table at S1 (five repeats, `-NoDebugLayer`) | the split of §4 (K1 largest); the 12 ms outside accounted |
| CL2 | Nat | digest of all constant blocks and texture selections per frame, old path against new path, at S1 and the five zones | equal |
| CL3 | Nat | images at Native1254 and Native1191 with the cached paths | identical |
| CL4 | Nat | a texture streaming change (low-resolution to full package) and a LOD change with the caches on | the new texture bound within one frame; digest equal to the rebuild |
| CL5 | Nat | persistent records: count of rebuilt records per frame in steady state, at a load and at a teleport | 0 in steady state; all after the invalidation events |
| CL6 | Nat | worker overlap (phase F): digest equal with and without the worker | equal |
| CL7 | Orig, Nat | (needs the original note, Q1-Q3) objects, children, parts per frame in the original and natively | equal selection |
| CL8 | Nat | `PrepareReadShaderRegister` calls per frame after C1, C2, C3 | under about 6,000 after C1; under 1,500 after C1-C3 |

## 9. Risks and open points

1. **The figures in §4 and §6 are estimates.** The 104 ms is measured; its split is not. C0 is cheap and decides the order of C2-C6; if K1 turns out below 25 ms the rest of the table is re-ranked, but C1 stays first because it removes work with no cache.
2. **Invalidation** (C4-C7): material replacement, texture streaming and LOD changes, animation rows, wet/damage values, fade, phantom rows, camera cuts. The shadow-compare mode is the guard; the version sources are engine-side writers listed in R8B-ENGINE-READS-STATIC §3 plus the native registries (`materials`, `textures`, `effects`).
3. **Threading**: C10 needs the records immutable; C7's persistent records are by construction (shared, const), the per-frame delta is built on the owner thread.
4. **Interaction with R8a** (phase C): R8a persists the *prepare-side* draw objects; C7 persists the *collect-side* records. Use one version key set so one invalidation drives both.
5. **OPEN**: whether the engine's walk (K6) is larger than estimated (it includes the occlusion-less culler and the world query); whether the 12 ms outside are the engine's update tasks; the allocator behaviour of the process heap under 30,000 small allocations per frame (C0's allocation counter).

## Exports

None. Evidence (outside Git): `evidence/I-0068/queries/collect/f1/` (decompiled `D24490`, `D24430`, `D245A0`), the retained run `evidence/I-0066/artifacts/i66-native-1276-m2/controller.jsonl`. Sources read: `native/frame/game_bootstrap.cpp` (`:2280-2480`, `:3044-3110`, `:3230-3363`, `:3430-4140`, `:4166-4353`, `:4401-4575`, `:4631-4723`, `:5426-5558`), `native/frame/game_model_selection.cpp`, `native/content/shader_reads.h`, `shader_constants.h`.

Preparation of C0-C7 (files, functions, lines, commit order, measures, witnesses): [COLLECT-K-PREP](../I-0066-independent-native-startup/COLLECT-K-PREP.md).
