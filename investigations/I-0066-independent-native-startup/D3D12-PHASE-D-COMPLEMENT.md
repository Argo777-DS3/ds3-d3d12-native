# D3D12 phase D: complement (what the implementation sheet leaves open, answered from the code; shared objects between frames in flight; the waits that remain)

Request (coordinator, 2026-10-11 17:11 UTC): the pilot has started phase D (cD0, audit counters, on the K branch `claude/dreamy-vaughan-fd4276` at `6b2da3c5`, **not yet in `main`**). Find what the [phase D sheet](D3D12-PHASE-D-IMPLEMENTATION-SHEET.md) leaves open, for example the resources shared between frames in flight and the waits that remain, and deliver a complement merged into `main`. Static reading only: nothing was built, run or measured here.

Tree read: `main` `933a9431` for `native/renderer/{backend.h,backend.cpp,scene_pass.h,scene_pass.cpp,model_draw.cpp,model_frame.h,primitive_pass.cpp}`, `native/frame/{game_bootstrap.cpp,game_texture_ingress.{h,cpp}}` (the line numbers below are that tree's; the sheet's moved after workstream K), the pilot's cD0 diff of `backend.h` (`WaitSite`, `GpuAudit`), and the design notes of I-0067/I-0068. Labels: **STATIC** (read in code), **INFERRED**, **OPEN** (needs a run), **derived** (arithmetic on read values). The complement was reviewed by an independent Opus 5.5 pass (no blocking finding, seven important ones); all are applied.

## 0. Answer first

1. **The sheet's lifetime rule L1 is incomplete in a way that matters the day the per-view and per-movie `Drain`s go (cD3).** Today a frame's inputs die at the end of `Clear()` only because `Clear()` has already drained. Two groups of GPU-referenced objects are not in the sheet's `FrameSlot` (`draws`, `kept`): the **environment-cube records** held by the main frame input's `lighting` (sampled through a descriptor copy), and the **target resources of the movie and of every menu view** (the renderer receives a raw `ID3D12Resource*`; the owning `shared_ptr` is a local of the caller and the engine can retire the proxy). Section 2.1; fix and a forced-overlap test in section 6.
2. **`Present` is a wait the sheet's inventory does not list** (its risk 3 names it in one sentence, its section 2.4 lists `Drain`/`Complete` only). The swap chain is two buffers, `FLIP_DISCARD`, no waitable object, `Present(0,0)`. The pacing row cannot separate the `Present` call from the rest of `present_ms`, and the M5 verdict (`slots=1` against `slots=2`) cannot be read without that split. The real apport here is the counter and the protocol (sections 2.2, 3 W2, 6).
3. **Four open points of the sheet close statically (section 4).** The stencil clear value is never assigned in the game path, so the depth recreation of D5 never fires there; the lighting programmes are resolved once per process, so D10 fires once from an empty state; `Present` does not need `gpu_submissions`; and the descriptor-chunk estimate of the sheet is **too optimistic**: a draw's chunk comes back one frame later than its slot retires, so the peak is (slots + 1) frames of draws (4.4).
4. **The sheet's "no GPU data crosses a frame" (2.6) is true today up to the queue order** (the scene copy bound by the selection is read by menu views rendered before the main view, ordered by the queue) **and false by design for the next passes.** The AO history, glare, after-image, depth-of-field table, shadow cascade caches and the exposure readback are persistent cross-frame resources in the I-0068 designs. The queue order keeps the GPU side valid; the CPU-side obligations are written as rules R-T1 to R-T5 (5.2).
5. **The CPU monotonic guards of the game path depend on where the frame is considered complete** (section 2.4): they are written at the end of the recording functions today; they must stay there.
6. **Commits (section 6):** nothing is added to the commit order; the complement amends cD0, cD1, cD3, cD4, cD6, cD7 and cD8, and adds tests T-D10 to T-D13 and witnesses RD10 to RD12. The amendments are also routed from the phase D sheet (last section there) and from `STATE.md`.

## 1. State and method

