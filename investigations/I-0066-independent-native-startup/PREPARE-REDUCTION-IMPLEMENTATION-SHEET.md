# Reducing `PrepareNativeModelFrame`: what it redoes every frame, how to invalidate it, R8b S1 to S4 as the same refactor (static, ready to implement)

Request (coordinator, 2026-10-11 16:48 UTC): after workstream K (collect 108 to 27 ms, 14.5 frames/s, frame about 70 ms of which prepare 24.6 ms), write the sheet ready to implement that reduces prepare: R8b S1 to S4 (prerequisite of the F2 cut of the [R13/R14 sheet](D3D12-R13-R14-RENDER-THREAD-PACING-SHEET.md)). It says what prepare redoes every frame that the original does not, how each element is invalidated, what gain to expect, with commits under flags, tests and witnesses. Static reading only: nothing was built, run or measured here.

Base read: the K thread's branch `claude/dreamy-vaughan-fd4276` at `22253413` (P1 to P6, R8a, P4b; **merged into `main` after the first draft of this sheet, at merge 73fbbd84 the file is in `main`; the lines may have shifted, re-anchor them at cP0**) for `native/frame/game_bootstrap.cpp` (every `:line` below is `22253413`'s), `main` for `native/renderer/model_draw.{h,cpp}`, `native/content/model_palette_indices.cpp` and the documents. `decompiled/DS3-1152-F-<RVA>` exports for the original. Labels: **STATIC** (read in code), **INFERRED**, **OPEN**, **proposed**, **derived** (arithmetic on measured figures). Measured figures are the K sheet's Native1298 (S1, no debug layer): collect 27.2, **prepare 24.6**, 14.5 frames/s; engine before the scene update 4.8, UI `PrepareFrame` 6.2, `Clear` 30.4 (C-0b); the K sheet also names prepare's remaining cost "constant writes 9.5 ms, resources 10.7 ms". The validation phase and the draws sub-split come from M4/Native1288 (before K) and are assumed unchanged; cP0 re-measures everything. The sheet was reviewed by an independent Opus 5.5 pass (three blocking, ten important findings); all are applied.

## 0. Answer first

1. **Prepare is 24.6 ms because it re-derives, for each of about 1,077 rows a frame, things that are functions of far fewer things.** The units (S1, Native1289/1298): 126 models, 1,001 meshes, 1,077 prepared rows, 1,062 draws; programmes, geometry, states and samplers are shared by many rows; group state, pose and placement are per **model** (about 8 rows each); the scene, view and time blocks are per **frame**. Prepare rebuilds each of them per row (section 3: 24 items PR-1 to PR-24).
2. **What the original does not redo (STATIC, section 2):** layout analysis, programme bytecode comparison, state-object assembly, descriptor-table construction, and the per-draw copy of pass-level blocks (it uploads the scene, view and light blocks once per pass scene and the pose palette once per model). **What the original does redo per draw** and is therefore *not* an excess against it: the programme selection (a virtual call, by pointer), the material and model parameter values (`FUN_14022D620/22E930`, `FUN_14005B3D0/140059470`, `FUN_1401A86C0`); the native side can still do better by building the model parameters per model, because it has many rows per model.
3. **Invalidation is by construction wherever possible (section 4).** Five classes: C0 programme lifetime (built when the `Programs` object is created), C1 mesh-stable (keyed by versions that already exist), C2 model per frame and C3 frame, **both call-local tables of one `PrepareNativeModelFrame` call** (no cross-frame staleness, no write into the immutable scene), C4 optional cross-frame reuse by snapshot digest, only if C2's measured dirty rate pays. The permanent guard is the in-run shadow comparison, as in K.
4. **R8b S1 to S4 and this reduction are the same edits** (section 5): turning the per-row engine reads into per-model snapshot values is what lets the model-level blocks be built once; it is also what makes prepare thread-independent.
5. **Expected gain: prepare 24.6 ms to about 7 to 12 ms (planning 10), i.e. 12 to 17 ms off the frame (70 to about 53 to 58 ms, 14.5 to about 17 to 19 frames/s)** (section 6, derived, a ceiling until cP0 re-measures the split). With the R13 F2 cut the engine thread no longer waits for prepare; the reduction then shortens the worker's load (about 30 to 15 ms) and the latency by the same amount.
6. **Commits (section 7):** cP0 split and counters; cP1 `model_draw` API (shared handles, legacy switch); cP2 extraction into a linkable unit, row set-up, resource plan; cP3 programme identity, state id, binding shape; cP4 palette-check cache; cP5 scene-level shared blocks (call-local table, `ConstantChunk` ranges); cP6 model-level values on the snapshot (= R8b S1 to S3); cP7 forward light and frame lighting (= S4); cP8 sampler tables and descriptor batching; cP9 M-run. Convention of K: `rebuild-prepare.flag` runs the legacy path, the new path is the default from the commit that introduces it, `prepare-shadow.flag` compares.

## 1. Base and units (STATIC unless marked)

