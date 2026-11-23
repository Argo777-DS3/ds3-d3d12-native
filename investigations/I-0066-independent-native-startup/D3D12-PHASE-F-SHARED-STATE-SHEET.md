# D3D12 phase F: the separate render thread, census of the state shared between the engine side and the render side, synchronisation points and pitfalls (static)

Request (coordinator, 2026-10-11 17:55 UTC): prepare phase F (the separate render thread, R13 and the F2 cut of the master plan) the way phase E was prepared: inventory of the data shared between the engine thread and the render thread, of the synchronisation points and of the pitfalls, then a sheet merged into `main`. Static reading only: nothing was built, run or measured here.

This sheet **complements** the [R13/R14 sheet](D3D12-R13-R14-RENDER-THREAD-PACING-SHEET.md) (worker protocol, immutable `FrameJob`, queue hardening, pacer, commits cR0 to cR6), which listed ten shared members of `SceneRenderer`. It adds what that sheet did not have: a **census of the locks, atomics, thread-locals and globals of the bootstrap module that the cut can touch**, a thread map, the **lock order written once with its exceptions**, the exit semantics, and a pre-flight checklist, taking into account what the later sheets established (the [phase D complement](D3D12-PHASE-D-COMPLEMENT.md): frame objects kept by the slot; the [prepare reduction](PREPARE-REDUCTION-IMPLEMENTATION-SHEET.md): model-level values and the zero-engine-call prepare; the [phase E sheet](D3D12-PHASE-E-FRAME-GRAPH-SHEET.md): an immutable compiled plan).

Naming: the R13 sheet's **F1** (render and present on the worker, prepare on the owner thread) and **F2** (prepare also on the worker) are the two cuts; this sheet's pre-flight items are named **cF.1 to cF.6** to avoid the clash with them and with R8b's "guard F6".