- **Pilot:** cD0 (commit `6b2da3c5`) adds `WaitSite` ids and a `GpuAudit` of atomics (drains and completes by site: other, main frame, offscreen, movie, depth recreate, movie planes, startup, resize, close, fixture; resources and heaps created; depth recreations; lighting programme changes; dynamic stream updates and bytes). It does not count the `Present` call separately, nor creation time; `DeviceQueues::Close`'s own `Drain` is still under `WaitOther`; `depth_recreations` is incremented only in the stencil branch (section 4.1).
- **Method:** for each persistent object of `SceneRenderer`/`DeviceQueues`/`TextureIngress`, who writes it on the CPU, who records it, what state it has at the frame boundary, and who owns it while a list that references it is in flight; then every blocking call on the owner thread in steady state.
- **Premise that holds:** everything is on one DIRECT queue, so GPU-GPU ordering between consecutive frames is the queue order; what can still go wrong at two slots is CPU-side: memory written while a pending list will read it, objects released while a pending list references them, descriptors rewritten, allocators reset, results read before completion, and CPU guards that assume completion. The sheet covers most of these; section 2 lists what it does not.

## 2. Gaps in the sheet

### 2.1 GPU-referenced objects not kept by the slot (G-1, G-2)

Only some frame-input members are GPU-referenced: `lighting->env_regions[i].cube.resource` (a `ComPtr`, `model_draw.h:28`) is; `velocity`, `point_batches`, `light_acc`/`light`/`fog` and `NativeMovieFrame` are CPU data copied into the arena or the upload buffer at record time (`scene_pass.cpp:1262-1265`, `:1481`) and need no keeping.

| GPU-referenced object | Owner today | Why it is safe today | Co-owners that already exist | Needed at two slots |
|---|---|---|---|---|
| environment-cube resources of `lighting` (sampled through `CopyDescriptorsSimple` at `scene_pass.cpp:1144` and the table at `:1155`) | `lighting` inside the local `model_frame` of `Clear()` (`game_bootstrap.cpp:724`), destroyed at its end; the renderer keeps only the draws (`scene_pass.cpp:1364, :1571`); plus the texture-record map until the engine retires the record (area change) | `Drain` before `Clear()` returns | none for the cube | keep the cube resources (or the whole frame input) in the slot |
| the movie target (`textures.Acquire(frame->proxy)` is a local `shared_ptr<const NativeTextureRecord>`, `RenderNativeMovieFrame`, `game_bootstrap.cpp:7461-7470`; the renderer gets `target->resource.Get()`) | the record map until `textures.Retire(output->proxy)` at movie close (`:7287`; `Retire` is a plain `erase`, `game_texture_ingress.cpp:226-228`) | `Complete` + `Drain` inside `RenderNativeMovie` (`scene_pass.cpp:1522`) | the UI build's `PrimitivePass` keeps the resource in `textures_` (a `ComPtr`, `primitive_pass.cpp:178-182`) when the packet samples it, and `RetirePrimitives` retires the pass by ticket | keep the resource in the slot |
| each menu view target and its frame input (`RenderNativeOffscreenViews`, `game_bootstrap.cpp:7039-7073`: `target` and `frame` are loop locals; `RenderNativeOffscreen(*frame, target->resource.Get(), ...)`) | the record map | `Complete` + `Drain` per view (`scene_pass.cpp:1409`) | same UI-build co-owner when sampled | same |
| `TargetSet` resources of a view (`offscreen_sets_`, persistent per extent) | the map; replaced only on a generation change (D4) | `Drain` before replacement | | as D4 |

**Hazard (STATIC ownership, INFERRED trigger):** once cD3 removes the per-view and per-movie waits, a movie close or an area change that retires the proxy releases a resource the GPU still writes, **when the UI packet of that frame does not sample it** (the first movie frame, a page closed during the frame); when the UI samples it, the UI build's `ComPtr` keeps it alive until the ticket. The environment cube has no such co-owner.
**Fix (cD1 and cD3), minimal form:** (a) the slot's existing `kept` (`shared_ptr<void>`) receives a `Held(ComPtr<ID3D12Resource>)` of the movie target, of each view target and of each environment cube, and the frame input's `shared_ptr` itself where simpler (no new field is required); the descriptor is consumed at record time (RTV, `CopyDescriptorsSimple`), so the record need not be kept; (b) debug assertion in `End`: every resource passed to a `Render*` call is in `kept`. Test T-D10 (with a frame where the UI does **not** sample the target) and T-D11 as an assertion test.