- **Structure of prepare** (`PrepareNativeModelFrame` `:5328-5974`): for each prepared row (`scene->prepared_meshes`) it builds a `NativeModelDrawInput` (elements, two shader bytecodes, geometry, fixed-function state, constants, SRVs, samplers) and calls `PrepareNativeModelDraw` (`:5885`), whose `OwnedNativeModelDraw::Prepare` (`native/renderer/model_draw.cpp:296-411`) holds the draw-cache recursive lock for almost its whole body (released only to create a pipeline): programme lookup, binding validation, pipeline key and lookup, constant copy into an upload chunk, descriptor writes. Then, per frame, the lighting inputs (`:5897-5950`).
- **Timers** (`native_prep_ticks`): `[0]` palette-index validation (`:5406-5408`), `[1]` the resource loop (`:5484` to `:5884`), `[2]` `PrepareNativeModelDraw` (`:5884-5888`). Native1288/M4 split (before K): validate 1.8; the draws part 9.4 = programmes 1.2, validation 1.1, pipeline key and lookup 1.8, constants and descriptors 5.0, other 0.3 (**derived**: the 1287 and 1288 runs give 9.4 and 9.5). Native1298 (after K): prepare 24.6, resources 10.7, draws 9.5. **Derived: 24.6 - (1.8 + 10.7 + 9.5) = 2.6 ms are in no phase** (the validation figure assumed unchanged).
- **Scope of the figures (derived from `:723-724`, `:7051`):** `prepare_ms` of the pacing row is the main-view prepare only, per frame; `native_prep_ticks` and the model-draw `prepare_ticks` accumulate **every** call including the menu offscreen views (run inside the render window) and are divided by the render count. The 2.6 ms is therefore exact only in a zone with no offscreen view (S1); cP0 separates main and offscreen and makes the counters atomic or thread-local (R13 X9).
- **What the 2.6 ms contain (INFERRED, to be split by cP0):** the row set-up before `:5484` (pose binding `:5360-5381`, element and bytecode copies, state set-up `:5421-5480`, the vertex span search `:5391-5403`), the forward-light production (`:5458`), the `AsmTally` destructors after the last timer (`:5338-5343`), the frame lighting (`:5897-5950`) and the tail of the function.
- **Units (S1):** 126 models, 1,001 meshes, 1,077 prepared rows, 1,062 draws, 8,928 recipe x stage pairs; 24.6 ms / 1,062 = about 23 us per draw. Per model about 8 rows. **C-5 of the K sheet: 113 of 126 models keep the same per-frame inputs from one collect to the next** (its signature excludes the group state and the wind block, `:4156-4174`, so it is a lower bound of the unchanged rate and R8B-3 stays needed). Blocks, SRVs and palette bytes per draw are **not measured** (cP0 adds them).
- **K state seen from prepare:** the blocks of `numeric_blocks`, `scene_blocks`, `view_blocks`, `instance_blocks` are `shared_ptr<const ShaderRegisterBlock>` (P2) but prepare still **copies** them (`block=*stored`, `:5495`, `:5496`, `:5670`, `:5674`), moves the vector into `input.constants`, and `Prepare` copies it a second time into the upload chunk; the register packing per draw uses plans made once per (recipe, stage, slot, name) (R8a, `PrepareNativeProgramRegister` `:5317`) but takes `draw_plan_lock` and compares the name string on every call; `rebuild-collect.flag` already disables those plans in prepare (`collect_rebuild`, `:5320`).

## 2. What the original does (STATIC, [COLLECT-ORIGINAL-STATIC](COLLECT-ORIGINAL-STATIC.md) Q1/Q2, `FUN_140140850`)

| Aspect | Original, per draw | Original, per other unit | Native prepare today |
|---|---|---|---|
| Programme | the programme is selected by a **virtual call on the material's pass object per draw** (`(*plVar11+0x10)`, `140140850.c:87`), by pointer; the per-material parameter record is indexed by context (`param_1[0x13] + [[ctx+8]+0x2200]*0x28`, `:141`) | created with the material | `CachedProgramOf` hashes a strided sample of the bytecode and compares the whole bytecode of VS and PS to the cached copy, per draw |
| Fixed-function state | D3D11 state objects chosen by index in the draw context (rows of `14473C9F8` for cull class) | | state fields rebuilt per row (`:5421-5480`) then appended byte by byte to a pipeline key string (`Prepare`, `Append*`) |
| Layout of constants | fixed when the material is created (no per-draw analysis) | | plans (R8a) looked up under a mutex by name; `ValidateBindings` re-checks every declared register per draw |
| Values of material parameters | rewritten per draw (`22D620/22E930`) | | produced at collect (`mesh.dynamic_bytes`) and packed per draw; **parity** |
| VS and PS model parameters | assembled **per draw** (`5B3D0/59470`, `1A86C0`) from the draw state | | rebuilt per row from engine memory (`Read`) and packed; **parity** (same multiplicity as the original); a per-model build is a native improvement |
| Pass-level blocks (scene, view, light, time) | | uploaded **once per pass scene** | rebuilt or copied **per draw and per stage** (slots 8, 0, 9, forward 5/6) |
| Pose palette | | once per model | packed and copied per row (twice: packed block then `std::copy`), plus a padded copy per row when the shader indexes beyond its declared size (`:5747`) |
| Palette index safety | none found (INFERRED from the absence; D3D11 returns zero for out-of-range constant reads, native comments `:5708`, `:5720`) | | `ValidateModelPaletteIndices` (`native/content/model_palette_indices.cpp:232-296`): copies the programme, re-runs the full DXBC reflection `InspectShaderInterface`, SHA-256 (BCrypt) of the bytecode, the instruction-pattern match, then scans every vertex index; per skinned row per frame |
| Resource binding | by slot on the immediate context; no tables | | per-draw CBV creation, per-SRV `CopyDescriptorsSimple`, per-draw sampler-set build and FNV hash (`SamplerTableOf`) |
| Engine state | direct member reads | | `Read` (a guarded copy) per field per row |

So the avoidable volume is not "the values" but the **D3D12-only plumbing** (programme identity, state key, tables, validation) and the **multiplicity of pass-level and pose data**; the model-parameter multiplicity is parity with the original.

## 3. What prepare redoes every frame (PR-1 to PR-24)

`Per` is the unit the item currently runs at; `Right unit` is the unit its inputs vary at. Costs are the phases that contain the item; the item-level split is cP0's job.

