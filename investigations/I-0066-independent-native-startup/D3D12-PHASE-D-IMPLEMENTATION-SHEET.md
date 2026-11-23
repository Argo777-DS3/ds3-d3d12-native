# D3D12 phase D: implementation sheet (frames in flight, one frame list, presentation, shutdown) and B5 (dynamic streams in video memory)

Request (coordinator, 2026-10-11): turn [D3D12-PHASES-D-G-PREP](D3D12-PHASES-D-G-PREP.md) into a sheet ready to implement for phase D (frames in flight). It must cover everything that today assumes one frame in flight (rings, upload arena, retirement tokens, CPU reads of GPU results, constants rewritten in place), the commit order under flags, the tests and witnesses, and what ER does as architectural reference. B5 (dynamic streams in video memory), deferred in phase B until this phase, is added. Static reading only: nothing was built, run or measured here.

Tree read: `main` `5be70746` (native renderer last touched by `cac80975`, phases A to C closed, [PREP section 10](D3D12-PHASES-A-C-PREP.md)). Line numbers are that tree's and will move. Labels: **observed** (read in that tree), **proposed**, **open**. ER is architectural evidence only ([review section 2](D3D12-DESIGN-REVIEW.md), [ER-DEEPENING-897](ER-DEEPENING-897.md)); nothing here is an ER layout or ABI. The sheet was reviewed by an independent Opus 5.5 pass (21 findings, applied).

## 0. Answer first

1. **The honest size of the gain changed.** After B4 the CPU wait on the world frame's main list is 1.2 ms (it was 19.5; this is the `Complete` wait, not a GPU timestamp). The render step is 5.0 ms (M4) in a frame of about 148 ms (6.73 fps) where collect is about 105 ms (PREP section 10; the M3 split was record 0.7, wait 1.4, present 0.8). Frames in flight hide at most about 1.2 to 2 ms (about 1 %) of that today. They remain worth building because (a) they are the prerequisite of the render thread (R13) and of the pacing work (R14), (b) the passes still to come (shadows, effects, YEBIS, SSAO) add GPU time that a per-frame wait would expose, (c) four structural blockers (one allocator, the member swap of the target sets, one submission per menu view and per movie frame, per-frame resource creation) are cheap to remove while the code is small. The flip itself stays a measured decision (M5, cD8).
2. **Order (section 4):** cD0 audit counters; cD1 `FrameSlot` ring with one slot (same behaviour) and a complete fault quarantine; cD2 explicit view contexts (no `SwapTargetSet`, no per-view heaps); cD3 one frame list (movie, menu views, UI in the main submission) and arena size classes; cD4 per-slot dynamic state, deferred readbacks and diagnostics; cD5 B5 (dynamic streams) in two steps; cD6 flip to two slots; cD7 M5 run and default; cD8 to cD9 swap chain (R14); cD10 shutdown and resize order (M14); the thread-safe submission mutex (cD-F) is needed by the render thread (phase F), not by phase D. Behaviour changes that can be flagged are behind a presence flag next to the module, as in phases B and C; the structural refactors (cD1 to cD3) are neutral and gated by the witness set, not by a flag (cD3 changes the submission structure and cannot keep two code paths cheaply).
3. **Eight facts that shape the order (several are not in the preparation note)**, each with its site in section 2:
   - `UploadArena::Retire(queues, ticket)` retires every range handed out since the previous call against that ticket. Today the menu view (`scene_pass.cpp:1409`) and the main frame (`:1634`) each retire, and the view's `Complete` + `Drain` + `CollectCompleted` return its chunk to the free list, where the main frame's next `Allocate` takes and zero-fills it (`backend.cpp:793, 802`). An arena range allocated at prepare time (B5a, UI) would therefore be retired by the **first** submission of the frame and could be reused before the main list reads it. One `Execute` and one `Retire` per frame (cD3) is a prerequisite of any arena allocation outside the recording call. (The movie does not use the arena.)
   - `DeviceQueues::Execute` is two steps (queue submission, then `next_++` and `Signal`) and `Complete` waits on one auto-reset event; a thread woken for another waiter's value throws "invalid completion" (`backend.cpp:298`) and the other waiter times out after 5 s. **In the game module only the owner thread submits and waits** (`Clear()`, including `AdmitPrimitiveWork`, and the close paths: `game_bootstrap.cpp:793`, `:1646`, `game_texture_ingress.cpp:231`); loaders only `RecordIngress`/`Commit` under `ingress_lock_`. The race is real as a mechanism but unreachable before R13, so it is not a phase D blocker.
   - the per-frame copy of environment-region cube descriptors goes into the **shader-visible** heap `native_light_views_` (`scene_pass.cpp:1144`), which the previous frame's GPU still reads.
   - `RetireWhenSubmittedWorkCompletes` (last submitted ticket) is safe only for owners released **after** their work was submitted; slots keep the draws until their own ticket.
   - the depth target is recreated, after a full `Drain`, whenever the stencil clear value changes (`:1362`, `:1566`): with slots it needs a ring-wide `WaitIdle`; it is not removable as such (risk 2 gives a way to make it rare).
   - the programme-change path of `NativeLightingResources` (`:916, :920, :937, :955, :959`) and the growth of `movie_upload_` (`:1479`) release GPU objects at once.
   - the frame's receipt is a completion contract (`Clear()` throws if a token is 0 or `debug_errors` is non-zero, `game_bootstrap.cpp:674`).
   - the dynamic streams create **one committed UPLOAD buffer per update** on engine threads (`UploadNativeModelBytes`, `game_bootstrap.cpp:5627`, called at `:1768` and `:7064`). They are not the only per-frame GPU creation: the UI admission creates a full-resolution depth texture and heaps every frame, each menu view creates an RTV heap (section 2.3, P1 to P3).
4. **B5 (cD5):** engine threads stop touching the GPU; they publish immutable CPU versions. The render thread binds, per frame, either an arena range (B5a) or a **single retained DEFAULT buffer per owner** updated by a copy recorded ahead of the frame list (B5b, the target). One buffer per owner is enough on one DIRECT queue: frame N+1's copy executes after frame N's reads in queue order (ER does the same, E5 with lag 2). B5b needs the single submission (cD3), not the slot ring or the flip.
5. **What would make the phase moot** is measured: if M5 shows the owner thread's frame time unchanged by two slots, keep `slots=1` as the default and start R13 on top of the ring.

## 1. State of the tree and baseline (observed)

| Item | Observed at `5be70746` |
|---|---|
| Tokens | `DeviceQueues::RetireAfter` / `CollectCompleted` / `LastSubmitted` exist (`backend.cpp:302-317`); the pacing row has `retirements_pending` |
| Arena | `UploadArena` (1 MB chunks, free list capped at 16, zero-filled ranges, `Retire` per call, `backend.cpp:786-832`) serves the lighting, point-light, environment, SSS and velocity blocks (`scene_pass.cpp:1034, 1078, 1139-1140, 1168, 1262`). A range over 1 MB gets its own chunk that is **never pooled** (`backend.cpp:794`, `:812`) |
| Ingress | `RecordIngress` / `Commit` / `SubmitIngress`: lists committed from any thread are submitted ahead of the next `Execute` (`backend.cpp:187-283`) |
| Geometry | static FLVER storage in DEFAULT memory (`AdmitNativeStaticBuffer`, `game_bootstrap.cpp:5646`, copy from COMMON then one COPY_DEST to vertex/index barrier in the same call); dynamic streams still UPLOAD (section 5) |
| Heaps | global shader-visible CBV/SRV heap (32 chunks of 8,192 descriptors) and one sampler heap on Tier 3 (`model_draw.cpp:148-179, 262-266`); constant chunks of 8 MB returned by `RetireWhenSubmittedWorkCompletes` (`:198-212`); per-draw table chunks return **without a ticket** when the last holder dies (`:222-224`) |
| World frame, M3/M4 | 2 executes, 1 drain, 2 completes per frame; render 5.0 ms, wait 1.2 ms, fps 6.73, collect 104-106 ms, arena 270 KB per frame |
| Flags convention | presence files next to the module placed by the runner: `no-debug-layer.flag`, `measure.flag`, `legacy-upload-geometry.flag`, `legacy-tables.flag` (`game_bootstrap.cpp:9020-9050`, `Run-StartupTrace.ps1`) |
| Test harness | `native/frame/Build-GameBootstrap.ps1` builds and runs `startup_primitive_test` and `game_texture_ingress_test` on WARP and hardware (`:225-235`), `Test-GameModelSelection.ps1`, `Test-GameModelPreparation.ps1`, `Test-GameTextureIngress.ps1`. `renderer/Test.ps1` runs only `ownership_test` of the separate `Backend` class from `evidence/I-0036/` (retired by the retention rule) |

Phase B exit check ("no `Drain` in the ingress, retirement and stream paths") is met for the game path. The `Drain`/`Complete` sites that remain are in section 2.4.

## 2. Inventory of what assumes one frame in flight

"Frame" means the sequence in `Clear()` (`game_bootstrap.cpp:647-736`): prepare the model frame, movie, menu views, UI admission, main list, present, drain.

### 2.1 Recording objects, guards, submission