### 2.2 `Present` and the swap chain are missing from the wait inventory (G-3)

Section 2.4 of the sheet lists `Drain`/`Complete` only; its risk 3 says in one sentence that the CPU can wait inside `Present`. The swap chain is two `FLIP_DISCARD` buffers, no waitable object, no maximum frame latency set (`FramePresenter::Open`, `backend.cpp:515-520`), `Present(0,0)` (`scene_pass.cpp:1654`, `:1684`, `:2113`). Two mechanisms are possible and the code cannot tell them apart (INFERRED, not measured): (a) the CPU blocks inside `Present` once DXGI's queue of presents is full (default maximum frame latency 3, which two slots do not reach); (b) the wait for a back buffer still held by the compositor is taken **on the GPU queue**, so it shows up as the next slot's wait (`slot_wait_ms`, W1) or, today, inside the `Drain`. In windowed flip mode without a tearing flag the compositor decides when buffers are released, so the effective ceiling is the display refresh (INFERRED), invisible at 14 frames/s and decisive once frames get short. The receipt's `present_ms` is not just the call: it spans the end of the `Complete` to the end of the frame (`scene_pass.cpp:1634-1658`), i.e. the `Present` call, the `Drain`, `CollectCompleted`, `Diagnostics()`, the sampled velocity readback (`ReadNativeVelocity`, one frame in 30) and the sampled back-buffer readback with its hash and file write (one frame in 120).
**Fix:** add `present_call_ms` (the call alone), `slot_wait_ms` and, for the witness, DXGI frame statistics (`GetFrameStatistics`) or GPU timestamps around the list; M5 is read from these, not from `present_ms` (section 6).

### 2.3 Wait sites that exist but are not counted (G-4)