| Id | Redone (`22253413` lines) | Per | Right unit | Original | Phase |
|---|---|---|---|---|---|
| PR-1 | `AsmTally` bookkeeping: map lookups `map[part]`, `families[family]`, a `NativeFamilyKey` string built from `program_path` per character row (`:5338-5343`), the per-frame `std::map`s and `LogNativeAsmSummary` (`:5329-5331`, `:5897`), sampling counters and `Read` of the material generator in failure branches (`:5540`) | row | diagnostics, sampled; the family key per `Programs` | none | row set-up |
| PR-2 | the vertex-element support loop (`:5345-5352`) | row | mesh layout (pure of `layout`, `slice.stride`, `stride2`) | none | row set-up |
| PR-3 | `NativeModelDrawInput` assembly: `input.elements` copy (structs holding `std::string` semantics), VS and PS bytecode **copied** from `ProgramBytes` (`:5356`, `:5358`), then in `Prepare` `CachedProgramOf` (strided hash + full compare per stage) and a `D3D12_INPUT_ELEMENT_DESC` array | row | `Programs` + recipe (identity), layout | programme pointer | row set-up + draws "programs" 1.2 |
| PR-4 | `ValidateModelPaletteIndices` (`:5391-5411`): bytecode copy + full `InspectShaderInterface` reflection + SHA-256 + pattern match + scan of all vertex indices, for rows whose VS reads b5/b12 dynamically. **No bytecode is rewritten** (`result.shader_bytecode` is the moved copy of the same bytes, `model_palette_indices.cpp:237-243`) | skinned row | (programme, layout, vertex source, row counts) | none | validate 1.8 |
| PR-5 | `SelectOrdinaryModelPoseBinding` two to four times per row (`:5360-5381`; four for velocity rows) | row | (pose version, cloth flag, flags bit 14): about 3 results per model | none | row set-up |
| PR-6 | fixed-function state tuple: rasteriser, depth/stencil, blend, formats (`:5421-5480`), appended field by field to a byte string (several hundred bytes) and looked up in an `unordered_map<std::string,...>` | row | tuple (cull class, depth-write, engine pass, blend mode, fading, phantom, velocity): a few dozen values | state index | draws "pipeline key and lookup" 1.8 |
| PR-7 | the per-stage resource loop (`:5484-5884`): for every declared resource, a string-compared chain (`slot==...`, `resource.name==...`), `std::find_if` over the constant declarations by name (several times per row), `PrepareNativeProgramRegister` taking `draw_plan_lock` and comparing the plan name | row x stage x resource | `Programs` x recipe x stage (a resource plan) | fixed layout | resources 10.7 |
| PR-8 | frame-constant blocks built or copied per draw: forward PS `cbSceneParam` with four memsets, `NativeShaderWordRanges` and a pack (`:5496-5505`); `cbPerFrame` per draw and stage (`:5662-5668`); slot 0 velocity matrices; forward `cbLight`/`cbFog` pack | row x stage | frame (x programme) | once per pass scene | resources |
| PR-9 | `cbvModelParam` (VS) and `cbModelParam` (PS) rebuilt per row: placement copies, `Read` of `group+0x50`, `+0x60`, animation flags, the 0x410-byte animation block, the 400-byte group and the 16-byte default row, `ProduceModelParamPs`, ranges, pack (`:5578-5659`) | row | model x fade x velocity x cloth override (raw bytes), packed per programme | per draw (parity) | resources |
| PR-10 | pose palette blocks: pack, then `std::copy` of the full palette over the block (`:5758`), a padded copy (`:5747`), for b5, b12 and, for velocity rows, b6; the identity cancel palette allocated per row (`:5700-5703`) | skinned row (x2 palettes) | model (pose version) | once per model | resources + constant writes |
| PR-11 | stored blocks copied: `block=*stored` for slots 3, 8, 4 and 0 (`:5495`, `:5496`, `:5670`, `:5674`), moved into the input, copied again by `Prepare` into the upload chunk | row x stage x slot | shared block | none | resources + constant writes |
| PR-12 | `ValidateBindings` (`model_draw.cpp`): rebuilds the bound sets, `find_if` over declarations by name, the root-shape insert (linear search under a global lock); `ValidateGeometry` (two `GetDesc`, two `GetGPUVirtualAddress`, two COM `QueryInterface`/`GetDevice` pairs) and a `SameDevice` COM pair **per SRV** | draw | (programme, recipe, bound-slot signature) for the shape; resource record version for the device check | none | draws "validation" 1.1 |
| PR-13 | sampler sets: two 16-entry `D3D12_SAMPLER_DESC` arrays (16 x 52 = 832 bytes each, 1.66 KB per draw) filled, hashed byte by byte (FNV) and compared (`memcmp`) per draw (`SamplerTableOf`) | draw x stage | (programme, recipe, stage, scene sampler bank version) | none | draws "constants and descriptors" 5.0 |
| PR-14 | descriptor writes: one `CreateConstantBufferView` per constant block and one `CopyDescriptorsSimple` per SRV, per draw, into a fresh range of the global heap | draw | SRV list identity (stable between frames); CBV targets are per frame | none | same 5.0 |
| PR-15 | forward-light production per forward draw: four `Read`s of the group key words, up to **three** engine functions (`1496D0` or `14A2D0`; `14E900`; `14E5D0` when blending) and two to four gparam record lookups (`:3258`, call `:5458`) | forward draw | the four key words (+ bank identity) per scene: draws with equal key words share the result | per pass | row set-up |
| PR-16 | frame lighting inputs: `AcquireNativeLightingPrograms` (mutex, resolved after the first call), `ProduceNativeLightAccParam`, `ProduceNativeSssInputs`, `ProduceNativeEnvRegions` (engine call `FUN_1400416A0`), point-light tile binning (`ds3tiles::Bin`, `:5897-5950`) | frame (and per menu view) | frame | per frame (the binning is the original's schedule too, `1A1DA0`) | frame-level part of the 2.6 |
| PR-17 | `Read(image+0x4929B20)` per PS `cbModelParam` (`:5641`); the matdyn failure-branch reads | row | once after engine initialisation (E8) | none | resources |
| PR-18 | `caches.lock` (recursive mutex) held through `Prepare` except pipeline creation: serialises preparation | draw | | | draws |
| PR-19 | each `OwnedNativeModelDraw` is a large object (the input with its vectors, 28 `ComPtr`s, tables) allocated per draw per frame and destroyed after the frame completes | draw | | none | draws "other" |
| PR-20 | the `srv_names` diagnostic string: a `sprintf_s` and a path copy per SRV for every character row and every offscreen forward row, not sampled (`:5859-5861`) | SRV | diagnostics, sampled | none | resources |
| PR-21 | `ProduceOrdinaryInactiveModelParameters` per PS row of material class 1/4 (`:5510-5517`) | row | pure function of (shader, damage, wet): C0 | none | resources |
| PR-22 | `cbTexHdrScaleParam` (`:5556-5577`): float metadata copied and produced per row | row | the mesh's lightmap texture (C1) | none | resources |
| PR-23 | empty-range blocks `PrepareNativeProgramRegister(...,{})` (`:5760`) for unmatched slots | row | C0 (a constant block per programme) | none | resources |
| PR-24 | the cloth/skinned padded palette copies and the identity-cancel allocation (see PR-10) when `padded_bytes` | skinned row | model | once per model | resources |

**What must stay per frame (the floor):** the values that really change each frame and per unit — the animated pose palettes once per model, the camera, view and time blocks once per scene, the per-mesh dynamic material values (made at collect), the fade/instance values once per model and fade, the texture residency test, and the CBV writes. That is of the order of the original's "at least three constant writes per draw".

## 4. Invalidation

### 4.1 Classes

| Class | Holder | Lifetime | Invalidated by | Staleness risk |
|---|---|---|---|---|
| **C0** programme-lifetime | the `NativeModelLayout::Programs` object, **built when it is created** (as K's plans, so no lock after publication) | the `Programs` object; replaced, never mutated | a new `Programs`. A `program_registry_generation` bump only *revalidates* and reuses `layout->programs[slot]` while the effect and authored inputs are unchanged (`:3499-3502`); a changed effect gives a new object | none by construction |
| **C1** mesh-stable | a small cache on the object that owns the changing input: palette check on the geometry storage (dies with it); sampler-table index and binding shape on the `Programs` | until its key changes or its owner dies | the version tuple in the key (4.2) **and the `model_draw` cache epoch** (below) | a missed version bump; guarded by the shadow comparison |
| **C2** model per frame | a local vector indexed by model (x fade) created at the start of a `PrepareNativeModelFrame` call | one call (so one table per menu view) | end of the call | none; thread-safe because local; nothing written into the `shared_ptr<const NativeSelectedScene>` |
| **C3** frame | a local table of the same call (shared scene/view/time/forward blocks and their upload ranges) | one call | end of the call | none; same remark |
| **C4** cross-frame reuse of C2/C3 values | a per-model record kept across frames | until the digest of its inputs changes | digest of the snapshot values (group state, placement, animation block, fade, pose version) | real; **only if** the measured dirty rate (row R8B-3; lower bound from C-5: 113 of 126 models unchanged) says C2 is not enough; shadow compare mandatory |

**Device epoch.** `BindDevice` (`model_draw.cpp:226-232`) clears `pipelines`, `sampler_tables` and `sampler_tables_used`; every C1 entry holding a pipeline pointer, a sampler-table index or a `BindingShape` therefore stores the `model_draw` cache epoch (a counter incremented in `BindDevice`) in its key, and keeps the `UINT_MAX` ("sampler cache full, per-draw tables") semantics.

### 4.2 Keys and version sources, per item

Existing version sources (STATIC at `22253413`): `program_registry_generation` (`:389`), `texture_registry_generation` (`:3396`), `TextureIngress::Version` (`textures.Version()`, `:4048`), the texture record pointer (weak) and its `resident/resource/view`, `ModelGeometryBinding::storage_generation` and `stream_generation`, the pose version's `generation` (`:5882`), `scene->serial`, `EngineSamplerBankVersion::generation` (`sampler_bank.h:8,35`).

| Item | Cache | Key | Bumped by / source | On miss | Shadow compare |
|---|---|---|---|---|---|
| PR-1 | family key and diagnostics tallies | `Programs` identity | new `Programs` | compute once | n/a (diagnostic) |
| PR-2, PR-3 | layout support flag; the draw's `ProgramSet` (shared bytecode identity, element list, `D3D12_INPUT_ELEMENT_DESC` array) | (`Programs`, recipe) + layout identity + (`slice.stride`, `stride2`) | new `Programs`; layout object replaced | build at `Programs` creation | compare produced bytecode identity and element descs |
| PR-4 | `PaletteCheck {status, max_index}` (no bytecode) | (`Programs`, recipe, mesh patch, authored `storage_generation`, `vertex_offset`, `vertex_count`, `stride`, `object_rows`, `cancel_rows`, vertex-source class: authored / physical stream / cloth) | storage replaced (generation); skeleton size change. **Streams are immutable versions** (STATIC): a physical stream rewrite makes a new `NativeModelStreamVersion` with `++model_stream_generation` (`:1855`); cloth makes a new `NativeClothStream` with `++cloth_stream_generation` (`:7594`). The physical stream rewrites position, normal and extra, while BLENDINDICES come from the authored bytes (INFERRED; T-P2 checks that no rewritten offset overlaps the BLENDINDICES element). **For cloth the check never sees bytes** (INFERRED from `:5391-5403`: the cloth resource is not in `model.streams`, so the span is empty and the result is `Invalid` after the copy, reflection and hash), so its result is invariant while the stream key would change every frame: the key uses the authored storage and the source class, not the stream generation | run `ValidateModelPaletteIndices`, store | recompute every Nth frame and compare status and `max_index` |
| PR-5 | pose binding | (pose version, `cloth_enabled`, `flags & 0x4000`) | pose generation | compute | compare kind and palette pointers |
| PR-6 | `DrawStateId` + the pre-expanded key fragment | the state tuple of PR-6 | none (pure of the tuple) | build the descs once per tuple | compare expanded descs byte for byte with the legacy builder and the legacy key string |
| PR-7 | `ResourcePlan` per (recipe, stage): ordered list {slot, kind, declaration pointer, producer id, name id} | `Programs`, recipe, stage | new `Programs` | build at `Programs` creation | compare the sequence of produced bindings with the legacy loop |
| PR-8 | scene block table (C3, extended from K's P2 pattern: forward scene block, time block, velocity matrices, forward light/fog packs) | (`Programs` bytes identity, recipe, stage, slot, variant) per call | the call | build on first use | block bytes equal |
| PR-9 | `ModelValues` per (model, fade, velocity, cloth override): raw VS bytes with coverage and raw PS state bytes | those four + the S1/S2 snapshot of the model | the snapshot (values; C2) | build from the snapshot | bytes equal against a build from live reads (the F6 comparison) |
| PR-10 | pose palette block per (model pose version, declared size, padding class, which palette) | as listed | pose generation | pack once | block bytes equal |
| PR-11 | no cache: the binding carries the shared block and the upload copies from it once (section 7, cP5) | block pointer (held alive: no ABA) | | | digest equal |
| PR-12 | `BindingShape` per (`Programs`, recipe, signature = bitmasks of bound constants, SRVs, samplers, device epoch); the device/extent checks of geometry and SRVs once per (storage generation) and per (texture record, version) | signature + generations | `Programs`; storage generation; texture version | run the full check | full check every Nth frame, must agree |
| PR-13 | sampler table index | (`Programs`, recipe, stage, `EngineSamplerBankVersion::generation`, device epoch) | the existing bank version (an option change bumps it); no hashing of the 13 descriptors needed | `SamplerTableOf` | index equal |
| PR-14 | batched writes: `CopyDescriptors` with arrays per stage | SRV list (record pointer + version) | texture versions | write | descriptor handle equality of the copied sources |
| PR-15 | forward light/fog entries | the four key words (+ bank identity) per call | the snapshot | produce once per distinct key | bytes equal |
| PR-16 | lighting inputs | per call | | produce | n/a (frame level) |
| PR-17 | default row (E8) | process | engine initialisation (row R8B-4) | read once after init | equal at frame 600 |
| PR-20 to PR-24 | PR-20 sampled; PR-21 and PR-23 C0 on `Programs`; PR-22 C1 on the lightmap record + version; PR-24 as PR-10 | | | | |

### 4.3 Rules

- **R-P1:** a cache entry stores the *whole key* it was built under and compares it on hit (K's practice), not only a hash.
- **R-P2:** C0/C1 caches hold CPU data and device-lifetime objects only (pipeline states, sampler-table indices), keyed by the device epoch; no per-frame GPU object (upload range, descriptor range) is cached across frames, which keeps them valid under the phase D frame ring.
- **R-P3:** every cache is bounded (LRU of 32 to 256 entries or tied to an owner's life), with a counter of entries and evictions.
- **R-P4 (replaces the first draft's stream-generation rule):** streams are immutable versions with a new generation on every rewrite (above); the palette-check key does not include a stream generation; the test T-P2 asserts the BLENDINDICES non-overlap and the cloth `Invalid` invariance.
- **R-P5:** the legacy path stays in the same build behind `rebuild-prepare.flag` until a later clean-up commit after the witnesses.

## 5. R8b S1 to S4 as the same refactor

[R8B-ENGINE-READS-STATIC](R8B-ENGINE-READS-STATIC.md) lists eight reads (E1 to E8) and four further engine touches (section 4 there). A grep of `Read(` in `:5328-5974` finds them all: E1 `:5540-5541`, E2/E3 `:5595-5596`, E4 to E6 `:5603-5613`, E7/E8 `:5641`, then the forward light, `NativeGparamRecords`, the programme repositories and `FUN_1400416A0`.

| R8b slice | What it provides | Reduction item | Done when |
|---|---|---|---|
| S1 `group_state` (0x190 B per model, taken at collect, replacing the partial reads) | immutable per-model values for E2, E3, E7 and the forward-light key words | PR-9 (VS `+0x60`, `+0x4B0`; PS state), PR-15 | R8B-1 zero differences; prepare makes no `Read` of `model.group` |
| S2 `animation_flags`, `wind_block` (0x410 B) | E4 to E6 as values | PR-9 (VS `+0x80`, `+0x4C0`) | foliage witness equal; no `Read` of `model.animation` |
| S3 material scalars (E1) dropped or captured at admission; E8 cached after engine initialisation | removes the failure-branch reads and the per-row default-row read | PR-1, PR-17 | R8B-4 equal and non-zero |
| S4 forward-light entries, gparam fallbacks, programme repositories, environment-region call to collect or one-time resolution | no engine function and no repository walk in prepare | PR-15, PR-16 | R8B-6 zero |

Two consequences for F2: (a) the snapshot is *also* the model-level cache's input, so no extra read appears at collect (126 copies of 0x190 + 0x410 bytes: about 0.2 MB a frame); (b) **where the work lives matters under F2**: collect is on the engine thread, which is the bottleneck there, so only what F2 needs (the S1 to S4 values and the forward-light entries) is produced at collect; the C2/C3 tables are built inside the prepare call (the worker's), never in the immutable scene (R13 rule V1).

## 6. Expected gain

Base: prepare 24.6 (Native1298) = validate 1.8 + resources 10.7 + draws 9.5 + out of phase 2.6; the draws part split as in Native1288 (programmes 1.2, validation 1.1, pipeline 1.8, constants and descriptors about 5.0). All figures **derived** under the stated assumption; cP0 replaces them.

| Group | Items | Present cost | Assumption | Expected gain (ms) |
|---|---|---|---|---|
| Palette check | PR-4 | 1.8 | the whole phase; the authored storage and cloth cases cache fully (cloth's result is invariant) | 1.5 to 1.7 |
| Programme/state/shape | PR-3, PR-6, PR-12 | 1.2 + 1.8 + 1.1 = 4.1 | identity and ids replace hashing, comparison, key strings and re-validation; a residual about 0.6 | 3.0 to 3.5 |
| Resource loop | PR-7, 8, 9, 10, 11, 17, 20 to 24 | 10.7 | scene-level blocks vanish; model-level blocks fall by the rows-per-model factor (about 8); numeric blocks and the SRV loop stay: 40 to 60 % of the phase removed | 4.3 to 6.4 |
| Constants and descriptors | PR-13, PR-14, shared upload ranges | about 5.0 | the sampler-set fill, FNV and compare (about 3.3 K byte iterations per draw), shared ranges for scene/view/palette blocks, SRV writes batched | 2 to 3.5 |
| Row set-up, diagnostics, forward light | PR-1, 2, 5, 15 | part of the 2.6 | bytecode and element copies, family strings, maps, the per-forward-draw engine calls | 1.5 to 2.3 |
| **Total** | | 24.6 | | **12.3 to 17.4** (planning 14.8) |

Prepare after: about 7 to 12 ms (planning 10). **Frame (K's 69 to 70 ms): about 52.6 to 57.7 ms, i.e. 17.3 to 19 frames/s from 14.5.** The same hoists apply to the menu offscreen views.

**With F2 of the R13 sheet** (prepare on the worker): the engine thread keeps about 4.8 + 27.2 + 6.2 + the rest of `Clear` outside prepare and render, i.e. about 40 ms (25 frames/s), whatever prepare costs, and the worker's load falls from about 30 to about 15 ms. The reduction then shortens the input-to-present latency (engine + worker in series) by about 14 ms and keeps the worker below the engine thread when collect falls further; without F2 it is a direct gain. Do it first: S1 to S4 are needed by F2 anyway and the hoists run on the snapshots they create.

**Not claimed:** instancing, draw merging, persistent per-draw descriptor tables, GPU-side changes.

## 7. Commits

Convention of K: `rebuild-prepare.flag` (runner `-RebuildPrepare`) runs the former per-frame path in the same build; **the new path is the default from the commit that introduces it**; `prepare-shadow.flag` (runner `-PrepareShadow`) recomputes every 30th prepared scene by the former path and compares; `no-prepare-profile.flag` turns the timers off. `rebuild-collect.flag` stays independent (it already disables R8a's plans in prepare). `model_draw` gets a switch like `SetModelDrawLegacyTables` (`SetModelDrawLegacyPrepare`) for cP3 and cP8. Testable logic lives in new linkable units (`native/frame/prepare_cache.{h,cpp}`, `native/renderer/model_draw.cpp`), because `game_bootstrap.cpp` is linked only into the DLL (`Build-GameBootstrap.ps1`); the producers move there with a `Read`/engine-function seam (cP2).

| Commit | Content | Files | Items | Tests / witnesses |
|---|---|---|---|---|
| cP0 | **Split and counters:** sub-timers inside `Prepare` (programmes, validation, pipeline, sampler hash, constants memcpy, CBV writes, SRV writes), inside the row set-up, forward light, frame lighting; counters per frame: rows, draws, models, blocks per draw, constant bytes written, palette bytes, SRV writes, `Read` calls, engine calls, palette-check status histogram; **main view and offscreen views separated**, counters atomic or thread-local; `prepare_digest` (FNV over draw order: pass, programme identity, state hash, each binding's stage/slot/bytes hash, each SRV's record identity and version, each sampler desc hash); pacing row fields | `game_bootstrap.cpp`, `model_draw.cpp`, pacing row | | PW-0; no behaviour change |
| cP1 | **`model_draw` API:** `NativeModelDrawInput` carries a shared `ProgramSet` handle (VS/PS identity, elements, element descs) instead of `CpuShaderProgram` and element vectors by value; `NativeModelConstantBinding` carries either bytes or a shared block with an optional shared GPU range; `SetModelDrawLegacyPrepare`; the old inputs still accepted | `model_draw.{h,cpp}` | enabler | existing tests; `startup_primitive_test` and `game_texture_ingress_test` still link |
| cP2 | **Extraction, row set-up, resource plan:** move the model-parameter producers (VS `cbvModelParam`, PS `cbModelParam`), the state builder and the forward-light/environment producers into `prepare_cache` with a `Read`/engine-function seam; family key per `Programs`; tallies sampled; support flag per layout; pose-binding memo; **`ResourcePlan` built at `Programs` creation (PR-7)**; PR-20 to PR-23 | `prepare_cache.*`, `game_bootstrap.cpp` | PR-1, 2, 5, 7, 20 to 23 | T-P1 (state builder), T-P9 (resource plan); shadow |
| cP3 | **Programme identity, state id, binding shape** on the cP1 handles; device epoch; `ValidateBindings` kept as the shadow path | `model_draw.{h,cpp}` | PR-3, 6, 12 | T-P1, T-P6; PW-1 |
| cP4 | **Palette-check cache** keyed as 4.2 (no stream generation) | `prepare_cache.*`, `game_bootstrap.cpp` | PR-4 | T-P2; PW-1, PW-3 (animated, cloth, physical stream) |
| cP5 | **Shared blocks:** the C3 table local to the call (forward scene block, time block, velocity matrices, forward packs, view/instance/palette blocks); **ranges allocated from the `ConstantChunk` pool of `model_draw.cpp` (`:198-212`) under `caches.lock`, not from the frame upload arena** (an arena range allocated outside the recording call is retired by the first `Execute` of the frame, phase D A1); every draw that points at a shared range pushes the chunk into its `pooled_` list (today `write_constant` pushes only the current chunk); key = the block's `shared_ptr`, kept alive (no ABA); CBV size at least the largest declared size among the sharing draws (padded palettes: 256 rows) | `model_draw.cpp`, `game_bootstrap.cpp` | PR-8, 10 (blocks), 11, 24 | T-P3; PW-1 |
| cP6 | **Model-level values on the snapshot = R8b S1, S2, S3:** `group_state`, `animation_flags`, `wind_block` in `NativeSelectedModel`; `ModelValues` (C2) for the VS and PS model parameters; E8 cached; `Read` of group/animation/material removed from prepare | `game_bootstrap.cpp`, `prepare_cache.*` | PR-9, 10, 17 | T-P4; R8B-1, R8B-4; PW-3 foliage/phantom/cloth |
| cP7 | **Forward light and frame lighting = R8b S4:** forward entries per distinct key; gparam fallbacks, repositories and environment regions resolved at collect or once; prepare free of engine calls | `game_bootstrap.cpp`, `prepare_cache.*` | PR-15, 16 | T-P7; R8B-6; PW-3 gparam transition |
| cP8 | **Samplers and descriptors:** sampler table index keyed by `EngineSamplerBankVersion::generation` and the device epoch; `CopyDescriptors` batching per stage. **Option B (not in this commit, after phase D cD1 to cD4):** persistent immutable SRV tables in a reserved region of the global heap, keyed by SRV list identity and versions; it relieves chunk exhaustion at `slots = 2` (phase D A4) and the per-frame SRV copies; it requires splitting the CBV and SRV tables in the root signature, hence its own sheet. (The model SRV tables have no descriptor-rewrite hazard of their own; phase D's D1 concerns the environment cubes.) | `model_draw.cpp` | PR-13, 14 | T-P5; PW-1 |
| cP9 | **M-run:** S1 to S5 with and without the debug layer, main and offscreen separately, the gain table recomputed from cP0; legacy path removal is a later clean-up | runners | | PW-4, exit checks |

Dependencies: K is now in `main`: re-anchor every `:line` of this sheet on `main`'s `game_bootstrap.cpp` before cP0 (the function, not the numbers, is the reference); cP0 first; cP1 before cP2/cP3/cP5; cP2 before cP3/cP4/cP6; cP6 and cP7 are the R8b S1 to S4 slices and gate F2 (R13 sheet cR5); cP8 option B belongs after phase D.

## 8. Tests and witnesses

### 8.1 Host/WARP tests (harness of `native/frame/Build-GameBootstrap.ps1`; new units added to its object and link lists; T-P3, T-P5, T-P6 need only `model_draw`, linked by `startup_primitive_test` and `game_texture_ingress_test`)

| Id | Content |
|---|---|
| T-P1 | State id (needs the cP2 extraction): over the cross product (cull class 0 to 2, depth-write on/off, engine pass in {deferred, 0x29, 0x1F, 0x21, 0x22, 0x34}, blend mode 0/2, fading, phantom, velocity) the new `DrawStateId` expansion equals the legacy descriptors and the legacy key string byte for byte; two draws that differ in any field never share a pipeline (WARP: same `ID3D12PipelineState*` exactly when the tuples are equal). |
| T-P2 | Palette-check cache: fixtures in range, out of range, wrong element format, empty; results equal `ValidateModelPaletteIndices`; the cloth case (empty span) is `Invalid` and the cache returns the same; the BLENDINDICES element does not overlap the offsets rewritten by the physical-stream path; a changed authored storage generation recomputes. |
| T-P3 | Shared blocks: n draws sharing a block produce one upload range, identical GPU address in every CBV, identical contents to the legacy per-draw copy; every sharing draw keeps the chunk alive until it is destroyed; a draw with a differing block (or a padded palette) gets its own range and a CBV size covering its declaration. |
| T-P4 | Model values (needs the cP2 extraction): `ModelValues` built from a snapshot equal the bytes built from live reads (a `Read` seam over fake memory) for every (fade, velocity, phantom, cloth) combination and every programme declaration size in the fixtures; after cP6 the `Read` counter is zero inside the extracted producers. |
| T-P5 | Sampler table cache: the index equals `SamplerTableOf` for every `common_banks` set; a different bank version gives a different index; a device change (epoch) never returns a stale index; the "cache full" path still falls back to per-draw tables. |
| T-P6 | Binding shape: equals `ValidateBindings` (shape and accept/refuse) for constructed inputs; a missing required binding is refused with the cache warm; geometry device/extent checks refuse a foreign-device resource. |
| T-P7 | Engine calls, **restricted to the extracted `ProduceNativeForwardLight` and `ProduceNativeEnvRegions`** with function tables injected: none of `1496D0`, `14A2D0`, `14E900`, `14E5D0`, `1400416A0`, `32770`, `38E70` and no repository walk; the whole-prepare version of the property remains the live counter R8B-6. |
| T-P8 | Shadow harness: an injected wrong cache entry (each class C0 to C3) is detected, dropped, replaced by the legacy value and logged as `native_prepare_cache_mismatch` with the first differing item; a clean run logs none. |
| T-P9 | Resource plan: for the model programme fixtures (as K's `register_plan_test`) the sequence of bindings produced through the plan equals the legacy loop's, for every (recipe, stage), including refusals. |

### 8.2 Live witnesses (D-0116 protocol, announced in chat; never from the static thread)

| Row | Action | Expected |
|---|---|---|
| PW-0 | cP0 on the K build, S1 (High Wall), no debug layer, 600 frames, then a character zone and a menu zone (offscreen views measured separately) | the real split replaces section 6's assumptions; blocks, SRVs, palette bytes per draw; rows per model by zone; where the 2.6 ms are |
| PW-1 | S1, `rebuild-prepare` against the new path, `prepare-shadow`, 600 frames | prepare ms down as in section 6; `native_prepare_cache_mismatch` = 0; `prepare_digest` equal between the two paths on the same scene within a run (shadow) |
| PW-2 | image equality: paused scene captures, legacy vs new, pixel diff, **including velocity rows (MotionBlur HIGH)** | identical (the method of Native1286 against 1283); 0 debug-layer errors |
| PW-3 | classes of change: animated character (palette), cloth model (cloth stream), a model with a **physical stream**, phantom, foliage with wind (animation block), a gparam transition (forward-light key), fade in/out (instance/fade), area change (texture residency flips), **equipment change or programme reload (new `Programs`, new storage)**, option change (sampler bank), menu views (Status with portraits) | zero mismatches, equal frames |
| PW-4 | cP9: S1 to S5 with and without the debug layer; the gain table recomputed | within section 6 or the table re-ranked from PW-0; prepare and render not worse |
| R8B-1, 2, 3, 4, 6 | rows of the R8b sheet at cP6/cP7 | zero differences; R8B-3 gives the dirty rate that decides C4 |

## 9. Risks and open points

1. **Staleness is the only real risk.** C0, C2, C3 are stale-free by construction; C1's keys are the existing versions plus the device epoch; C4 is optional and gated by R8B-3. The shadow comparison runs in diagnostic builds permanently.
2. **Line anchors** (K is in `main`; section 7). The numbers of section 6 assume K's measured prepare (24.6 ms, Native1298); PW-0 settles the split.
3. **Rows per model (about 8) is an S1 figure** (1,001 meshes / 126 models); a character-heavy zone has more rows per model and more palettes, a map zone fewer. PW-0 repeats it per zone.
4. **Interaction with phase D:** R-P2 keeps GPU objects out of the cross-frame caches; shared ranges come from the `ConstantChunk` pool (returned by ticket when the last holder dies), not from the frame arena; cP8 option B changes the root signature and waits for phase D.
5. **Interaction with R13:** C0/C1 caches are shared and built at `Programs` creation or under a leaf lock; C2/C3 are call-local and thread-safe; after cP6/cP7 prepare is movable to the worker (F2) with no engine memory read; `caches.lock` still serialises `Prepare` calls (PR-18) and bounds the benefit of preparing on two threads.
6. **OPEN:** per-draw counts of blocks/SRVs/palette bytes (cP0); the exact share of `SamplerTableOf` inside the 5.0 ms; the cost of `OwnedNativeModelDraw` construction and destruction (PR-19), whose reuse belongs to a persistent-draw design (R8a, folded into K by D-0164) and is not attempted here; whether the cloth case really yields `Invalid` (counter in cP0).

## 10. Evidence and limits

K branch `22253413` `native/frame/game_bootstrap.cpp` (`PrepareNativeModelFrame` `:5328-5974`, `PrepareNativeProgramRegister` `:5317`, `ProduceNativeForwardLight` `:3258`, `AcquireNativeLightingPrograms` `:2817`, `NativeShaderWordRanges` `:2597`, `Read` `:597`) and its K sheet section 9 (Native1298); `main` `native/renderer/model_draw.{h,cpp}`, `native/content/model_palette_indices.{h,cpp}`, `native/content/shader_constants.h`; [D3D12-PHASES-A-C-PREP](D3D12-PHASES-A-C-PREP.md) sections 2.9 and 10 (M4, Native1287/1288), [COLLECT-K-PREP](COLLECT-K-PREP.md), [K-COLLECT-IMPLEMENTATION-SHEET](K-COLLECT-IMPLEMENTATION-SHEET.md), [COLLECT-ORIGINAL-STATIC](COLLECT-ORIGINAL-STATIC.md) Q1/Q2, [R8B-ENGINE-READS-STATIC](R8B-ENGINE-READS-STATIC.md), the [R13/R14 sheet](D3D12-R13-R14-RENDER-THREAD-PACING-SHEET.md) and the [phase D sheet](D3D12-PHASE-D-IMPLEMENTATION-SHEET.md); original: `decompiled/DS3-1152-F-` `00140850`, `0022D620`, `0022E930`, `001A86C0`, `001A8580`, `0005B3D0`, `001A9250`. No build, run or measurement: every gain is derived from the Native1288/1298 figures and the S1 counters and is a ceiling until PW-0 and PW-4.

## 11. cP0-cP5 and cP8a: implementation and results (2026-10-11, Native1299-1320, pilot thread)

Done in place, ahead of the cP2 extraction, as PILOT-WORK-PLAN section 2 allows (lane P). Every slice is behind
`rebuild-prepare.flag` (the former paths, same build) and checked by `prepare-shadow.flag`.

- **cD0, cP0.** The audit counters of the phase D sheet (`native_frame_audit`) and the prepare split (`native_prepare_split`,
  self milliseconds per prepare in 17 phases and the prepare counters per prepare).
- **cP1, cP3, cP8a.** The programmes and vertex elements of a recipe live in a shared `NativeModelProgramSet`. The set keeps
  the verified programme entries, the pipelines by fixed-function state words, and the two sampler tables by sampler bank
  (bank object and generation) for one device epoch. A miss runs the former path.
- **cP4.** The palette check is cached per `Programs`. Its key holds the storage and stream generations, the counts and the
  row counts; a physical stream rewrite therefore misses (more conservative than R-P4).
- **cP5.** Constant bindings carry immutable shared payloads, so the prepare copies nothing. Two kinds of block are shared:
  - call-local blocks: forward `cbSceneParam`, `cbPerFrame`, velocity `cbMatrixWVP`, pose palettes per palette and padding,
    unproduced registers, `cbTexHdrScaleParam` per lightmap record, `cbTexScroll` per model;
  - value-keyed blocks per `Programs` (64 entries each): `cbMatDynParam` by the mesh's dynamic bytes, coverage, inactive
    flag and wet key; `cbTextureTilingScale` by the row's values.

  Uploading each shared payload once per constant-chunk range was measured and removed: Native1310 (shared ranges) gave
  prepare 18.3 ms and 15.6 fps, against Native1311 (a copy per draw) at 15.4 ms and 17.1 fps. The table lookup cost more than
  the copies it saved.

**Same build at S1 (High Wall, no debug layer, 120-frame windows, medians):**

| Run | fps | prepare | row set-up | palette check | stored | matdyn | draw programmes | draw pipeline | draw samplers | draw constants | palettes | other blocks | samplers/SRVs |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| Native1320, `rebuild-prepare` | 14.1 | 26.3 | 2.96 | 1.89 | 1.08 | 2.72 | 1.24 | 1.90 | 1.84 | 2.87 | 2.08 | 2.39 | 1.96 |
| Native1319, new paths | 16.3 | 16.2 | 1.55 | 0.02 | 0.52 | 0.37 | 0.12 | 0.41 | 0.05 | 2.95 | 2.40 | 2.12 | 2.00 |

Per prepare: 1,077 rows, 1,062 draws, 10,483 constant blocks, 18.7 MB of constant bytes and 6,126 SRV writes, the same on
both paths. The pair Native1314/1315 gives the same difference, but its new-path run overlapped a runaway script on the
same machine and is not used. Planned in section 6: 12.3 to 17.4 ms of gain; measured: 10.1 ms.

**Merge gate (PILOT-WORK-PLAN section 6; runner `Run-PrepareGate.ps1`, summary `Get-GateSummary.ps1`):**

- **Native1312 and 1318 (debug layer, High Wall).** MotionBlur HIGH, in-game taps, the Equipment page, the Load Game
  portraits, and both shadows. Result: exit 0, 0 debug errors.
  - Collect shadow: 0 mismatches over 66 and 68 scenes (60,015 and 62,021 meshes).
  - Prepare shadow: 0 mismatches. In 1318 it also covers the matdyn and tiling caches.
- **Native1313 (`rebuild-prepare`, same protocol).** The screenshots (title, Load Game, in game, Esc, Equipment page) cannot
  be told apart from 1312's (PW-2, visual comparison; the menus do not pause the scene).
- **Native1316 (Cemetery of Ash, slot 1).** Teleport to Firelink Shrine (area streaming, NPCs with cloth), white phantom 80,
  camera turns. Result: exit 0, 0 errors, 0 mismatches (123 collect-shadow scenes).
- **Native1317 (New Game).** The Bink opening movie and character creation, with 0 debug errors and 0 mismatches. Closing
  the window from character creation faults at `DarkSoulsIII.exe+0xD12759`. Native1250 and Reference1250 fault at the same
  address, so this is not a regression; it is recorded as an open point of the character-creation close.
- **Host tests.** `model_draw_test` (WARP and hardware, debug layer) covers T-P3, T-P5 and T-P6. Unchanged: the
  `register_plan_test` and `collect_profile_test` controls.

**Pre-merge review (Opus 5.5, fresh agent).** It found no case where a normal run differs from the former paths. Applied:
- the prepare counters are reported per prepare (they were per collect);
- the matdyn and tiling caches are under the shadow, with `native_prepare_cache_mismatch` rows;
- pipeline creation outside the lock checks the device epoch rather than the device pointer;
- `native_collect_mode` records the prepare flags;
- `model_draw_test` draws the kept values before the shadow draw, and tests bank identity and bank version separately.

Recorded, not changed:
- program sets keep old-device pipelines until their next prepare (this matters only if the device is recreated);
- the palette key includes the physical stream generation (a miss on every rewrite), pending T-P2;
- model-draw shadow mismatches are counted but not itemised (T-P8).

**Open, carried to the next prepare slices (cP2, cP6, cP7, cP8b):**
- The remaining 16 ms is mostly per-draw constant production and writes: 10.5 k blocks and 18.7 MB a frame, the palettes
  padded to their declarations, other blocks, SRVs.
- T-P1, T-P2, T-P4, T-P7, T-P8 and T-P9 are due with the cP2 extraction.
- PW-3 cases not yet witnessed: gparam transition, fade, equipment change or programme reload, option change (sampler bank
  version).
