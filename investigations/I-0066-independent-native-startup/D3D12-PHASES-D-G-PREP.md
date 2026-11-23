# D3D12 review, phases D to G: preparation of the remaining reprises (static, 2026-10-11)

Request (coordinator, 2026-10-11): prepare phases D to G of [D3D12-DESIGN-REVIEW](D3D12-DESIGN-REVIEW.md) (R1 frames in flight and the rest up to R16), taking into account the facts found in [phases A to C](D3D12-PHASES-A-C-PREP.md). Same format: native files and functions touched, dependencies, safest commit order, what to measure before and after, live witnesses. No native code was changed, nothing was launched. Written by the static thread (I-0068); filed under I-0066 beside the review. The coordinator accepted the phase B exit check "no `Drain` in the ingress, retirement and stream paths" (2026-10-11); it is recorded in both notes and in the review's section 6.

Phases: **D** R1, R12, R14, shutdown order (M14); **E** R10, R11; **F** R13, R8b; **G** R16, root CBVs (R7c), pipeline library (R9d), other beyond-ER options.

Labels: **observed** (native source at `bbe48c5c`/`5c12c7a6`, line numbers are that tree's), **review**, **proposed**, **open**.

## 0. Answer first

1. **Order**: D1 frame slots (allocator, readbacks, per-slot arenas) behind a switch with 1 slot = today; D2 explicit target sets and per-view contexts (R12 prerequisite, no behaviour change); D3 menu views, Bink and UI admission as passes of the frame list (R12); D4 flip to 2 frames in flight, the per-frame `Complete`/`Drain` replaced by tokens (R1); D5 swap chain: 3 buffers, waitable object, tearing and the original's present interval (R14); D6 shutdown and resize order (M14). Then E (graph and aliasing), F (worker thread, then the engine-side publication), G (only what the measurements justify). Details in section 4.
2. **Six facts found in the code that shape D to G** (none contradicts the review):
   - **One thread does collect, prepare, record and present.** The selection ("collect") runs inside the engine's scene update (`CollectNativeSelectedScene`, `game_bootstrap.cpp:5507`); `Clear()` (the `EndFrame` hook, `:7327`) then prepares, records and presents on the same owner thread. A second engine thread, the *menu thread* (`native_menu_thread`, `:6138`), selects the menu offscreen scenes. R13's natural cut is therefore *after* prepare, not before.
   - **Prepare still reads live engine memory** at eight sites per draw (`game_bootstrap.cpp:4946-4947`, `:5001-5002`, `:5009-5010`, `:5019`, `:5047`; among them the wind block `model.animation+0x210`). Until R8b moves those reads into collect-time snapshots, prepare cannot leave the owner thread.
   - **The offscreen views are not reentrant.** `RenderNativeOffscreen` swaps sixteen renderer members with a per-extent `TargetSet` (`SwapTargetSet`, `scene_pass.cpp:1344-1352`, struct `scene_pass.h:348`), reuses the one allocator and list (`recorder_`), and drains after each view (`:1419`). Frames in flight need the target set as an explicit argument first.
   - **`FramePlan` cannot yet drive barriers.** It knows resources by identity and generation and passes by `Read/Write/ReadWrite` use (`frame_plan.h`, caps 128 passes and 16,384 resources, one trailing `Present`), and is used only by the fixtures (`scene_pass.cpp:1909-1912`). The in-game frame has 84 hand-written `Transition` calls (`scene_pass.cpp`, helper `:66`). R10 starts by extending the plan with states and subresources.
   - **The presenter is the simplest D3D12 flip chain**: two `R8G8B8A8` buffers, `FLIP_DISCARD`, flags 0, `Present(0,0)` (`backend.cpp:390-411`, `scene_pass.cpp:1664`), the back buffers held in a `std::array<ComPtr,2>`, and the startup path checks `BufferCount==buffers.size()` (`scene_pass.cpp:1705`). The original passes the interval chosen by the engine and runs a gamma pass and a mode state machine (I-0068 DISPLAY-SWAPCHAIN-NATIVE §3 to §5): R14 should be done with that specification, not as a buffer-count change.
   - **The frame's own receipt is a completion contract.** `Clear()` throws unless `receipt.fence`, `receipt.presentation_fence` are non-zero and `debug_errors` is 0 at the end of every frame (`game_bootstrap.cpp:644`); with frames in flight these become tokens and the diagnostics check moves to token completion.
3. **The A to C facts carry over**: the global `gpu_submissions` mutex must disappear from the frame path before R1 gains anything (it is held for the whole frame, `:621`); the pools return by refcount and R2/R3 turn that into tokens; the Resource Binding Tier read in phase A decides how R6 and R11 may use unwritten slots and heap mixing; the phase B exit check does not cover the per-frame waits, which phase D removes.
4. **What would invalidate this plan** is measured, not argued: if phase A shows the CPU is the limit (prepare 32 ms, collect 41 ms, review figures) and the GPU main list takes far less, R1 gains up to the GPU wait only (about 20 ms of 100 in the load-in window) and R13/R8b carry the real gain. Phase D therefore keeps its flip switch (section 2.1) so the gain is measured before E and F are started.

## 1. What D to G assume from A to C (and what to check first)

| Assumption | From | Check before starting D |
|---|---|---|
| `DeviceQueues::RetireAfter/CollectCompleted` exist and every retirement path uses them | R3 | no `Drain` in storage, stream, texture, scene retirement (counter `drains` in the pacing row) |
| An `UploadArena` with token return serves constants, UI, point lights, movie, streams, staging; the per-frame in-place buffers (lighting, environment, SSS, velocity constants, point ring) are arena allocations or per-slot arrays | R2 | the list of in-place buffers in A to C section 1 is empty |
| Ingress has its own copy context and ticket; textures become resident asynchronously | R5 | `missing_textures` has no persistent gap; the lock order is fixed (`gpu_submissions` then the copy context's lock) |
| Geometry is in DEFAULT memory, dynamic streams updated by a recorded copy at the start of the frame list | R4 | one buffer per stream; ring of per-slot buffers is a D1 task (see 2.1) |
| Global frame-linear descriptor heaps, used registers only, stable samplers | R6 | `heap_switches` per frame small; the Tier is known |
| Persistent draws with version checks, constants from the arena | R8a | `persistent_draw_rebuilds` near 0 in an idle frame |

If any row is not true, the corresponding D to G slice waits; none of them should be absorbed silently into phase D.

## 2. The reprises

Each card: **touches**, **depends on / enables**, **slices**, **pitfalls found in the code**.

### 2.1 R1 - frames in flight (phase D)

- **Touches**: `renderer/backend.h/.cpp` (`DeviceQueues`: tickets, `Complete`, `Drain`, `Close`; `FrameRecorder` single allocator and list `:377-381`; `FramePresenter`); `scene_pass.cpp` `RenderNativeModels` (`:1545-1670`: reset of the one allocator `:1584`, `Execute`+`Complete` `:1644`, readback `:1647-1663`, `Present` and `Drain` `:1664-1665`, `native_model_inflight_` guard `:1550`), `RenderStartupFrame` (`:1672-1698`), the fixture `Render` (`:2084-2115`, left on its own path), `Resize` (`:2119`), `Close` and the destructor quarantine (`:2125-2191`); `game_bootstrap.cpp` `Clear()` (`:617-690`, the receipt checks `:644`) and the pacing witness (`:660-690`).
- **Depends on / enables**: A to C (section 1) / R12, R14, R10, R13.
- **Slices**:
  - **D1a frame slots, `slots=1`**: a `FrameSlot {allocator, list, ticket, arena cursor, retire list, readback buffers, in-flight draws}` ring in `DeviceQueues`/`SceneRenderer`; `slots=1` reproduces today exactly (wait for the slot's ticket before reuse = the present `Complete`+`Drain`). The readback buffers (`readback_`, `velocity_readback_`, `motion_readback_`, environment and specular readbacks) become per-slot; witness frames (hash, velocity picture) wait for their slot's token on request.
  - **D1b thread-safe submission**: `Execute` takes an internal mutex and assigns monotonic tokens, so the loader copy context, the frame and the worker (F1) can all submit; `gpu_submissions` shrinks to a lock around CPU-side shared structures and leaves the frame path. This is the step that finally removes the lock order hazard of A to C section 6.
  - **D1c per-slot dynamic state**: every buffer rewritten in place per frame (the list in A to C section 1) is checked to be arena- or slot-owned; the `native_model_inflight_` guard becomes "this slot's previous work has completed" (`CompletedValue >= slot ticket`), and `native_model_inflight_` becomes per slot.
  - **D1d flip to `slots=2`** (lag 2 as ER E2, review): the CPU waits only for the slot being reused; `Present` is followed by `Signal`; `Complete(receipt.fence)` and `Drain` leave the frame. The receipt fields stay non-zero tokens, `debug_errors` is read when a slot is retired.
  - **D1e switch** `frames-in-flight.flag` (1 or 2 slots): both settings run from one build so the gain is measured and the fallback is immediate.
- **Pitfalls**: (a) pooled constant chunks and table chunks return when their tokens complete, not when the last draw object dies (A to C 2.3, 2.6); a draw held by a slot is released at slot reuse. (b) `DestroyModelStorage`, stream and texture retirement use tokens of the *last frame that could reference them*, which now means the ring (up to 2 frames). (c) The scene colour copy (`scene_colour_copy_`, `scene_pass.cpp:1598-1604`) and the lit HDR target are written every frame; with 2 frames in flight they are used by one frame's forward pass while the next frame's G-buffer is recorded: same queue, same order, no hazard, but **cross-frame readers** (menu offscreen targets sampled by the UI of the same frame, the velocity previous-frame data) need explicit ordering in the list (D3, E). (d) `Diagnostics()` per frame becomes a per-token scan: with the debug layer on, a message belongs to the work submitted before it; report it with the frame it was found in. (e) `faulted_`/`throw` semantics: a device removal is discovered late (a completed value of `UINT64_MAX`, `backend.cpp:195`); `Clear()`'s catch path (`EndFrame` `ExitProcess(0xD3126607)`, `:7329-7333`) must stay the single exit.

### 2.2 R12 - one frame list (phase D)

- **Touches**: `scene_pass.cpp` `RenderNativeOffscreen` (`:1354-1431`, Execute+Complete+Drain `:1419`), `RenderNativeMovie` (`:1432-1543`, `:1532`), `AdmitPrimitiveWork` (`:1814-1836`, `Drain` `:1827`, `Complete` `:1830`), `SwapTargetSet` and `TargetSet` (`scene_pass.h:348-359`), `NativeModelTargets` (`:808`), `NativeLightingResources` (`:825`); `game_bootstrap.cpp` `Clear()` (`:617-640`: movie, offscreen views, UI admission, then the model frame) and `RenderNativeOffscreenViews` (`:6418`, which prepares each view's draws every frame and renders it).
- **Depends on / enables**: R1 (slots), R2 (arena for the movie planes and UI buffers), R5/R3 / R10, R11.
- **Slices**: D2 **explicit target sets**: the 16 swapped members become fields of a `ViewContext` passed to the encode functions (`EncodeNativeLighting`, `EncodeNativePointLights`, `EncodeNativeSss`, `EncodeNativeEnvRegions`, `EncodeNativeVelocity`, `NativeLightingResources`); `SwapTargetSet` is deleted. No behaviour change, test by the control suites and Status/portraits. D3a the **Bink frame** recorded as the first pass of the frame list (its output target is sampled by the UI later in the same list; the order is the list's order). D3b the **menu offscreen views** as passes before the main view, one `ViewContext` each, no per-view submission; the readback of `pixel_file` stays an on-request witness with a token wait. D3c the **UI admission**: today it drains, rebuilds the primitive packet's resources and submits its own upload list each frame (`primitives_.Clear()`, `Build`); it becomes an arena/ring build recorded into the same list, with the primitive packet's textures and buffers per slot.
- **Pitfalls**: the per-extent sets are kept until the device closes (review M7): the explicit contexts are the place to apply R11 later. The menu offscreen preparation runs on the menu thread (`native_menu_thread`) and the target is `textures.Acquire(s.proxy)` (a resident texture from ingress): with asynchronous residency (R5) a view is skipped until its target is resident, which the code already tolerates (`native_offscreen_view_target_absent`, `:6431`). The target texture is a `render_target` record sampled by the UI and written by the view: its state at the boundaries (`PIXEL|NON_PIXEL_SHADER_RESOURCE` outside, `RENDER_TARGET` inside) is the first thing R10 must own.

### 2.3 R14 - presentation (phase D)

- **Touches**: `backend.cpp` `FramePresenter::Open/Acquire/Resize` (`:382-411`, `BufferCount=2`, `ResizeBuffers(2,...)`), `backend.h` `buffers` (a 2-array), `scene_pass.cpp` the three `Present(0,0)` sites (`:1664`, `:1694`, `:2114`) and `ReadStartupView` (`:1700-1710`, the buffer-count check); `Resize` (`:2119`); the display state machine of DISPLAY-SWAPCHAIN-NATIVE (not yet native).
- **Depends on / enables**: R1 (the waitable object bounds the queue; it does not replace slots) / the display modes, the gamma pass, an upscaler hook.
- **Slices**: D5a buffer count and format from the display mode (inputs, D-0151), buffers in a vector; D5b `DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT` with `SetMaximumFrameLatency(2)`; the frame start waits on the handle (and the token of the slot); D5c `DXGI_FEATURE_PRESENT_ALLOW_TEARING` when the engine's interval is 0 and the display allows it (`ResizeBuffers` and `Present` flags kept consistent); D5d the **original's interval**: `Present(interval, 0)` with the interval the engine's pacing (`FUN_1400D5E30`, DISPLAY-SWAPCHAIN-NATIVE §3.3) provides, `DXGI_STATUS_OCCLUDED` accepted; D5e the gamma pass and full-screen mode changes belong to the display workstream and are only *enabled* by this slice (a back buffer in a format the pass can write).
- **Pitfalls**: `Present(0,0)` with no pacing is what the native frames (60 to 200 ms) hide today; the first fast frame will expose the absence of the engine's frame pacing (input latch note, review M12). Changing the buffer count touches `presenter_.buffers`, `Acquire`, the RTV heap (2 descriptors) and the check at `scene_pass.cpp:1705`. Exclusive full screen needs an interval or the waitable object (DISPLAY-SWAPCHAIN §5).

### 2.4 Shutdown and resize order (M14, phase D)

- **Touches**: `game_bootstrap.cpp` `Close()` (`:718-748`: UI sessions, SFX assets, stage textures, environment regions, selected scene, common images, sampler bank, simulation time, model and cloth streams, face images, then the storage `Drain` and reset, `textures.Close()`, `renderer.Close()`), `scene_pass.cpp` `Close` and `~SceneRenderer` (`:2125-2191`, the quarantine of everything on a failed wait), `Resize` (`:2119`), `DeviceQueues::Close` (`backend.cpp:205`, a `Drain`), `TextureIngress::Close` (`game_texture_ingress.cpp:238`).
- **Slices**: D6a one `WaitIdle()` on the last token (replaces the several `Drain`s, which stay as asserts in debug), then `CollectAll()` of the retirement list, then release in dependency order: draws and per-slot state, arena, descriptor heaps, geometry and textures, then the device; D6b `Resize` waits for all slots' tokens, recreates size-dependent targets from the new size, and the queue is idle only at that point; D6c the failed-wait quarantine (`~SceneRenderer`) is extended to the slot ring and the retirement list: a timeout must leak the in-flight objects, never release them.
- **Pitfalls**: the order "withdraw CPU consumers, then GPU" of `Close()` (CPU references released later by the engine) must stay; `CloseNativeModelStreams` and `CloseNativeClothStreams` currently drain: they must use the new `WaitIdle` once, not each. Witness: Native1262 (quit in game) and a quit during a load.

### 2.5 R10 - an in-game frame graph with states (phase E)

- **Touches**: `renderer/frame_plan.h/.cpp` (`ResourceUse` gains a required state and a subresource range; `FrameResource` gains an initial state; a `StateTracker` per resource; caps raised), `scene_pass.cpp` (the 84 `Transition` calls and the pass encoders: `RenderNativeModels`, `EncodeNativeLighting`, `EncodeNativePointLights`, `EncodeNativeEnvRegions`, `EncodeNativeSss`, `EncodeNativeVelocity`, `RenderNativeOffscreen`, `RenderNativeMovie`, `primitives_.Encode`), `primitive_pass.cpp` (its own transitions for uploads and textures), `game_texture_ingress.cpp` (target records outside the graph), the I-0068 family designs (SHADOWS-NATIVE §12, EFFECTS-D3D12-DESIGN-NATIVE §2.1).
- **Depends on / enables**: R12 (a single list per frame with named passes) / R11, and the new families (effects, shadows, YEBIS, SSAO) which then declare their uses instead of hand-writing barriers.
- **Slices**: E1 extend `FramePlan` (states per use, subresource ranges for cascades and mips, UAV and aliasing barrier kinds, `Lifetimes()` already computed); E2 **shadow mode**: the graph computes the barriers the frame needs, the hand-written transitions still run, and every difference is logged (`graph_barrier_mismatch` rows) for 600 frames per witness; E3 convert pass by pass (model G-buffer, lighting, forward, velocity, composition, UI, movie, offscreen) so that a pass's hand-written transitions are removed only after its shadow mode shows 0 mismatches; E4 batch barriers per group (ER E8); E5 the "left readable between frames" conventions (the lit HDR returns to `RENDER_TARGET` at the end of the frame, `scene_pass.cpp:1625`, `:1608`) become initial states of the next frame in the graph.
- **Pitfalls**: the plan insists on one trailing `Present` pass and a pass cap of 128: a frame of eight menu views plus the world plus effects can approach it (each view contributes about ten passes); raise the cap with the extension. The order of passes must remain the original's (NATIVE-PASS-ORDER); the graph may reorder only independent passes and E3 forbids that until the witnesses pass. Depth read-only plus SRV (effects design 2.3) needs a `DEPTH_READ|PIXEL_SHADER_RESOURCE` state in the plan's vocabulary.
- **Settled in part BD** ([D3D12-MASTER-PLAN](../I-0068-static-yebis-ffx-lights-spec/D3D12-MASTER-PLAN.md) §6 S2): the cap is a safety check of `CompileFramePlan` (O(P·D·P), microseconds); passes are groups and draws, tiles and regions are nodes carried by the group; the cap becomes 256 and `plan_passes_peak` records the real number; cross-frame history is an `initialized` input with its own identity, a ping-pong needs two identities, and a `PostProcess` kind (or `Composition`) carries the colour chain.

### 2.6 R11 - aliased transient targets (phase E)

- **Touches**: `scene_pass.cpp` `Targets` (`:640-712`: six G-buffer colours, depth, two lighting colours, resolved colour, encoded display source, shadow mask, environment accumulation, readbacks), `NativeLightingResources` (`:825-1030`: HDR and lighting targets, SSS temporary, env target), `NativeVelocityResources` (`:1218`), `SceneColourCopy` (`:1334`), the offscreen sets, the primitive depth target (`:1804`), `Heap(D3D12_HEAP_TYPE_DEFAULT)` helpers; all `CreateCommittedResource` calls listed by `grep` in `scene_pass.cpp` (about thirty).
- **Depends on / enables**: R10 (lifetimes and first-use discipline) / the memory room for the new families.
- **Slices**: E6 **measure first**: the committed bytes of every target set (`GetResourceAllocationInfo`) and the per-extent set count at the High Wall, a menu with portraits, and a character-creation screen; if the total is small against the adapter's memory, stop here. E7 a placed-resource arena for render targets and depth (needs `ResourceHeapTier`, see below) with lifetimes from the graph; E8 aliasing barriers and the first-use `DiscardResource` or clear for each aliased resource; E9 menu views share transient storage with the main view (their lifetimes are disjoint inside the frame list).
- **Pitfalls**: `ResourceHeapTier 1` forbids render targets and non-RT textures in one heap (query it in phase A together with the binding tier); aliasing errors are silent (review R11): every aliased resource needs an initial barrier and a first-use clear or discard, and the debug layer's GPU-based validation should be run once on the aliased build. The long-lived targets (previous-frame velocity, scene colour copy) are not aliased.

### 2.7 R13 - a render worker thread (phase F)

- **Touches**: `game_bootstrap.cpp` `Clear()` (split: collect and prepare stay on the owner thread, `RenderNativeMovieFrame`, `RenderNativeOffscreenViews` and `renderer.Render*` plus `Present` move to a worker), the `EndFrame` hook (`:7320-7335`: it sets the frame clock after `Clear()` returns, `:7328`), `RenderNativeOffscreenViews` (`:6418`, the menu-thread preparation), the UI admission (`native_ui_frame`, `:629-640`), the pacing witness, `Refuse`/fault propagation (a worker fault must reach the owner thread: `EndFrame`'s catch `ExitProcess(0xD3126607)`), `SceneRenderer` thread annotations.
- **Depends on / enables**: R1 (slots, thread-safe `Execute`), R12 (one list), R10 helpful / overlap of CPU preparation with recording (ER E1).
- **Slices**: F1 a bounded `FrameJob` queue of depth 1 (the owner thread blocks on the next publication until the worker has taken the previous job, ER K-0016); the job is the existing immutable `NativeModelFrameInput` plus the UI packet, the movie frame and the offscreen frames already prepared; F2 move `Render*` and `Present` to the worker with an explicit fault channel; F3 witness mode reads (hash, velocity) happen on the worker and are reported through the pacing row; F4 the owner-thread `Prepare` becomes optional once R8b has removed its engine reads.
- **Pitfalls**: the **eight engine reads in prepare** (section 0) and the menu thread's preparation must stay on their threads: only `Render*`/`Present` can move first, which already overlaps recording with the next frame's collect and prepare. The immutable job holds `shared_ptr`s to the scene and to draws; the engine may retire a model while a job is queued, which R3 tokens and the storage `shared_ptr`s cover as long as the job keeps its references until its slot retires. Order of `EndFrame`'s `clock+0x48` update relative to `Present` (the engine's frame time) must be preserved.

### 2.8 R8b - engine-side publication instead of a native scan (phase F)

- **Touches**: `game_bootstrap.cpp` `CollectNativeSelectedScene` (`:4586`, the visitor over the engine's entities; 41 to 99 ms per frame in the review), `NativeSelectedScene` and `AcquireNativeSelectedScene` (`:2424`), the eight engine reads in `PrepareNativeModelFrame` (listed above), `UpdateNativeSceneView` (`:5480-5510`), the engine's per-frame update functions that change what is drawn (display groups `64F000`/`619800`, model item update `D3E950`, pose and animation updates).
- **Depends on / enables**: R8a (persistent draws) and R13 / the end of the CPU bound.
- **Slices**: this reprise needs **a static study of its own** before any slice (done for the eight prepare reads: [R8B-ENGINE-READS-STATIC.md](R8B-ENGINE-READS-STATIC.md), with the value-snapshot slices S1 to S6 that precede F7): which engine fields change per frame for a drawn entity (visibility, pose, animation block, material parameters, wind) and which engine functions write them; the contract proposed here is only the *verification shape*. F5 versions on each input of a persistent draw (already needed by R8a). F6 a *shadow mode* comparison: every Nth frame run the full scan and compare the selected set and the draw inputs against the incrementally maintained ones, logging differences. F7 move the eight prepare reads into collect-time snapshots. F8 incremental selection per entity from the engine's dirty events, switchable by a flag file, with the full scan as the reference mode.
- **Pitfalls**: the review rates R8b high risk: a missed dirty signal draws stale data and no refusal fires. The mitigation is F6 on every witness, permanently available in a diagnostic build. The wind block (`animation+0x210`, effects F2) and the effect-driven fields change every frame by design and stay always-dirty.

### 2.9 R16 - other queues (phase G)

- **Touches**: `backend.h` (`DeviceQueues` becomes a set of queues with their own fences; a cross-queue `Wait`), the ingress copy context (R5, a COPY queue instead of the direct queue), future compute work (the E07 GPU particles of EFFECTS-D3D12-DESIGN-NATIVE §2.5; the original's SSAO is pixel-shader work, SSAO-NATIVE, so it is not a compute candidate without a conversion).
- **Depends on / enables**: R5, R3, R10 (cross-queue ownership transitions) / streaming without direct-queue contention.
- **Slices**: G1 only after phase A and D numbers show contention between ingress copies and the frame; a COPY queue for `Admit*` with a cross-queue fence the first use waits on (the consumer's token wait in collect); resources promoted from COMMON; G2 async compute for E07 when the effects workstream has the passes (compute update and sort on a compute queue, the draw on direct, a fence between).
- **Pitfalls**: placed-resource and cross-queue state rules; beyond ER for per-frame use (review). Do not start without a measured stall.

### 2.10 Options of phase G (each needs a measurement first)

| Option | Review | State of this code | Condition |
|---|---|---|---|
| Root CBVs for hot blocks | R7c, beyond ER | the persistent-draw design (A to C 2.9) keeps descriptors for constants; a root CBV makes per-draw constant rebinding a pointer set | after C: `prepare_ms`/`record_ms` still dominated by CBV descriptor writes |
| Pipeline library on disk | R9d | PSOs prewarmed by R9; a cold start still compiles | startup PSO creation measurably long |
| Enhanced barriers (`ID3D12Device10`) | beyond ER | the graph (E) emits legacy barriers | only if the supported Windows versions all have it; otherwise skip |
| Bindless / dynamic resources | not in the review | the programmes are the original SM5.0 DXBC and their register layout is data (review M3): not applicable without translating them | out of scope |
| ExecuteIndirect for model draws, GPU culling | not in the review | selection is the engine's CPU walk (collect); the original's draw list is CPU data | out of scope (the E07 indirect draws are a separate case) |
| GPU decompression/streaming APIs | not in the review | DS3 archives use DCX codecs read on the CPU (DIRECT-CONNECTION) | out of scope |

## 3. Dependencies and the safest order

```
(A to C: tokens, arena, ingress, heaps, persistent draws)
   D1a slots(1) -> D1b thread-safe Execute -> D1c per-slot state
   D2 explicit target sets (no behaviour change) -> D3 menu views, Bink, UI as passes
   D1d flip to 2 slots (needs D1c, D3)   -> D5 swap chain (waitable, buffers, tearing, interval)  -> D6 shutdown/resize order
                                          -> [measure M5: is the GPU wait now hidden?]
   E1 plan with states -> E2 shadow mode -> E3 pass by pass -> E4/E5   -> [E6 measure memory] -> E7/E8/E9 aliasing
   F1 job queue (render+present on a worker) -> F2 fault channel;   F5 versions -> F6 shadow scan -> F7 snapshots -> F8 incremental selection
   G on measurement
```

| Commit | Content | Reprise (slices of section 2) | Behaviour change | Flag |
|---|---|---|---|---|
| cD1 | `FrameSlot` ring with `slots=1`, readbacks per slot | R1a | none | `frames-in-flight.flag` |
| cD2 | thread-safe `Execute`, tokens monotonic | R1b | none | |
| cD3 | per-slot dynamic state checked, guard by slot ticket | R1c | none | |
| cD4 | `ViewContext` replaces `SwapTargetSet` | R12 (D2) | none | |
| cD5 | Bink frame, then menu views, then UI admission recorded in the frame list | R12 (D3) | per-view submissions disappear | |
| cD6 | **flip to 2 slots**, per-frame `Complete`/`Drain` replaced by tokens | R1d | CPU/GPU overlap; receipt semantics | `frames-in-flight.flag` |
| cD7 | **M5 run** | D | | |
| cD8 | swap-chain buffers from the display mode, waitable object | R14a, b | pacing | `legacy-present.flag` |
| cD9 | tearing flag and the original's interval | R14c, d | pacing, latency | same |
| cD10 | `WaitIdle` + `CollectAll` shutdown and resize order | M14 | none | |
| cE1 | plan with states and subresources, tests | R10 | none | |
| cE2 | graph shadow mode in every witness | R10 | none (log only) | `graph-shadow.flag` |
| cE3 | pass-by-pass conversion, hand transitions removed per pass | R10 | none | |
| cE4 | batching, cross-frame initial states | R10 | none | |
| cE5 | **M6 run** (memory report, `GetResourceAllocationInfo`) | R11 measure | | |
| cE6 | placed-resource arena, aliasing barriers, first-use discipline | R11 | memory | `no-alias.flag` |
| cF1 | render and present on a worker, job queue depth 1, fault channel | R13 | threading | `render-thread.flag` |
| cF2 | versions on draw inputs; shadow scan; snapshots | R8b | none (log) | `scan-compare.flag` |
| cF3 | incremental selection | R8b | CPU | `full-scan.flag` |
| cG | on measurement | R16, options | | |

Why this order: slots at `1` and the explicit target sets change nothing visible but remove the two structural blockers (single allocator, member swap) before any wait is removed; the flip (D6) is a one-line change of the slot count on top of them; R14 follows because the waitable object only makes sense with slots; the shutdown order comes last in D because it must be validated against the slot ring that now exists. E waits for D because the graph must know the one-list frame; F waits for E only softly (R13 does not need the graph) but for D hard (slots, thread-safe submit); R8b is last because it is the largest change to engine contracts (D-0157) and a static study is its first deliverable.

## 4. What to measure

### 4.1 Runs (all with `-NoDebugLayer` for timing, plus one run with the layer on for correctness)

| Run | Build | Question |
|---|---|---|
| M5 | after D6/D7, `slots=1` against `slots=2` from one build | how much of the GPU wait is hidden; does frame-time variance change |
| M6 | after D9 | pacing and latency with the waitable object and the original's interval |
| M7 | after E | memory with and without aliasing (`GetResourceAllocationInfo`, process private bytes, DXGI local memory) |
| M8 | after F1 | CPU overlap: owner-thread busy time against worker busy time |
| M9 | after F3 | collect time, scan against incremental, with `scan-compare` clean |

Scenes as in A to C section 3.1 (S1 High Wall idle, S2 title and Status with portraits, S3 load or teleport) plus **S4 a stress window** (a fight or a crowded area with many NPCs) and **S5 a menu-heavy window** (Status with portraits, character creation, with several offscreen views).

### 4.2 Counters added in D to G

| Counter | Where | Expected |
|---|---|---|
| `slot_wait_ms` (time blocked on the slot token), `waitable_wait_ms` (frame latency object) | slot reuse, frame start | D: small when the GPU keeps up |
| `cpu_frame_ms` (owner thread), `worker_busy_ms`, `queue_wait_ms` | F1 | F: owner thread no longer includes recording and present |
| frame time percentiles: p50, p95, p99, max over the window | `Clear()` pacing | variance is the user-visible gain |
| `input_to_present_ms`: QPC from the engine's input latch to the `Present` return | latch and present | latency not worse (D9) |
| `graph_barrier_mismatch`, `barriers_per_frame` | E2, E4 | 0 mismatches; batched barriers |
| resident target bytes by set, aliased bytes | E5/E6 | memory report |
| `scan_vs_incremental_diff` | F2/F3 | 0 |
| `drains` per frame | A to C counter | 0 in the frame after D6 |

### 4.3 Exit checks

D: two frames in flight, the GPU wait of the main list no longer in the owner thread's frame time (the `gpu_wait_ms` of the pacing row near 0), 0 debug errors for 600 frames on every witness, clean quit as Native1262. E: the graph drives the in-game frame with 0 mismatches over the shadow-mode runs; aliased build with 0 errors (once under GPU-based validation). F: parity witnesses with `scan-compare` clean. G: measured gain or dropped.

## 5. Live witnesses

All the A to C witnesses stay the non-regression set (Native1254 in game, title and menus, Status and portraits, character creation, NPC and cloth, velocity Native1191, Bink, area change, Native1262). New or sharpened for D to G:

| Row | Reprise | Action | Expected |
|---|---|---|---|
| RD1 | R1 | `slots=1` against `slots=2`, S1 and S4, 600 frames | identical image on a still scene; `gpu_wait_ms` hidden; no `native device removed` |
| RD2 | R1 | inject a present failure (`InjectPresentFailure`, the test build) and a timeout | the fault path ends in the single exit; in-flight objects are leaked, not released |
| RD3 | R1/R3 | teleport or bonfire warp with 2 frames in flight | no use-after-retire in the debug layer; private bytes return to baseline |
| RD4 | R12 | Status with portraits and character creation | views drawn in the same frame, no per-view submission, no flicker |
| RD5 | R12 | Bink movie then the title | movie frame sampled by the UI in the order of the list |
| RD6 | R14 | window resize, windowed to borderless, minimise and restore, alt-tab | no fault; buffers recreated; occlusion handled |
| RD7 | R14 | frame pacing with the original's interval | fps follows the engine's pacing; input latency not worse than before |
| RD8 | M14 | quit in game, quit during a load, quit during a menu offscreen view | exit 0, no leak, no hang; the quarantine only on a forced timeout |
| RD9 | soak | 30 minutes of play across areas, menus and a fight | stable private bytes and descriptor counts; frame percentiles stable |
| RE1 | R10 | every witness with `graph-shadow.flag` | 0 `graph_barrier_mismatch` rows |
| RE2 | R11 | the aliased build on S1, S2, S5 | image identical; 0 errors, once with GPU-based validation; memory down by the measured amount |
| RF1 | R13 | `render-thread.flag` on, S1 and S4 | owner-thread frame time down; image identical; a worker fault reaches `EndFrame` |
| RF2 | R8b | `scan-compare.flag`, 10,000 frames of play | 0 differences between the scan and the incremental set |

## 6. Risks specific to this code

1. **Hidden single-thread assumptions.** `Clear()` calls `PublishNativeSceneCopy`, `PrepareNativeModelFrame`, the movie and offscreen steps, the UI admission and the render in one lock scope; moving any of them across threads changes lock order with `scene_view_cpu_lock`, `native_offscreen_lock`, `model_storage_lock`, `gpu_submissions` and the draw-cache lock. Write the global lock order once (A to C section 6 item 1 is its first line) and test it with a thread-sanitiser-style stress (two threads preparing while a third retires).
2. **The receipt contract** is used by the pilot rows (`native_model_draw_submitted` with fences) and by the pacing rows; tokens must keep non-zero, increasing values so `Pilot-NativeWitness.ps1` and the runners keep working.
3. **`FrameSlot` and the previous-frame data** (velocity, scene colour copy, the lit HDR state at frame start) cross slot boundaries; each such resource is listed in the plan with an explicit cross-frame use, or it is wrong the first time two frames overlap.
4. **Aliasing and menus.** The offscreen target of a view is a texture record owned by ingress (not by the renderer); only the view's *transient* G-buffer set is aliased, never that record.
5. **Display workstream coupling.** R14 and the display modes are one design (DISPLAY-SWAPCHAIN-NATIVE); doing R14 first with fixed assumptions would be redone.
6. **R8b without its study** is the one reprise this note cannot slice safely; its first commit is a document.

## 7. Limits

Static reading of the tree at `5c12c7a6`; no build, no run, no measurement. The review's gains remain expectations until the phase A table exists. Numbers quoted for the CPU/GPU split are the review's. Line numbers will move. ER evidence is the review's section 2 and is not extended; what is beyond ER is marked so.

## Amendments by the phase E sheet

The [phase E sheet](D3D12-PHASE-E-FRAME-GRAPH-SHEET.md) turns sections 2.5 (R10) and 2.6 (R11) into an implementation sheet with the inventory of passes, targets, lifetimes and alias sets: R10 is the gate of the coming families and R11 is conditional on a measurement (its E6 gate); the slices E1 to E5 of 2.5 and E6 to E9 of 2.6 become the commits E0 to E10 there (E0 adds the rows, E6 is the decision); the shadow-mode criterion is equality of state at each use, not of barrier lists; `DuplicateUse` of the plan is redefined as an overlap of subresource ranges; the scene colour copy is the water/effects "FrameCopy" and never aliases.

## Note on the word "phase G"

In this document phase G is R16 (other queues) and its API options (2.9, 2.10). The coordinator's request of 2026-10-11 also used "phase G" for the game's **graphics options**; that inventory (what each option changes in the engine, what native does, what it recreates, the change classes P/G/R/H and the option-change mechanism) is in the [graphics options sheet](D3D12-PHASE-G-GRAPHICS-OPTIONS-SHEET.md), whose part B updates 2.9 and 2.10.