Beyond section 3: the `Log` mutex with file I/O (rows from `Clear()` and from engine threads); committed-resource creation per UI admission (`PrimitiveDepthTarget`, `scene_pass.cpp:1794-1803`: a full-resolution D32S8 texture, and the build's heaps, `primitive_pass.cpp:141-171`; these are waits inside the driver, not just creations); the **release** of those objects on the owner thread in `CollectCompleted` (`backend.cpp:307-315`: COM releases of the previous frame's UI depth and heaps every frame); short CPU locks with possible contention (`TextureIngress::mutex_` in `Acquire`, `ingress_lock_`, `retire_lock_`, `caches.lock`, `native_offscreen_lock`, the scene lock taken before `gpu_submissions`); the Bink decoder wait `BinkDoFrameAsyncWait(bink,1000)` (up to 1 ms of CPU in the movie player's render step, `game_bootstrap.cpp:7303`, not a GPU wait and unchanged by phase D). cD0 counts creations; it should also time creations and releases (`create_ms`, `release_ms`). For completeness: `AdmitNativeStaticBuffer` performs no `Complete` (a plain ingress `Commit`).

### 2.4 Completion-dependent CPU guards of the game path (G-5)

Game-path admission guards (STATIC): `RenderNativeModels` refuses `input.serial<=max(startup_serial_,last_serial_)` (`scene_pass.cpp:1537`) and writes `last_serial_=input.serial` at its end (`:1659`, after `Present` and `Drain`); the startup path refuses `serial<=startup_serial_` (`:1663`) and writes it after its frame (`:1687`); `AdmitPrimitiveWork` reads `scene_ready_` (false in the game path, which `RenderNativeModels` requires) and therefore compares `input.serial<=startup_serial_` (`:1814`), not `last_serial_`. (`:1859` guards `RenderFrame`, the work-mode path of D9, and `incarnation_` matters only when `scene_ready_` is true; `startup_serial_=max(startup_serial_,last_serial_)` at `:1790` is also work mode.) The only guard really placed after a `Complete` in the game path is `primitive_stencil_initialized_` (`:1635`, `:1683`), already treated by A7.
**Rule:** admission guards (`last_serial_`, `startup_serial_`, the primitive generations) are written at the end of the *recording* function, not at slot retirement; completion-dependent values (`pixel_hash`, `debug_errors`, the completed serial of the receipt) live in the retiring slot's `CompletedFrame`. T-D13 targets `RenderNativeModels` and the startup frame.

### 2.5 Startup frames are the UI-heavy ones (G-6)

When `model_frame` is null (title, menus, loading, Bink), `Clear()` takes `RenderStartupPrimitives`/`RenderStartup` (`game_bootstrap.cpp:731-740`), which end in `Execute`, `Complete`, `Present`, `Drain` (`scene_pass.cpp:1682-1685`). The sheet routes them through the slot path at cD1 and RD4/RD5 already run UI screens; what is missing is the explicit statement that cD6 flips them as well and the comparison `slots=1` against `slots=2` with `WaitStartup` at zero (RD11).

### 2.6 Text of the sheet that the code or the R13 sheet supersedes (G-7)

(a) Risk 2 and D5 (depth recreation on `clear_stencil` change): closed by 4.1. (b) Risk 5 "whether `Present` may move outside `gpu_submissions`": answered in 4.3. (c) R8, section 3.2 step 5, G4 and the B5 allocation point are already amended by the [R13/R14 sheet](D3D12-R13-R14-RENDER-THREAD-PACING-SHEET.md) for the threaded build and stand for the single-thread phase D. (d) Section 2.6 ("none is read across frames today") needs the forward note of 5.2. (e) A3's "peak pool sizes double" is too low (4.4). (f) The sheet's line numbers are those of `5be70746`; after the K merge use the functions, not the numbers.

## 3. The waits that remain at two slots (steady state, game path, owner thread)

"Designed" means the sheet already has it. The pilot's `WaitSite` ids are given where they exist.

| Id | Wait | Blocks | Bound | Frequency | Sheet | Counter |
|---|---|---|---|---|---|---|
| W1 | slot wait in `FrameRing::Begin` (ticket of frame N-2) | owner thread, before `gpu_submissions` | GPU time of one frame | every frame; near 0 if the GPU is faster than the CPU frame | designed | `slot_wait_ms` |
| W2 | `Present` call, or the buffer wait taken on the queue (mechanisms in 2.2) | owner thread, inside `Clear()` (and under `gpu_submissions` today) | DXGI latency, compositor | every frame | one sentence only | add `present_call_ms`; `GetFrameStatistics` |
| W3 | creation and release of GPU objects on the frame path: UI admission depth and heaps (P1, P2), first use of arena/constant/global chunks and of pipelines, first lighting frame (PSO creation) | owner thread | driver/OS | warm-up; per frame for P1, P2 until cD3 | partly (P1 to P3, A2) | `resources_created`, `heaps_created` (cD0) plus `create_ms`, `release_ms` |
| W4 | `Complete` + `Drain` per world frame | owner thread | GPU time | every frame until cD6 | designed | `WaitMainFrame` |
| W5 | `Complete` + `Drain` per movie frame and per menu view | owner thread | GPU time of the small pass | per movie frame, per view | designed (cD3) | `WaitMovie`, `WaitOffscreen` |
| W6 | `Complete` + `Drain` of startup frames | owner thread | GPU time | every title/menu/loading frame until cD6 | designed | `WaitStartup` |
| W7 | depth recreation `Drain` | owner thread | GPU time | **never in the game path** (4.1) | designed | `WaitDepthRecreate` (reads 0) |
| W8 | movie plane recreation `Drain` | owner thread | GPU time | first movie frame, size change | designed | `WaitMoviePlanes` |
| W9 | `gpu_submissions` held by `Clear()` during W2 (and W4 to W6 today; W1 is designed outside it) | loaders and close paths that take the lock | one frame | other holders are rare (4.3) | R5 | contention counter |
| W10 | `Log` mutex and file I/O; the short CPU locks of 2.3 | any thread | disk, contention | sampled rows | no | none |
| W11 | Bink async decode wait | owner thread, movie render step | 1 ms | per movie frame | no | none |
| W12 | `Close`, `CloseNativeModelStreams`, `TextureIngress::Close` (`idle_` wait, `SubmitIngress`, `Drain`); `DeviceQueues::Close`'s `Drain` is still `WaitOther` in cD0 | owner thread | GPU, loader admissions | shutdown | cD10 | `WaitClose` (to be used by `DeviceQueues::Close`) |
| W13 | `Resize` / display-mode change | owner thread | GPU | **no caller in the game path** (`Resize` is reached only through the older `Service`, `native_service.cpp:138`; `game_bootstrap.cpp` has none) | cD10 | `WaitResize` |
| W14 | debug layer serialisation | everything | | debug runs only | risk 4 | judge timing with `no-debug-layer.flag` |

After cD6 the steady state is W1 + W2 + W3 (warm-up, then the UI admission until cD3 removes it) and whatever the frame itself costs; W4 to W6 must read zero in `drains_by_site` for the game path, and W7 must read zero always.

## 4. Open points of the sheet answered from the code

### 4.1 Depth recreation on stencil change (risk 2, D5) — closed

`NativeModelFrameInput::clear_stencil` defaults to 0 (`model_frame.h:65`) and **nothing in the game path assigns it** (an exhaustive search under `native/` finds only that default and the work-mode `SceneFrameInput`, `scene_pass.h:98`; `game_ui_session.cpp` touches `clear_stencil_before`, a UI field). `depth_clear_stencil_` therefore stays 0 and the `Drain` + `DepthTarget` of `:1362` (offscreen) and `:1566` (main) never fires. The depth target is created with the targets (`NativeModelTargets()` to `DepthTarget(...,0)`, `:653`, `:817`) and again only on an extent change (not wired, W13). The work-mode branch at `:1886-1889` belongs to D9 and is out of scope.
**Action:** `gpu_audit.depth_recreations` (incremented by the pilot only in these two branches) must read **0 in the game path**; the initial creations appear in `resources_created`. Replace the two game-path branches by an assertion (or a guarded `WaitIdle`) that the value is unchanged.

### 4.2 Lighting programme change (D10) — closed

`AcquireNativeLightingPrograms` (`game_bootstrap.cpp:2817-2847`) holds a function-static `resolved` assigned once and never reset, and returns an empty pointer until all its programmes are found. `native_light_programs_` is written only in `NativeLightingResources` (`scene_pass.cpp:960`) and reset neither by `Close` nor by `SwapTargetSet`. `native_light_programs_ != input.programs` is therefore true **once per process**, from an empty state: the "release the old objects at once" code releases nothing in flight, and `lighting_programme_changes` should read 1. What remains is a one-time pipeline-creation hitch (W3), not an overlap hazard. (A device removal or a programme-registry reload that clears `resolved` would reopen it; none exists in the game path.)

### 4.3 `Present` outside `gpu_submissions` (risk 5) — answered

`gpu_submissions` is a C++ mutex local to the module. Its holders are `Clear()` (the whole frame, `game_bootstrap.cpp:720`), the startup block (`:9553`), `ReadStartupView` (`:9632`), the close paths (`:878-883`), `CloseNativeModelStreams` (`:1732`), the UI session constructor (`:9719`, which reaches only `game_ui_image_owner.cpp:397`, a feature check) and `TextureIngress::Close` (`game_texture_ingress.cpp:231`; the admissions no longer hold it). Nothing depends on `Present` being inside the lock, and the engine's order is unchanged because `Clear()` still runs inside the same hook. **Action (cD4):** end the lock scope after the last CPU-structure access (before `Execute`), so that W1 and W2 never hold it.

### 4.4 Descriptor and constant chunks at two and three slots (risk 5, A3, A4) — derived, and the sheet's estimate corrected

A model draw takes `extent[VS] + extent[PS]` descriptors from a chunk, where an extent is `max(highest bound CBV register + 1, 14 + highest SRV register + 1)` per stage (`model_draw.cpp`, global-heap branch; `cbv_count` is 14). The PS side reaches `t30` (IBL) and `t26` (lightmap), so a draw is about 30 to 60 descriptors. For the 1,062 draws of S1 that is about 32 to 64 K descriptors, i.e. **4 to 8 of the 32 chunks of 8,192 per frame** (12 to 25 % of the heap).
**The lag:** a `GlobalChunk` returns through `RetireWhenSubmittedWorkCompletes` when its last holder dies (`model_draw.cpp:176-179`), keyed on `LastSubmitted` *at that moment*. `RetireSlot(N)` frees the draws of frame N at the start of frame N+2, when `LastSubmitted` already is the ticket of N+1; the chunk therefore returns when **N+1** completes, one frame later than the slot's own ticket. The peak is about (slots + 1) frames of draws, plus the "current" chunk that straddles two frames: two slots 12 to 24 (+1) chunks (up to about 78 % of 32), three slots 16 to 32 (+1), i.e. possible exhaustion and a silent fall back to per-draw tables (`legacy_table_draws`, A4). The same lag applies to the 8 MB `ConstantChunk`s (A3's "double" is closer to "triple"). The sheet's conclusion that the counts are enough is therefore only safe at two slots and the current scenes.
**Fix (cD1, cD6):** either let `RetireSlot` release the draws through a path that returns chunks at once (the ticket is already known to be complete there: a "released after completion" flag consulted by the chunk deleters), or size the pools for slots + 1 frames and gate on `global_chunks_in_use_peak` (cD0) at most half the capacity in the heaviest witness. The shadow cascades and post families multiply the draws; the persistent SRV tables of [PREPARE-REDUCTION cP8 option B](PREPARE-REDUCTION-IMPLEMENTATION-SHEET.md) are the structural answer. The sampler heap is append-only and content-keyed (128 tables), independent of the slot count.