Tree read: `main` `933a9431` and the merges after it (`native/` is unchanged by them): `native/frame/game_bootstrap.cpp` (the line numbers below are that tree's), `native/renderer/{scene_pass.cpp,backend.h,backend.cpp,model_draw.cpp}`, `native/frame/game_texture_ingress.{h,cpp}`. Labels: **STATIC** (read in code), **INFERRED**, **OPEN** (needs a run), **proposed**. The sheet was reviewed by an independent Opus 5.5 pass (one blocking, eight important findings); all are applied.

## 0. Answer first

1. **The cut is smaller than the module looks.** `game_bootstrap.cpp` declares about 45 locks and many globals, but the **collect step already snapshots almost everything into an immutable `NativeSelectedScene`** under short, sequential locks (`AdmitNativeModelCandidate`, from `:4179`: pose `:4262-4270`, storage `:4288`, stream `:4307`, `:4323`, material `:4353`, layout `:4377`, cloth `:4398`, policy `:4669`, each locked alone). Prepare and record read that scene, the texture records it holds, and **a short list of things that are not in it** (section 2.2): the engine memory reads (R8b E1 to E8, the forward-light and environment engine functions), a few registries (`gparams` through `NativeGparamRecords`, the SSS profiles, the lighting programmes, the cube-name map), the texture-ingress map, the journal, and a thread-local flag. Everything else is engine-thread-only or already protected.
2. **Thread map (section 1):** one **owner thread** runs every step hook that touches the renderer (start, begin frame, the render step with the UI frame, end frame with `Clear()`, the SFX and view steps; each refuses another thread: `:6690`, `:7876`, `:7947`, `:9608`, `:9620`, `:10192`, `:10298`); the **collect** runs in `UpdateNativeSceneCpu` (`:5979`) with no thread check (INFERRED the same thread, row R8B-2), and **a second collect, the menu-view fallback, runs inside `Clear()` under `gpu_submissions`** (section 1); **engine worker threads** call the stream, cloth, pose and retirement hooks (thread and step not identified); loaders and the UI sessions use the ingress context only.
3. **What conditions each cut (section 5):** **F1** needs only the lock order and its assertion, the exit semantics (below), the allow-list check, and the R13 sheet's cR0 to cR3 (prepare and its diagnostics stay on the owner thread). **F2** additionally needs the thread-local and the witness file writes out of prepare (cF.1, cF.3), the registries of 2.2 resolved at collect or made leaf calls (cF.6, prepare reduction cP7), and the engine reads removed (cP6, cP7).
4. **There is not one lock chain; there is one chain plus one cycle risk to remove.** The census found the orders that exist (section 3.3). Two of them look opposite and are not (`Close`: `gpu_submissions` then `model_storage_lock`; `CloseNativeModelStreams`: `model_stream_lock` then `gpu_submissions`; together a chain), **but the menu-view fallback collect runs under `gpu_submissions` and takes `model_stream_lock` (and the storage, pose, material, layout, cloth, policy, simulation-time, common-image and gparam locks) through `AdmitNativeModelCandidate`**, which is the exact reverse of `CloseNativeModelStreams`. No deadlock today because both run on one thread; the "single order" claim is false until `CloseNativeModelStreams` is changed to set its flag under `model_stream_lock`, release it, and only then take `gpu_submissions` (cF.4).
5. **Exit semantics are the opposite of the first draft.** `ExitProcess` terminates the **other threads first** and then runs the DLL detach and the static destructors: the global `SceneRenderer renderer` (`:296`) is destroyed there and its `Close()` takes `retire_lock_` (`backend.cpp:321-324`); a worker killed while holding that lock makes the exit hang, and a still-joinable `std::thread` destroyed then calls `std::terminate`. The render-thread object and the renderer must be detached or deliberately leaked (as RCA879 does for `ui_sessions`) and no destructor run at detach may take a lock the worker can hold (section 3.1 Y13, section 4 item 7).
6. **The engine tolerates the cut** (STATIC for the original: depth-1 queue, clock after the post, [R13/R14 sheet](D3D12-R13-R14-RENDER-THREAD-PACING-SHEET.md) section 1); only the native hooks assumed `Present` inside `Clear()`.
7. **Commits:** the R13 sheet's order stands; this sheet inserts a behaviour-neutral pre-flight commit **cF** (cF.1 to cF.6) before cR2 and adds tests and witnesses (section 6).

## 1. Thread map (STATIC unless marked)

| Thread | Runs | Evidence | Touches the cut |
|---|---|---|---|
| **owner thread** (the thread of the first bootstrap frame; also the window's thread, INFERRED: the `native_window` row logs both ids) | `Start` (`:9608`), `FrameBegin` (`:6690`), the render step incl. UI `PrepareFrame` and `native_ui_frame` (`:7876`), `EndFrame` and `Clear()` (`:7947`), the SFX phase (`:10192`), the view steps (`:9620`, `:10298`); the collect when it runs (R8B-2). `Close()` can be reached from these before their thread check (the stop path: `:9528`, `:9619`, `:10191`, `:10297`) | `owner_thread` checks that `Refuse` otherwise | everything now; after the cut: collect, UI admission build, job construction, `Post` |
| **menu-view fallback collect, inside `Clear()`** | `RenderNativeOffscreenViews` calls `CollectNativeOffscreenScenes` (`:7042-7043`) when the presentation thread is the menu thread (title, Load Game: no scene CPU update); it runs `AdmitNativeSelectedObject` for each listed view under **`gpu_submissions`** (`Clear()` holds it from `:720`) | comments `:7040-7041`; `native_menu_thread` `:6759`; try-lock `native_offscreen_collect_lock` `:6941` | the lock order (3.3); the thread-identity test `:7042` stops being true if view preparation moves to the render thread (F2) |
| **render worker** (new) | `Render*`, `Execute`, `Present`, slot retirement, witness readbacks | R13 sheet | the renderer and `DeviceQueues` submission side |
| **engine worker threads** | hooks for physical streams (`0x51B940`, which takes an engine lock `image+0x4793C48` before `model_stream_lock`, `:1816-1823`), cloth upload (`0x23D5BC0`: designed for concurrent callers, MENUS-MAP), pose updates, the retirement hooks of storage, streams, poses, materials, layouts, effects, shaders, textures | the locks of 2.1; INFERRED, steps not identified | registries (2.1), `RetireWhenSubmittedWorkCompletes` |
| **loader and UI-session threads** | texture admission (`RecordIngress`/`Commit`), UI image import; the UI frame preparation under `ui_frame_preparation_lock` and `ui_sessions_lock` (`:7902-7904`, `:9738`) | `game_texture_ingress.h:31-35` | `ingress_lock_`, `TextureIngress::mutex_` |
| **game thread feeding the movie** | `native_movie::pending` written at `:7384-7387`, consumed by the presentation thread | | the pending movie frame (Y15) |
| **Bink threads** | `BinkDoFrameAsyncMulti` decode workers; the movie render step waits up to 1 ms (`:7303`) | | |
| **vectored exception handler** | any thread; `guarded_copy_active` is `thread_local` (`:591`, `:6453`), so `Read()` is safe on the worker | STATIC | none |

## 2. Census of the shared state

Classes of the R13 sheet: **J** immutable job data, **E** engine/owner-thread only, **W** worker-owned, **S** shared and synchronised. "Prepare/record" = read by `PrepareNativeModelFrame`, by `RenderNativeMovieFrame`, `RenderNativeOffscreenViews` or by the `renderer.Render*` calls.

### 2.1 Registries filled by engine hooks, snapshotted at collect (no prepare/record access unless noted)

| Global | Lock | Writers (hooks) | Collect read | Prepare/record | Class after the cut |
|---|---|---|---|---|---|
| `model_storage` | `model_storage_lock` | `AdmitModelStorage`, `AdmitModelCpuGeometry`, `RetireModelStorage`, `Close` | `AdmitNativeModelCandidate` (shared_ptr copy) | none: the scene holds the storage | S, J |
| `model_streams`, `cloth_streams` | `model_stream_lock`, `cloth_stream_lock` | `UpdateNativePhysicalModelStreams`, `NativeClothVertexUpload`, `NativeClothSetRelease`, `Close*` | same | none: immutable versions in the scene | S, J |
| `model_poses`, `model_policy_flags` | `model_pose_lock`, `model_policy_lock` | `InitializeNativeModelPose`, `UpdateNativeModelPose[Public]`, `RetireNativeModelPose` | same | none | S, J |
| `model_materials`, `materials`, `model_layouts` | `model_material_lock`, `material_lock`, `model_layout_lock` | `AdmitMaterial`, `BuildModelMaterial`, `DestroyModel*`, `RetireMaterial` | same + `Programs` build | none (the scene holds `Programs`) | S, J |
| `shaders`, `effects`, `composed_effects` | `shader_lock`, `effect_lock`, `composed_effect_lock` | `AdmitShader`, `AdmitEffect`, `ComposeNativeShaderGroup`, `Retire*` | `AcquireNativeModelPrograms`, `PlanNativeModelTextures` | `FindLoadedProgramme` (lighting programmes, 2.2) | S |
| `native_cube_names`, `native_texture_constants` | `native_cube_name_lock`, `native_texture_constant_lock` | texture admission hooks (`:6369`) | admission | **`ProduceNativeEnvRegions` reads `native_cube_names` under its lock (`:3062`, `:3093`)** | S (see 2.2) |
| `decal_cpu_registries`, `scene_tree_cpu_owners`, `scene_*_cpu_owners`, `env_region_cpu` | `decal_cpu_lock`, `scene_tree_cpu_lock`, `scene_view_cpu_lock` (recursive), `env_region_cpu_lock` | the engine-facing CPU object hooks | collect | none | E |
| `native_selected_scene`, `native_velocity_history` | `scene_view_cpu_lock` (taken **briefly** by the collect: `:5119`, `:5141`, `:5159`, `:5190`, `:5240`, `:5243`) | collect | `AcquireNativeSelectedScene` (`Clear()`) | the scene is read by prepare | the **handoff object** (J) |
| `native_simulation_time` | `simulation_time_lock` | `UpdateNativeSimulationTime` | collect | via the scene | S, J |
| `native_common_images` | `common_image_lock` | init/close | collect (`:5184`, `:6987`) | via the scene | S, J |
| `native_offscreen_scenes`, `native_offscreen_proxies`, `native_offscreen_armed` | `native_offscreen_lock` | the view hooks, the fallback and scene-update collects (`:7031-7035`) | collect | `RenderNativeOffscreenViews` (`:7048`) copies the list under the lock | S, then J (Y16) |
| `native_movie::pending` | `native_movie::lock` | the game thread (`:7384-7387`) | | `RenderNativeMovieFrame` swaps it out (`:7463`) | S, then J (Y15) |

These registries need **no change** for the cut. Their only R13 relevance is the retirement path: hooks on engine threads call `RetireWhenSubmittedWorkCompletes`, which must stay legal from any thread (R13 sheet 5.2) and must not free objects the worker's slot still holds (phase D complement 2.1).

### 2.2 Read by prepare/record but not in the scene (the real cut)

| Item | Where | Today | Problem for the worker | Fix |
|---|---|---|---|---|
| engine memory: `model.group`, `model.animation`, `material+0xD0` (E1 to E8) | prepare resource loop via `Read` | owner thread | engine writers run concurrently (R8B-1) | R8b S1 to S3 / prepare cP6 (F2) |
| forward light: four engine functions, gparam bank lookups | `ProduceNativeForwardLight` (`:3258`); **its `NativeGparamRecords` constructor takes `gparam_lock` (`:3143`)** | owner thread | engine code on a non-engine thread; a registry lock inside prepare | S4 / cP7 (F2; INFERRED fine as functions: they ran on the GX task pool in the original, COLLECT-ORIGINAL-STATIC section 0 point 4) |
| environment regions | `ProduceNativeEnvRegions` (`:3020`): engine function `FUN_1400416A0`, region records, **`native_cube_name_lock`** (`:3062`, `:3093`), **`textures.Acquire` (`TextureIngress::mutex_`, `:3069`)**, E8 row `image+0x4929B20` (`:3025`) | owner thread (main view only) | engine call, two leaf locks, a `.data` read | S4 / cP7 |
| lighting programmes | `AcquireNativeLightingPrograms` (`:2817`): function-static mutex + `resolved`, **then `shader_lock` through `FindLoadedProgramme` (`:2819` then `:2806`)**: a nesting | owner thread | first call on the worker while the engine loads programmes (it returns empty until all are found, so no partial state) | resolve at collect or first use; do not hold the static mutex across `FindLoadedProgramme` (cF.6) |
| light accumulation parameters | `ProduceNativeLightAccParam` (`:3101-3116`): **pure computation, no lock, no engine read** | owner thread | none | none (earlier draft wrongly listed a lock here) |
| SSS profiles | `ProduceNativeSssInputs` (`:2991`; `native_sss_lock`; reads the executable image on its first call, `:2948`) | owner thread | a leaf lock and a one-time image read | acceptable as a leaf, or a snapshot at collect |
| `image+0x4929B20` default row (E8) | PS `cbModelParam`, env regions | owner thread | `.data` read per row | cached after engine init (R8B-4) |
| benign reads written once | `output_width`, `output_height` (`:5336`, written `:9594`), `collect_rebuild` (`:5324`), `gpu.device` (`:5889`) | | none after start | none |
| **thread-local `native_offscreen_preparing`** (`:2590`) | read by prepare's diagnostics (`TextureWitness`, `:5283`) and by collect (`:4747-4773`); set by `RenderNativeOffscreenViews` around `PrepareNativeModelFrame` (`:7055`) | same thread, so it works | on the worker the flag is false: wrong sample buckets (prepare); collect stays on the owner thread | use the existing parameter `offscreen` in prepare's diagnostics (cF.1); the colour-witness rows at `:2626` and `:3849` belong to collect |
| `native_prep_ticks[3]` (non-atomic, `:664`), function-local `static LONG64` sampling counters | prepare | owner thread | two writers while both threads run it | atomics or worker-local copies (X9 of the R13 sheet; cR0) |
| **witness file writes inside prepare** | `std::ofstream` of `hair-forward-ps.dxbc` and `hair-forward-vs.dxbc` (`:5825`, `:5879`: once per process), reading `control->journal` | owner thread | disk I/O on the worker's critical path, a global (`control`) read | run once on the owner thread at collect, or drop (cF.3) |
| `NativeShaderWordRanges` (`:2600`) | prepare | calls `Refuse` on a coverage error | `ExitProcess` from the worker | acceptable (same as any `Refuse`); see 3.1 Y13 |
| `RenderNativeMovieFrame` (`:7461-7490`) | `native_movie::lock`, `textures.Acquire`, `control->journal`, a non-atomic `static uint32_t next_picture` | owner thread (in `Clear()`) | stays on the owner thread in F1 (it builds the movie part of the job); a job field in F2 | the `static` becomes a job-built value or an atomic |
| `RenderNativeOffscreenViews` (`:7039-7073`) | `native_offscreen_lock`, `textures.Acquire`, a `static unordered_map drawn` (`:7063`), `control->journal` (`:7064`) | owner thread | same | same; the `drawn` map is single-writer on the owner thread |
| `Log()` / `journal_lock` (`:295`, `:581-588`) | everywhere | mutex-protected | contention and disk I/O on the critical path of both threads | a leaf; bounded rows already; an async log is out of scope |
| `ModelDrawCaches::lock` (recursive) | `OwnedNativeModelDraw::Prepare` | owner thread | serialises every `Prepare`; chunk deleters take it from any thread; `root_shapes_lock` is taken under it (`model_draw.cpp:80`, `:117`) | stays; with the worker as the only preparer it is uncontended |
| `Programs` locks: `numeric_lock`, `texture_choice_lock`, `shader_texture_lock`, `draw_plan_lock`, `programs_lock` | collect (K caches) and prepare (`draw_plan_lock`) | owner thread | collect (engine side) and prepare (worker) run concurrently on one `Programs` | already mutex-protected leaves; row RF-5 |

### 2.3 Renderer, queue and ingress (the R13 sheet's X1 to X10 plus)

Inventoried in the R13 sheet section 3.3; the additions from the later sheets: the slot's `kept` set must hold the frame inputs and the movie/view target resources and the environment cubes (phase D complement 2.1); the compiled plan is immutable job data with declared boundary states and a structural cache key, the back-buffer slot being an execution parameter (phase E sheet 5.1); the `native_scene_copy` record (`native_scene_copy_lock`, `:667`) is refreshed when the renderer recreates the scene copy: the writer (`PublishNativeSceneCopy`, called from `Clear()` before prepare) and the resource creation stay on the owner thread (X2), the worker records against the job's pointer. Leaf locks taken by destructors on any thread: the descriptor pool, the ingress pool and the `UploadArena` pool locks (`backend.cpp`).

### 2.4 Pure owner-thread state that must stay out of the job

`native_ui_frame` (a plain `std::optional`, no lock: written by the render step, consumed by `Clear()`, `:7877`, `:731-738`), `owner_thread`, `opened`, `lifecycle_frames`, `world_gate_*`, `native_colour_settings` (`colour_settings_lock`; written by the render step `:7934`, no reader on the render side), the input-latch atomics (`native_mouse_*`, `native_key_presses`, `:7697-7701`): single-writer, no worker access; **none of them may be dereferenced by the worker**; the job carries values.

## 3. Synchronisation points

### 3.1 Inventory

| Id | Edge | Mechanism today | Mechanism after the cut | Hazard |
|---|---|---|---|---|
| Y1 | engine thread to render: the frame | in-line call | `Post(FrameJob)`, depth-1, idle event (R13 8.1) | post under a native lock, message pump re-entrancy |
| Y2 | render to engine: the next post waits | none | idle wait (W2 pumped wait) | unbounded in the original; native deadline proposed |
| Y3 | scene handoff collect to `Clear()` | `scene_view_cpu_lock` (brief) | unchanged, then the scene enters the job | pointer only |
| Y4 | UI frame handoff render step to `Clear()` | plain global, same thread | same thread; the UI snapshot enters the job (X1) | must not be read by the worker |
| Y5 | ingress commit to submission | `ingress_lock_`, swap inside `Execute` | `submit_mutex_` (**proposed**, cR1) covers the swap | order of ingress vs frame list |
| Y6 | retirement (any thread to the queue) | `retire_lock_`, `RetireAfter`, `CollectCompleted` by the frame owner | `CollectCompleted` on the worker; hooks call `RetireWhenSubmittedWorkCompletes` from any thread | release outside the lock; frame objects held by the slot |
| Y7 | completion and receipts | `Complete`, receipt returned; check `:743-744` | `ReceiptCell`, worker mirror (X7, X10) | receipt of job N-1 |
| Y8 | faults | `Refuse`/catch in `EndFrame` | fault channel, `exception_ptr` | `std::terminate` on an uncaught worker exception; `BeginFrame` `noexcept` |
| Y9 | quiesce | `Drain` in `Close` paths | `Quiesce()` before any native lock | lock order (3.3); `Close()` is reachable before the thread checks (section 1) |
| Y10 | `Present` vs the window | owner thread | worker thread (DXGI is free-threaded for flip model; `DXGI_MWA_NO_ALT_ENTER`, `backend.cpp:526`; `ResizeBuffers` and full screen are not wired in the game path) | the window thread must not wait without pumping while the worker presents (the pumped `Post` wait; INFERRED, low risk) |
| Y11 | prepare caches vs collect caches | single thread | two threads on one `Programs` | leaf locks exist; row RF-5 |
| Y12 | witness channel | public members | `WitnessChannel` (X3) | |
| Y13 | process exit | `Refuse` calls `ExitProcess` from any thread | same | **`ExitProcess` terminates the other threads first, then runs the DLL detach and the static destructors** (`SceneRenderer renderer`, `:296`, whose `Close()` takes `retire_lock_`, `backend.cpp:321-324`); a worker killed while holding a lock hangs the exit; a joinable `std::thread` destroyed there calls `std::terminate`. Requirements: the worker object is detached or deliberately leaked; the renderer is leaked or its destructor takes no lock the worker can hold (RCA879 does this for `ui_sessions`, `:331-336`); the worker is never joined from detach |
| Y14 | engine clock | after `Clear()` | after the post (R13 8.3) | RT-9 witness |
| Y15 | pending movie frame | `native_movie::lock`, `pending` swapped in `RenderNativeMovieFrame` (`:7463`) | the movie frame enters the job by value | single consumer |
| Y16 | offscreen scene list | `native_offscreen_lock`, copied in `RenderNativeOffscreenViews` (`:7048`) | the list enters the job (views by value) | the collect (menu thread or scene update) is the writer |

### 3.2 The rule for crossing

Everything that crosses is (a) a `shared_ptr<const>` snapshot, (b) a value, or (c) an atomic mirror; nothing crossing is a pointer into a registry container, a reference to a `thread_local`, a raw engine pointer or a raw `ID3D12Resource*` whose owner lives on the other side.

### 3.3 Lock order: the chain, its exceptions, and the cycle to remove (proposed to be written once in `render_worker.h`)

```
 0  Post / Quiesce / idle wait               (never under any lock below)
 1  ui_frame_preparation_lock -> ui_sessions_lock        (UI frame; before gpu_submissions at :9712-9719)
 2  scene_view_cpu_lock (recursive)          (taken BRIEFLY by collect and by AcquireNativeSelectedScene; also held across some engine calls, :1374-1383, and re-entered, :1298-1305)
 3  gpu_submissions                          (Clear, Close, Start, ReadStartupView, CloseNativeModelStreams after the fix, UI session ctor)
 4  model_storage_lock                       (Close: after gpu_submissions)
 5  the short registry locks                 (model_stream, pose, policy, material, model_material, layout, cloth, shader, effect, composed, gparam, sss, simulation_time, native_scene_copy, common_image, native_cube_name, native_texture_constant): taken alone; the exceptions: AcquireNativeLightingPrograms' static mutex -> shader_lock
 6  under gpu_submissions in Clear:          TextureIngress::mutex_, native_movie::lock, native_offscreen_lock, the descriptor-pool lock
 7  DeviceQueues: submit_mutex_ (proposed) -> ingress_lock_ ; submit_mutex_ -> retire_lock_ ; ModelDrawCaches::lock -> retire_lock_ ; ModelDrawCaches::lock -> root_shapes_lock ; Programs leaf locks
 8  leaf: journal_lock / Log
```

**The cycle to remove.** The menu-view fallback collect runs inside `Clear()` (`:7042-7043`) with `gpu_submissions` held and then takes the step-5 locks (`model_stream_lock` at `:4307`, `:4323`, and the others), i.e. `gpu_submissions` then `model_stream_lock`; `CloseNativeModelStreams` (`:1730-1732`) takes `model_stream_lock` then `gpu_submissions`. Fix (cF.4): `CloseNativeModelStreams` sets `model_streams_closed` under `model_stream_lock`, releases it, takes `gpu_submissions`, drains, and clears the table under `model_stream_lock` again (step 5 below step 3); the alternative, moving the fallback collect out of `Clear()` before `gpu_submissions`, is cleaner for F2 (view preparation then runs without engine-facing locks held) and is recorded as the preferred follow-up.

Evidence for the edges: `Clear()` comment "Acquire current CPU selection before the submission lock; resource retirement uses submission -> CPU-owner order elsewhere" (`:717-719`); `Close()` `:857-887` (`:878-883` for the pair); `CloseNativeModelStreams` `:1730-1732`; `AdmitNativeModelCandidate` takes the step-5 locks one at a time (`:4262-4398`); `UpdateNativePhysicalModelStreams` takes an **engine** lock (`image+0x4793C48`) before `model_stream_lock` (`:1816-1823`; the engine-side order is OPEN); the `try_lock` of `native_offscreen_collect_lock` (`:6941`; its comment `:6937-6941` predates the brief-lock collect and is stale). Recursive locks (`scene_view_cpu_lock`, `decal_cpu_lock`, `env_region_cpu_lock`, `ModelDrawCaches::lock`) mean the debug assertion helper (cF.4) must accept re-entry of the same lock. **OPEN:** nestings in the engine-facing hooks of 2.1 that were not read.

## 4. Pitfalls

1. **Thread-locals** (`current_ui_selection`, `native_cpu_prepare_active`, `cpu_mesh_context`, `cpu_decode_scope`, `glyph_cpu_allocation`, `current_ui_uniform_scope`, `native_decal_query_scope`, `model_layout_material`, `native_offscreen_preparing`, `guarded_copy_active`, and the inline `thread_local` of `collect_profile.h:49`; `:176`, `:242-287`, `:254`, `:559`, `:591`, `:2590`): only `native_offscreen_preparing` is read in prepare's path; the others belong to collect/UI/engine hooks that stay on their threads. The allow-list check (cF.5) makes a new one in the worker's functions a failure.
2. **Function-local statics** (witness helpers, `:1950`, `:2039`, `:2092`, `:2203-2354`, `:4172-4193`, `:4962`, `:6244`, `:6827-6850`; about 214 `static` declarations in the file, 148 of them `static LONG64` counters): initialised on first use from whichever thread arrives first (thread-safe since C++11) and mutex-guarded or atomic; several sit in the prepare path as sampling counters (`InterlockedIncrement64`): fine. The non-atomic exceptions: `next_picture` (`:7470-7471`), `drawn` (`:7063`), `native_prep_ticks`.
3. **Static-guarded initialisation inside prepare** (`AcquireNativeLightingPrograms`' `resolved`): after F2 the first call happens on the worker while the engine thread may be loading the programmes; the function returns empty until everything is found, so only the nesting `static mutex -> shader_lock` (3.3) matters.
4. **Engine functions on a non-engine thread** (forward light, environment regions): INFERRED safe (the GX task pool ran such code in the original); removed from prepare by cP7 before F2.
5. **Disk I/O and logging on the critical path:** the witness dumps and `Log` rows; move the one-time dumps to the owner thread, keep sampled rows.
6. **Uncaught exceptions:** a `std::thread` without a catch calls `std::terminate`; the vectored handler logs faults but the process then dies without the module's exit code. The worker body is a catch-all feeding the fault channel (R13 8.2); SEH faults inside `GuardedCopy` stay local.
7. **Exit and loader lock:** see Y13: the worker is created at `Start`, never in `DllMain`; stopped by `Close` after `Quiesce`; never joined at detach; the worker object and the renderer are leaked (or the destructor-time `Close()` is made lock-free) because the other threads are already dead when the destructors run.
8. **`owner_thread` checks:** `Start`, `Views`, `FrameBegin`, the render step and `EndFrame` refuse any other thread; they keep their meaning (engine-side hooks). The renderer methods the worker calls must not acquire that check; the R13 sheet's thread-affinity assertions (T-R5) become "owner or worker as appropriate". `Close()` may run before those checks on the stop path (section 1).
9. **Receipt-based exits:** `Clear()` throws on a zero token or `debug_errors` (`:743-744`); with the worker the check consumes the previous job's receipt, and the first frame has none (R13 X10).
10. **Window messages:** pumping while `Post` waits can re-enter the engine's window procedure from inside the end-of-frame hook (OPEN, RT-6); `Post` and `Quiesce` outside every native lock (3.3 step 0).
11. **Debug layer:** the info queue is process-global; the worker is the only reader after the cut (X6).
12. **Priority and timers:** the worker runs at normal priority; the pacer's coarse sleep rounds up (`Sleep(ceil)`) and depends on the timer resolution the engine sets (INFERRED).
13. **First frames:** startup, title and loading frames take `RenderStartup*` (no model frame): the job with `models == null` must exercise the same post/idle path from the first frame.
14. **The fallback collect inside `Clear()`** (section 1): with F2 the view *preparation* moves to the worker while the *selection* (collect) stays on the owner thread; the test `GetCurrentThreadId()==native_menu_thread` (`:7042`) is evaluated on the owner thread only and keeps its meaning, but the comment "presentation and menu steps then run on one thread, so no menu teardown can interleave" no longer covers the worker's use of the selected scenes: the scenes it receives are immutable snapshots (`NativeSelectedScene`), which is what makes it safe.

## 5. Pre-flight commit cF (before cR2 of the R13 sheet) and what conditions F1 and F2

| Item | Content | Conditions | Test |
|---|---|---|---|
| cF.1 | `native_offscreen_preparing` no longer read in prepare's call tree (use the `offscreen` parameter in `TextureWitness`) | F2 | static check S-1 (below) |
| cF.2 | `native_prep_ticks` and the prepare sampling counters atomic or per-thread (this is the R13 sheet's X9 / cR0; listed so it is not forgotten) | F2 (F1 if the counters are written from both threads) | T-F2 |
| cF.3 | the one-time witness dumps (`hair-forward-*.dxbc`) and the `control->journal` path moved out of prepare (owner-thread helper at collect, or removed); the `static` `next_picture` and `drawn` made atomic or confirmed single-writer | F2 | S-1 |
| cF.4 | the lock order of 3.3 documented in `render_worker.h`, a debug assertion helper (a thread-local "held level" that accepts re-entry of the same lock and is checked on acquisition of steps 1 to 4 and on `Post`/`Quiesce`), and the change to `CloseNativeModelStreams` (flag under its lock, release, `gpu_submissions`, drain, clear) | **F1** | T-F4 |
| cF.5 | `scripts/Test-ThreadAffinity.ps1`: scope = an explicit list of functions (`PrepareNativeModelFrame` and its named callees, `RenderNativeMovieFrame`, `RenderNativeOffscreenViews`, the `Render*` functions of `scene_pass.cpp`, `OwnedNativeModelDraw::Prepare`) and the headers they include; it reports `thread_local` and mutable function-local `static` declarations (excluding `static const`, `constexpr` and function declarations) and compares them with a reviewed allow-list indexed by (file, function, name); a regular expression cannot compute reachability, hence the explicit list | **F1** | T-F5 |
| cF.6 | `AcquireNativeLightingPrograms` does not hold its static mutex across `FindLoadedProgramme`; `ProduceNativeSssInputs`/`NativeGparamRecords` documented as leaf calls | F2 | RF-5 |
| cF.7 | exit hygiene: the worker object and `renderer` leaked or their destructors made lock-free; an injectable exit function in `Refuse` for the test host | **F1** | T-F6 |

F1 (render and present on the worker, prepare on the owner thread) needs cF.4, cF.5, cF.7 and the R13 sheet's cR0 to cR3; F2 adds cF.1 to cF.3, cF.6 and the prepare reduction's cP6 and cP7.

## 6. Tests and witnesses

| Id | Content | Where it can run |
|---|---|---|
| S-1 | Static check (a script, part of cF.5's tooling): the named prepare functions contain no read of `native_offscreen_preparing`, no `std::ofstream`, no access to `control->journal` | no build |
| T-F2 | Counter races: the atomic counter struct (a small header, like `collect_profile.h`) incremented by two threads gives the exact sum | host test, header-only |
| T-F4 | Lock order: the assertion helper (header-only, with fake mutexes) trips on a reverse acquisition, accepts re-entry; the live `Close`, `CloseNativeModelStreams`, `Clear` sequences, including the fallback collect, are checked by the helper's counters in a real run (RF-1) | host test + live |
| T-F5 | The allow-list check passes on the tree and fails when a `thread_local` is added to a listed function | script |
| T-F6 | Exit: with an injectable exit function, `Refuse` on the owner and on the worker while a job is in flight calls it once with the expected code; a host that leaks the worker and the renderer reaches process end without hang or `std::terminate` | host test with the seam of cF.7 |
| RF-5 | Concurrent collect-side and prepare-side caches on one `Programs`: the K shadow comparison as oracle over 600 frames with the worker (F2) | live only (the oracle exists only in a real run) |

| Row | Action | Expected |
|---|---|---|
| RF-1 | `render-thread.flag` against off with the `native_threads` rows of the R13 sheet cR0 (thread ids of collect, prepare, record, window) and the lock-order helper on, including a title/Load Game run (the fallback collect) | same image; thread ids as the map of section 1; no assertion |
| RF-2 | quit, area change and movie close with a job in flight (phase D RD10) under the worker | exit 0, no hang, no use after release |
| RF-3 | slow-disk witness: journal on a throttled path, 600 frames with the worker | `owner_frame_ms` and `worker_busy_ms` unaffected by the log (rows bounded) |
| RF-4 | engine-side stress: area changes while prepare runs on the worker (F2 only, after cP6/cP7) | zero R8B differences, no engine call from the worker (R8B-6) |

## 7. Risks and open points

1. **The census is of the bootstrap module as read;** a lock or global reached only through an engine hook of an unread subsystem (decals, env regions, scene views, SFX) is outside the cut by construction but is not proved absent: the allow-list and RF-1's rows are the net.
2. **R8B-2 (the thread of collect)** is still open: if collect runs on an engine worker thread rather than the owner thread, the scene handoff (Y3) is a cross-thread edge today as well; the design already treats it as one.
3. **Message pump re-entrancy (RT-6)** and the **window thread identity** stay live-only.
4. **The engine's own lock order** (`image+0x4793C48` before `model_stream_lock`, engine calls under `scene_view_cpu_lock` and `gpu_submissions`) is not mapped: OPEN.
5. **Not covered:** a second render queue or compute threads (phase G), moving collect off the owner thread (workstream K's parallel pool, a separate sheet).

## 8. Evidence and limits

Code: `native/frame/game_bootstrap.cpp` (the locks and globals at `:101-450`, `:557-567`, `:591`, `:664-667`, `:1708`, `:1888-1892`, `:2552-2590`, `:2755`, `:2806-2819`, `:2942`, `:3020-3143`, `:5979`, `:6728-6759`, `:6937-6941`, `:7039-7073`, `:7461-7490`, `:7697-7701`; `Clear()` `:716-745`; `Close` `:857-887`; `CloseNativeModelStreams` `:1729-1732`; owner checks `:6690`, `:7876`, `:7947`, `:9608`, `:9620`, `:10192`, `:10298`; the lock-to-function map produced by a scan of every `lock_guard`/`unique_lock` site), `native/frame/game_texture_ingress.{h,cpp}`, `native/renderer/{scene_pass.cpp,backend.cpp,model_draw.cpp}`. Documents: the R13/R14, phase D complement, prepare reduction and phase E sheets, [R8B-ENGINE-READS-STATIC](R8B-ENGINE-READS-STATIC.md), [COLLECT-ORIGINAL-STATIC](COLLECT-ORIGINAL-STATIC.md), MENUS-MAP (cloth upload). No build, run or measurement; nestings not read are marked OPEN.

## Amendments by the graphics options sheet

The [graphics options sheet](D3D12-PHASE-G-GRAPHICS-OPTIONS-SHEET.md) section 4 adds to the census of section 2: `native_sampler_bank` (`EngineSamplerBankOwner`, its own mutex, read at the render step `:7926`, at the collect `:5188` and at the offscreen collect `:6989`, which can run inside `Clear()`), the quality object and its applied copy at `GXRS+0x5360/0x52F0` (written at the render step, read by the point-light fade at collect), and the velocity function-static table copy (`NativeMotionBlurQualityRow`, `:5105`). The three direct reads of the levels are to be replaced by one per-frame snapshot carried in the frame input (the `OptionChange` mechanism).

## Amendments by the engine step and thread map

The [engine step and thread map](ENGINE-STEP-THREAD-MAP.md) answers the thread points this sheet marked INFERRED or OPEN. The owner thread is the window thread in **506 of 506** retained runs (OBSERVED); `Start` sets it and `BeginFrame`, the render step and `EndFrame` refuse any other thread (they have not refused, bar one shutdown artifact). The collect (`UpdateNativeSceneCpu`) runs inside the `DrawStep` step (row 79 of the engine's 88-row, six-lane frame table, alone in its row) on the owner thread (INFERRED, strongly). The engine's own tasks run on the owner thread **and** on up to five pool threads (`FD4JobWorker:N`): the `g63`/`g62`/`g14` steps are pool-capable (the player-model update through the `ChrIns` Ez tasks ran on a pool thread in 32 % of 3,073 periodic samples, OBSERVED), and the Scaleform UI factory ran on a pool thread in 472 of 475 runs. Threads outside the task table also exist (`GXWorker` x7, `HavokWorkerThread`, loaders). So the thread census of section 2 gains "pool threads" and "other worker threads" as caller classes for the model, pose, stream, UI and storage hooks (section 4 of the map), and `Test-ThreadAffinity.ps1` (cF.5) takes its owner-only list from there. The locks already listed cover these callers; the owner-thread checks of section 8 keep their meaning.

## Amendments by the delayed-deletion audit

[DELAY-DELETE-AUDIT](DELAY-DELETE-AUDIT.md) adds to the kept-set rule: `RetireWhenSubmittedWorkCompletes` (`backend.cpp:779-782`) takes `LastSubmitted()` at the call, which is right only while submission is synchronous with the retirement caller. Once record runs on the render worker (F1/F2), a retirement at `DelayDeleteStep` (row 85) of frame N, before job N has submitted, receives the ticket of frame N-1: the job must hold its own `shared_ptr` to everything captured at collect until its own ticket, or retirement must use the maximum of `LastSubmitted()` and the ticket reserved for the job in flight. The pose retirement is safe (`Withdraw()` resets the owner's current version; held versions stay valid); the weakly held texture selection (`game_bootstrap.cpp:521-523`) needs the same check.