| Id | Site | Assumption | Breaks with two slots | Fix | Test |
|---|---|---|---|---|---|
| R1 | `FrameRecorder` (`backend.h:172-176`): one `allocator`, one `list`; reset at `scene_pass.cpp:1366-1367` (menu view), `:1491-1492` (movie), `:1574-1575` (main), `:1669-1670` (startup) | Every use is followed by a full wait | `ID3D12CommandAllocator::Reset` while the GPU executes the previous list is a device error | `FrameSlot` owns allocator and list; one list per frame (cD1, cD3) | T-D1 |
| R2 | `native_model_inflight_` (`scene_pass.h:388`), tested at `:1540` and `:1346`, filled `:1571-1573`, `:1364`, cleared `:1655`, `:1409`, `:2127`; quarantined by the destructor `:2162` | Draws are kept until `Complete`; nobody records a second frame meanwhile | The guard throws on the second frame; draws and their pooled chunks must live until their own ticket | The slot owns `draws`, released at its retirement; the guard becomes "this slot's previous ticket has completed" (cD1) | T-D1, T-D4 |
| R3 | `DeviceQueues::Execute` / `SubmitWithIngress` (`backend.cpp:250-283`): `ExecuteCommandLists`, then `next_++`, then `Signal` | One submitting thread. This is **true in the game module today** (section 0.3) | Reachable only with a second submitting thread (R13) | The submission mutex and a per-wait event belong to phase F (cD-F), kept small; until then assert "owner thread" in debug | T-D2 (phase F) |
| R4 | `Complete` (`backend.cpp:290-301`): one auto-reset event, 5 s timeout | One waiting thread | A wake-up for another waiter's value throws "invalid completion" and the other waiter times out | same | same |
| R5 | `Clear()` holds `gpu_submissions` from before `PrepareNativeModelFrame` to after the pacing row (`:651`) | The whole frame, including its wait, is under the lock | A slot wait under the lock blocks the other holders (startup, `Views` on the owner thread, the UI image owner constructor); smaller than a loader stall but avoidable | The slot wait happens before the lock (cD4) | T-D5 |
| R6 | The receipt (`FrameReceipt`, `backend.h:20-29`; check at `game_bootstrap.cpp:674`) | `fence`, `presentation_fence` are completed values; `debug_errors` is the frame's | Tokens are submission facts; `pixel_hash`, `changed_pixels`, `debug_errors` exist only after the GPU finished | `fence` and `presentation_fence` stay non-zero, increasing tokens; the rest arrives in a `CompletedFrame` when the slot retires (cD4) | T-D3 |
| R7 | `faulted_` and the catch in `RenderNativeModels` (`:1660`); `EndFrame` `ExitProcess(0xD3126607)` (`:7429`) | A throw means nothing is in flight | A throw after `Execute` leaves a frame in flight; the destructor quarantine is **already incomplete** (it does not detach `movie_*`, `native_light_targets_/views_/quad_/list_`, `native_env_*`, `native_sss_temp_`, `velocity_*`, `scene_colour_copy_`, `display_source_`; compare `:2158-2188` with `scene_pass.h:258-386`) | The quarantine is completed and extended to the ring in cD1, before any overlap exists (section 3.6) | T-D4 |
| R8 | `EndFrame` writes the engine clock after `Clear()` (`:7424-7425`) | `Present` and the wait are inside `Clear()` | Order unchanged; the clock must still be written after `Present` returns (it is the engine's frame time) | kept; noted so cD6 does not move `Present` after it | RD7 |

### 2.2 Rings, pools and arenas

| Id | Site | Assumption | Breaks with two slots | Fix | Test |
|---|---|---|---|---|---|
| A1 | `frame_uploads_.Retire(gpu_, ticket)` after each `Execute` (`scene_pass.cpp:1409`, `:1634`); `Retire` takes "every range handed out since the previous call" (`backend.cpp:805`) | Every range is consumed by the list whose ticket retires it | Ranges allocated outside a recording call or before the last `Execute` of the frame are retired by an earlier ticket than their last reader (fact 1 of section 0.3) | One `Execute` per frame (cD3) and one `Retire` after it | T-D6 |
| A2 | Arena sizing: 1 MB chunks, free list capped at 16, ranges over 1 MB take their own **unpooled** chunk (`backend.cpp:794, 812`); physical streams reach 32 MB (`game_bootstrap.cpp:1701`), cloth 64 MB (`:7052`), Bink planes at 720p about 1.38 MB | Ranges are small (270 KB per frame) | Moving Bink or streams onto the arena as it is would create a committed buffer per movie frame and per large stream, and two frames outstanding exceed the cap of 16 | Size classes (1, 4, 16, 64 MB) pooled with a total-bytes cap, in cD3; gate becomes "no resource creation in steady state after warm-up" | T-D6 |
| A3 | Constant chunks (8 MB) return through `RetireWhenSubmittedWorkCompletes` (`model_draw.cpp:198-212`); **table chunks return without a ticket** when the last holder dies (`:222-224`); global chunks by ticket (`:176-179`) | The draw dies after its list was submitted and completed | A draw released between record and `Execute` would free table chunks immediately (the per-draw fallback) or retire constants against the previous frame | Draws live in the slot until its ticket; peak pool sizes double, counted (`constant_chunks_in_use_peak`, `global_chunks_in_use_peak`) | T-D1, RD3 |
| A4 | Global CBV/SRV heap: 32 chunks of 8,192 descriptors by ticket; fallback to per-draw tables when none is free | One frame of draws | Two frames can exhaust the chunks and silently fall back (slower, still correct) | `legacy_table_draws` must stay 0 at `slots=2`; raise the chunk count if not (cD6) | RD1 |
| A5 | CPU descriptor pool (`AllocateCpuDescriptor`, `backend.cpp:229-248`) | Leases returned by refcount | Unchanged (CPU-only, read at prepare) | none | |
| A6 | UI pass: pooled upload chunks and targets returned by ticket (`primitive_pass.cpp:52-74`); `RetirePrimitives` (`scene_pass.cpp:1804-1810`); `AdmitPrimitiveWork` ends with `SubmitIngress` (`:1829`) | The previous frame's list was already submitted when the next admission retires it | Holds (admission precedes record). With one list per frame the build's ingress list rides the frame's `Execute`; keeping a separate `SubmitIngress` is an efficiency loss (an extra submission), not a correctness fault | cD3 removes `SubmitIngress` from the game path | RD4 |
| A7 | `primitive_depth_`, `primitive_stencil_initialized_` (`scene_pass.h:301-304`; the flag is set after `Complete` at `:1635, :1683`) | One writer per frame, ordered on the queue | Fine on one queue; the flag is CPU state | none (set it at record time when the slot ring defers completion) | |

### 2.3 Descriptors, resources and objects created or rewritten per frame

| Id | Site | What | Why it was safe | Fix |
|---|---|---|---|---|
| D1 | `EncodeNativeEnvRegions` (`scene_pass.cpp:1144`): `CopyDescriptorsSimple` of each region cube into `env_cube_views + i` of the **shader-visible** `native_light_views_` | Every frame | The previous frame had completed | A range per view **and** per slot (`env_cube_views + (slot * views + view) * env_region_limit`), or the cubes go through the global heap chunks like model draws. Menu views of one extent share a `TargetSet` and its `light_views` heap (`:1337, :1357`), so a per-slot range alone is not enough once cD3 records several views in one list (cD4) |
| D2 | `movie_upload_` (`:1479-1486`): one UPLOAD buffer (planes plus the 144-byte block), rewritten each movie frame, grown by releasing the old one at once | Every movie frame | `Drain` after the movie submission (`:1522`) | Planes and block from the arena size classes (B2d) recorded in the frame list (cD3) |
| D3 | `movie_rtv_` (`:1487-1490`): `CreateRenderTargetView` into a one-slot RTV heap each frame | RTV descriptor (CPU-only heap) | Safe regardless: RTVs are read at record time | none (note kept so nobody "fixes" it) |
| D4 | `Targets`, `NativeLightingResources`, `NativeVelocityResources`, `SceneColourCopy`, `DepthTarget` (`:640-730, 964-1022, 1233-1250, 1324-1333`): resources reassigned (`IID_PPV_ARGS` on a `ComPtr` releases the old one) and shader-visible views rewritten, keyed by `target_generation_` | On extent or depth change only | Preceded by `Drain` (`:1362`, `:1566`) or `Resize` (`:2121`); `SceneColourCopy` is preceded by nothing and relies on its holders (`:1328`) | A ring-wide `WaitIdle()` before any of them; replaced resources go to `RetireAfter(LastSubmitted)` |
| D5 | `DepthTarget` on `clear_stencil` change (`:1362`, `:1566`) | Depth resource and four view sets | `gpu_.Drain()` right before | as D4; counted (`depth_recreations`) |
| D6 | Static blocks created once: `native_light_quad_`, `native_light_list_`, `native_light_zeros_`, `movie_quad_` | Never | n/a | none |
| D7 | `native_light_constants_[]` (`scene_pass.h:333`): GPU addresses of this frame's arena ranges | Every frame | Single recording thread | none; assert owner thread |
| D8 | Per-version UPLOAD resources of dynamic streams (section 5) | Created per update, never rewritten | Immutable versions | B5 |
| D9 | Fixture and work-mode paths (`Render`, `RenderFrame`, `AdmitSceneWork`, `Textures`, `material.constants`, `resolve_constants_`, `mask_constants_`) | Rewritten in place with `Drain` | Own `Drain`s (`:638, :1745, :1790, :1889, :2083, :2113`) | Out of scope: slot 0 with `WaitIdle`; the game path never takes them |
| D10 | Programme-change path of `NativeLightingResources` (`native_light_programs_ != input.programs`): lighting PSOs replaced and the old released (`:916`), SSS and environment PSOs reset (`:920, :937`), `native_env_box_` re-uploaded (`:955`), `native_point_psos_` cleared (`:959`) | When the lighting programmes change | Nothing was in flight | Old objects to `RetireAfter(LastSubmitted)` after the change is detected, or a ring `WaitIdle` before it; how often it fires is unknown (counter) |
| P1 | `AdmitPrimitiveWork` (every frame with UI): `PrimitiveDepthTarget` creates a full-resolution committed D32S8 texture and a DSV heap (`:1794-1803`, called at `:1828`) | Every UI admission | Retired by ticket (`RetirePrimitives`) | Persistent per extent (its stencil is re-initialised each frame by `initialize_primitive_stencil`) (cD3) |
| P2 | `PrimitivePass::Build` creates a shader-visible CBV/SRV heap and a sampler heap, plus RTV/DSV heaps when there are targets (`primitive_pass.cpp:141-171`) | Every admission | Retired by ticket | Counted in cD0; candidates for the global heap after the measurement |
| P3 | `RenderNativeOffscreen` creates an RTV heap per view per frame (`:1383-1384`) | Every view | Safe (RTV) but a creation | Persistent in the view's `ViewContext` (cD2) |

### 2.4 Every remaining `Drain` / `Complete` (observed, game path first)

| Site | Role | Action |
|---|---|---|
| `scene_pass.cpp:1634, :1655` | main frame: `Complete` then `Drain` after `Present` | replaced by the slot ticket (cD1 keeps them at `slots=1` through `End`, cD6 removes them) |
| `:1409`, `:1522` | menu view and movie submissions | disappear with the single list (cD3) |
| `:1362`, `:1566`, `:1462` | depth recreation (offscreen, main), movie plane resize | `WaitIdle` of the ring (kept, rare) |
| `:1682, :1685` | startup frame | same slot path (cD1) |
| `:2121` (`Resize`), `:2126` (`gpu_.Close`) | resize, close | cD10 |
| `game_bootstrap.cpp:793`, `:1646` | `Close()` storage drain, `CloseNativeModelStreams` | one `WaitIdle` (cD10) |
| `game_texture_ingress.cpp:231` | `TextureIngress::Close` | one `WaitIdle` (cD10) |
| `:638`, `:1745`, `:1790`, `:1889`, `:2083`, `:2113` | fixture / work mode | unchanged (D9) |
| `backend.cpp:596, 605, 702, 726` | the separate `Backend` class (camera and content hosts) | unchanged |
| `content/gpu_texture_packages.cpp:35, 118` | not in the game module | unchanged |

### 2.5 CPU reads of GPU results

| Id | Site | What is read | Today | With slots |
|---|---|---|---|---|
| G1 | Backbuffer readback `readback_`, copy at `scene_pass.cpp:1623-1630`, map, hash and pixel file `:1637-1653` (every 120th frame with `witness_diagnostics`, or when `pixel_file` is set) | Presented image | After `Complete` of the same frame | Per-slot readback buffer; map and hash at slot retirement; the `pixel_file` path returns through the completion event. `measure.flag` keeps it off |
| G2 | Velocity and motion-vector readbacks (copy `:1276-1287`, `ReadNativeVelocity` `:1636`/`:1290-1323`, every 30th frame or with `velocity_file`); `velocity_witness` consumed by `Clear()` (`game_bootstrap.cpp:684-688`); `velocity_sampled_` | Statistics and picture | After `Complete` | Per-slot buffers and flag; statistics produced at retirement and logged by `Clear()` through `TakeCompleted()` a frame later |
| G3 | Menu view and movie readbacks (`:1396-1419`, `:1511-1532`): a new READBACK buffer per call when `pixel_file` is set, mapped after the drain | Witness images | Immediately | Kept in the slot's `kept` list; written at retirement |
| G4 | `Diagnostics()` (`:773-789`) reads and **clears** the whole info queue; called after every frame (`:1655`, `:1685`), after each view and movie (`:1410, :1523`) and in the UI admission (`:1830`) | Debug-layer messages | Right after the drain | At slot retirement and at flip points; the UI-admission call stays (it reports CPU-time validation of the build); a message belongs to the work submitted before it and is reported with the retiring frame's receipt (errors raised while recording N+1 may therefore show in N's receipt) |
| G5 | `ReadStartupView` (`:1690-1701`) | Swap chain description, buffer count | n/a | Must follow R14's buffer count (cD8) |
| G6 | Pacing: `gpu_wait_ms`, `present_ms`, `record_ms` (`:1634, 1657-1658`) | Timestamps | wait = `Complete` | `gpu_wait_ms` keeps its name and means the slot wait at frame start; new `slot_wait_ms` / `waitable_wait_ms` once cD8 adds the waitable object |