### 4.5 Which engine threads call the stream hooks (open point 5) — partly

The original's cloth upload `FUN_1423D5BC0` takes a deferred context from a pool and retries with a yield until one is available ([MENUS-MAP](../I-0067-static-menus-subsystems-map/MENUS-MAP.md), section on cloth, step 3), i.e. it is designed for **several concurrent callers on engine worker threads** (INFERRED); the step and thread are not identified. The native hook is thread-agnostic (it publishes versions under `cloth_stream_lock`). The B5 design (versions, no GPU object from engine threads) already assumes any thread; the open point stays OPEN and costs nothing to the design.

### 4.6 Still live-only

The buffer state rule of B5 under the debug layer, the number of dynamic streams per frame, the attribution of `Diagnostics()` messages to a frame, and the Present mechanism of 2.2 stay for the runs (RB5-1, RB5-2, cD0 counters, RD12).

## 5. Shared objects across frames in flight

### 5.1 Today (STATIC): by class

| Class | Objects | Frame-boundary convention | CPU-side hazard at two slots | Status |
|---|---|---|---|---|
| G-buffer, depth, lighting, environment, SSS targets of the main view and of each `TargetSet` | `colors_[6]`, `depth_`, `native_light_targets_[4]`, `native_env_target_`, `native_sss_temp_`, `lighting_colors_` | every pass returns them to `RENDER_TARGET`/`DEPTH_WRITE` (`EncodeNativeLighting` end, `:1120-1127`; offscreen end, `:1406`); serial queue order keeps it exact | resource replacement on a generation change (D4); views per set | covered (D4, cD2) |
| Velocity and motion vectors | `velocity_target_`, `motion_vectors_` | render target at frame start | readbacks (G2); replacement (D4) | covered |
| Scene colour copy | `scene_colour_copy_` | shader-readable between frames (`:1331`, `:1591-1593`); copy destination only inside the forward pass. The selection binds it at slot 45 (`game_bootstrap.cpp:3985, :4082`), so **menu views rendered before the main view read the previous frame's copy**: valid by queue order, but a real cross-frame GPU read | replacement on an extent change (D4: "relies on its holders") | covered; the note corrects 2.6 |
| Back buffers | `presenter_.buffers[2]`, RTV heap | `PRESENT` to `RENDER_TARGET` and back | `Present` blocking (2.2); `ResizeBuffers` needs every reference released (no game-path caller) | **G-3 new** |
| Menu view targets, movie target and planes | ingress textures, `movie_planes_[3]`, `movie_upload_` | shader-readable between uses; written then sampled inside one list once cD3 lands | ownership of the target resources (2.1); the single UPLOAD buffer (D2) | **2.1 new**; D2 covered |
| Upload memory | arena chunks, constant chunks, UI chunks, `movie_upload_` | written before the list that reads it | L3, A1 to A3; chunk lag (4.4); shared constant ranges of the prepare reduction come from the constant pool, not the arena | covered, A3 corrected |
| Shader-visible descriptors | `native_light_views_` (env cubes, D1), global CBV/SRV chunks (A4), sampler heap, `bindings_` and `velocity_views_` (generation-gated writes), `movie_views_` (rewritten when the planes are recreated, `:1467`, after W8), UI pass heaps (per admission) | | D1 per slot and per view; chunk exhaustion (4.4) | covered |
| Pipelines, root signatures, static blocks | model PSOs (device lifetime), light/SSS/env PSOs (once, 4.2), `native_light_quad_`, `native_light_list_`, `native_light_zeros_`, `native_env_box_` | | none after 4.2 | closed |
| GPU-referenced frame objects | section 2.1 | | G-1, G-2 | **new** |
| CPU guards | section 2.4 | | G-5 | **new** |

