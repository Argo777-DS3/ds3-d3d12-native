# Workstream K, implementation sheet: collect measurement (C-0 to C-5) and the pure-function caches (static)

STATIC (native code at this branch, which contains main `01de1ce8`; Ghidra mirror of `DarkSoulsIII.exe` 1.15.2.0). Nothing was launched, no native code changed.
Status words: **STATIC** read in code or data, **INFERRED** deduced, **OPEN** not known statically. "Reviewed" marks facts established by the independent review of the first version of this sheet and re-checked in the sources it cites.
Inputs: [COLLECT-ORIGINAL-STATIC.md](COLLECT-ORIGINAL-STATIC.md) (the original side, rows C-0 to C-7, section 8 proposal), [I-0068 COLLECT-NATIVE-REPRISES.md](../I-0068-static-yebis-ffx-lights-spec/COLLECT-NATIVE-REPRISES.md) (reprises C0 to C10, which this sheet turns into code-level work for C0 to C6 and C8), [R8B-ENGINE-READS-STATIC.md](R8B-ENGINE-READS-STATIC.md) (the engine reads and the snapshot).
Line numbers refer to `native/frame/game_bootstrap.cpp` at this branch; they move with every merge, so each site is also named by a landmark (function and a quoted fragment) that a search finds.
Evidence outside Git: `evidence/I-0066/queries/k-sheet/` (texture registry and holder queries). Exports: listed at the end.

## 0. Scope and decisions