### 2.6 Cross-frame GPU state (observed; none is read across frames today)

- The lit HDR, the six G-buffer targets, the neutral images, the environment and SSS targets are written and read **inside** the frame and return to the state the next frame expects (`scene_pass.cpp:1123-1127`, `:1615`, `:1407`). On one queue, serial execution keeps this correct with two frames in flight; the convention must stay exact (R10's E5 makes it explicit later).
- `scene_colour_copy_` is copied at the start of the forward pass and read in the same pass (`:1588-1594`).
- The velocity target is cleared each frame (`:1259`); the motion vectors come from a CPU matrix: no GPU data crosses a frame.
- The menu view target (ingress texture) is written by its view and sampled by the UI of the same frame (`PIXEL|NON_PIXEL` outside, `RENDER_TARGET` inside, `:1386-1405`).

## 3. Target design

### 3.1 Objects (proposed names)

```cpp
// backend.h
constexpr uint32_t max_frame_slots=3;
struct FrameSlot {
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> allocator;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> list;    // the frame's single list (cD3)
    uint64_t ticket{};                  // ticket of the Signal after this slot's Present (0 = idle)
    uint64_t serial{};
    std::vector<std::shared_ptr<const OwnedNativeModelDraw>> draws;   // replaces native_model_inflight_
    std::vector<std::shared_ptr<void>> kept;                          // primitive snapshot, stream versions, witness readbacks
    ReadbackSet readbacks;              // backbuffer, velocity, motion; lazily created, empty on measure runs
    FrameReceipt receipt;               // completed when the slot retires
};
class FrameRing {                       // owned by SceneRenderer; owner thread only
public:
    void Open(DeviceQueues&,uint32_t slots);
    FrameSlot& Begin();                 // next slot; waits for its ticket (slot_wait_ms); RetireSlot() on the previous holder
    void End(FrameSlot&,uint64_t ticket);   // records the ticket; at slots=1 also waits and retires at once
    void WaitIdle();                    // every slot, then CollectCompleted
    CompletedFrame* TakeCompleted();    // witness/diagnostic results of retired slots
    void QuarantineToProcessExit() noexcept;
};
```

`RetireSlot(slot)` is the one place that runs after a slot's ticket completed: `draws.clear()`, `kept.clear()`, readbacks mapped and processed, `Diagnostics()`, `CollectCompleted()`, receipt filled and queued. At `slots=1` it runs at `End`, at `slots>=2` at the next `Begin` of that slot. **The deferred code paths are therefore exercised at `slots=1`, but the overlap hazards (D1, D2, D10, readback reuse) cannot trigger there** (the CPU never records while the GPU runs): the overlap tests need a forced overlap (section 6, T-D1).

### 3.2 Frame sequence (cD6 form)

1. `BeginFrame` (before `gpu_submissions`): `FrameRing::Begin()`; slot wait; `CollectCompleted`.
2. Under `gpu_submissions` (CPU structures only): `PublishNativeSceneCopy`, `PrepareNativeModelFrame`, movie frame, menu views, UI admission build (ingress committed, not submitted), dynamic-stream copies (B5b, ingress committed).
3. Record **one** list: movie, menu views (each with its own `ViewContext`), main G-buffer, lighting, forward, velocity, composition, UI overlay, witness copies. The ingress lists committed above precede it in the same `ExecuteCommandLists` call.
4. `Execute(list)`, arena `Retire` at that ticket, `Present(interval, flags)`, `Signal` → slot ticket. (DS3's choice: the post-`Present` signal covers the allocator, the draws and the `Present`; ER's ring token is queued in the frame's records and signalled by the worker before its `Present` callback, [ER-DEEPENING (a)(b)](ER-DEEPENING-897.md), so this is not derived from ER.)
5. `EndFrame` writes the engine clock (unchanged).

### 3.3 Tokens and lifetimes (rules to assert)

- L1: nothing recorded into a list is released before the slot's ticket: draws, ingress `kept`, stream versions and buffers, primitive snapshot, readbacks.
- L2: owners released by other threads (storage, streams, textures, cloth) keep using `RetireWhenSubmittedWorkCompletes`; safe because every consumer holds its own reference while recording (invariant of PREP section 9).
- L3: `UploadArena::Retire` runs once per `Execute`; arena allocations happen on the owner thread **before the frame's single `Execute`** (in prepare or in recording), never after it and never on another thread.
- L4: resources replaced at a generation change or a programme change go to `RetireAfter(LastSubmitted)` after a `WaitIdle` where views are rewritten, never released at once.
- L5: a slot is reused only after `Wait(ticket)`; ticket 0 never waits.
- L6: `CollectCompleted` runs in `RetireSlot` and in `WaitIdle`, nowhere else in the frame path.

### 3.4 Receipt contract

`fence` = ticket of the `Execute`, `presentation_fence` = ticket of the signal after `Present` (non-zero, increasing), `present_status` the `Present` HRESULT. `pixel_hash`, `changed_pixels`, `debug_errors` are filled when the slot retires and delivered by `TakeCompleted()`; `Clear()` throws at once on `FAILED(present_status)` or a zero token, and at the next frame start on a completed frame's `debug_errors`. At `slots=1` the throw comes in the same frame, at `slots>=2` in a later one: the runners that look for a fault row must tolerate one frame of delay. `Pilot-NativeWitness.ps1` reads `fence` / `presentation_fence` from `native_model_draw_submitted`: they stay.

### 3.5 Flags

- `frames-in-flight.flag` (presence = two slots) is read at `Start` from cD1 (still one slot) and takes effect at cD6; after M5 and the witnesses (cD7) the default is inverted and the legacy setting becomes `single-frame.flag` (presence = one slot, equals today). Changing it needs a restart.
- `legacy-dynamic-streams.flag` (B5, UPLOAD per version, equals today).
- `legacy-present.flag` (R14, equals today's two buffers and `Present(0,0)`).

### 3.6 Fault path

A throw after `Execute` and before the slot ticket is known marks the ring faulted; `~SceneRenderer` keeps, to process exit, every slot's draws, `kept`, allocators and lists **and the members the current quarantine misses** (R7). A wait that times out (5 s) is a fault, never a completion. The single exit stays `EndFrame`'s catch. This lands in cD1, before any overlap exists.

## 4. Commit order under flags

Each commit builds alone and has its test. Rollback = revert the commit or set the flag.

| Commit | Content | Files | Behaviour | Flag | Tests |
|---|---|---|---|---|---|
| cD0 | Audit counters: `drains` and `completes` by call-site id; `retirements_pending` peak; `constant_chunks_in_use_peak`; `global_chunks_in_use_peak`; `depth_recreations`; `lighting_programme_changes`; `resources_created_per_frame` (committed resources of any heap type) and `descriptor_heaps_created_per_frame` (P1 to P3, D8); dynamic-stream updates and bytes per frame (B5 baseline); section 2.4 checked against a run | `backend.*`, `model_draw.cpp`, `primitive_pass.cpp`, `scene_pass.cpp`, pacing row | none | | short run (S1, S5) |
| cD1 | `FrameSlot`/`FrameRing` with `slots=1`; recorder objects move into the slot; `native_model_inflight_` becomes `slot.draws`; readbacks per slot; `RetireSlot` introduced and run at `End`; quarantine completed and extended to the ring (R7) | `backend.*`, `scene_pass.*` | none | `frames-in-flight.flag` read (still 1) | T-D1 (forced overlap), T-D4, A-C witnesses |
| cD2 | `ViewContext` replaces `SwapTargetSet`: the 16 swapped members become fields passed to `NativeModelTargets`, `NativeLightingResources`, `EncodeNative*`, `NativeVelocityResources`; per-view RTV heap persistent (P3); `offscreen_sets_` keeps the per-extent contexts | `scene_pass.*` | none | | T-D7, RD4 |
| cD3 | One frame list: Bink (planes from the arena size classes, B2d), menu views, UI admission (no `SubmitIngress`, persistent primitive depth per extent, P1), main view in one `Execute`; per-view `Drain`/`Complete` removed; arena size classes (A2); readbacks of views through `kept`; `Retire` once | `scene_pass.*`, `backend.*`, `primitive_pass.cpp`, `game_bootstrap.cpp` (`RenderNativeMovieFrame`, `RenderNativeOffscreenViews`, `Clear`) | submissions per frame fall from 2 + views to 1; resource creations per frame fall | none (structural; gated by the witness set) | T-D6, RD4, RD5 |
| cD4 | Per-slot and per-view dynamic state: environment cube descriptors (D1), replaced resources to `RetireAfter` after `WaitIdle` (D4, D10), deferred readbacks and diagnostics through `TakeCompleted` (G1 to G4), receipt split (R6), the slot wait before the lock (R5) | `scene_pass.*`, `game_bootstrap.cpp` | receipts completed later at `slots>=2` only | | T-D3, T-D5, T-D9 |
| cD5a | B5a: dynamic streams as immutable CPU versions; the render thread binds an arena range per frame; engine threads create no GPU object | `game_bootstrap.cpp` (stream and cloth code), `model_geometry.h`, `model_draw.cpp`, `backend.h` (`Range` carries its chunk resource), `scene_pass.*` | stream resource creation per frame falls to 0 | `legacy-dynamic-streams.flag` | T-D8 |
| cD5b | B5b: one retained DEFAULT buffer per owner updated by a copy recorded ahead of the frame list | same | geometry of cloth and physical models read from video memory | same | T-D8, GPU-time comparison |
| cD6 | Flip: `slots=2` under `frames-in-flight.flag`; per-frame `Complete`/`Drain` removed at `slots>=2`; chunk counts raised if the counters say so | `scene_pass.cpp`, `model_draw.cpp` | CPU/GPU overlap | `frames-in-flight.flag` | RD1, RD2, RD3 |
| cD7 | **M5 run** (`slots=1` against `slots=2`, one build, S1 to S5, with and without the debug layer); then, if it passes, the default is inverted (`single-frame.flag` is the legacy setting) | runners, `game_bootstrap.cpp` | default | `single-frame.flag` | RD9, exit check 9.3 |
| cD8 | R14a-b: buffer count from the display mode, waitable object (`SetMaximumFrameLatency`), buffers in a vector | `backend.*`, `scene_pass.cpp:1695` | pacing | `legacy-present.flag` | RD6, RD7 |
| cD9 | R14c-d: tearing flag when the interval is 0, `Present(interval,0)` from the engine pacing (`FUN_1400D5E30`, [DISPLAY-SWAPCHAIN-NATIVE](../I-0068-static-yebis-ffx-lights-spec/DISPLAY-SWAPCHAIN-NATIVE.md) 3.3 and 5), `DXGI_STATUS_OCCLUDED` accepted | same | pacing, latency | same | RD7 |
| cD10 | M14: one `WaitIdle`, `CollectAll`, release in dependency order; `Resize` waits every slot | `scene_pass.cpp:2118-2189`, `game_bootstrap.cpp:771-801, 1646`, `game_texture_ingress.cpp:231` | none | | RD8, T-D4 |
| cD-F | (phase F, not D) submission mutex and per-wait event so the render thread can submit and wait | `backend.*` | none | | T-D2 |

Why this order: the ring and the fault path first because they change nothing visible and every later commit depends on them; the view contexts and the single list next because they remove the two structural blockers (member swap, one submission per view and per movie frame) and make the arena rule (L3) true; per-slot state before the flip so the deferred paths already run; B5 after the single list (its arena ranges and copies need one `Execute`) and before the flip (it removes the engine-thread GPU creation that the overlap would make harder to reason about); the flip is then a change of the slot count plus the removal of two waits; R14 follows because the waitable object only bounds the queue and does not replace slots; shutdown last because it must be validated against the ring that now exists.

## 5. B5: dynamic streams in video memory

### 5.1 Observed

- Physical streams (`UpdateNativePhysicalModelStreams`, `game_bootstrap.cpp:1726-1775`, hook `0x51B940`): per update, a new `NativeModelStreamVersion` copies `owner.authored` (every stream's `bytes`), refills positions, normals and the extra element from the engine's CPU physics arrays, and calls `UploadNativeModelBytes` per stream (`:1768`): one committed UPLOAD buffer, mapped, filled, unmapped, per stream per update (`:5627-5645`). The version is published as `owner.current`; the owner is moved out of the map when retired (`:1637`).
- Cloth streams (`NativeClothVertexUpload`, hook `0x23D5BC0`, `:7042-7087`): per simulated frame, per cloth mesh, the CPU copy is replicated onto duplicated vertices by the engine's `34F1B0`/`34F090`, copied into a `std::vector`, fingerprinted and uploaded the same way (`:7064`); **a new `NativeClothStream` replaces the map entry each update** (`:7059, :7069`): there is no persistent cloth owner object. The set release erases map entries (`:7084`).
- Selection reads the current version (`:3574-3590`, `cloth_streams.find(object)` at `:3663`). It refuses a stream without a resource (`:3651`) and the cloth binding requires `cloth->resource` (`:3664`); palette validation finds the stream's bytes by comparing `candidate.resource` with `mesh.geometry.vertex_resource` (`:4871-4875`); `ValidateGeometry` requires `vertex_resource` and bounds-checks the views against its width and address (`model_draw.cpp:60-79`); `BindModelGeometry` replaces vertex slot 0 by the stream resource at offset 0 (`model_geometry.h:18-47`); `UploadArena::Range` has no resource pointer (`backend.h:128`).
- Both updates run outside the frame lock (phase B6) and create GPU resources from engine threads. Which engine threads call them is **open**.

### 5.2 Design

- **Threads:** the update functions produce only CPU data: `std::shared_ptr<const std::vector<BYTE>> bytes`, `generation`, `fingerprint`, `simulated`, size. They never touch the device, an arena or a command list. (The cloth copy `current` that is allocated and dropped today is kept in the version.)
- **B5a (arena-bound, stepping stone):** `PrepareNativeModelFrame` runs on the render thread inside `Clear()`; for each selected dynamic stream it allocates `bytes.size()` (4-byte aligned) from `frame_uploads_`, copies the bytes and binds the vertex buffer view to the range. No resource creation, vertex fetch from UPLOAD memory as today, a memcpy per selected stream per frame. **Requires cD3** (one `Execute`, L3) and the arena size classes (A2: streams reach 32 and 64 MB). `Range` gains its chunk `ID3D12Resource*` and offset so that `ModelGeometryBinding` can keep `vertex_resource` alive and the extent checks (`model_geometry.h:33-36`, `ValidateGeometry`) keep working with an offset.
- **B5b (target, one DEFAULT buffer per owner):** `DynamicStreamBuffer { ComPtr<ID3D12Resource> buffer; uint64_t bytes; std::atomic<uint64_t> copied_generation; known state }`, a shared object created lazily at the first prepare in COMMON, held by the physical-stream owner and, for cloth, carried from the previous `cloth_streams` entry to the next so that it survives updates (the cloth map entry keeps it; the set release drops it). In `Clear()`, for each selected stream whose `version.generation` differs from `copied_generation`: allocate an arena range, `memcpy`, record `CopyBufferRegion` into the buffer on a list from `RecordIngress()` (committed, so it executes ahead of the frame list in the same `Execute`), `Keep` the owner and the version, and set `copied_generation`. **One version per owner per frame: the first selection of the frame decides**, so the menu views and the main view of one frame bind the same bytes (pose and stream stay a pair for the selection that came first). A stream whose version did not change costs nothing.
- **Why one buffer is enough:** all work is on one DIRECT queue and frame N+1's copy is recorded and executed after frame N's reads (ER E5 runs an in-place copy update under lag 2, E2). Per-slot buffers (`slots * bytes` of memory, per-slot generation) would only be needed if the copy moved to an async COPY queue (phase G) or if the CPU wrote a mapped buffer the GPU reads.
- **States (must be confirmed under the debug layer, T-D8):** the first copy follows `AdmitNativeStaticBuffer` (`game_bootstrap.cpp:5656-5660`): record the copy from COMMON (implicit promotion to COPY_DEST), then one COPY_DEST to `VERTEX_AND_CONSTANT_BUFFER` barrier in the same call. For later updates track the state on the owner: a buffer left by an explicit barrier stays in that state, so the next copy needs a `VERTEX_AND_CONSTANT_BUFFER` to COPY_DEST barrier first; if the layer reports a state mismatch (the decay rule for resources promoted or read through COMMON is the point to verify), switch to the pure-promotion pattern (no explicit barrier, copy from COMMON each call). Both patterns are recorded here so the test decides, not a guess.
- **Lifetime:** the buffer lives in the owner; selected draws hold their own reference through `ModelGeometryBinding.vertex_resource`; owner retirement keeps `RetireWhenSubmittedWorkCompletes` (L2). The palette validation compares `stream_generation` (already in the binding) instead of resource identity, because several versions now share one resource.
- **Synchronisation of `copied_generation`:** written by the render thread only; read by the render thread only; engine threads write the version map under the existing locks and never read it.
- **Memory:** `bytes` per dynamic stream (one buffer), counted by `dynamic_stream_bytes_resident`; cloth sets are capped by the existing 64 MB check (`:7052`).

### 5.3 Commit slices

cD5a then cD5b (section 4), each with `legacy-dynamic-streams.flag`. **Measure first** (cD0): if the number of dynamic streams per frame in the target scenes is small, B5a is enough and B5b waits for GPU time to matter. The gate is "no resource creation for streams in steady state after warm-up" (not "zero committed buffers per frame": other per-frame creations are the subject of P1 to P3).

## 6. Tests

Harness: `native/frame/Build-GameBootstrap.ps1` (it already builds and runs `startup_primitive_test` and `game_texture_ingress_test` on `warp` and `hardware`), `Test-GameModelSelection.ps1`, `Test-GameModelPreparation.ps1`, `Test-GameTextureIngress.ps1`. New tests join `startup_primitive_test.cpp` or a sibling built the same way. `renderer/Test.ps1` stays for the separate `Backend` class.

| Id | Content |
|---|---|
| T-D1 | Ring with 1, 2 and 3 slots on a real device: record N dummy frames; **forced overlap**: before recording frame N+1, make the queue wait on a fence the CPU signals later (`queue->Wait`), so the GPU is provably still busy with frame N while N+1 records; check the allocator is never reset before its ticket completed (debug layer 0 errors), `Begin` waits only for the slot being reused, draws held by a slot expire exactly at its retirement (weak pointer), `slots=1` retires at `End`. Without the gate the test can pass at `slots=2` on a fast GPU without any overlap. |
| T-D2 | (phase F) Two to four threads call `Execute` and `Wait` concurrently for 10,000 submissions: tickets strictly increase in `Signal` order, the fence value never decreases, every waiter returns, no wrong wake-up. |
| T-D3 | Receipt contract: tokens non-zero and increasing at `slots=2`; `pixel_hash` and `debug_errors` arrive for a sampled frame through `TakeCompleted()` after its slot retires; an error injected while recording N+1 is reported with the retiring frame and ends in the single exit. |
| T-D4 | Fault path: extend the test-fault hook (`InjectPresentFailure` exists only on `Backend` under `I0036_TEST_FAULT`; add the equivalent to `SceneRenderer` under a test define) and a forced 5 s timeout with two frames recorded: nothing in flight is released (counters of released draws and chunks stay 0), the destructor quarantines the ring and the members listed in R7. |
| T-D5 | The slot wait runs without `gpu_submissions`: a thread taking the lock during a forced wait is not blocked. |
| T-D6 | Arena rule: the frame with a menu view, a movie and the main view issues exactly one `Execute` and one `Retire`; size classes reuse a 4 MB and a 16 MB chunk; an allocation after the frame's `Execute` is refused in debug. |
| T-D7 | Equivalence: the same offscreen view with the old `SwapTargetSet` build (reference hash from the current witness files) and with `ViewContext`; the control suites unchanged. |
| T-D8 | B5: a stream updated every frame for 200 frames at `slots=1,2,3` with a forced overlap: the drawn vertex data equals the version of that frame (readback of a one-triangle fixture drawn through the real pipeline), no resource creation after warm-up, an unchanged version costs no copy, a stream retired while two frames are in flight keeps its buffer until the last ticket, debug layer 0 errors on the state pattern of 5.2 (repeat with the pure-promotion pattern if it reports a mismatch). |
| T-D9 | Descriptor ring: eight environment regions on two views and two slots recorded back to back with the forced overlap; verify through draws that sample the cubes and read back, not by reading the heap (descriptor contents are opaque). |

Existing suites that must stay green at every commit: the ones above plus `game_primitive_*` and `game_ui_mesh_output_test` built by `Build-GameBootstrap.ps1`.

## 7. Live witnesses (live only, announced under D-0116; never from the static thread)

The A to C set stays the non-regression set (Native1254 in game, title and menus, Status and portraits, character creation, NPC and cloth, velocity Native1191, Bink, area change, Native1262). Rows RD1 to RD9 of the preparation note stand; sharpened or added:

| Row | Action | Expected |
|---|---|---|
| RD1 | `slots=1` against `slots=2`, S1 and S4, 600 frames, one build | identical image on a still scene; `gpu_wait_ms` near 0; `legacy_table_draws` 0; no device removal |
| RD2 | injected present failure and timeout with two frames recorded | single exit; in-flight objects leaked, not released |
| RD3 | bonfire warp and teleport with two frames in flight | debug layer: no use after release; `retirements_pending` and private bytes return to baseline |
| RD4 | Status with portraits, character creation, with `ViewContext` and the single list | views drawn in the same frame; `executes` per frame 1 plus ingress; no flicker; witness files unchanged on a still scene |
| RD5 | Bink then title | movie sampled by the UI in list order; movie planes from the arena classes, no committed buffer per movie frame |
| RB5-1 (new) | a cloth NPC (the Native1184 and Native1185 cloth witnesses) at `legacy-dynamic-streams.flag`, B5a and B5b, 600 frames | same image; no stream resource creation in steady state; GPU time of the frame in B5b not worse than B5a |
| RB5-2 (new) | a cloth NPC leaving the area while two frames are in flight (area change) | no use after release, debug layer 0 errors |
| RD6 to RD9 | unchanged from the preparation note | |

## 8. What ER does (architectural reference)

| ER observation ([review section 2](D3D12-DESIGN-REVIEW.md), [ER-DEEPENING-897](ER-DEEPENING-897.md)) | What to take | What not to take |
|---|---|---|
| E2: a ring of 6 fence tokens, one signalled per frame on the queue; at frame end the CPU waits for the token of frame `N - lag`, lag 2 (`renderer+0xF1C`, only integer writers set 2) | Two frames outstanding: `slots=2` (frame N reuses the slot of N-2, so N-1 and N are in flight) | A ring of 6 tokens is ER's bookkeeping; DS3 keeps its submission tickets and a ring of 2 or 3 slots |
| E2 detail: the wait is skipped for the first 6 frames; a diagnostic flag bounds the spin | Ticket 0 never waits (L5); a bounded wait with a fault on timeout (the existing 5 s) | The diagnostic early return |
| E3: withdrawn resources go to delayed buckets destroyed at frame `F + d + 1` after the lag-bounded wait (safe for `d >= 1` with lag 2) | The invariant "destroy after the completion of the last frame that could record a use"; DS3 expresses it per resource by ticket (built, B1) | A fixed frame delay without the ring wait |
| E12: `Present`, then `Signal`, then `GetCurrentBackBufferIndex` (K-0004) | A fenced presentation event in one ownership domain. The slot ticket after `Present` is a DS3 choice: ER's ring token (E2) is queued in the frame's records and signalled by the worker before its `Present` callback, and E12's post-`Present` signal is not shown to be that token | |
| E1 (K-0013, K-0016, K-0017): a worker consumes the previous frame's ordered records, runs its `Present`, the next publication waits for the worker; threaded mode only (`renderer+0x11C`) | Phase F (R13) builds on the slot ring; job queue depth 1 | The non-threaded branch is not covered by the evidence |
| E4: per-context upload allocators over persistently mapped UPLOAD buffers, blocks tagged with queue class and fence value, recycled when `GetCompletedValue` passes | `UploadArena` (done, B2c), the size classes of cD3 and the rule L3 | |
| E5: updates of retained owner buffers copy from the upload space into the owner's buffer (`CopyBufferRegion`) bracketed by transitions that restore the previous state; frequency and heap type not stated, DEFAULT is an inference | **B5b**: one retained buffer per owner updated by a recorded copy, in place, under frame lag 2 as in ER | The claim that ER's heap type is DEFAULT is an inference; ER's restoring transitions are one pattern, the state handling of 5.2 is verified separately |
| E6: initial data through a temporary COPY-type list on its own queue, waited per call | The ingress context (B3a) is DS3's batched form; no COPY queue until G | The per-call CPU wait |
| E9: heaps set once per list (compute lists only in the evidence); allocator reset after a fence check | Allocator reset only after the slot ticket (L5) | The compute-only observation is not evidence for graphics frame slots |
| E11: selection binds the owner's buffer; collectors copy parameter bytes into transient upload space each preparation | B5: engine owners publish bytes, the frame copies; constants stay transient | |
| Not in the ER evidence | PSO precreation, pipeline library, root CBVs, bindless, enhanced barriers, async compute per frame, `ExecuteIndirect` (all later, on measurement) | |

## 9. Counters and exit checks

### 9.1 Counters (pacing row, added in cD0 and later commits)

`drains_by_site`, `completes_by_site`, `slot_wait_ms`, `waitable_wait_ms`, `constant_chunks_in_use_peak`, `global_chunks_in_use_peak`, `legacy_table_draws` (exists), `depth_recreations`, `lighting_programme_changes`, `resources_created_per_frame`, `descriptor_heaps_created_per_frame`, `dynamic_stream_updates`, `dynamic_stream_bytes`, `dynamic_stream_copies` (B5b), `dynamic_stream_bytes_resident`, arena chunks by size class, `executes` per frame (exists; expect 1 plus ingress after cD3), `retirements_pending` peak (exists), frame percentiles p50, p95, p99, max, `input_to_present_ms`.

### 9.2 Per-commit gates

cD1 to cD4: A to C witness set equal, 0 debug errors, `slots=1` image hashes equal to the previous commit. cD5: no stream resource creation in steady state, hashes equal on the still scene. cD6: RD1 to RD3.

### 9.3 Phase exit (matches the review)

Two frames in flight; the main list's wait no longer in the owner thread's frame time (`gpu_wait_ms` near 0 in S1 and S4); 0 debug errors for 600 frames on every witness; the per-frame `Complete`/`Drain` gone (`drains_by_site` shows only resize, depth change, plane resize, shutdown); `legacy_table_draws` 0; clean quit as Native1262; B5: no per-update GPU object creation. If M5 shows no gain, record it, keep `slots=1` as the default and start R13 on the ring.

## 10. Risks and open points

1. **Hidden single-thread assumptions** (PREP section 6): the global lock order must be written once (`gpu_submissions`, `scene_view_cpu_lock`, `native_offscreen_lock`, `model_storage_lock`, `model_stream_lock`, `cloth_stream_lock`, the draw cache, the retire lock, and in phase F the submission mutex). T-D5 tests the edges that exist in D.
2. **The depth recreation on `clear_stencil` change** is the one remaining full stall; how often it fires is unknown (counter in cD0). If it fires every few frames, give each stencil value its own depth resource instead of recreating one.
3. **Back buffers and slots:** with two slots and two buffers the CPU can wait inside `Present`; R14 (three buffers, waitable object) addresses it; until then M5 may understate the gain.
4. **Debug layer:** the validation layer serialises work; judge timing with `no-debug-layer.flag`, correctness with it on.
5. **Open:** which engine threads call the stream update hooks; whether `Present` may move outside `gpu_submissions` without a change of the engine's frame order; the number of dynamic streams per frame (cD0); the buffer state rule of 5.2 under the debug layer; whether the sampler tables and the 32 descriptor chunks suffice at two slots; how often the lighting programmes change (D10); whether a `Diagnostics()` call at retirement can attribute a message to a frame better than "the frame found in".
6. **R13 and R8b are not designed here**; they start from this ring (PREP sections 2.7 and 2.8).

## 11. Evidence and limits

Static reading of `main` `5be70746`; sources read: `native/renderer/{backend.h,backend.cpp,scene_pass.h,scene_pass.cpp,model_draw.h,model_draw.cpp,model_geometry.h,primitive_pass.cpp}`, `native/frame/{game_bootstrap.cpp,startup_primitive_test.cpp,Build-GameBootstrap.ps1}`, `native/renderer/Test.ps1`; documents: D3D12-PHASES-D-G-PREP, D3D12-PHASES-A-C-PREP (sections 9 and 10), D3D12-DESIGN-REVIEW (sections 2 and 3), ER-DEEPENING-897, I-0068 DISPLAY-SWAPCHAIN-NATIVE (sections 3 and 5). No build, run or measurement. The expected gains are the review's and the M3/M4 tables'; the 1.2 to 2 ms figure is computed from those tables, not measured for two slots. ER statements are the review's and are not extended.

## Amendments by the R13/R14 sheet

The [R13/R14 sheet](D3D12-R13-R14-RENDER-THREAD-PACING-SHEET.md) amends three points of this sheet once the render thread exists (not before): R8 and section 3.2 step 5 (the frame clock is written after the post, as the original does, not after `Present`; section 8.3 there, with witness RT-9); section 5.2 (B5a and B5b both allocate arena ranges on the prepare thread, which is invalid when prepare and record are on different threads; the worker-side variant B5w replaces them under F1, section 3.4 there); G4 (the UI-admission `Diagnostics()` call is removed, X6 there). cD-F of this sheet is cR1 there.

## Amendments by the complement

The [phase D complement](D3D12-PHASE-D-COMPLEMENT.md) amends this sheet: L1 and the slot contents (the movie target, the view targets and the environment cubes are kept until the slot ticket, cD1/cD3); the wait inventory (`Present` is a wait, its own counter, cD0 follow-up; M5 is provisional until the buffer-count variant, cD7/cD8); the closure of risk 2 and D5 (the stencil clear value is never assigned in the game path) and of D10 (the lighting programmes are resolved once); the answer to risk 5 (`Present` does not need `gpu_submissions`); the chunk estimate of A3/A4 (the peak is slots + 1 frames of draws); the game-path guards (`last_serial_`, `startup_serial_` stay at record time); the temporal-resource rules R-T1 to R-T5 for the coming passes; tests T-D10 to T-D14 and witnesses RD10 to RD12.

## Implementation log

**cD1 (2026-10-11, pilot thread, commits `454f6366` and the review fixes, Native1321-1323).**

What changed:
- `FrameRing`/`FrameSlot` (`backend.{h,cpp}`) replace the single `FrameRecorder` and `native_model_inflight_` of `SceneRenderer`. Every recording path goes through `Begin`/`End`: fixture texture uploads, menu views, the Bink movie, the main frame, the startup frame and the scene-work fixture frame.
- A slot holds its draws and the GPU-referenced inputs of complement 2.1(a): the lighting input with its environment cubes, and the movie and view targets (`HoldResource`).
- One slot. `End` waits only when the fence has not passed (it always has, after the existing `Complete`/`Drain`), then releases the slot. The waits and wait-site counters are unchanged.
- Complement 4.4: while a completed slot is released, owners that reach `RetireWhenSubmittedWorkCompletes` are released at once (`ReleaseAfterCompletion`, rule L2 noted on the slot).
- The quarantine of `~SceneRenderer` now covers the ring and the members R7 listed. The menu-view catch no longer releases draws that may be in flight.
- `frames-in-flight.flag` is read and logged (`native_frame_ring`); the ring stays at one slot until cD6.

Tests (`frame_ring_test`, WARP and hardware, debug layer):
- T-D1 under a forced overlap: two gates; `Begin` blocks for the slot it reuses only, and frames 1.. stay pending.
- The release rule of 4.4.
- T-D4 at ring level: a ring quarantined after `End`, or left recording after its `Execute`, releases nothing.

Live (B0 subset):
- Native1321: High Wall, menus, Equipment page, shadows. 0 debug errors, 0 mismatches, exit 0.
- Native1322 (measure): 16.2 fps, prepare 16.1 ms (unchanged).
- Native1323: Bink opening movie and character creation, 0 errors; the close fault at `+0xD12759` is the known one of Reference1250.

Opus review: no defect. Applied: the stronger T-D1 and the one-slot T-D4 case.

Moved:
- the `End` assertion of complement 2.1(b) (T-D11) goes to cD3, where the per-view waits disappear;
- `slot_wait_ms` and `frame_inputs_held_peak` go to cD4/cD6, where a slot can wait.

Open: the SceneRenderer-level T-D4 (a fault hook and the 5 s timeout) is not written.

**cD2 (2026-10-11, pilot threads, commits `724da404` and the review fix `81aa6e0b`, Native1324-1327).**

What changed:
- `ViewContext` replaces `SwapTargetSet`: the per-view resources (G-buffer and composition bindings, targets, depth, lighting targets and views, environment and SSS targets, readback) live in one context for the main view and one per menu view extent. The recording functions use the context `view_` points to; a menu view points it at its own context for its recording, with no member swap.
- P3: the menu view's composition RTV heap is kept with its context.
- Review fix: the former swap also exchanged the presenter's extent. `NativeModelTargets` and `NativeLightingResources` read `ViewWidth/ViewHeight` (the presentation for the main view, the context's own for a menu view); `Close` resets the whole main view context.

Live:
- Native1325 (build before the fix): the character-creation view was refused (784x784 viewport on 784x720 targets, exit 0xD3126607).
- Native1324 and Native1326 (High Wall, portraits, Equipment page, debug layer, both shadows): 0 debug errors, 0 collect and prepare shadow mismatches, exit 0.
- Native1327 (Bink opening movie, character creation): the 784x784 view drawn, 0 debug errors; the close fault is the known one of Native1323.

Opus review: one defect (the view extent), fixed before the merge.

**cD3 (2026-10-11, pilot thread, commits `09bb502d` and the review fixes `10ed83e6`, Native1328-1332).**

What changed:
- One list per frame. The Bink movie, the menu views and the frame record into the slot that is recording (the first of them begins it, `FrameListSlot`); only `RenderNativeModels` and `RenderStartupFrame` submit it, once, and retire the arena once after that `Execute` (rule L3, also for startup frames, which had no `Retire` before). The per-view and per-movie `Execute`/`Complete`/`Drain`/`Diagnostics` are gone; the code's order (movie, views, main, UI) is kept (D-1).
- Arena size classes (A2): chunks of 1, 4, 16 and 64 MB, pooled per class up to 16, 4, 2 and 1; a range above 64 MB has its own chunk, never pooled. `Range` carries its chunk resource and offset; `Allocate` takes an alignment and may skip the zero fill.
- B2d: the movie planes and constant block come from one arena range (512-byte aligned, written whole) instead of the reused upload buffer.
- G3: the witness copies of views and movie are recorded into the list and written once the frame's slot is released.
- D1: each recording that draws env regions copies its cubes into its own range of the view context's heap (slot x 32 recordings x 64 cubes).
- UI: the build's ingress list rides the frame's submission (no `SubmitIngress`); the UI depth-stencil target persists per extent and the slot that records it holds it (P1, D-12); a build admitted without its frame has its uploads submitted before it retires.
- T-D11: `FrameSlot::referenced` (movie and view targets) must be held by the slot before the submission (`FrameRing::RequireHeld`).
- Review fixes: movie plane recreation, a lighting programme change and an offscreen depth recreation are refused while a frame list is open (each releases or rewrites objects at once; none fires in the game path: the movie records first, the programmes resolve once, the stencil clear value never changes).

Tests (`frame_ring_test`, `startup_primitive_test`, WARP and hardware, debug layer): T-D11; arena classes (4 MB and 16 MB chunks pooled and reused, a 64 MB + 1 range not pooled, a non power of two alignment refused).

Live:
- Native1328 (High Wall, Load Game portraits, Equipment page, both shadows): exit 0, 0 debug errors, 0 collect and prepare shadow mismatches; `executes` per frame 1.00 (title, menus and game).
- Native1329 (measure, S1) against Native1322: 16.3 -> 17.0 frames/s, render 4.9 -> 4.3 ms, collect 27.9 -> 26.8, prepare 16.0 -> 15.4, GPU wait 1.2 ms unchanged.
- Native1330 (Bink and character creation): the movie witness frames 240 and 2400 are identical to the byte to Native1327; the 784x784 view drawn; the close fault is the known one of Native1323.
- Native1331 and Native1332 (the build with the review fixes; gate and character creation): exit 0, 0 debug errors, 0 shadow mismatches; the movie witness frame 240 is again identical to the byte; the same known close fault.

What a reader of the trace must know:
- `gpu_wait_ms` now includes the GPU time of the movie and the views; `record_ms` still starts in `RenderNativeModels` (view recording falls in the render phase); `primitive_admission_ms[3]` no longer contains a submission.
- The `WaitOffscreen` and `WaitMovie` sites only label `Begin` now (0 drains and completes).
- CPU-time debug messages of the movie and views are read by the next `Diagnostics()` call (the UI admission's or the frame's), so they are reported under that name. cD4 moves `Diagnostics()` to slot retirement (X6).
- `arena_kb_per_frame` now counts alignment padding.

Opus review: no blocker; two important points fixed (the open-list guards, the pixel files after the release), the new stencil-initialisation reset at admission restored, the rest noted above.

**cD4 (2026-10-11/12, pilot thread, commits `d85910ce`, `7c7d238b` and the review fixes `46948a4f`, Native1333-1336).**

What changed (in the R13 forms, D-12):
- Release-time work: `FrameSlot::completions` run in order when the slot is released (after its ticket), before its draws and kept objects go; a failing completion still releases the slot and its exception is rethrown. `WaitIdle` waits and releases every slot before rethrowing.
- G1, G2: the back-buffer and velocity/motion readbacks are per slot (`SlotReadbacks`, with their footprints), created on first use at the presentation extent; the frame's completion keeps the buffers it reads.
- G3, G4, X6: the witness files, the hash, the velocity statistics and `Diagnostics()` run in the frame's completion; the UI admission no longer reads the debug layer.
- R6, X3: completed frames go through a locked channel (`TakeCompleted`, `RequestVelocityPicture`); a frame released at its own End (one slot) fills its receipt; errors of any released frame end the frame function.
- R5: `WaitFrameSlot` waits for the slot the next frame reuses before `Clear()` takes `gpu_submissions`, on the ring's own event (`CompleteOn`), and `slot_wait_ms` joins `native_frame_audit`; the release itself stays in `Begin`, under the lock.
- D4: the scene-colour copy is held by the slot that copies into it.

Not done here, on purpose:
- Ending the `gpu_submissions` scope before `Execute` (complement 4.3) waits for cR1. `Close`, `CloseNativeModelStreams` and `TextureIngress::Close` drain on the queue's shared completion event under that lock; moving `Clear()`'s `Complete`/`Drain` outside it would reopen the lost-wake-up race of the R13 sheet (5.1).
- The X10 receipt cell (the check consumes the previous job's receipt) belongs to cR3; today the receipt of a frame released at its own End carries its results.
- T-D3 at the renderer level (an error injected while recording N+1) and T-D9 (env descriptor ring by sampling draws) are not written; the ring-level T-D3 and T-D5 are.

Tests (`frame_ring_test`, WARP and hardware, debug layer): completions run at the release and not at End with two slots, in order, before the kept objects go; `WaitNext` waits on its own event while another thread waits on the shared one for a later ticket (both return), it measures the wait and leaves the release to `Begin`; a failing completion releases its slot and `WaitIdle` releases every slot before rethrowing.

Live:
- Native1333 (High Wall, portraits, Equipment, both shadows): exit 0, 0 debug errors, 0 shadow mismatches, 63 velocity rows, `slot_wait_ms` 0.
- Native1334 (measure, S1): 17.4 frames/s, render 3.9 ms (Native1329: 17.0, 4.3), GPU wait 1.2 ms.
- Native1335 (Bink, character creation): the movie witness identical to the byte to Native1327/1330; the known close fault.
- Native1336 (gate, build with the review fixes): exit 0, 0 debug errors, 0 shadow mismatches, 65 velocity rows and the 12 velocity pictures written at completion.

Opus review: no blocker. Fixed: velocity footprints per slot, `WaitIdle` on every slot, `WaitNext` without the release, the event reset. Noted for cD6/cD10: `Diagnostics()` runs before the slot's own objects are released, so messages raised by those releases reach the next frame; `Close` drops completions still pending (cD10 waits first).

**cD5 (2026-10-12, pilot thread, commits `26218c42`, the review fixes `d1ce9d87` and `77b9d1b5`, Native1337-1341): B5 in a pooled form, a deliberate deviation from cD5a/cD5b.**

Measured first (D-4): about 4.5 dynamic stream updates and 46 KB per frame at S1 (Native1331), up to 10.5 at Firelink with cloth NPCs (Native1339), each one a committed UPLOAD buffer created on an engine thread.

What changed:
- `UploadVersionPool` (backend): an engine thread publishes an immutable `UploadVersion` of a physical model stream or a cloth copy into a persistently mapped UPLOAD buffer taken from a pool keyed by a 4 KB size class (64 free per class, 64 MB in all), created only when the pool has none (warm-up, a new size).
- `ModelGeometryBinding::stream_owner`: the binding holds the version (`selected.streams` for physical streams, the `NativeClothStream` for cloth), and a dynamic stream without its owner is refused. A version's buffer returns to the pool only when its last holder is gone, through `RetireWhenSubmittedWorkCompletes` (at once inside a completed slot's release); a returner that cannot be queued is kept to process exit.
- `legacy-dynamic-streams.flag` (runner `-LegacyDynamicStreams`) keeps the former buffer per update.
- `native_frame_audit` gains `stream_buffers_created` (the gate: 0 in steady state) and `stream_buffers_free`.

Why not cD5a or cD5b: cD5a copies every selected stream into the arena every frame and allocates arena ranges at prepare time, which the R13 sheet (3.4) invalidates once prepare and record are on different threads (hence its B5w). The pooled form copies each update once, on the engine thread, touches no arena and no device after warm-up, and holds under every threading variant (V2): B5w is not needed, and cR3 no longer needs `legacy-dynamic-streams.flag` once a test publishes and records on different threads (T-R8). Vertex fetch stays in UPLOAD memory, as with cD5a; cD5b (one DEFAULT buffer per owner, recorded copy) stays the option for when GPU time matters.

Amendment to rule L1 and to R13 V2: a pooled stream buffer is protected by its version, not by a reference to the resource; whoever draws a dynamic stream holds its version.

Tests (`frame_ring_test`, WARP and hardware): a version held by a pending slot is never reused; released versions return after completion; 20 updates in steady state create nothing; another size takes another buffer; a reset pool takes nothing back.

Live:
- Native1337 (High Wall, debug layer, both shadows): exit 0, 0 debug errors, 0 shadow mismatches; 4.5 stream updates per frame with 0 resources created (4.5 before).
- Native1338 (Firelink, cloth NPCs, phantom) against Native1339 (the same with `-LegacyDynamicStreams`): both exit 0, 0 errors, 0 shadow mismatches; up to 10.5 updates per frame, 0 creations pooled against 10.5 legacy; the capes drawn.
- Native1340 (gate, build with the review fixes): fail-fast 0xC0000409 at the first audit row (frame 121): the new stream cell overflowed the 48-character audit buffer of `sprintf_s`; written in two cells (`77b9d1b5`).
- Native1341 (gate, fixed build): exit 0, 0 debug errors, 0 shadow mismatches; `stream_buffers_created` 0 in every window after warm-up (69 buffers created in all, 18 to 69 free).

Not done: T-D8 in full (concurrent publish and destroy from several threads, slots 1 and 3, the drawn data compared with the version of each frame), RB5-2 (a cloth NPC leaving the area with two frames in flight, for cD6).

Opus review: no blocker. Fixed: the owner required by the binding, the gate counter, the class cap raised to 64, the destructor's failure paths, the device stored at Publish.

**cD6 and cD7 (2026-10-12, pilot thread, commits `e991ba12` and the review fixes `b46f89bf`, Native1342-1350).**

cD6, what changed:
- `SceneRenderer::SetFrameSlots` (before `Start`) opens the ring with two slots when `frames-in-flight.flag` is present (runners `-FramesInFlight`; `Run-PrepareGate` names such a series `-slots2`). With more than one slot the frame and startup paths submit without the completion wait and take the slot ticket from `DeviceQueues::Signal()` after `Present`; at one slot nothing changes.
- Review fixes: `Close` waits for and releases the frames still in flight (their results stay in the channel; `native_close_frames`); a lighting programme change waits for the ring; the slot count is logged from the renderer after `Start`; `slot_release_ms` joins `native_frame_audit`.
- Test (`startup_primitive_test`, WARP and hardware): two slots under a forced overlap (frame 4 records while frame 3 is gated on the queue), no wait or drain at the startup site, results two frames late (frame N's at frame N+2's `Begin`), the slot count fixed once started, `Close` releasing frames 5 and 6.

Live, two slots (debug layer, both shadows):
- Native1342 (High Wall, portraits, Equipment) and Native1348 (fixed build): exit 0, 0 debug errors, 0 shadow mismatches; 1 execute and 0 drains and completes per frame; `draws_global_legacy` 0; `global_chunks_peak` 22 (15 at one slot), `constant_chunks_peak` 9 (6).
- Native1343 (Firelink, cloth NPCs, area streaming, phantom): exit 0, 0 errors, 0 mismatches (RB5-2 in part: the area streams with two frames in flight).
- Native1344 (Bink, character creation): 0 errors; the movie witness identical to the byte to Native1327; the known close fault.
- Native1347 (one slot, fixed build): exit 0, 0 errors, 0 mismatches.
- Not reached: `native_close_frames` (the quit path ends the process before `Close`, as in every run since at least Native1336); at two slots the debug-layer messages of the last two frames before quit are therefore not read (cD10 and the phase F exit semantics D-10).

cD7, the M5 run (S1, no debug layer, same build):

| Run | Slots | fps | render ms | record ms | GPU wait ms | slot release ms |
|---|---|---|---|---|---|---|
| Native1346 | 1 | 17.5 | 4.25 | 0.59 | 1.20 | - |
| Native1345 | 2 | 17.2 | 7.4 | 4.9 | 0.07 | - |
| Native1350 | 1 | 17.5 | 4.26 | 0.59 | 1.2 | 0.006 |
| Native1349 | 2 | 17.6 | 6.98 | 4.83 | 0.07 | 3.9 to 4.2 |

Reading: two slots remove the 1.2 ms GPU wait but move the release of frame N-2's draws (about 4 ms of destructors and chunk returns) into the next frame's `Begin`, inside `record_ms`; at one slot the same release happens when `Clear()`'s frame input dies, outside the timed phases. Frame rate is unchanged (17.2 to 17.6 against 17.5): the frame is bound by the engine thread (collect 24 to 27 ms, prepare 15 to 16 ms), not by the GPU. Verdict (D-5, sheet 9.3): **the default stays one slot**; `frames-in-flight.flag` keeps two slots available, and R13 starts on the ring. Provisional until RD12 (three back buffers and the waitable object, cD8), as the complement requires. Exit check 9.3: two frames in flight work with 0 debug errors and no per-frame `Complete`/`Drain`; the gain is not there yet.

**cD8 and cD10 in part (2026-10-12, pilot thread, commits `33ee268b` and the review fixes `5b3f7859`, Native1351-1356).**

cD8 (R14a-b), what changed:
- `FramePresenter` takes its back-buffer count (2 to 4, buffers in a vector) and an optional frame-latency waitable object (`SetMaximumFrameLatency` = the slot count); `SceneRenderer::SetPresentation` sets them before `Start`. Deviation from the sheet, on purpose: the new chain is opt-in under `waitable-present.flag` (three buffers, runner `-WaitablePresent`) rather than the default with a `legacy-present.flag`, because the M5 verdict keeps the former behaviour as default; the buffer count does not follow the display mode yet (the descriptor's `+0x28` is not read natively).
- `WaitFrameSlot` waits on the latency object before the slot wait, outside `gpu_submissions`; review fixes: not alertable (each frame consumes one count), bounded at 100 ms with `waitable_timeouts`, skipped while the window is hidden or minimized (a hidden window retires no present: Native1351 and Native1353 waited 1 s per frame through the quit).
- `native_frame_audit` gains `waitable_wait_ms`, `present_call_ms` (the `Present` calls alone, W2) and `waitable_timeouts`.
- Test (`startup_primitive_test`): three buffers with the waitable object at two slots, the presentation fixed once started.

cD10, part done: `Resize` refuses an open frame list, then waits for and releases every slot before its drain. Not done: the single `WaitIdle`/`CollectAll` and the release order of the bootstrap close paths (M14), RD8 and the renderer-level T-D4; the game's quit never reaches `Close` (the process ends first), so these belong to the phase F exit semantics (D-10).

RD12 (S1, no debug layer, three buffers and the waitable object):

| Run | Slots | fps | render ms | present ms | waitable wait ms |
|---|---|---|---|---|---|
| Native1353 | 1 | 17.4 | 4.4 | 0.60 | 0.001 |
| Native1354 | 2 | 17.5 | 7.2 | 0.47 | 0.001 |
| Native1356 | 2 (review fixes) | 17.3 | 7.3 | 0.47 (`present_call_ms` 0.2) | 0.001, 0 timeouts |

Reading: the presentation does not limit the frame (the latency object never waits in steady state, `Present` itself costs under a millisecond); the engine thread does (collect and prepare, about 40 ms of a 57 ms frame). The M5 verdict stands, no longer provisional: **one slot and the two-buffer chain stay the default**; two slots and the waitable chain remain available under their flags, and they become useful once R13 moves the render off the engine thread and the frame gets shorter.

Live with the waitable chain (debug layer): Native1351 (one slot) and Native1352 (two slots): exit 0, 0 debug errors, 0 shadow mismatches. Native1355 (two slots, review fixes): the same, 0 timeouts, and the quit no longer waits (its last pacing window 4.2 s against 121.7 s in Native1351).

**Phase D exit (sheet 9.3), 2026-10-12.** Met: two frames in flight work with 0 debug errors in every witness that ran (High Wall, Firelink, character creation, menus); no per-frame `Complete`/`Drain` at two slots (`drains_by_site` shows none in the frame path); `draws_global_legacy` 0; B5: no buffer creation per stream update. Not met: a frame-rate gain (the frame is CPU-bound; M5 and RD12); not done and handed on: cD9 (the present interval and tearing need the interval object, row RP-1 of the R13 sheet, with the pacer cR4), cD10's close order (phase F, D-10), T-D9, T-D8 in full, RD8/RD9.