### 5.2 Tomorrow: temporal resources of the coming passes

The I-0068 designs make these persistent cross-frame resources: `AoHistory[2]` ([SSAO-D3D12-DESIGN](../I-0068-static-yebis-ffx-lights-spec/SSAO-D3D12-DESIGN-NATIVE.md) section 4, "ping-pong by frame parity"), `AfterHist`, `GlareFinal`, `DofTable`, the exposure readback ring and the shadow cascade caches ([YEBIS-D3D12-DESIGN](../I-0068-static-yebis-ffx-lights-spec/YEBIS-D3D12-DESIGN-NATIVE.md) sections 3, 4 and 5, [master plan](../I-0068-static-yebis-ffx-lights-spec/D3D12-MASTER-PLAN.md) risk 2). They state that frames in flight on one queue keep the GPU side valid. The CPU-side rules phase D must provide before the first of them lands (R-T1 **amends** the designs' "parity by frame": it redefines parity as the parity of the passes actually recorded):

- **R-T1 (parity follows recording):** the ping-pong `cur/prev` index advances when a frame *records* the pass, not when it completes and not with the frame number; a frame that skips the pass (gate off, lighting absent) neither advances it nor marks the history valid.
- **R-T2 (reset is data of the frame):** a history invalidation (resize, teleport, camera cut, load) travels in the frame's input and is applied in recording order; it is never a renderer flag read at a different time from the frame that set it. (The SSAO design's corrected form, "no explicit reset; validity byte at record+0x60", is compatible if that byte and the previous-frame object are the data that travel.)
- **R-T3 (per-slot CPU data):** previous-frame matrices and blend factors (`g_mCurToPrevScreen`, `AoView`) are carried in the frame input (a value), not read from a member the next frame overwrites.
- **R-T4 (readback rings per slot):** an exposure or statistics readback is a per-slot buffer read at that slot's retirement (the G1/G2 pattern), never read at record time.
- **R-T5 (replacement):** a persistent temporal resource replaced on a resize goes through the ring-wide `WaitIdle`, `RetireAfter` and a history invalidation (R-T2), tested at the five zones (YB6, SS7).

