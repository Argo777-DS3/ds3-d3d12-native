# Workstream K (collect), reprises C0 to C7: preparation (static, 2026-10-11)

Request (coordinator, 2026-10-11): prepare workstream K the way phases A to C were prepared ([D3D12-PHASES-A-C-PREP](D3D12-PHASES-A-C-PREP.md)). For C0 to C7 of [COLLECT-NATIVE-REPRISES](../I-0068-static-yebis-ffx-lights-spec/COLLECT-NATIVE-REPRISES.md) (part BE): the files, functions and lines touched, the dependencies, a reversible commit order, the measurement before and after, and the visual non-regression witnesses. The note of the menus static thread on the original's `E006A0` collect is integrated in §8. No native code was changed, nothing was launched.

Labels: **observed** = read in the native source at main `01de1ce8` (line numbers are that tree's and will move); **estimate** = an unmeasured figure from the study, to be re-based on C0; **proposed** = this note's design; **open** = needs a read or a measurement first.

## 0. Answer first

1. **Commit order** (details in §4): K0 sub-phase timers and counters (C0); K1 the collect digest and the `verify-collect` shadow mode, no behaviour change; K2 produce only what the rows use (C1); K3 shared scene, view and instance blocks (C2, C3) and a prepared-layout cache, in three commits; K4 texture-binding cache with a registry generation (C5); K5 dynamic-constant cache (C6); K6 material-block cache (C4); K7 the persistent admission record (C7). Clean-ups C8 can go in anywhere; C9 and C10 follow C7. Each commit builds, passes the unit tests, keeps the previous behaviour behind a flag file, and is judged by the digest first and by milliseconds second.
2. **Four facts of the code shape the design**, none contradicts the study:
   - **The block consumers read only the row's own recipe.** In `PrepareNativeModelFrame` the blocks are read at `numeric_blocks[prepared.recipe][stage]` (`:4948`), `scene_blocks[...]` (`:4949`), `instance_blocks[prepared.fade][prepared.recipe][stage]` (`:5123`), `view_blocks[prepared.recipe]` (`:5127`) and `shader_textures[prepared.recipe][stage]` (`:5297`); the only collect-side reader of another recipe is the phantom alternate test (`:4526-4527`). So C1 (produce at row time) is safe by construction.
   - **`Programs` objects are per layout, not per shader.** `NativeModelLayout::Programs` (`:465-495`) owns its own `interfaces` and `reads` (reflection of the DXBC, built once per layout in `AcquireNativeModelPrograms`, `:3363`). Two meshes with different layouts and the same programme hold different `ShaderInterface` copies, so a "once per shader" table must be keyed by the **programme bytes identity** (`&ProgramBytes(recipe, stage)`, stable inside the shared `NativeShader`), never by the interface pointer.
   - **The caches can be keyed by a digest of the inputs, not by an engine version.** Everything C4-C6 needs is already read from engine memory (the material fields, the parameter-animation rows, the wet inputs); hashing those values costs less than rebuilding the vectors, and a changed byte gives a new key, so staleness is impossible by construction. Engine-side versions (R8b F5) can replace the digest later without changing the cache interface.
   - **Prepare copies the blocks again.** `ShaderRegisterBlock block; block = mesh.numeric_blocks[...]` copies the byte vector per row, per stage, per frame (`:4948`, `:4949`, `:5123`, `:5127`). Making the stored blocks `shared_ptr<const>` (K3a) removes that copy in collect and gives R8a a stable handle; it is a type change that touches prepare's four lines.
3. **Measurement is the gate** (§3): K0 first, five repeats at S1 with the phase A settings; the order of K3 to K6 is re-ranked by its table. A reprise whose measured gain is under half of its estimate is stopped after its flag test, not abandoned silently.
4. **Non-regression has an automatic half**: the digest of every block, texture lease and dynamic byte a row uses, computed by the old and the new path in the same frame (`verify-collect.flag`) and compared (§5); the visual half reuses the phase A-C pilots plus five cases that exercise the caches (§5.2).

## 1. What the code does today (observed)

| Region | Lines (`01de1ce8`) | What |
|---|---|---|
| tick window | `:5551-5555` | `native_phase_ticks[0]` around `CollectNativeSelectedScene` and `CollectNativeOffscreenScenes`; read at `:696` into `collect_ms`; the pacing row is a 1024-byte buffer (`:700-710`) |
| collect set-up | `:4648-4685` | fresh `NativeSelectedScene`, common images, sampler bank, environment rows (`134A20` ×2), wet catalog (`:4672`), view constants, `CollectNativeVelocityInputs` |
| walk | `:4686-4701` | engine culler visitor (`234620`), world query (`C1FB0`) calling `AdmitNativeSelectedObject` (`:4167`) |
| candidate admission | `:3431-4140` | per model (`AdmitNativeModelCandidate`), per mesh (`:3594-4122`) |
| per mesh: material | `:3602-3620` | `model_material_lock`, about 10 guarded reads |
| per mesh: layout, programmes | `:3625-3629`, `:3654` | `model_layout_lock`; `AcquireNativeModelPrograms` (`:3231`): `material_lock` find, engine `D24430`, `effect_lock`, `programs_lock` |
| per mesh: texture bindings | `:3655-3728` | candidate path vector, engine `D24490` per candidate, `textures.Acquire`; facegen slot (`:3663`) separate |
| per mesh: scene bindings | `:3729-3761` | slots 38, 45, 41, 42, 30, 31, 33, 44, 59-62 |
| per mesh: shader textures | `:3762-3824` | `SelectModelShaderTextures` per recipe and stage |
| per mesh: dynamic constants | `:3831-4048` | `dynamic_bytes`, generated/damage/wet/phantom writes, 22E930 block |
| per mesh: numeric parameters, blocks | `:4049-4119` | `numeric_writes`; per recipe and stage the four `PrepareReadShaderRegister` families |
| record move | `:4123` | `selected.meshes.push_back(std::move(bound))` |
| pass walk | `:4402-4558` | 8 engine passes × models × fades × meshes; `prepared_meshes.push_back` at `:4553` |
| light collectors | `:2741`, `:3045` | point lights, `cbLight`/`cbFog` |
| publication | `:4707` | `native_selected_scene = result` (the previous scene is destroyed here) |
| consumers in prepare | `:4948-4949`, `:5014`, `:5123`, `:5127`, `:5297-5313` | blocks, lightmap binding, shader textures, scene bindings |
| flag files | `:8975-8986` | `no-debug-layer.flag`, `measure.flag` read next to the module at startup into globals (`measure_run`, `:619`) |
| build | `Build-GameBootstrap.ps1:131-136`, `:159`, `:222` | the unit list, the object list, a test executable per component, the run line |

## 2. The reprises

### 2.1 C0 - sub-phase timers and counters (K0)

- **Touches**: new `frame/collect_metrics.h/.cpp` (a `CollectScope` RAII timer over `QueryPerformanceCounter`, a counter struct, `FormatRow()` that writes the row `native_collect_phases`) and `frame/collect_metrics_test.cpp` (the formatting and the accumulation); `game_bootstrap.cpp`: scope markers at `:4648-4685` (set-up), `:4686-4701` (walk, with the admission scopes subtracted), `:3602-3620` (material), `:3625-3654` (layout, programmes), `:3655-3761` (texture bindings and scene bindings), `:3762-3824` (shader textures), `:3831-4048` (dynamic), `:4049-4119` (blocks), `:4402-4558` (passes), `:4703-4706` (lights), `:4707` (publication and teardown), and the offscreen call at `:5554`; the emission next to the pacing row (`:707`) as its own row; `Get-PacingMedians.ps1` (this folder) prints the new row's medians; `Build-GameBootstrap.ps1` unit and test lists.
- **Fields** per window of 120, per frame averages: times (with the four scopes of the original note's row C-0b for the ~12 ms outside the three phases: `frame_begin_to_scene_update`, `render_step_body` (UI `PrepareFrame` apart), `end_frame_to_clear`, `clear_tail`) `setup`, `walk_engine`, `material`, `programs`, `textures`, `scene_bindings`, `shader_textures`, `dynamic`, `blocks`, `passes`, `lights`, `teardown`, `offscreen`; counts `objects`, `children`, `parts`, `models`, `meshes`, `resolved_recipes`, `rows`, `register_calls` (a counting wrapper over `PrepareReadShaderRegister`), `engine_texture_lookups` (calls of `D24490`), `texture_acquires`, `programs_created` (`:3363`), `mutex_acquisitions` (the five locks), `guarded_reads`, `block_bytes` (sum of produced block sizes), and under `measure_run` only `heap_allocations` (a development `operator new` counter).
- **Depends on / enables**: nothing / the re-ranking of K3-K6 and the baseline for every later row.
- **Slices**: K0a the metrics component and its test; K0b the scopes (no behaviour change, the scopes compile to nothing without `measure_run` except the two timestamps per scope, about 40 ns each, so a few thousand per frame); K0c the row and the medians script.
- **Pitfalls**: the scopes must not change the order of locks or of reads; the timers must also work on the offscreen path (`native_offscreen_preparing`) without double counting; the counting wrapper must not change `PrepareReadShaderRegister`'s signature for the tests of `content/`.

### 2.2 K1 - the collect digest and the verify mode (no reprise number; the guard of C1-C7)

- **Touches**: new `frame/collect_digest.h/.cpp`: `DigestSelectedScene(const NativeSelectedScene&)` returning a 64-bit FNV-1a over the ordered prepared rows: model entity, mesh ordinal, engine pass, fade, recipe, then for each stage the used blocks (`numeric`, `scene`, `view`, `instance` bytes and statuses), the `ShaderTextures` status and registers, the texture lease identities (record pointer and its generation) and the mesh's `dynamic_bytes` and `dynamic_written`; a `DigestOfRow` variant for the mismatch report; `collect_digest_test.cpp`; `game_bootstrap.cpp`: after `PrepareNativeSelectedPasses` (`:4702`) when `verify-collect.flag` holds, rebuild the same scene through the **legacy path** (a build-time kept function, `AdmitNativeModelCandidate` with the eager producers) into a second `NativeSelectedScene` and compare the two digests; log `native_collect_verify` with the first mismatching row and slot.
- **Depends on / enables**: K0 (for the cost of verify mode, never counted in a measurement run) / every later slice.
- **Slices**: K1a digest and test; K1b the flag and the dual build (the legacy path is `AdmitNativeModelCandidate` as it stands, selected by a parameter `eager=true`, so no code is duplicated; K2 keeps the old behaviour under that parameter).
- **Pitfalls**: the digest must ignore pointer values that differ legitimately between two builds in the same frame (the shared `shared_ptr` identities are the same objects; compare record pointers, not heap addresses of blocks); dynamic content between the two builds must be identical, so both builds read the engine within the same call (no engine update in between: it is the same owner thread).

### 2.3 C1 - produce only what the rows use (K2)

- **Touches**: `game_bootstrap.cpp`: move the shader-texture selection (`:3762-3824`) and the four block families (`:4059-4119`) out of `AdmitNativeModelCandidate` into two functions `EnsureShaderTextures(scene, model, mesh, recipe)` and `EnsureRecipeBlocks(scene, model, mesh, recipe, fade)`; call them from `PrepareNativeSelectedPasses` (`:4402`): `EnsureShaderTextures` for the phantom alternate before the readiness test (`:4524-4527`), then both for the chosen `recipe` just before `scene.prepared_meshes.push_back` (`:4553`); `NativeSelectedMesh` (`:2281`) gains `uint16_t blocks_done` (one bit per recipe) and `std::array<uint16_t,2> instance_done` (per fade); in `PrepareNativeSelectedPasses` the iteration takes `scene.models[model_index]` by non-const reference (the scene is private until `:4707`); the hoisting of `scene_ranges` and `view_range` out of the per-model code (`:3592-3593`) into two members of `NativeSelectedScene` computed once per frame.
- **Depends on / enables**: K1 (guard) / K3.
- **Slices**: K2a introduce the two functions and call them from the old place (pure refactor, digest equal); K2b move the calls to the row emission and delete the eager loop behind `eager-blocks.flag` (the flag keeps K2a's eager call).
- **Pitfalls**: a row emitted twice (two fades, the split engine5 row) must not recompute the recipe's blocks (the bit mask); the instance block is per fade, so the second fade needs its own entry; the `Unsupported` texture-envelope witness (`:3770-3774`) must keep firing once per (mesh, recipe) at first computation; the scroll/tiling code at `:4501-4550` runs before the recipe is known to be `Resolved`, so the call goes after the `Resolved` check (`:4531`).
- **Expected**: `register_calls` and `SelectModelShaderTextures` calls fall to the number of rows × stages (at most about 1,077 × 2 for textures, about 8,000 for blocks) from about 34,000 and 8,928; collect -20 to -35 ms (estimate).

### 2.4 C2 and C3 - shared scene, view and instance blocks (K3a, K3b)

- **Touches**: `NativeSelectedMesh` block members become `std::shared_ptr<const ds3content::ShaderRegisterBlock>` (`:2325-2333`); the four read sites in prepare (`:4948`, `:4949`, `:5123`, `:5127`) take the pointer (a null pointer = missing); `NativeSelectedScene` gains `ShaderBlockTable` keyed by `{program bytes identity, stage, slot}` for slots 8 and 0 (scene and view), filled by `EnsureRecipeBlocks`; `NativeSelectedModel` gains a small `unordered_map` keyed by `{program bytes identity, stage, fade}` for slot 4 (instance blocks), since `InstanceConstants` are per model and fade (`:2368-2372`).
- **Depends on / enables**: K2 / K5-K7 (the cached material blocks use the same pointer type).
- **Slices**: K3a the type change (digest equal, `register_calls` unchanged, a prepare-side copy saved); K3b the two tables (digest equal, `register_calls` falls by the scene/view and instance shares); K3c (added after the original-side note, §8) a **layout cache** per (shader interface, reads, slot): the resource and declaration lookups and the read-word mask that `PrepareReadShaderRegister` recomputes on every call become a prepared layout, so a call only fills bytes (the native form of the original's "layout fixed at creation, values per draw"); it also speeds the calls that remain (numeric and instance blocks).
- **Pitfalls**: the key is the programme bytes, not the interface (§0 fact 2); the scene block depends on `camera_constants` of the scene, which is per frame, so the table is per scene and dies with it; the forward pixel stage builds its own scene block in prepare (`:4950-4960`), unchanged; offscreen scenes (menu views) have their own camera constants and their own table.
- **Expected**: the scene/view share (about 1/3 of the remaining calls) and the instance share (about 1/3, divided by the meshes per model, 5 to 9) disappear: `register_calls` ≤ about 1,500 after K2+K3 (estimate).

### 2.5 C5 - texture bindings cached by registry generation (K4)

- **Touches**: `frame/game_texture_ingress.h/.cpp`: a `std::atomic<uint64_t> generation_` bumped whenever a record is inserted, becomes resident, is replaced or is retired (the writers of `records_`; `Acquire` at `:230`, `Retire` at `:234`) and a `Generation()` accessor, with a case in `game_texture_ingress_test.cpp`; `game_bootstrap.cpp`: in `AdmitNativeModelCandidate` the binding loop (`:3655-3728`) becomes a lookup in a cache hung on the `Programs` object (`mutable`, under its own mutex): key `{texture registry generation, common images identity, native scene copy identity}`, value the resolved `NativeTextureRecord` per binding; the facegen slot (`:3663-3670`, owner-dependent) stays outside the cache and is resolved every frame; the scene bindings (`:3729-3761`) are computed once per scene into a table `slot -> record` and filtered per mesh by a bitmask of the slots the programme names (`unnamed`, `:3752`).
- **Depends on / enables**: K2 (the mesh record shape) / K7.
- **Slices**: K4a the generation counter and its test; K4b the binding cache; K4c the scene-binding table.
- **Pitfalls**: every event that changes what a path resolves to must bump the generation: residency, retirement, replacement of a low-resolution by a full package, a new `_atlas000`, and the system/common images (`result->common_images` identity is part of the key); `NativeDefaultBoundTexture` and the SYSTEX fallbacks are included in the cached value; the cache is bounded (it lives and dies with `Programs`).
- **Expected**: `engine_texture_lookups` → 0 in steady state; collect -9 to -13 ms (estimate).

### 2.6 C6 - dynamic constants cached by input digest (K5)

- **Touches**: `game_bootstrap.cpp` `:3831-4048` split in two: a **gather** step producing a plain `DynamicInputs` struct (the guarded reads already there: material `+0xD0` flags and blocks, floats, colours, descriptors, provider reads, the parameter-animation rows digest, the wet draw inputs, `cloth_stream`, the SSS profile) and a **produce** function over that struct (the existing `ProduceMatDynScalars` calls, the 22E930 value building, the `pow`); the cache on `Programs`: key `{hash(DynamicInputs), programs identity}`, value `{dynamic_bytes, dynamic_written}` shared and const; new `frame/collect_dynamic.h/.cpp` for the produce function and `collect_dynamic_test.cpp` with golden vectors computed by the old code on recorded inputs.
- **Depends on / enables**: K1 / K7.
- **Slices**: K5a the gather/produce split (digest equal, no cache); K5b the cache.
- **Pitfalls**: the gather must capture **every** input the old code read, including the provider-callback results (`query(provider)` at `:4025` and `:4038`, which call engine code: they stay in the gather and enter the hash); `PosePolicy`, phantom blocks (`:3877-3917`) are inputs too; the `std::pow(…,2.2f)` is deterministic but keep it in produce so the digest comparison is exact.
- **Expected**: `admit_dynamic` -70 % (estimate, -4 to -7 ms).

### 2.7 C4 - material blocks cached by write digest (K6)

- **Touches**: `game_bootstrap.cpp` `:4049-4119` (`numeric_writes`, `stage_ranges`, the `cbMtdParam` fallbacks and `PrepareReadShaderRegister(..., 3, ...)`): inside `EnsureRecipeBlocks` the numeric block is looked up in a cache on `Programs`: key `{programs identity, recipe, stage, hash(numeric_writes: offsets, byte counts, words)}`; the authored parameters feeding `ProduceMtdParams` are immutable for a `Programs` instance (`authored` incarnation is part of the programs identity at `:3287-3289`), so they are not in the key; value a `shared_ptr<const ShaderRegisterBlock>`.
- **Depends on / enables**: K3a (pointer type), K5 (shared digest helpers) / K7.
- **Slices**: K6a the cache with a bound (64 entries per `Programs`, cleared when full); K6b the digest helper shared with K5.
- **Pitfalls**: the "composed group" branch (`:4083-4098`) with no SPX declarations produces `cbMtdParam` from the material's MTD values by name: also covered by `Programs`; a `Programs` rebuilt on an effect or authored change creates a new identity, which drops its caches.
- **Expected**: the numeric share of the remaining block calls → about 0 in steady state.

### 2.8 C7 - the persistent admission record (K7)

- **Touches**: new `frame/collect_records.h/.cpp`: `MeshStatic` (the immutable part of `NativeSelectedMesh`: `current` mesh fields, `slice`, `material`, `scroll`, `layout`, `programs`, `geometry` binding, `texture_bindings`, `dynamic_bytes`/`written`, `numeric_writes`, masks, pose policy, wet parameters, base deformation, material class) and `ModelStatic` (the model-level immutable part: geometry storage, streams, decal choice); `game_bootstrap.cpp`: `NativeSelectedMesh` and `NativeSelectedModel` (`:2281`, `:2336`) hold `shared_ptr<const MeshStatic>` plus the per-frame fields (placement, fade, pose and previous pose, group copies, instance constants, preparation flags, capabilities, cloth world, wind-independent state); `AdmitNativeModelCandidate` computes the **key digest** per mesh (material pointer and the fields read at `:3602-3620`, layout and programs identity, storage and stream generations, texture registry generation, parameter-animation digest, wet inputs digest, `cloth_stream`, `prefer_channel`) and rebuilds `MeshStatic` only on a miss; the store is a map `(model, mesh ordinal) -> record` retired when the model leaves the selection for N frames or its storage generation changes (retirement through the phase B tokens when they exist).
- **Depends on / enables**: K2-K6 (the record is their composition) / C9, C10, R8a, R8b (the same key set).
- **Slices**: K7a records and the map, keyed, behind `no-persistent-selection.flag`, with the digest verify on every miss and every 60th hit; K7b per-frame delta only (stop re-reading unchanged material fields; keep the membership validation of `ReadCurrentModelInputsAt`, which reads the mesh table twice, `game_model_selection.cpp:166-203`); K7c retirement.
- **Pitfalls**: invalidation completeness is the whole risk: the verify mode compares every miss and a sample of hits; a model whose geometry arrives late (missing_geometry rows) must not cache the failure; the record must never hold borrowed engine addresses (the study of R8b E1-E8 lists the reads that stay per frame: placement, fade, group state, wind, pose); keys must also include the `variant`/LOD selection (`lods` mask at `:3590`).
- **Expected**: collect to about 25-35 ms with C1-C6 (teardown and glue gone); to 12-25 ms with C8 and C9 (estimates).

### 2.9 C8, C9, C10 (named only)

C8 clean-ups (witness mutex, wet catalog, `reserve`/`move`) can land anywhere after K0; C9 (pass-row cache inside the persistent record) and C10 (R13 overlap) follow K7 and are prepared with R8b / phase F.

## 3. Measurement, before and after

**Runs** (same settings as phase A: Argo's save, `-Measure -NoDebugLayer`, MotionBlur HIGH, 1280×720, `Run-PilotPair.ps1` after its switch fix, first three in-game windows dropped, at least six steady windows of 120 frames):

| Run | Build | Purpose |
|---|---|---|
| K-M0 | K0 (timers only), five repeats | baseline split of the 104 ms; the **only** run that decides the order of K3-K6 |
| K-M1 | after K2 | gain of C1 |
| K-M2 | after K3b | gain of C2, C3 |
| K-M3 | after K4, K5, K6 | gain of the caches |
| K-M4 | after K7 | gain of the persistent record |
| each | with `verify-collect.flag` once (not for timing) | digest equality |

Scenes: S1 (High Wall idle, the world path), S2 (title and Status with portraits, the offscreen collect), S3 (a warp or load: cache invalidation under load; the cached paths must not make S3's slowest frame worse by more than the cost of the misses).

**Fields and expected values** (from the study; `register_calls` and `engine_texture_lookups` are the structural ones):

| Field (per frame) | K-M0 | after K2 | after K3b | after K4-K6 | after K7 |
|---|---|---|---|---|---|
| `register_calls` | about 34,000 | about 8,000 | about 1,500 | about 300 | about 100 |
| `SelectModelShaderTextures` calls | 8,928 | about 2,200 | about 2,200 | about 2,200 | about 0-200 |
| `engine_texture_lookups` | 2,000-9,000 | same | same | about 0 | 0 |
| `heap_allocations` (measure run) | tens of thousands | -60 % | -75 % | -90 % | -95 % |
| `collect_ms` (estimate) | 104 | 65-80 | 55-70 | 40-55 | 25-35 (12-25 with C8-C9) |
| `prepare_ms` | 26 | 26 | 25 | 25 | 25 |

Decision rule: after each run, a reprise that gains under half of its estimate is held after its flag test and the table is re-ranked from the K-M0 split; prepare and render must not get worse (the shared `shared_ptr` blocks may even reduce prepare's copies).

## 4. Dependencies and the safest order

```
K0 timers ----------------------------------------------------------+
K1 digest + verify (guard) --> K2 C1 produce-at-row --> K3a type --> K3b shared blocks
                                           |                              |
                                           +--> K4 C5 textures (needs registry generation)
                                           +--> K5 C6 dynamic (gather/produce split)
                                                         K3a + K5 --> K6 C4 material blocks
                                           K2..K6 --> K7 C7 persistent record --> C9 rows, C10 overlap (R13)
R8a (phase C last slice) and R8b F5 share K7's key set
```

| # | Commit | Reprise | Behaviour change | Flag (returns to the previous behaviour) |
|---|---|---|---|---|
| K0 | collect metrics component, scopes, row, medians script | C0 | none | none (rows only under the existing `measure.flag` semantics: the row is emitted always, it is one row per window) |
| run | **K-M0** | | | |
| K1 | collect digest, verify mode, test | guard | none | `verify-collect.flag` enables the check |
| K2a | `EnsureShaderTextures` / `EnsureRecipeBlocks` called from the old place | C1 | none | |
| K2b | calls moved to the row emission | C1 | collect faster, blocks only for rows | `eager-blocks.flag` |
| run | K-M1 | | | |
| K3a | blocks as `shared_ptr<const>`; prepare takes the pointer | C2 | none (one copy fewer in prepare) | |
| K3b | scene/view table, instance table | C2, C3 | collect faster | `eager-blocks.flag` |
| K3c | prepared register layouts per (interface, reads, slot) | C2 (layout) | collect faster, outputs byte-identical | `no-layout-cache.flag` |
| run | K-M2 | | | |
| K4a | registry generation + test | C5 | none | |
| K4b | binding cache on `Programs`; K4c scene-binding table | C5 | collect faster | `no-binding-cache.flag` |
| K5a | dynamic gather/produce split + golden test | C6 | none | |
| K5b | dynamic cache | C6 | collect faster | `no-dynamic-cache.flag` |
| K6 | material-block cache | C4 | collect faster | `no-material-cache.flag` |
| run | K-M3 | | | |
| K7a-c | records, per-frame delta, retirement | C7 | collect faster, structure | `no-persistent-selection.flag` |
| run | **K-M4** | | | |

Why this order: the metrics first because they pick the order; the digest before any change because it is the proof of every later commit; produce-at-row before the caches because it removes three quarters of the work without any invalidation question; the type change before the sharing so each step is a small diff; the texture cache after K2 because it needs the registry generation (a test-covered counter in another component); the dynamic cache needs the gather/produce split, which is a refactor in its own commit; the persistent record last because it composes the others and carries the invalidation risk. Every commit is reversible alone: `git revert` of one, or its flag file.

Interaction with phases B and C: K touches `AdmitNativeModelCandidate` (`:3431-4140`), `PrepareNativeSelectedPasses` (`:4402-4558`), the two struct definitions and four read lines of prepare; B and C touch the arena, descriptors, PSOs and `PrepareNativeModelDraw`. The only shared lines are the block reads (`:4948-4949`, `:5123`, `:5127`): do K3a in the same window as, or before, R6/R8a work on those lines, and tell the phase C owner (R8a keys: the digest of K7 is the version it needs).

## 5. Visual non-regression witnesses

### 5.1 Automatic: the digest

Row `native_collect_verify` (flag `verify-collect.flag`): per verified frame, `legacy` and `current` digests, `rows`, `mismatches`, and for the first mismatch the model, mesh ordinal, pass, fade, recipe, stage, slot and which input differs. Pass condition: 0 mismatches over 600 frames at each of S1, S2 and every witness of §5.2, run once after each commit that changes behaviour (K2b, K3b, K4b, K5b, K6, K7) and with the cached paths forced to miss (a debug switch that clears the caches every frame, to prove the miss path equals the legacy path).

### 5.2 Pilot witnesses (each once with the debug layer on, 600 frames, 0 errors; the images compared as in A-C §5)

| Witness | What it exercises for K | Pass condition |
|---|---|---|
| Native1254 in-game, High Wall (`Pilot-NativeWitness.ps1`) | the S1 world path: all block families, texture bindings, dynamic values | `content_draws` equal; the still frame's `changed_pixels` and saved image equal to the previous commit's within the documented dynamic content |
| Status screen and portraits (`Pilot-StatusScreen.ps1`, `Pilot-StatusCycle.ps1`) | the offscreen collect, character parts, FaceGen slot (kept outside the texture cache) | views drawn as before, no persistent gap in the portrait |
| Equipment change in the menu (`Pilot-EquipList.ps1`) | material/programme change on a persisting model (K4, K6, K7 invalidation) | the new armour's textures and constants at once, no stale mesh |
| Character creation (`Pilot-CharMakeWitness.ps1`) | FaceGen composed texture, streams, dynamic materials | face drawn, the composed face updates |
| NPC and cloth (Native1184-1185 witnesses) | cloth streams (key includes `cloth_stream`), pose | cloth moves, no stale vertices |
| Velocity Native1191 | `velocity_rows`, previous pose, `Velo` recipes (blocks by row) | velocity rows and pictures as before |
| Phantoms (Native1218-1224) and the alternate forward recipe (`:4524-4527`) | `EnsureShaderTextures(alternate)` before the readiness test | the same recipe chosen as before the commit |
| Area change or bonfire warp and back | S3: texture streaming (registry generation), storage and stream retirement, cache drops | no fault, the new area's textures within a few frames, caches bounded |
| Rain, water or a hit taken on a character | wet and damage parameters changing (dynamic digest) | the visible change appears the same frame as in the legacy path (digest) |
| Quit (Native1262) | shutdown with persistent records | exit 0 |

### 5.3 Live rows

CL1-CL8 of COLLECT-NATIVE-REPRISES §8, with these additions: **CK1** (K0) the sub-phase table at S1, five repeats; **CK2** (K1) `native_collect_verify` mismatches 0; **CK3** (K4) a texture streaming change with the cache on: the new record bound within one frame; **CK4** (K5) the golden vectors of the dynamic produce function equal the legacy outputs on recorded inputs; **CK5** (K7) records rebuilt per frame in steady state: 0; at a warp: all, then 0.

## 6. Risks specific to this code

1. **A stale cache draws old data.** The digest keys make it impossible for the inputs K4-K7 read; the residual risk is an input read by the old code that the gather/key omits. The verify mode on every miss and on a sample of hits, plus the forced-miss switch, are the controls; a code review of K5a against `:3831-4048` line by line is part of its acceptance.
2. **The scene is private only until `:4707`.** K2 mutates `NativeSelectedScene` during `PrepareNativeSelectedPasses`; nothing may read it from another thread before publication. R13 (phase F) must still see an immutable scene: K2's row-time production finishes inside collect.
3. **The `Programs` caches live on shared objects.** They need their own mutex and a size bound; an unbounded map keyed by write digests would grow with every animated value (wet/damage).
4. **Offscreen scenes** (menu views) run the same code with their own camera constants and `fixed_key` lights; every table keyed by scene must be per scene.
5. **`measure_run` consistency.** Timers and counters must not change what is logged when the flag is absent beyond the new row.
6. **Merge conflicts.** `game_bootstrap.cpp` is shared with B and C; new logic goes in new files (`collect_metrics`, `collect_digest`, `collect_dynamic`, `collect_records`), and the call-site edits are short.

## 7. Limits

Static reading of the tree at `01de1ce8`; no build, no run. Every millisecond figure is an estimate from COLLECT-NATIVE-REPRISES; the only measured number is the 104 ms of phase A. The key sets of K4-K7 are designed from the code read here; the review of each commit's gather step against the old lines is part of its acceptance.

## 8. The original's collect (integrated)

The menus static thread's note [COLLECT-ORIGINAL-STATIC](COLLECT-ORIGINAL-STATIC.md) (main, 2026-10-11) answers the questions this note had posed, and corrects two of its premises. What it establishes and where it lands here:

1. **`E006A0` is not a collect** (its verdict 1): it is the scene stage's per-frame update and visits no entity; the 104 ms is native-only work. The original traverses per render **pass scene** on the draw side (verdict 2), without occlusion for the G-buffer, forward and shadow scenes (verdict 3), and runs the pass scenes of one composite as parallel job tasks on the GX queue by default (verdict 4). The native build does one synchronous union walk. So the walk itself (K6 of the study, `walk_engine`) is not the difference; the per-frame admission is.
2. **Q2 - the original caches no values but fixes layouts**: the per-material record is persistent and pre-laid-out, the values are rewritten on every draw by `FUN_14022D620`/`FUN_14022E930` (called from `FUN_140140850`), with no dirty or version gate found at the call site; there is **no layout analysis per draw**. Consequences for this plan: (a) the cache design of K4-K6 stays valid because it is keyed by input digests, not by engine versions; (b) a new slice **K3c** caches the **layout result** of `PrepareReadShaderRegister` per (shader interface, reads, slot) - the resource and declaration lookups, the read-word mask - so each call only fills bytes; this is the native equivalent of "layout fixed at creation, values per draw" and is safe because those inputs do not depend on the frame; (c) the dynamic-constant production (study K4) and the pass-row evaluation (study K5) are **not excess** relative to the original (it also computes the values per draw, and evaluates its rows in every pass scene): they stay optional (C6, C9), ranked after the block and texture work.
3. **Do not reuse whole `NativeSelectedModel` records across frames** (its section 8 item 2, reviewed): they carry per-frame data (`scene_blocks`, `view_blocks`, `instance_blocks`, pose and previous pose, stream and cloth generations, texture residency with the `_l`/`_atlas000` fallback) that no engine signal covers. C7 (§2.8) already honours this by construction: its persistent `MeshStatic` is **digest-keyed** (the key includes the stream, storage and texture-registry generations and the parameter-animation digest) and the scene, view and instance blocks, pose, previous pose and cloth world stay per-frame; the review's warning becomes an explicit acceptance rule of K7a: no field that is a function of the frame may sit in `MeshStatic`.
4. **Q3 - the ~12 ms outside the three phases** are the engine's own CPU frame (FD4 steps `0x00-0x4F`: game flow, Havok, ChrIns, cloth, AI, sound, network, the Scaleform CPU update) plus the native hooks outside the brackets (`FrameBegin`, the body of `NativeRenderStep` including the UI `PrepareFrame`, the `EndFrame` prelude, the start of `Clear()`). Their split is the original note's row C-0b; **K0 adds the four scopes** (§2.1) so the number is measured instead of subtracted.
5. **Q1 - draws**: the native main view draws 1,062 rows per frame at S1 (`content_draws`); the original draws the same rows plus the rows of the pass scenes the native build does not run (shadow maps, other views); the total needs the original's row C-6 (a live read). Not needed to start K.
6. **The missing denominator**: the number of admitted **meshes** is not logged (the counters have different units: `program_recipe_pairs` sums resolved recipes, `prepared_meshes` counts rows). K0 logs `meshes` (the original's row C-0). The ratios used in COLLECT-NATIVE-REPRISES §3 (recipes per mesh) are therefore estimates until K0 runs.
7. **Parallelism** (its section 8 item 5): enabling the node-job byte would do nothing here (`DAT_144961BC3` has no writer; the jobs go to the GX task queue the native build does not consume). A native job queue splitting the traversal by layer or pass scene is R13's engine-side work (C10), not a flag.
8. **Original rows adopted**: C-0 (meshes), C-0b (the outside split) and C-1 to C-4 are covered by K0's scopes; C-5 (models whose dirty signals changed per frame) is the reuse rate a version scheme could reach and belongs to R8b; C-6 and C-7 (the original's draw and thread counts) are live reads for the original only.