- **Two deliverables, in this order:** (1) the measurement of section 1 (no behaviour change, behind the existing `measure.flag` family), then (2) six caches P1 to P6 of section 2, each a pure function with an exact key, shipped behind `rebuild-collect.flag` (returns to today's path) and verified by a byte-for-byte shadow comparison (section 3).
- **What is a "pure function" here:** an output fully determined by the key, with no engine memory read during the computation. Everything that reads the engine per frame (placement, fade, group state, pose, wind, the dynamic material block `material+0xD0`) is **not** cached by this sheet; it is the R8b snapshot and the later version scheme (C7). Section 2.8 lists what was left out and why. P4 is the one cache that touches engine memory (the texture holder); it re-reads the two holder words on every hit, so it is validated, not trusted.
- **Threading (reviewed):** collect runs on the owner thread inside `UpdateNativeSceneCpu`; the fallback offscreen collect can also run from `RenderNativeOffscreenViews` (`:6468`, called from `Clear()` at `:654`, outside the `collect_ms` bracket and gated by `native_menu_thread`, `:6421`). The caches are therefore written to be read from more than one thread: values are `shared_ptr<const>`, every mutable per-`Programs` map has its own mutex (the layout's `programs_lock` belongs to the layout, not to the const `Programs`), and the generation counters are atomics.

## 1. Measurement

### 1.1 Infrastructure

One profiling object, active only when `measure.flag` is set (`measure_run`, already read for the pacing row), so the default build pays one predictable branch.

```
enum class KPhase : uint8_t {
  SceneSetup,        // CollectNativeSelectedScene start .. before the visitor (images, sampler bank, rows, wet catalog, velocity inputs)
  Traversal,         // decal_world_query_original(world, &visitor)  [C-1]
  EntityFilter,      // AdmitNativeSelectedObject self time          [C-2]
  ModelAdmission,    // AdmitNativeModelCandidate self time          [C-3a]
  MeshLookups,       // per mesh: model_materials / model_layouts lookups, stream and cloth reads, BindModelGeometry [C-3b]
  MeshPrograms,      // per mesh: AcquireNativeModelPrograms alone (the programme chain, P6) [C-3b']
  MeshTextures,      // per mesh: texture inputs (D24490 chain) and scene admission  [C-3c]
  MeshShaderTex,     // per mesh: SelectModelShaderTextures loop + record checks     [C-3d]
  MeshMatDyn,        // per mesh: generated, damage, wet, phantom, dither, 22E930 blocks [C-3e]
  MeshBlocks,        // per mesh: numeric parameters and the recipe x stage block loop [C-3f]
  Passes,            // PrepareNativeSelectedPasses                  [C-4]
  Lights,            // CollectNativeLightBlocks + CollectNativePointLights [C-4]
  Offscreen,         // CollectNativeOffscreenScenes (and the fallback offscreen collect, flagged separately)
  Teardown,          // release of the previous NativeSelectedScene
  Count };
struct KProfile { int64_t ticks[Count]; uint32_t calls[Count]; /* one instance per thread, merged at the pacing row */ };
struct KScope { KPhase p; int64_t t0; KScope* parent; ... };  // RAII: subtracts child time so each phase reports SELF time
```

- `KScope` keeps a thread-local stack pointer; on destruction it adds `elapsed - children` to `ticks[p]` and `elapsed` to the parent's `children`. QPC pair per scope (about 20 to 40 ns). The mesh-level phases are entered once per mesh, about 1,100 times a frame, so the overhead is about 0.1 to 0.3 ms (reviewed as plausible); row C-0c measures it. Because the offscreen fallback collect runs outside the bracket, its time is reported in its own field and excluded from the "split sums to `collect_ms`" check.
- **Counters** (plain `uint64_t`, same struct): `visited_entities` (visitor callbacks, already `result.objects`), `admitted_models`, **`admitted_meshes`** (increment where `selected.meshes.push_back(std::move(bound))`, `:4123`), `resolved_recipes` (`program_recipe_pairs`, existing), `recipe_stage_pairs` (inner iterations of the loop at `:4060`), `prepare_register_calls[slot]` for slots 0, 3, 4, 8 (one increment before each `PrepareReadShaderRegister` call at `:4083`, `:4097`, `:4100`, `:4101`, `:4116`), `select_shader_textures_calls` (`:3765`), `d24490_calls` (the call at `:3693`), `d24430_calls` (`AcquireNativeModelPrograms`), `materials_lookups`, `effects_scan_iterations` (the composed-group `for` at `:3268-3271`), `witness_lock_acquisitions` (`CharacterWitness`, `:3438-3448`), `scene_alloc_bytes` optional.
- **Output (reviewed: not in the existing row).** The existing `native_present_pacing` row is formatted into `char pacing[1024]` (`:703`) with a format string already about 540 characters; adding the split, the counters and the C-0b fields would overflow it and `sprintf_s` would abort the process. Emit a **separate row** `native_collect_split` (buffer 4096, once per pacing window, after the pacing row at `:709`) carrying `collect_split_ms` (self times), `collect_counts`, the C-0b fields and `unattributed_ms`. The existing `collect_ms` stays as is. The sum of the split against `collect_ms` is itself a check: a gap above 3 % means an unscoped region.
- **Reset:** the arrays are cleared where the pacing row clears the phase ticks (`:697`).
- **Test:** a unit test for `KScope` nesting (self-time subtraction, three levels) in `native/frame/` next to the other `_test.cpp` files, no engine needed.

### 1.2 The rows

| Row | Measures | Insertion point (landmark) | Reading and decision |
|---|---|---|---|
| **C-0** | the denominators | `admitted_meshes`, `admitted_models`, `recipe_stage_pairs` counters (above) next to `program_recipe_pairs` and `prepared_meshes` | fixes the unit mismatch of COLLECT-ORIGINAL-STATIC §4: recipes per mesh = `program_recipe_pairs / admitted_meshes` |
| **C-0b** | the frame residual (~12 ms) | `QueryPerformanceCounter` stamps kept in a small struct and diffed in the split row: `BeginFrame` entry (`:6112`), `UpdateNativeSceneCpu` entry (`:5427`) and exit, `NativeRenderStep` entry (`:7282`) and exit with a nested scope around `session->PrepareFrame` (`:7329`), `EndFrame` entry (`:7364`), `Clear()` entry (`:644`) and exit (about nine stamps) | `engine_update_ms = update_entry - begin_frame`, `scene_update_other_ms = (update_exit - update_entry) - collect`, `render_step_ms`, `ui_prepare_frame_ms`, `endframe_prelude_ms`, `clear_tail_ms`; plus `unattributed_ms = 1000/fps - sum` |
| **C-0c** | the instrumentation overhead | the same S1 run with and without the scopes (flag file `no-kprofile.flag`) | total `collect_ms` difference; accept at under 2 % |
| **C-1** | traversal alone | scope `Traversal` around `decal_world_query_original(world, ...)` (`:4698`) | small if the tree masks work; the callbacks inside are subtracted (self-time rule) |
| **C-2** | the per-entity filter | scope `EntityFilter` at the top of `AdmitNativeSelectedObject` (`:4167`), excluding the nested `AdmitNativeModelCandidate` | the cost of `classify`, `D03050`/`D03430`, DrawGroups, category calls for the 328 visited entities |
| **C-3** | per-mesh admission, parts | scopes: `ModelAdmission` (`AdmitNativeModelCandidate`, `:3431`, excluding the mesh loop), then in the mesh loop (`for(const auto& mesh:inputs.meshes)`, `:3594`): `MeshLookups` (`NativeSelectedMesh bound;` `:3601` to `:3652`: `model_materials`, masks, `model_layouts`, stream and cloth reads, `BindModelGeometry`), **`MeshPrograms`** (only the call `AcquireNativeModelPrograms`, `:3654`), `MeshTextures` (`:3655-3761`: candidate loop and scene admission), `MeshShaderTex` (`:3762-3824`: the `SelectModelShaderTextures` loop and, `:3794-3823`, the per-mesh record checks), `MeshMatDyn` (**`:3831-4048`**: generated words `:3839`, damage `:3847`, wet `:3858`, phantom `:3877-3918`, dither `:3921`, the `22E930` generator `:3922-3964`, then `:3965-4048`), `MeshBlocks` (`:4049-4122`: numeric parameters and the recipe x stage loop) | the largest of the last six names the cache to build first (decision table below) |
| **C-4** | pass walk and lights | scopes `Passes` (`PrepareNativeSelectedPasses(*result)`, `:4702`) and `Lights` (`CollectNativeLightBlocks`, `CollectNativePointLights`, `:4703-4706`); `CollectNativeVelocityInputs` (`:4685`) into `SceneSetup`, `CollectNativeOffscreenScenes` (`:5554`) into `Offscreen` | N7 and N8 of COLLECT-ORIGINAL-STATIC §6 |
| **C-5** | the reuse rate | for each admitted model, a 64-bit signature of its **per-frame inputs** (placement 48 B, fade state, `group_state`, wind block, pose version, stream/cloth generations; the R8B snapshot fields) at the end of `AdmitNativeModelCandidate`; a map `entity -> (signature, last_seen_frame)` holds the previous frame's, **entries not seen for 120 frames are erased** (the map must not grow without bound); counters `models_unchanged` / `models_changed` / `models_new` | the fraction a persistent admission record (C7) could skip; also the data for R8B-3 |

**Decision table (read at S1 after C-0 to C-5):**

| If the largest self time is | build first |
|---|---|
| `MeshBlocks` (the recipe x stage loop) | P1 then P2 then P3 (section 2) |
| `MeshShaderTex` | P5 |
| `MeshTextures` | P4 |
| `MeshPrograms` | P6 |
| `MeshLookups` or `MeshMatDyn` | no pure-function cache of this sheet: the lookups are model-layout bindings, MatDyn is C6 (engine reads) |
| `ModelAdmission` or `EntityFilter` | no pure-function cache helps: R8B Level 1 snapshot and C7 |
| `Passes` | row evaluation reuse (C9), not in this sheet |
| `Traversal` | the walk itself (R13 engine-side overlap), not in this sheet |

Report at S1 the three-window medians (as phase A did) and keep the split table in the sheet's follow-up.

## 2. The caches

Common rules: values are immutable (`shared_ptr<const T>`); every cache has a **validation at lookup** (integers compared, and for P4 the two holder words re-read) and a **drop-all on its signal**; the shadow comparison of section 3 verifies hits. A cache is flagged off by `rebuild-collect.flag`. Per-`Programs` maps (P3, P5) are guarded by a mutex stored in the `Programs` object (it is built `const`; give it a `mutable std::mutex`), and `Programs` becomes non-copyable because the plan points into it.

### 2.1 P1, register plan (the layout half of `PrepareReadShaderRegister`)

**What it removes.** `PrepareReadShaderRegister` (`native/content/shader_reads.h:11`) does, per call: a linear scan of `shader.resources` for the slot, a linear scan of `shader.constants` for the name, the declaration bound checks, and `ds3reads::FloatMask(reads, slot, bytes)` (a vector); only then `PackShaderConstant` with the ranges. Everything before the pack is a function of the interface and the reads, never of the ranges (reviewed).
**Exact key.** `(Programs object identity, recipe, stage, slot, expected_name)` with `slot` in {0, 3, 4, 8}. Five plans per `[recipe][stage]`: slot 0 `cbMatrixWVP`, slot 3 unnamed (`:4083`), slot 3 `cbMtdParam` (`:4097`), slot 4 `cbInstanceData`, slot 8 `cbSceneParam`. The plans live **inside** `NativeModelLayout::Programs`, built in `AcquireNativeModelPrograms` right after `interfaces[index][stage]` and `reads[index][stage]` are set (`:3350-3358`), before `layout->programs=prepared` publishes the object: lifetime is the Programs object's, **no external invalidation**, and no lock (read-only after publication).
**How to write it so equivalence holds by construction (reviewed).** Do not re-implement the early outcomes: split `PrepareReadShaderRegister` itself into `PlanReadShaderRegister(shader, reads, slot, expected_name) -> RegisterPlan` and `PackRegisterPlan(plan, ranges) -> ShaderRegisterBlock`, where `PrepareReadShaderRegister` is the composition of the two. `RegisterPlan` carries whatever the original function computes before looking at `ranges`: the early `Failed(status)` results, the `!reads.ok` route into `PrepareShaderRegister`, the "no resource and not referenced" route, or the resolved `{declaration pointer, read words, required}`. The existing call sites keep calling the composition; the cache stores plans.
**Test.** `native/content/register_plan_test.cpp` (new): run `PrepareReadShaderRegister` and plan+pack on (a) the model programme DXBC files the pilot extracts (`hair-forward-vs.dxbc`, `hair-forward-ps.dxbc` in the run artifacts, which declare the model constant buffers) and (b) hand-built `ShaderInterface` structs with `cbMtdParam`, `cbInstanceData`, `cbSceneParam`, `cbMatrixWVP` declarations, ambiguous declarations, a missing resource, `reads.ok` false, with empty, full and random partial ranges. (The UI programme fixtures of `game_primitive_programs_test.cpp` declare none of these buffers and do not suffice.)
**Engine invalidation signal.** None: the interface and reads come from the DXBC bytes; if the engine reloads a programme group, `AdmitEffect`/`RetireEffect` (engine `FUN_1400A9370`/`FUN_1400A9230`) change `effects`, `AcquireNativeModelPrograms` builds a new `Programs` and the plans go with the old one.
**Gaps.** Plans point into `interfaces` of the same object (build after, destroy with it, non-copyable). **Expected effect:** measure the C-3f split before and after; no figure is quoted.

### 2.2 P2, frame-scope blocks shared across meshes (scene, view, instance)

**What it removes.** `scene_blocks` (`cbSceneParam`, slot 8, `:4100`) and `view_blocks` (`cbMatrixWVP`, slot 0, stage 0 only, `:4101`) are built from `scene_ranges` and `view_range`, which come from `result.camera_constants` and `result.view_constants` (`:3592-3593`): **independent of the mesh**, constant for the whole scene. `instance_blocks` (slot 4, `:4116`) depend on `selected.instances[fade]` and the shader, so constant across the meshes of one model. Today each mesh and recipe rebuilds all three. `NativeShaderWordRanges(...)` itself is rebuilt per model (`:3592`) from the same scene data.
**Exact keys (frame scope).**
- scene and view: `(NativeShader program identity, stage, slot, expected_name)` inside a table owned by the **scene being built**; the program identity is the pointer to the program's `bytes` vector (`Programs::ProgramBytes` returns `effect->programs.at(..).program->bytes`, or the shared Velo pixel programme's bytes, `:483-486`). **Each entry retains a `shared_ptr` to the object that owns those bytes (the `NativeEffect` or the shared pixel programme)** so the address cannot be reused by a different programme if a `Programs` object dies mid-scene (reviewed).
- instance: `(program identity, stage, fade index)` inside a table **local to one `AdmitNativeModelCandidate` call** (reviewed: the model is a stack local `selected` until it is moved into `result.models` at `:4139`; a key made of the model's address would alias across candidates, as the next candidate reuses the same stack address). The table is created at the top of the call and dies with it.
**Value.** `std::shared_ptr<const ShaderRegisterBlock>`; the mesh holds the pointer. `NativeSelectedMesh::scene_blocks`, `view_blocks`, `instance_blocks` are value arrays today (`:2329-2334`: 14 recipes x 2 stages for the numeric and scene blocks, 14 for the view blocks and 2 fades x 14 x 2 for the instance blocks, each a struct with a byte vector, embedded in every mesh); they change to `shared_ptr<const>` and every reader in prepare dereferences. Hoist `scene_ranges`/`view_range` to once per scene.
**Engine invalidation signal.** None (frame scope; the camera constants are produced once per frame by the native producers at the top of collect).
**Gaps.** (a) Equal bytes imply equal interface and reads (both derive from the bytes; the shared Velo pixel recipe uses `velocity_common_pixel->bytes`, still the bytes of one programme); asserted in shadow mode. (b) `selected.instances[fade]` is per-frame engine-derived data: the instance entry is valid for **this call only**, never carried over. (c) The view block for stage 1 does not exist (`if(!stage)`): keep that asymmetry.

### 2.3 P3, material parameter blocks (`cbMtdParam`, slot 3)

**What it removes.** For every resolved recipe and stage the loop builds `stage_ranges` (copy of the numeric-write ranges, zero-fill ranges for partly covered `cbMtdParam` variables, `mtd_fallback` ranges from `ProduceMtdParams`: `:4064-4082`) and prepares the block (`:4083`); in composed groups it builds `ProduceMtdParams` over the whole layout (`:4091-4097`).
**Exact keys.**
- the `:4097` branch (`bound.programs->effect->parameter_declarations.empty()`), whose block replaces the `:4083` one: `(Programs identity, recipe, stage, exact numeric writes)`. Composed groups can still carry **builtin numeric parameters** (`:3328-3340`), which enter `numeric_writes`, so the key is **not** `(Programs, recipe, stage)` alone (reviewed); the result of `:4097` itself is a function of `authored->parameters` and the interface, but `:4083` runs first and the latter assignment wins, so the cache stores the final block of the branch that executes.
- declared groups: `(Programs identity, recipe, stage, exact numeric writes)`.
`exact numeric writes` is the ordered list of `(byte_offset, bytes, words)` of `bound.numeric_writes` stored **in the entry and compared on hit**; a 64-bit hash is used only to find the bucket (reviewed: a hash alone is not an exact key, a collision would be silently wrong on frames not shadow-compared).
**Value.** `shared_ptr<const ShaderRegisterBlock>` in a per-`Programs` map under the Programs' own mutex, bounded to 32 entries with least-recently-used eviction (animated values repeat across the meshes of a material; many entities sharing a material with different ExParam values can still thrash a small map, which C-5 and the K-1 counters will show).
**Engine-side origin of the animated writes.** They derive from `parameter_animations` read from the entity (`entity+0xA40`, `ReadModelParameterAnimations`, `:3462`); the engine side that blends them per draw is `FUN_14022D620` (`ExParam_*` weights `entry[5]`, MENUS-MAP section 68, I-0068 CHR-FADE-EXPARAM-REFLECTION). The writers of the entity's parameter set are **not traced** to a task (R8B §3 point 3); a value-keyed cache does not need them.
**Gaps.** (a) Comparing the writes costs a pass over a few words per material; measure in C-3f. (b) Floating-point values compare by bits: -0.0 and 0.0 differ, which only causes a miss. (c) `mtd_fallback` depends on which words are `covered`: the covering set is a function of the offsets, which are inside the key.

### 2.4 P4, texture bindings of a material input (the engine registry chain)

**What it removes.** For every texture input of every mesh, every frame: up to three genuine `FUN_140D24490(resolver, candidate)` calls (path normalisation `FUN_1402FF080` into a fresh string, then the registry map lookup `FUN_140D4ED20` -> `FUN_142356180`), the holder checks, `textures.Acquire(texture)`, and wide-string building for the `_l` and `_atlas000` variants (`:3655-3728`, the `D24490` call at `:3693`).
**What the engine does (STATIC, reviewed).** `D24490` never reads its first argument (RCX is overwritten at `140D244B8`): the registry `[144792F58]+0x60` is global, so the key does not need the resolver. It returns `[entry+0x60]`, the **holder** (`SprjGxTexture`, vtable `1429559E8`, ctor `FUN_140D4ACA0`); the holder's `+0x10` is the inner texture (refcounted) and `+0x18` a deferral queue pointer. A registry **entry** is a 0x80-byte object (vtable `142956900`, ctor `FUN_140D50AB0`) with the holder at `+0x60`, two counts at `+0x58` and `+0x5C` and a handle at `+0x78`.
- *Holder's texture writers:* `FUN_140D4B000` (SetTexture: immediate swap when `+0x18 == 0`, otherwise it queues; callers: the ctor, `FUN_140D50FE0`, `FUN_140D514C0`), and the queue paths `FUN_140D4B0D0` and `FUN_140D4B200` (flush), which write `holder+0x10` directly from the queue head; `FUN_140D4B9D0` is only a refcounted assignment into the queue head slot (also used by the destructor `FUN_140D4AD00` and by `D4B0D0`), **not** an executor. No static caller was found for `D4B0D0`/`D4B200`: they are reached indirectly (vtable or `.pdata` only).
- *Registry insert paths:* `FUN_140D4F250` (acquire by name, per name in `FUN_140D52640` for fewer than three names), `FUN_140D4F470` -> `FUN_140D4E100` (the three-or-more-names path of `D52640`), `FUN_140D4F690`/`FUN_140D4F770` (find-or-insert with the default texture; callers `FUN_140D39170`, `FUN_14066D850`, `FUN_1406497A0`), and the map insert itself in `FUN_140D4EE10`.
- *Registry erase/release:* `FUN_140D4F7A0(registry, entry)` releases (its second argument is the **entry**, not the holder); `FUN_140D4F4B0(registry, entry)` either erases the entry (`FUN_1423563d0(registry+0x60)` succeeds, then the entry is deleted) or, while counts remain, resets the holder to the default texture through `FUN_140D50FE0` -> `D4B000`.
**Exact key.** `input.path` (the candidate strings `{path, variant("_l"), variant("_atlas000")}` are a function of it; the resolver is redundant).
**Value.** `TextureResolution { uint8_t chosen_candidate; uintptr_t holder; uintptr_t texture; std::shared_ptr<const NativeTextureRecord> record; }`, or a recorded **miss**. Not cached: the FaceGen slot (`NativeComposedFace(c.face_owner)`, per entity) and the system-texture fallbacks that depend on `result.common_images` (per scene, cheap; the `_wcsicmp` scans become a map in P2b).
**Validation on every hit (this replaces "observers on the writers").** (1) `registry_generation` unchanged since the entry was filled; (2) re-read `[holder]` equals the holder vtable `1429559E8` and `[holder+0x10] == texture` (exactly what the code does today at `:3697`, `:3700`); (3) `textures.Acquire(texture).get() == record.get()` and the record is resident; (4) for a cached miss or a chosen candidate other than the first, the candidates ranked above the chosen one are re-tried when the native store generation changed (below). Because (2) re-reads the live words, **replacements by any writer** (`D4B000`, `D4B0D0`, `D4B200`, or one not found) are caught without observing them.
**Invalidation signals (only what protects the raw `holder` pointer and the native record).**
1. `registry_generation`: bumped **after completion** of the registry mutators `FUN_140D4F250`, `FUN_140D4F470` (and `D4E100`), `FUN_140D4F690`, `FUN_140D4F770`, `FUN_140D4EE10` (insert), `FUN_140D4F4B0`, `FUN_140D4F7A0` (release/erase) by pass-through observers; a bump on the constructor path alone would precede the map insert (`D4ACA0` runs inside `D50AB0` before `D4EE10`, reviewed), so the hook is on the registry function, not on `D4B000`. On a bump the whole cache is dropped; a `holder` is dereferenced only while the generation is unchanged.
2. **Native texture store (reviewed correction):** `TextureIngress::generation_` is bumped when a record is **admitted**, at which point `resident` is still false; `resident=true` is set later in place under the lock with no bump (`game_texture_ingress.cpp:51`, `:69`, `:82`, `:155`, `:168`, `:200`, `:208`, `:227`), and the records are mutable `shared_ptr<NativeTextureRecord>` (`game_texture_ingress.h:52`). Add a bump at each place `resident` flips (and at `Retire`), expose the getter `Generation()`, and make P4's validation (4) depend on it; otherwise a cached miss or a cached `_l` choice survives the upload finishing and the K6 witness would fail.
**Gaps.** (a) Writers of `holder+0x10` not found statically are harmless to correctness thanks to validation (2); (b) the erase path's map function `FUN_1423563d0` is observed through `D4F4B0`, not itself; (c) whether any registry mutation bypasses the functions above is OPEN (rows K-4, K-5 and the periodic revalidation every 120 frames bound the staleness; the shadow compare reports divergence).

### 2.5 P5, shader texture selection (`SelectModelShaderTextures`)

**What it removes.** `SelectModelShaderTextures(shader, texture_bindings, scene_slots, registers)` per resolved recipe and stage and per mesh (`:3765`), 8,928 calls a frame by the YEBIS count.
**Exact key.** `(Programs identity, recipe, stage, scene-slot signature)` where the signature is the ordered list of slot numbers in `scene_slots` (`:3762-3763`); reviewed: the function is a pure function of the shader, `programs->texture_bindings` and the ordered scene slots (`model_textures.cpp:88-110`), and `bound.texture_bindings` runs parallel to the programme's bindings (`:3727`).
**Value.** `{ModelTextureStatus status; registers}` in a per-`Programs` map under the Programs' mutex.
**Not cached (stays per mesh).** The second half of the loop (`:3794-3823`): per request, the texture record for the binding, the `Missing` downgrade when the record is null, and the shape check against `texture->description`.
**Engine invalidation signal.** None (declarations and interface are programme data); the key lifetime is the Programs object.
**Gaps.** The 1203/1035 witnesses (`native_texture_envelope_refused`, `native_selected_texture_unbound`) log only on a first computation, not on hits.

### 2.6 P6, the programme chain before the per-layout cache

**What it removes.** Every mesh, every frame, before `layout->programs` is returned (`:3231-3290`): `materials.find` under `material_lock`, the genuine `FUN_140D24430(resolver, shader)` call, `effects.find` under `effect_lock`, and, for composed groups (owner 0), two `leaf()` calls and a scan of all effects x programme names with `_wcsnicmp` (`:3249-3272`).
**Where it lives (reviewed).** In `NativeModelLayout` (a member next to `programs` and `programs_lock`), **not** in a global map keyed by the layout address: a global map would alias freed layouts, and creating or destroying a layout bumps no generation, so "they reach P6 through the key" only holds if the store is the layout's own.
**Exact key.** `(prefer_channel)` inside the layout; the cached tuple is `{authored, owner, effect, Programs, materials_generation, effects_generation, shaders_generation}`.
**Validation at lookup.** the three generations equal the stored ones, and `layout->programs->effect/authored/prefer_channel` equal the cached ones (the check that exists today is `:3288-3290`). On a match: skip `materials.find`, `D24430`, `effects.find` and the scan. **Failures are never cached** (reviewed: `ComposeNativeShaderGroup` can fail depending on the `shaders` map, which `AdmitShader` `:7415` and `RetireShader` `:7388` change).
**Generations (reviewed).** Counters are `std::atomic<uint64_t>` (written on loader threads under the locks, read lock-free on the owner thread). Do not add homonyms: extend the existing `effect_generation` (`:382`, bumped at `:3226` and `:7514`) and `material_generation` (`:428`, bumped at `:7618`) so that they are also bumped on **retire** (`RetireEffect` `:7433`, `RetireMaterial` `:7539`) and on every `emplace`/`insert_or_assign` (`:7517`, `:7623`), and add `shaders_generation` bumped at `AdmitShader`/`RetireShader`. Engine-side origin: these are native hooks on engine functions `FUN_1400A9370`/`FUN_1400A9230` (effects) and `FUN_140D43BB0`/`FUN_140D439A0` (materials) (hook table `:10009-10012`).
**Gaps.** (a) `FUN_140D24430` is the engine's authored-name lookup in the programme-group registry (`FUN_140D446D0`); its result depends on the engine registry, which native sees only through the hooks above (INFERRED complete: row K-3). (b) `AcquireNativeLightingPrograms` (body near `:2700`, called at `:5352`) already caches statically once resolved; its repository walk repeats only until resolution, so it is not part of P6.

### 2.7 P2b, hoisting inside a scene (no cache object)

Build `scene_ranges`, `view_range` (once per scene) and a small name map of `common_images->system` (instead of the linear `_wcsicmp` scans at `:3669`-`:3760`, `:3707`, `:3714`, `:3754`) when `common_images` is acquired. **Keep the first-match semantics (reviewed):** the scan at `:3755` returns the first name match even when the texture is null, whereas `:3709` and `:3716` skip null textures; a map must store the first entry per name and the two skipping lookups must still skip.

### 2.8 What is deliberately not cached here

| Item | Why not a pure function | Where it goes |
|---|---|---|
| MatDyn blocks (`:3831-4048`, `material+0xD0` reads) | read engine memory that `FUN_14022D620` rewrites per draw from the entity's ExParam set | R8B snapshot + C6 |
| Admission records per model | carry per-frame data: `scene_blocks`, `instance_blocks`, pose and previous pose, stream and cloth generations, residency | C7, after R8B-1 and R8B-3 |
| Pass rows (`PrepareNativeSelectedPasses`) | depend on model state per frame | C9 |
| Instance placement, fade, group state, wind | engine writers per update (R8B §3) | R8B Level 1/2 |

## 3. Shadow comparison

- Flag `collect-shadow.flag` (diagnostic builds only): every Nth scene (N=30 by default; the section 2 statement "every hit" is replaced by this rule, with N=1 available for short witnesses) the cached result is compared with a recomputation, per mesh: `status`, `required`, `bytes` of every `scene/view/numeric/instance` block, `shader_textures[recipe][stage].status` and `registers`, `texture_bindings[i].texture.get()`, and for P6 the tuple `(effect, authored, prefer_channel)` (not the `Programs` pointer: it differs legitimately once P6 keys by `prefer_channel`).
- **Side effects (reviewed).** The old path of `AcquireNativeModelPrograms` writes `layout->programs` (`:3363`), can compose a group (`composed_effects` insert `:3229`, `effect_generation` bump `:3226`, which would invalidate the caches) and consumes the first-N witness counters. The comparison therefore calls a **read-only variant** (`ResolveNativeModelProgramsReadOnly`: the same lookups, no compose, no store, no counters) and skips the composed-group case (owner 0) except for the recomputed-vs-cached `effect`; witness counters are saved and restored around it.
- A mismatch logs `native_collect_cache_mismatch {cache:"P1..P6", mesh, recipe, stage, first_differing_offset}` and **uses the freshly recomputed result for that frame** (the cached entry is dropped); the count is in the `native_collect_split` row.
- Acceptance for each slice: **zero mismatches** over the High Wall idle witness (S1), a Load Game pass through three zones, and the title and Status menu (S2, where the offscreen fallback collect on the menu thread also runs: the caches are read from that thread).

## 4. Slices (order and witnesses)

| Slice | Content | Witness |
|---|---|---|
| K1 | section 1: `KProfile`, `KScope`, counters, the separate `native_collect_split` row, C-0 to C-5, C-0c; reading of S1 and the decision table | the split table at S1 (three windows), overhead under 2 % |
| K2 | P2b hoists, then P1 (`PlanReadShaderRegister`/`PackRegisterPlan`, `register_plan_test.cpp`) | unit test equivalence; shadow compare zero mismatches; C-3f time before/after |
| K3 | P2 (frame-scope blocks via `shared_ptr<const>` members; per-call instance table) | shadow compare; prepare reads unchanged (constants hash row equal on Native1254 and the five zones) |
| K4 | P5 (shader texture selection) | shadow compare; `native_texture_*` witness rows unchanged |
| K5 | P6 (programme chain, atomics on the existing counters, retire bumps, `shaders_generation`) | shadow compare; row K-3 (owner stability) |
| K6 | P4 (texture resolution, registry observers, native store bump at `resident` flips) | shadow compare; streaming witness: a `_l` texture becomes full resolution mid-run and the binding changes within one frame |
| K7 | P3 (material blocks by exact writes) | shadow compare on animated materials (wet, damage, phantom) |
| K8 | re-measure with K1 and decide C7 | the split table again |

K2 to K5 touch different code regions of `AdmitNativeModelCandidate` and can be separate commits; K6 is last of the caches because it adds engine observers.

## 5. Rows beyond C-0 to C-5 (read-only)

| Row | Action | Expected |
|---|---|---|
| K-1 | log `admitted_meshes`, `recipe_stage_pairs` and the five register-call counters at S1 | the counts the YEBIS note estimated (34,000 calls) become measured |
| K-2 | count shadow-compare mismatches per cache over a Load Game pass | zero |
| K-3 | log whether `FUN_140D24430` ever returns a different owner for the same (resolver, shader) while the three generations are unchanged | never (P6 complete) |
| K-4 | log, per window, `registry_generation` bumps by source (`D4F250`, `D4F470`, `D4F690`, `D4F770`, `D4EE10`, `D4F4B0`, `D4F7A0`) and the native store generation | the churn the P4 cache must survive; at idle S1 near zero |
| K-5 | Reference observation: one streamed texture replaced while watching `[holder+0x10]` (write watch) | names the writer among `D4B000`, `D4B0D0`, `D4B200` (closes the P4 writer question) |

## 6. Open gaps (summary)

1. P4: whether any registry mutation bypasses the functions listed (rows K-4, K-5, periodic revalidation).
2. P6: that the native generations cover every change `FUN_140D24430` can observe (row K-3).
3. P3: the cost of comparing the writes (unmeasured) and the LRU size under heavy ExParam variation.
4. P2: that equal programme bytes imply equal interface and reads (asserted in shadow mode).
5. The measurement overhead (C-0c).

## 7. Exports

`decompiled/DS3-1152-F-` + `00D24490`, `00D24430`, `00D4ED20`, `002FF080` (lookup chain); `00D4B000`, `00D4ACA0`, `00D4ADE0`, `00D4B0D0`, `00D4B200`, `00D4B9D0`, `00D50FE0`, `00D514C0` (holder and its writers); `00D4F250`, `00D4F7A0`, `00D4F4B0`, `00D4F470`, `00D4E100`, `00D4F690`, `00D4F770`, `00D4EE10`, `00D50AB0`, `00D52640`, `00D34BD0` (registry); and from earlier notes `00E006A0`, `0022D620`, `00140850`.

## 8. K1 result (2026-10-11, Native1289, D3D12 pilot thread)

Code `31b53f27`: `native/frame/collect_profile.h` (thread-local phase attribution, `Scope` and `Switch`, enabled by
`measure.flag`) and the `native_collect_split` row (one per pacing window: self ms per collect for the 14 phases, calls per
collect, counters recipe x stage pairs / admitted meshes / admitted models, and the split sum). Not done: the `KScope` unit
test, the C-0b frame residual stamps, C-0c overhead run, C-5 reuse signatures.

S1 (High Wall idle, no debug layer, measure switch; three steady windows of 120 collects, split sum = collect_ms by
construction):

| Phase | self ms per collect | calls per collect |
|---|---|---|
| SceneSetup | 0.02 | 1 |
| Traversal | 0.12 | 1 |
| EntityFilter | 0.22 | 327 |
| ModelAdmission | 5.4 | 1,318 |
| MeshLookups | 2.8 | 1,007 |
| MeshPrograms | 8.4 | 1,001 |
| MeshTextures | 14.2 | 1,001 |
| MeshShaderTex | 3.2 | 1,001 |
| MeshMatDyn | 1.7 | 1,001 |
| **MeshBlocks** | **68.4-75.4** | 1,001 |
| Passes | 1.3 | 1 |
| Lights | 0.08 | 1 |
| Offscreen | 0 | 1 |
| Teardown | 4.4 | 1 |
| total (= collect_ms) | 110-121 | |

Counters per collect: 8,928 recipe x stage pairs, 1,001 admitted meshes, 126 admitted models (8.9 pairs per mesh).

Decision (table of section 1.2): **MeshBlocks is about 63 % of collect**: build P1 (register plans), then P2 (frame-scope
scene, view and instance blocks shared across meshes), then P3 (material blocks); then P4 for MeshTextures (14 ms) and P6 for
MeshPrograms (8 ms). Teardown (4.4 ms) is the release of the previous scene's 1,001 meshes with their embedded block arrays,
which P2's shared blocks also shrink.

## 9. Caches P1-P6 and R8a: implementation and results (2026-10-11, Native1290-1298, K thread)

Branch `claude/dreamy-vaughan-fd4276`, commits `d4f88c11` (P1, legacy path and shadow compare), `944389bd` (P2, P3),
`23d461b7` (P6), `fdc39804` (alignment fix), `aa81c330` (P4a, P5), `c0fca4cc` (K1 remainder: C-0b, C-0c, C-5),
`5e90b37d` (TextureIngress version), `6f06f0f7` (R8a), `14d504f6` (review fixes), `22253413` (P4b).

What was built (each the former computation split into a part that depends only on its key, kept, and a per-mesh part):
- **P1** `PrepareReadShaderRegister` = `PackShaderRegisterPlan(PlanReadShaderRegister)`; the plan holds an index, not a pointer
  (`register_plan_test`: 4,096 cases against the pre-K code). Each `Programs` plans its four collect families per recipe and
  stage once, with the 1112 MTD fallback words per `cbMtdParam` variable, the `cbInstanceData` zero tail and a composed group's
  final `cbMtdParam` block.
- **P2** block arrays of `NativeSelectedMesh` are `shared_ptr<const ShaderRegisterBlock>` (null = the default block); scene and
  view blocks per programme bytes per scene (entries keep their `Programs`), instance blocks per programme bytes and fade per
  model admission.
- **P3** numeric blocks per `Programs` and exact numeric writes (32 entries, LRU, own mutex).
- **P6** two `Programs` per layout (one per prefer_channel: the single slot rebuilt programmes on every alternation) and the
  last successful choice per prefer_channel, valid while `program_registry_generation` (bumped on every shader, effect,
  material and composed-group change) holds; the material is held weakly; shadow scenes repeat D24430 and compare owner and
  effect (row K-3).
- **P4a** per binding, once per `Programs`: FaceGen test, file stem, ordered D24490 candidates, named scene slots; the scene's
  SYSTEX entries once. **P4b** six pass-through observers of the named texture registry (D4F250, D4E100, D4F690, D4EE10,
  D4F4B0, D4F7A0; pinned) bump `texture_registry_generation`; a binding keeps its lookup only when the first candidate gave a
  resident texture, and a hit needs both versions unchanged (registry and `TextureIngress::Version`, which now moves at every
  admission, residency flip, retirement and close), the record alive (weak) and the holder's vtable and texture words
  unchanged on a fresh read. Later candidates and misses are looked up every frame.
- **P5** `SelectModelShaderTextures` of every recipe and stage per `Programs` and ordered scene-slot list (8 entries).
- **R8a** (D-0164): prepare's twelve per-draw register families pack from plans made once per (recipe, stage, slot, name).
- **K1 remainder**: `collect_profile_test` (self-time attribution), C-0b stamps (`stamp_ms` per frame), C-0c (`no-kprofile.flag`),
  C-5 (`reuse` per collect), counters `ProgramsBuilt`, `SharedBlocksBuilt`, `NumericBlockMisses`, `ShaderTextureMisses`,
  `TextureLookups`.
- Controls: `rebuild-collect.flag` (runner `-RebuildCollect`) runs the pre-K per-mesh paths in the same build;
  `collect-shadow.flag` (`-CollectShadow`) recomputes every 30th scene by those paths, compares status, required flag and bytes
  of every block, every texture lease, scene binding and selection, logs `native_collect_cache_mismatch`, drops the wrong cache
  entries and keeps the legacy values. Tools: `Run-CollectSeries.ps1`, `Get-CollectSplit.ps1`.
- Defect found on the way: `ForwardLightContext::view` is read by 1496D0 with aligned SSE loads; Native1290 faulted (MULPS
  [RSI+0x30]) once the new scene members moved it to an 8-byte boundary. Now `alignas(16)`.

Runs (Thomas's save, Load Game, High Wall idle, MotionBlur HIGH; announced, offline, restoration by hash):

| Run | Build | Settings | Result |
|---|---|---|---|
| Native1290 | P1-P6 | debug layer, shadow | fault at Load Game in 1496D0 (alignment, fixed `fdc39804`) |
| Native1291 | + fix, P4a, P5 | debug layer, shadow | exit 0, 54 shadow scenes, 0 mismatches, 0 debug errors, 1,062 draws |
| Native1292 | same | measure | collect 29.75 ms, fps 14.0 |
| Native1293 | same | measure, legacy | did not load (title menu, pilot); repeated as 1296 |
| Native1294 | same | measure, no K split | collect 29.61 ms: C-0c overhead 0.5 % |
| Native1295 | + review fixes | debug layer, shadow | exit 0, 55 shadow scenes, 0 mismatches, 0 debug errors |
| Native1296 | same | measure, legacy (`-RebuildCollect`) | collect 108.2 ms, fps 6.5 (same-build baseline) |
| Native1297 | + P4b | debug layer, shadow, Equipment page, 4 s walk | exit 0, 59 shadow scenes, 0 mismatches, 0 debug errors, 1,062-1,107 draws |
| Native1298 | same | measure | collect 27.2 ms, fps 14.5 |

Self milliseconds per collect at S1 (medians; `Get-CollectSplit.ps1`):

| Phase | K1 Native1289 | legacy Native1296 | K Native1298 |
|---|---|---|---|
| ModelAdmission | 5.6 | 5.3 | 5.0 |
| MeshLookups | 2.9 | 2.7 | 2.1 |
| MeshPrograms | 8.8 | 8.1 | 0.13 |
| MeshTextures | 14.9 | 13.5 | 5.9 |
| MeshShaderTex | 3.4 | 3.0 | 1.2 |
| MeshMatDyn | 1.8 | 1.5 | 1.4 |
| MeshBlocks | 72.3 | 66.2 | 6.8 |
| Passes | 1.4 | 1.4 | 1.2 |
| Teardown | 4.6 | 6.0 | 3.0 |
| **collect** | **116.2** | **108.2** | **27.2** |
| prepare (pacing row) | 28.7 | 27.0 | 24.6 |
| fps | 6.05 | 6.5 | 14.5 |

Counters per collect at Native1298: 8,928 recipe x stage pairs, 1,001 meshes, 126 models; 0 programmes built, 0 numeric and
0 selection misses in steady state, 1,677 shared blocks packed (scene, view and instance), 2,420 genuine texture lookup chains
(bindings whose first candidate is not resident or with no texture keep the per-frame lookup). C-5: 113 of 126 models keep
the same per-frame inputs from one collect to the next, 13 change. C-0b at S1 (ms per frame): 4.8 before the scene update
(engine), 27.2 scene update (= collect), 6.2 UI PrepareFrame, 30.4 Clear (prepare 24.6, record, render), the rest under 0.2.

Reading: MeshBlocks fell 66 -> 6.8 ms (P1-P3), MeshPrograms 8.1 -> 0.13 (P6), MeshTextures 13.5 -> 5.9 (P4a, P4b), the
selection 3.0 -> 1.2 (P5); prepare gained 2.4 ms (R8a plans). The remaining collect is per-frame engine reads and admission
(ModelAdmission 5, MeshLookups 2, MatDyn 1.4: C6/C7 territory, the persistent record and the R8b snapshot), the packs
(6.8) and the release of the previous scene (3). The frame is now led by prepare (24.6) and collect (27.2), then the UI (6.2)
and the engine (4.8). Open: C6/C7 (persistent admission record, needs the R8b snapshot), the remaining texture lookups
(non-first candidates and misses), prepare's per-draw production (constant writes 9.5 ms, resources 10.7 ms), witnesses
of §5.2 not run in this tranche (character creation, NPC cloth, phantoms, area change S3, quit).

Pre-merge reviews (Opus 5.5, D-0163), two fresh agents: no case where a normal run differs from the legacy path. Applied:
the shadow compare drops the cache entries it finds wrong (P3 entry, P2 scene/view/instance entries, P5 entry, P4b choice of
the binding), the P6 choice holds the material weakly and verifies D24430 and its effect outside the layout lock, C-5
hashing outside the C-3a scope, the shadow mesh count once, a kept texture choice also compares the registry root. Recorded,
not changed: the registry has mutators not observed (D4A120 via D4C200 -> D50AB0; E4B1A0/E4B8C0 delete and recreate it),
which the first-candidate rule, the root and holder re-reads and the shadow cover (gap (c) open, no periodic revalidation);
a composed group's `cbMtdParam` block is a pure function of its immutable `Programs` and is not dropped by a mismatch (a
mismatch there would be a code defect, reported at every shadow scene); P5 makes the 1035/1203 selection witnesses fire on
cache misses only. These last fixes were built and passed the controls; they change only shadow-scene eviction and add one
hit condition, so no further live run was made.