## 6. Amendments to the commit order, tests and witnesses

| Commit | Amendment |
|---|---|
| cD0 follow-up | add `present_call_ms`, `create_ms`, `release_ms`; `depth_recreations` reads 0 in the game path (4.1); `DeviceQueues::Close`'s `Drain` under `WaitClose`; `slot_wait_ms` and `frame_inputs_held_peak` once the ring exists (cD1) |
| cD1 | `kept` receives the GPU-referenced frame objects of 2.1 (movie target, view targets, environment cubes) and the debug assertion; pool release path or sizing for slots + 1 frames (4.4) |
| cD3 | the single list keeps the same objects in the slot; the entry points are checked, not necessarily re-typed |
| cD4 | admission guards stay at record time (2.4); lock scope ends before `Execute` (4.3); `CompletedFrame` carries the completed serial |
| cD6 | flips the startup paths too (2.5); descriptor/constant chunk gate (4.4) |
| cD7/M5 | **the verdict is provisional until RD12, which needs the buffer count of cD8 (R14a):** to avoid reordering the commits, cD7 may add a temporary buffer-count flag (a test setting of the swap chain's `BufferCount`); the protocol reports `slot_wait_ms`, `present_call_ms`, frame percentiles and DXGI frame statistics separately |
| cD8 | the temporal rules R-T1 to R-T5 become acceptance criteria for the first temporal pass |

| Test | Content |
|---|---|
| T-D10 | Lifetime under retirement with the forced overlap of T-D1: record a frame that draws into a movie target and a view target **that the UI packet of that frame does not sample**, and samples an environment cube; retire the proxy and the cube record (as `Retire` does at movie close and area change) before the slot ticket; no resource is destroyed before the ticket (debug layer 0 errors, weak pointers expire exactly at retirement). Includes a run with the pre-fix ownership (locals) to show the test can see the hazard. |
| T-D11 | Assertion test: every resource passed to `RenderNativeMovie`/`RenderNativeOffscreen` and every environment cube of the frame is in the slot's `kept` at `End`. |
| T-D12 | Temporal parity under overlap with a fake pass: a ping-pong written by frame N and read by frame N+1 with the queue wait of T-D1 gives the right data; a skipped frame leaves parity and validity unchanged (R-T1); an invalidation applies to the frame that carried it (R-T2). |
| T-D13 | Guards: with deferred completion, `RenderNativeModels` for frame N+1 is checked against frame N's serial and `AdmitPrimitiveWork` against `startup_serial_` as today; the startup frame likewise. |
| T-D14 | Chunk lag: with draws released from `RetireSlot`, the chunks return at the slot's own ticket (or the pool is sized for slots + 1 frames) and `legacy_table_draws` stays 0 over 600 frames at three slots. |

| Row | Action | Expected |
|---|---|---|
| RD10 | movie close and area change with two frames in flight, debug layer on; a frame where the movie target is drawn but not sampled | 0 errors; no use after release; `retirements_pending` returns to baseline |
| RD11 | title, Load Game, Status with portraits (startup and UI-heavy paths) at `slots=1` against `slots=2` | `WaitStartup` falls to 0; image equal; frame percentiles reported |
| RD12 | M5 with the temporary buffer count (cD7) and later the waitable object (cD8); DXGI frame statistics or GPU timestamps | the verdict on the gain of two slots is read from `slot_wait_ms`, `present_call_ms` and the statistics, and settles which of the two mechanisms of 2.2 applies |

## 7. Risks and what stays open

1. The inventory is static: a blocking call hidden in a library the frame path calls (the driver, DXGI, the Bink decoder, Scaleform on the engine side) is not visible here; `present_call_ms`, `create_ms`, `release_ms` and the audit counters are the net.
2. The Present-blocking account (2.2) is INFERRED from DXGI's documented behaviour; RD12 settles it.
3. The temporal rules are designed for passes that do not exist yet; they are cheap now and expensive after the first pass lands.
4. **OPEN:** whether the window's thread is the owner thread (the `native_window` row logs both ids); the engine threads of the stream hooks (4.5); the buffer state pattern of B5; the dynamic stream counts.

## 8. Evidence and limits

Sources: `native/renderer/{backend.h,backend.cpp,scene_pass.h,scene_pass.cpp,model_draw.h,model_draw.cpp,model_frame.h,primitive_pass.cpp}`, `native/frame/{game_bootstrap.cpp,game_texture_ingress.h,game_texture_ingress.cpp}` at `main` `933a9431`; the pilot's `6b2da3c5` diff of `backend.h` and the scene-pass counters; documents: the phase D sheet, the R13/R14 sheet, [PREPARE-REDUCTION-IMPLEMENTATION-SHEET](PREPARE-REDUCTION-IMPLEMENTATION-SHEET.md), MENUS-MAP (cloth upload), SSAO-D3D12-DESIGN, YEBIS-D3D12-DESIGN, D3D12-MASTER-PLAN. No build, run or measurement; the descriptor arithmetic and the DXGI behaviour are derived or inferred as labelled.
