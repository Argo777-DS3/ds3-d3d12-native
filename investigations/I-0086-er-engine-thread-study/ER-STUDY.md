# I-0086: Elden Ring's engine thread and frame preparation, against the native engine thread (third tranche preparation)

Study thread of 2026-10-22 (Opus, with five Sonnet 5.5 workers), prepared while the night endurance thread (I-0085) owned the
PC. Rule: D-0195 with Thomas's nuance (2026-10-21): copy ER's architecture **without its known defects** (I-0080). Target of
the third tranche « fil moteur vers 120 images/s »: 8.3 ms a frame for the whole engine thread, DS3's own update included.

## How to read this document

- **ER-DATA**: read directly in the ER 2.7.1 image's data (`eldenring_2.7.1.0_25080141_1A354710.exe`, tables decoded by
  script). **ER-ASM**: read in a targeted disassembly (`llvm-objdump` on an address range, plus a reference scan for
  calls, RIP-relative operands and pointers): light enough to run beside the endurance test, which forbade Ghidra and heavy
  work until its gate. **ER-DEC**: read in the decompiler (headless Ghidra 12.1.3, `-readOnly -noanalysis`, only after the
  PC was free). **checked** marks what Opus read itself; the rest is a worker's reading, non-canonical until a design relies
  on it (then check it first). Scripts, decoded tables and the workers' notes stay in the session scratchpad (D-0142); a
  design that relies on an address re-reads it.
- **DS3-DATA**: the DS3 1.15.2 tables, decoded by the same script as ER's; the script reproduced ENGINE-STEP-THREAD-MAP's
  table row for row (the decoder's control). The two decoders are kept here ([Get-TimelineSteps.ps1](Get-TimelineSteps.ps1),
  [Get-TimelineRows.ps1](Get-TimelineRows.ps1)), so the tables can be produced again from either image.
- **MEASURED**: a number already on record (run id given); nothing was run for this study.
- **DEDUCED**: an inference; never a decompiled fact. **OURS**: the native code at `main` 95aec11c (file:line).

## Summary

| Topic | ER 2.7.1 | ER's defect (not to copy) | Ours (m45) | Plan item |
|---|---|---|---|---|
| Frame timeline | DS3's FD4 timeline enlarged: 169 steps, 137 rows x 6 lanes; much more of the update spread over the lanes; the draw rows on the main thread alone | the main thread polls the row barrier under a critical section instead of waiting | DS3's own timeline (97 steps, 88 rows); our work inside `DrawStep` | (context) |
| Where the draw work runs | main thread for the draw rows; GX pool (N-1 workers, caller helps) for traversal, entity draw, recording; a render thread one frame behind for playback and Present | pool-wide joins | engine thread + admission pool (7) + render worker + recording pool (3) | P2 |
| UI (Scaleform) | Advance and Capture as a task on one worker; the HAL Display (every movie, every frame, from the retained tree) queued as a closure in the frame's command list and **played on the render thread** | the global Scaleform lock held over the whole UI draw; Advance serialised on one worker and joined in the same call | the Scaleform display and our HAL on the engine thread, 1.28 ms | **P1** |
| Effects | the update spread over the GX pool (per effect, then two item lists, three barriers), with an update decimation of effects; effect lights handed to the scene before the collect; one scene entity records per-node callbacks | task 0 polls each barrier with `Sleep(0)`; a configuration can force one thread | our transcription on one thread, 0.38 ms (the original 0.16 ms) | P4 |
| Per-entity draw | rebuilt every frame and view, no prepared draw kept; per-thread bump allocators, no general heap call | the cross-fade doubles the work; bone palettes copied twice | records kept across frames, but allocation, zeroing and serial pieces on the engine thread | P2, P3, P5 |
| Per-view constants | one 0x7E0 view block per pass entry, re-uploaded only when changed | none found | scene and view blocks packed per programme (about 1,400 a collect while moving) | P6 |
| Frame memory | linear frame arena (72 MiB, spin-locked bump), per-thread chunks without lock, per-thread heap | the arena spin lock, a sort temp buffer from the CRT | general heap: `RtlAllocateHeap` 4.5 % of the engine thread (0.75 ms) | P3 |
| Limiter, frame lag | limiter at the tick's start, pure QPC spin when focused; frame-lag wait polls the fence with `Sleep(0)` | both spins (I-0080) | DS3's limiter spins: 29-31 % of the engine thread at the 60 cap | P7 |
| Parameter updates | 28 `GXUpdateJob<T>` jobs in `RenderingSystemUpdate` (row 73), serial on the main thread by default | a parallel path compiled but never enabled | the collect's set-up, lights, offscreen views serial on the engine thread (2.7 ms) | P2 |

## 1. The frame timeline: ER spreads the update, keeps the draw rows on the main thread

**ER-DATA and ER-ASM, checked.** The builder 140EB4721 loops over 169 step records of 0x20 bytes at 142C04D20
(`{id, name, lane mask low, lane mask high, c, d}`, through 140EB6CB0), then over 137 rows of six `int` lanes at 142C06240
(through 140EB7020): the same two-table shape as DS3's 97 steps and 88 rows (1429BA580, 1429BB1A0, ENGINE-STEP-THREAD-MAP
§2). Six lanes in both. The mask's low word is the set of lanes a step may use (bit i = lane i).

**ER-ASM, workers (W1).** A row cell is `{step, kind}`; a row ends when its blocking cells are done; a step whose high mask
word is 1 has every lane of its row pull tasks from one shared queue (dynamic balancing); without it a multi-lane mask
collapses to the row's lowest lane (140EB4F40). `d` sizes the step's task ring (INFERRED). `Manager::Run` 14268EE50 hands
one lane job to each of six `FD4JobWorker` threads and runs lane 0 on the calling thread, the main thread. Lanes 1-5 wait on
events between rows; **lane 0 does not wait: it polls the row state machine under a critical section** (the wait is
skipped for lane 0 at 1426D751C; INFERRED that the main thread spins at each row barrier).

| | DS3 1.15.2 | ER 2.7.1 |
|---|---|---|
| steps, rows | 97, 88 | 169, 137 |
| rows using lanes 1-5 | 12 | 33 |
| steps on all six lanes | `ChrIns_PreBehavior`, `_PrePhysics`, `_PreCloth`, `_PostPhysics`, `HavokClothUpdate_Pre_ClothModelIns`, `ChrIns_UpdateDraw` (4), `HavokBehavior_SceneModifier` (5) | the same character steps, plus `EmkSystem_ConditionStatus`, `WorldChrMan_CalcOmissionLevel`, `ChrIns_NaviCache`, `ChrIns_AILogic`, `ChrIns_GarbageCollection` (5), `LocationUpdate_PrePhysics_Parallel`, `LocationUpdate_PostCloth_Parallel`, `DmgMan_ShapeCast`, `Grass_ResourceLoad` (4), `ChrIns_CalcUpdateInfo` (3) |
| background steps beside lane 0 | `FileStep`, three Scaleform steps, silhouettes, `UpdateLodIns` | `ResStep`, the AI formation (rows 104 to 129, **spanning the draw rows**), `HavokAi_World`, garbage collection begin and end (the end beside `DrawStep`), model creation, backread requests, `Geom_UpdateDraw`, grass, `BeginShiftWorldPosition` |
| the draw rows | 79 `DrawStep` alone, then `DrawBegin` ... `FrameEnd` alone | 125 `DrawStep` (with `AiEndCollectGabage` on lane 1), 126 `DrawBegin`, 127 `GameSceneDraw`, 128 `AdhocDraw`, 129 `DrawEnd` (with the AI formation's end), 130 `Draw_Post`, 133 `Flip`, 134 `DelayDeleteStep`, 136 `FrameEnd` |
| Scaleform | `ScaleformStep` on lane 1 (row 16), `ScaleformCapture`/`CaptureFE` on lane 1 (rows 52-53) | one `ScaleformStep` on lane 0 (row 115) that only arms an update flag (§3) |
| rendering parameters | `GraphicsStep` with `UpdateLodIns` on lanes 1-3 (row 72) | `RenderingSystemUpdate` alone (row 73, between `HavokWorldUpdate_Pre` and `_Post`): the GX rendering system's update, inline on the main thread (§2) |

**`CS::CSDrawStep` binds to five rows, checked** (ER-ASM 140B155E0, the registration wrapper 140B142B0 with the step id in
`edx`): `FrameBegin` -> vt+0x98 (140B15980); `DrawBegin` 0x9A -> vt+0xC0 (140B15E80, the renderer's `BeginFrame` 1419E77F0);
`AdhocDraw` 0x9C -> vt+0xD8 (140B15B40, the UI, §3); `DrawEnd` 0x9D -> vt+0xC8 (140B16840, `EndFrame` 1419E82C0); `Flip`
0xA4 -> vt+0xD0 (140B169A0). The class's own state `STEP_Update` (140B15780) is the `DrawStep` row 125 (INFERRED from the
class name): it calls the GUI manager (140CC9420), the effects' update (`CSSfxImp::Update` 140D92D30, §4) and a view set-up
(140B53230).

**The main thread's tick (W1, ER-ASM).** The main loop 140C915D0 raises the main thread to HIGHEST priority, then repeats
the tick 140DEC1E0: frame counter, message pump, **the limiter** (`CSFlipperImp`, 140E84830, §6), dt, then the whole
timeline (`TaskSystem::Run` -> `Manager::Run`), then a few post-frame objects (the load balancer 140E0F220 among them).
With threaded rendering, the main thread posts frame N's command list at `DrawEnd` (row 129), finishes rows 130-136, and
runs frame N+1's rows 0-128 while `GXRenderThread` plays and presents frame N; the first synchronisation is in frame N+1's
`EndFrame` (§6).

DEDUCED: (1) in both engines the draw rows run on the main thread alone, so their parallelism comes from the GX pool and the
render thread, and the main thread's time in those rows is the critical path; (2) ER moved work out of the frame's serial
path where it could (AI, navigation, omission levels, collection, location updates, shape casts, grass loading) and lets
background work span the draw rows; (3) DS3's timeline is fixed in its image; DS3's own update (about 4.7 ms a frame at m45,
I-0078) is not the native backend's to re-lane. What the native backend controls is its own work inside `DrawStep` and DS3
functions it hooks (§7, P9).

## 2. ER's draw rows and the per-view scene render

**ER-ASM (W2), BeginFrame** (`DrawBegin`): skip flags (`g+0x761`, `g+0x762`); on a mode change only, a wait for the render
thread; `g+0x11C` (threaded this frame) = `g+0x11D` (requested); the frame counter; affinity upkeep for the pool and render
threads when the masks changed; a recording context popped from a free stack; when threaded, the frame's begin closures
queued to the render thread. **EndFrame** (`DrawEnd`): the main context's recording handed over, a recording context for the
present blit (1419ED180), **a join of the GX pool** (141AF38A0 at 1419E8470, the caller helping, I-0077 §6), the frame-lag
wait (§6), then the tail 1419EC1F0: wait for the previous render job (141AF5360, which pumps messages), flip `g+0x708`,
post the list to the render thread. Not threaded, the list is sorted and played inline (1419E85F0).

**Parameter updates (W2, ER-ASM).** The 28 `GXBS::Tasks::GXUpdateJob<T>` classes (lights, light sets, shadows, fog, SSAO,
SSR, tone, glow, colour grading, motion blur, water, wet, environment maps, ...) are persistent objects built once by
`GXSR::GXEnvironmentManager`; its update 141A95D20 runs them with a ticket job (`GXEnvironmentUpdateJob`, Run 141A93FA0) on
the GX pool **only when the byte 1447F5248 is set, which nothing in the image writes**; otherwise serially, on the main
thread, in `RenderingSystemUpdate` (row 73), after a pool join (141A0AE30). Checked in the decompiler (ER-DEC 141A95D20:
the ticket loop runs inline when the byte is 0; its only reference is that read; 141A0AE30 calls it after the pool join). DEDUCED: ER does its rendering-parameter work
early in the frame, outside the draw rows, and serial by default.

**The per-view scene render: in the `AdhocDraw` row, on the main thread.** The composite scene `GXSR::GXSgCompositeScene` (vtable 142F0E840,
checked: +0x38 = 141B660D0 renders one view, +0x50 = 141A0CB60 jumps to the prepare 141A06B10, whose end resets the frame
arena, W5) is reached only by virtual calls (the first reading found no direct caller), and no `CSDrawStep`
handler is bound to `GameSceneDraw` (0x9B). I-0077's reading (the traversal, entity draw jobs, sort and chunk recording on
the draw-step thread with the pool, the render thread playing the result one frame behind) stands; its placement
"between BeginFrame and EndFrame" was INFERRED until the hunt below. ER-DEC (W4, W6): the composite's render 141B660D0 runs the traversal on its
caller's thread with the pool and, with the default data, calls its own record slot inline (141B6639x); only in the deferred
mode does it queue record and submit closures (141B66440, 141B665A0) to the render thread. Who calls it was narrowed (W6,
ER-DEC): composites are built only by `GXSR::GXSgSceneFactory`, used from three places: the menu model previews
(`CS::CSMenuAsmModelRend` and its kin), debug viewer code, and **`CS::StageRend`**, the game scene (constructor 140CCA7E0,
0x7A0 bytes), built once by `RendMan` (`[143D7F130]`, `CS::RendManImp`) at `RendMan+0x10`, which stores the composite at
`StageRend+0x398` and appends it to a scene list at `RendMan+8` (141ADA830). **Found, checked by Opus (ER-ASM):** the `AdhocDraw`
handler 140B15B40 calls `RendMan::140CC8850`, which updates the stage (`StageRend` 140CCC080), stores the frame context at
`g+0x30` and calls the scene list's render-all 141ADA8D0 (`RendMan+8`, the call at 140CC88F1): it gathers the registered
scenes, sorts them (141ADA540), prepares each (vt+0x18, then vt+8), **calls each scene's vt+0x38 in order** (141ADAB9C: the
composite's render, traversal and inline record) and writes the scene count into the current frame list's key
(`g+0x6E8[g+0x708]+0x38`). So ER's per-frame scene render runs **on the main thread in the `AdhocDraw` row (128)**, between
`BeginFrame` (row 126) and `EndFrame` (row 129), helped by the GX pool: I-0077's placement holds. The `DrawStep` row's
`RendMan::140CC9420` only updates the stage's views (`StageRend` 140CCCDD0, the composite's vt+0x18) and posts nothing.

**The threaded-render switch (W2).** `g+0x11D` is written once, at the end of renderer init, from `g+0x1E4` (1419EBF4E),
which the init descriptor takes from a graphics-configuration byte (`config+0xE2`, 140CA5500); its shipped default was not
found statically. `DAT_1447F59B9` (recording deferred to the render thread) is set in 141A09100 only when the second command
list exists.

## 3. ER's UI: Advance and Capture on a pool, Display on the render thread

**ER-ASM (W3), the Display closure checked by Opus.**
- `ScaleformStep` (row 115) only arms an update flag (`system+0x1E19`, 140D7BAC0).
- `AdhocDraw` (row 128) runs the GUI manager's update (140CC9EB0 -> `CSScaleformSystem::Update` 140D7BB40 -> 140D74270):
  with the asynchronous switch (byte 143B412E0 = 1 in the image, no writer found) the Advance of every movie (movie vt+0xC0,
  140D738C0) and the Capture (`Capture(onChangeOnly=true)`, movie vt+0xC8, 140D726F0) run as one task (vtable 142BC04C8, Run
  140D79E10) on `EzWorkPool_LOW_Single_Cluster0_withoutCore0` (two threads), not on the main thread.
- A few GUI calls later the main thread **waits for that task without a timeout** (`PrepareRender` 140D7A770 -> 140E83030),
  takes the movies captured this frame (flag `movie+0x80`, at most 32, sorted by `movie+0x20`) and **queues one closure,
  140D7F4D0, into the frame's command list** `g+0x6E8[g+0x708]` through 141AEC810 with the automatic key (checked,
  140D7AB63..AB8E). The closure's arrays come from the list's own arena, so they live until the render thread has played it.
- The closure takes Scaleform's global lock, binds the engine's final colour target and depth with explicit barriers
  (the same pair the other GUI layers draw on: the UI is redrawn on it every frame, no persistent UI target), runs the HAL's
  `BeginFrame`, then per movie `NextCapture` (141159620) and the HAL's `Display` (14115EC50), then `EndFrame` (140D7F210,
  140D73450, 140D7F380). ER-DEC (W3's Ghidra pass): `NextCapture` is true for any movie that has ever had a snapshot, and
  `Display` redraws the retained render tree on every call; only the tree's dirty propagation is skipped when nothing
  changed (the assembly reading "Display skipped without a new snapshot" was wrong). The closure is played by the list
  player (type-2 record: flush the pending batch, call the closure), which the render thread loop 141AF5180 reaches through
  1419ECCA0 (checked: 141AF52BB calls 1419ECCA0, which appends its frame begin and end closures, sorts and plays the list);
  not threaded, `EndFrame` plays it inline.
- ER-DEC: the Advance and Capture task runs on **one** worker (pool 6's pattern gives one thread, at a low priority; the "2"
  of its descriptor is the priority class, not a count). The main thread's work between the post and the join, in the same
  `AdhocDraw` call, is **the whole scene render** (140CC8850 renders every registered scene, §2) and the GUI's 2D layers
  (140CC8790): the UI's Advance and Capture overlap the scene render, and the join (140CC8340 -> 140D7A770) comes after it.
- The HAL is Scaleform's own D3D12 HAL (vtable 142C09890), told the render thread's id at init (140D7A08E); it records its own
  command lists and submits them per scene on the engine's queue (INFERRED for the queue's identity).
- `CS::CSScaleformThreadCommandQueue` (vtable 142BC0148) executes a pushed command at once on the calling thread: no queue,
  no thread of its own.

**DS3, the same design in D3D11** (MENUS-MAP: `FUN_140E5B940`): the UI frame's begin and each movie's `NextCapture` and
`Display` (`FUN_140E52EE0`) are queued on GX's command queue `[GX+0x398+[GX+0x3B8]*8]` when the frame context allows it,
under the movie list lock `system+0xC38` and the movies' own locks. **OURS**: `GameUiSession::PrepareFrame`
(`game_ui_session.cpp:1001-1045`) does the selection under `manager+0xC38`, then calls `E52EE0` for each movie on the engine
thread, with our native HAL behind it (1.28 ms of the 1.35 ms UI prepare at m45, run 93).

**ER's defects here** (DEDUCED from the structure): the global Scaleform lock is held over the whole UI draw on the render
thread while the next frame's Capture needs it; the Advance and Capture of every movie are serialised on one low-priority
worker (the main thread waits for them only if they outlast the scene render); a failed gate (140B1B140) skips the join
and leaves the task in flight to the next frame.

## 4. ER's effects: the update spread over the GX pool

**ER-ASM (W4), the dispatch and the fork-join checked by Opus.** `CSDrawStep::STEP_Update` (row 125) calls
`CSSfxImp::Update` 140D92D30, which calls the scene control's update (`GXFFX::GXFfxSceneCtrl` vtable 14305D5E8, slot 10 =
141CA5E70):
- sub-system updates, among them the VFX effector receivers as a `GXReceiverUpdateJob` over the GX pool (141CB24F0);
- **the main update, path A** (141CA8B80, chosen at 141CA5F8E when 143C2A059 is set, 1 in the image, and the configuration
  byte `IsProceedModelSingleThread` 1447FA217 is clear; its default is 0, `IsProceedMultiThread` defaults to 1, ER-DEC in
  the `CSSfxImp` constructor 140D8ED70): first, serially, each effect is added to the work list with **an update
  decimation**: an effect may skip frames and accumulate its dt (state at `effect+0x30`, thresholds at `updater+0xF4..0xFC`,
  141C9F9C0); then a stack `GXFFX::GXFfxUpdater<I>` job (Run 141CA37A0) is run by **`141A74440(updater, n, n, wait=1)`**
  (checked, 141CA8D00): `min(effects, pool)` tasks, the caller helping. ER-DEC (W4's Ghidra pass): four phases and three
  barriers: (A) per effect by ticket, its emitters then its node chain updated (1420DAAF0), each task writing the finer
  items into its own vectors; barrier; task 0 merges the vectors serially; (B) the update list by ticket; barrier; (C) the
  proceed list by ticket; barrier. At each barrier task 0 polls the counter with `Sleep(0)` and the others wait on an event;
- path B (141CA8530): the same per-effect work ticketed over the effects without phases, or a plain serial loop;
- then the effect lights handed to the scene (141A0A8E0), before the frame's collect.

The effects reach the draw as **one scene entity** (`TGXSgEntity<GXFfxSceneCtrl>`, Draw 141CA3B90) whose draw records, per
node, a callback with the **live node pointer** and a per-frame block of view constants (141CDB7E0 -> 141AD35B0); the
callback (141CDC210) calls the node's draw at recording time, from live particle state. ER-DEC: with the default data the
composite records inline in the frame (141B660D0 calls its record slot at 141B6639x), so the next frame's update cannot
overlap that recording; in the deferred mode (`DAT_1447F59B9`) a second record pass is queued to the render thread, and
whether effect records fall into it is open. GPU-simulated VFX exist (`GXVfxGpuTask*`).

**DS3**: the original updates its effect lists with `GXFfxUpdateJob`/`GXFfxUpdateRootJob` jobs (RTTI) and measured 0.16 ms
a frame at m45 (I-0078 original-04, E6E8D6). **OURS**: `NativeSceneCtrlUpdate` (`21_effects.inc:688`) runs our transcription
`effect_system::Update` on one thread (`effect_system.cpp:440`: "slot 8 runs on one thread"), 0.38 ms at m45 (run 89), plus
the effects record 0.56 ms inside the collect.

## 5. ER's per-entity draw, instance data, view constants and frame memory

**ER-ASM (W5), the allocator fast path checked by Opus.**
- **The entity draw is rebuilt every frame and view** (`CSModelDispEntity` vt+8 140B31550 -> `CSFD4ModelItem` vt+0x90
  140B71DA0 -> 140B73FD0 -> 141AB4280 -> 141AB9D60 -> 141AB6900): a once-per-frame visibility guard (`+0x28C` against the
  frame counter), LOD and fade per view, at most eight owners, the pass switch over a 128-bit mesh mask, then per mesh group
  and pass list a 0x110-byte packet copied from the entity, a 0x170-byte block, the bone-palette snapshots, one command and
  one 0x18-byte record. Nothing is kept as a prepared draw; nothing is compared with the previous frame; a LOD cross-fade
  draws twice. About 80 to 120 calls per entity and view (INFERRED order of magnitude).
- **Instance data and palettes**: instance sets by a `GXModelInstanceRegisterJob` on the GX pool (141A79F00); at recording
  time the executor 141AB43D0 copies instance records (0xD0 per instance) and palettes into the per-thread upload ring.
  Bone palettes are computed once per frame per model by `CSFD4LocationGxModelMatricesAndAabbExporter` (140B4AD90 ->
  141A929C0), INFERRED in the parallel location-update steps on the six lanes; skinning in the vertex shader.
- **View constants: one block per pass entry, not per draw**: the pass-enter snapshot (141B3C540) copies a 0x7E0-byte view
  block into a fresh upload slice, skipped when the previous slice is still current; every draw of the chunk binds it.
  No per-programme scene block was found.
- **Frame memory: no general heap call on the entity-draw path.** A linear frame arena (`GXBS::GXLinearAllocator`, 72 MiB,
  184 MiB by a flag; a bump pointer behind a spin lock; reset at the start of each scene render, 141B452B0); in each draw
  context two chunked bump allocators without lock (2 KiB command chunks, 32 KiB data chunks; checked: 141AD1FC0 ->
  141C4ACD0 aligns, compares with the chunk end and refills from the arena only when full); a per-thread single-threaded
  heap (384 KiB) for transient pairs. The only CRT allocations found are `std::stable_sort` temp buffers (tens a frame).

## 6. Waits, the limiter and the frame lag

**ER-ASM (W1; the spins checked in I-0080).** The limiter `CSFlipperImp::140E84830` runs at the start of each tick, before
the timeline: the in-game option gives 1/30, 1/60, 1/85 or 1/120 s (with a debug table of other modes); with the window
focused it **spins on `QueryPerformanceCounter`** until the deadline; unfocused it sleeps to about 5 ms before it, then
spins; `timeBeginPeriod(1)`; no waitable timer is imported. dt is the mean of the last 16 frames, capped at 1/20 s. The
`Flip` row (140B169A0) hands the present interval to the renderer (`g+0x1A8`). The frame-lag wait (1419EE770 -> 141AFC9A0; checked
in the decompiler: fence slot `(frame - lag) mod 6`, the lag at `g+0xF1C`, below 6) polls the fence with `Sleep(0)`; at
HIGHEST priority this is a busy wait. Rows of the
timeline end with the main thread polling (§1). Pool joins help (the caller runs jobs) but are pool-wide.

## 7. Ours: the native engine thread at m45

**OURS (code map, `main` 95aec11c).** On the engine (owner) thread, inside DS3's `DrawStep` (row 79): `BeginFrame`
(`15_cpu_service_and_input.inc:325`), the collect `UpdateNativeSceneCpu` (`14_scene_step_and_storage.inc:7`, DS3 E006A0),
the render step `NativeRenderStep` with the UI prepare (`17_render_step.inc:415`, `game_ui_session.cpp:1001`), `EndFrame`
and `Clear()` (`17_render_step.inc:497`, `02_frame_lifecycle.inc:307`), which posts the frame job to the render worker
(`render_worker.cpp:76`, depth 1: the engine thread's only wait on the render worker is `WaitIdle` inside `Post`). The render
worker prepares (chunks on the recording pool, I-0075), waits for its frame slot and the swap chain's waitable object,
records, paces and presents (`scene_pass.cpp:1719-1734`, `:856-872`). The effects' update (`NativeSceneCtrlUpdate`,
`21_effects.inc:688`) runs in DS3's SFX step on one thread, outside the native stamps. In native mode, pass-through observers
stay hooked on DS3's update path (player model update 8C1BC0, character Ez task execute 888C00, character manager schedule
8DD2E0, world update gate 8FA3E0, part pose and asm model updates; `22_setup.inc:390-412`), each with guarded reads
(`01_state.inc:902`).

The collect (`12_scene_collect.inc:825-1043`), in order, with the admission pool (`work_pool.h:60`: seven workers beside
the caller; ticket jobs; the caller works, then blocks on a condition variable; six `Run` barriers a collect, four more for
the reflection):

| Part | Thread | Code |
|---|---|---|
| set-up: light keys, options, decal and paint snapshots, dominant light rows, wet catalogue, camera and view constants, velocity, glow, AO, YEBIS inputs | engine, serial | `12:846-923` |
| effects record (DS3 24E790, `effect_draw::Execute`, `BuildNativeEffectFrame`) | engine, serial | `12:924`, `21_effects.inc:591-683` |
| main walk: visitors built, layers by ticket, gather, expand per entity | engine + pool (2 runs) | `12:731-824` |
| model admission, then placement | engine + pool (2 runs) | `09:1097-1148` |
| sun-shadow caster walk | engine, serial | `12:389-427` |
| pass rows (11 passes, every model, rebuilt each frame) | engine + pool (2 runs) | `10:170-458` |
| lights (blocks, point, forward, scene lighting) | engine, serial | `12:996-1002`, `07_scene_lighting.inc` |
| reflection (set-up serial, walk and admission on the pool, passes serial) | engine + pool (4 runs) | `12:599-730` |
| carry store, record sweep, publication | engine, serial | `12:1008-1019` |
| menu views (offscreen) | engine, serial | `16:313-378` |

**MEASURED** (m45, measure mode). I-0078 run 93 (`i78f`, the current `main` collect): the engine thread's stamps a frame
are DS3's own work before the collect 11.14 ms (its update and its limiter's spin at the 60 cap), the collect 4.08 ms, the
UI prepare 1.35 ms (of which the Scaleform display with our HAL 1.28 ms: submissions 0.81, shader constants 0.37, primitives
0.38; `native_ui_split`), `Clear` 0.08 ms. The collect's phases, self time summed over threads (`native_collect_split`):
set-up 1.61 ms (engine thread only), lights 0.62 (engine only), menu views 0.44 (engine only), passes 1.10, entity filter
0.96, model admission 7.19 (mostly on the pool), traversal 0.26.

The sample profile at the 60 cap (I-0078 run 89, `i74-native-89-m45-prof-i78c`, 30,000 samples of the engine thread, so
1 % is 0.17 ms a frame), inclusive:

| Item | Share | ms a frame |
|---|---|---|
| DS3's limiter spinning on QPC (EEDC60) | 28.8 % | 4.8 (idle at the cap) |
| the collect (`UpdateNativeSceneCpu`) | 26.0 % | 4.3 |
| of which `WorkPool::Run` (the caller's share and its waits) | 13.8 % | 2.3 |
| of which the condition-variable wait at the joins | 2.6 % | 0.43 |
| of which the admission | 10.7 % | 1.8 |
| of which the reflection | 4.4 % | 0.74 |
| of which the pass rows | 3.8 % | 0.63 |
| of which the effects record | 3.4 % | 0.56 |
| of which the main walk | 3.3 % | 0.55 |
| of which the zero-initialised model vector (`vector<NativeSelectedModel>::resize`) | 2.0 % | 0.33 |
| of which the shadow-caster walk (self) | 1.2 % | 0.20 |
| the UI prepare (`PrepareFrame`) | 8.3 % | 1.39 |
| DS3's 8F55AB (I-0078: "the Scaleform update"; the original 4.9 %, 0.81 ms) | 6.3 % | 1.05 |
| the effects update (`NativeSceneCtrlUpdate`; the original's E6E8D6 1.0 %, 0.16 ms) | 2.3 % | 0.38 |
| general heap: `RtlAllocateHeap` inclusive / `operator new` | 4.5 % / 4.3 % | 0.75 / 0.72 |
| guarded reads (`GuardedCopy` leaf, `Read`) | 1.3 % / 0.6 % | 0.22 / 0.09 |
| `memset` (leaf) | 2.2 % | 0.36 |

DS3 functions measured slower under the native renderer than in the original at the same scene (I-0078 original-04,
inclusive): 8F3506 4.19 % against 2.45 %, 608467 2.05 % against 1.40 %, D58DF4 2.33 % against 1.57 %: about 0.5 ms a frame
together, of which the observers' share is not isolated (P9).

The original at the same scene and cap (I-0078 original-04): its frame step without the vertical-blank wait inside Present
about 4.3 ms, its limiter's spin 26.6 %. Unlocked it ran m45 at 159 frames/s (6.3 ms, original-05). DEDUCED: at 120
frames/s the engine thread has 8.3 ms for DS3's own update (about 4.7 ms at m45) and our work, which is 5.5 ms today
(collect 4.07 + UI 1.38 + end of frame 0.07, run 93): it must fall to about 3.6 ms or less, with margin for heavier scenes
(Archdragon Peak, Firelink while walking).

## 8. ER defects not to copy on this path

- The main thread's busy waits: the focused limiter on QPC, the frame-lag wait with `Sleep(0)`, the polling at every row
  barrier (W1). Ours: keep event and timer waits; replace DS3's own limiter spin (P7).
- Pool-wide joins (the waiting thread waits for every queued job, not for its own group; I-0077 §6, EndFrame's join):
  ours must join per group, so a background job (the reflection, the menu views) does not hold the collect.
- The global Scaleform lock held across the whole UI draw on the render thread, which the next frame's Capture needs (§3):
  ours takes per-movie locks only (DS3's `E52EE0` already does). ER's Advance of every movie on one low-priority worker,
  overlapping the scene render: DS3 already advances its movies in its own Scaleform steps, on lane 1; nothing to copy.
- The effects' barriers polled with `Sleep(0)` by task 0 (§4): ours, an event or a counter the last arrival signals.
- A parallel parameter update compiled but left off (`1447F5248`): ours should run independent preparation work in
  parallel by default, and keep a diagnostic switch only.
- Per-entity draw data rebuilt for every view and frame without any change test, and bone palettes copied twice (§5):
  ours keeps what is stable (I-0070, I-0074) and should keep doing so; only the per-frame allocation and zeroing pattern of
  ER is to copy.
- The frame arena's spin lock per chunk refill: ours, per-thread chunks from a pre-sized arena without a spin.

## 9. Third tranche: prioritised plan of measurable gains

Order by expected gain on the engine thread at m45, then by risk. Every item is validated on the same scenes (D-0193: m45,
Archdragon Peak, the Firelink walk, sprint and camera turn), with the picture, CPU, frames/s and p99, the guards
(`record-revalidation.flag`), the debug layer, and the replayed path gate at the merge (D-0202). The gains are estimates
from the measurements in §7, not measurements.

| # | Change (ER reference) | Expected gain, engine thread | Check |
|---|---|---|---|
| P0 | **Measure the engine thread unlocked first**: a native run with the game's rate raised as `original-unlock.flag` does for the original (timing object +0x354/+0x358), at m45 and the Firelink walk, with the sampler; so the tranche aims at the real 120 frames/s frame, not the 60-capped one | baseline | frames/s and engine-thread ms unlocked, native against original-05/06 (159 and 220 frames/s) |
| P1 | **The UI's display on the render worker** (§3: ER plays the Display closure on the render thread; DS3 queued it on GX's command queue). The engine thread keeps the selection under `manager+0xC38` (flag clear, sort) and passes the movie list in the frame job; the worker calls `E52EE0` per movie, then builds the packet. Needs: the movies held alive until the job ends, our HAL's state touched by the worker only, per-movie locks only (not ER's global lock) | about 1.3 ms (the display 1.28 of 1.35) | UI stamp under 0.1 ms; menus, Status, HUD, in-game menu pictures equal; the debug layer and the menu check (I-0078 `Run-MenuCheck.ps1`) |
| P2 | **The collect as concurrent sub-collects** (I-0077 §5 and §1: ER's sub-scene jobs, layer jobs and entity jobs pushed without blocking, the caller helping): the effects record, the lights, the sun-shadow caster walk, the reflection and the menu views as tasks beside the main walk and admission, on a pool with non-blocking push and **per-group** joins (not ER's pool-wide join), fewer barriers than the six `Run` of today | about 1.5 to 2 ms (the engine-only serial parts: set-up 1.61 with the effects record, lights 0.62, menu views 0.44, shadow walk 0.20, the reflection's serial part, overlapped with the walk and admission chain) | collect stamp; `native_collect_split`; guards 0 mismatch; pictures; a static review of the DS3 functions each task calls (24E790, the light manager reads) for concurrent use |
| P3 | **Frame memory without the general heap** (§5): the scene objects recycled from a small ring with their capacities (collect N, render worker N-1, release), default rather than zero initialisation of the per-frame arrays, per-worker bump chunks for per-frame blocks | about 0.6 to 1.0 ms (heap 0.75, `memset` 0.36, the model vector's zeroing 0.33, part overlapping) | `RtlAllocateHeap` under 1 % of the engine thread in the sampler; collect stamp |
| P4 | **The effects' update in parallel** (§4: ER's per-effect tickets then the item lists on the pool, as DS3's own `GXFfxUpdateJob` did; per-task output vectors merged once; ER's update decimation of effects to read on DS3 before any copy): our transcription's per-effect and per-list work over the pool, joined before the collect, barriers on events rather than ER's `Sleep(0)` poll | about 0.2 to 0.3 ms (0.38 against the original's 0.16) | `native_effect_update` time; FFX pictures (bonfire, E01-E07 witnesses) |
| P5 | **Pass rows and admission without per-frame rebuild where nothing changed** (I-0077 open), only if P2 and P3 leave the rows on the critical path: ER rebuilds per frame, so this goes beyond ER and needs its invalidation proven | up to 0.5 ms (rows 0.63 on the engine thread before P2) | rows built per frame; guards; pictures |
| P6 | **Scene and view blocks while moving** (§5: ER uploads one view block per pass entry and skips it when unchanged): pack the camera-dependent blocks once per distinct register plan per frame instead of per programme | measured first (the walk's collect is 4.02 against 4.07 standing) | blocks packed per collect while walking; collect stamp in the walk |
| P7 | **DS3's limiter without its spin** (§6: ER spins too, a defect): the wait until the deadline on a high-resolution waitable timer with a short final spin, as our present pacer does (`frame_pacer.cpp:15-23`) | no frame-time gain; about 4.8 ms of a core a frame freed at the 60 cap (29-31 % of the engine thread) for the pools and the render worker | owner-thread samples in QPC under 3 %; frame pacing and p99 of the stutter series unchanged |
| P8 | **The render worker's budget at 120**: after P1 the worker carries the UI display (1.3 ms) beside prepare and record (2.9 + 1.9 ms at m45, I-0075); its frame must stay under 8.3 ms | none on the engine thread; a guard | worker job ms and `queue_wait_ms` in `Clear` (the engine thread must not wait on `Post`) |
| P9 | **DS3's own steps slower under the native renderer** (§7: about 0.5 ms at m45): an A/B without the pass-through observers on the update path (keep only the hooks the backend needs), then the cache effect of the pools if any remains | up to 0.5 ms | DS3 frames 8F3506, 608467, D58DF4 against the original's in the sampler |

Order of work: P0, then P1 (largest, isolated, an ER and DS3 design both), P2 with P3 (they share the scene object and the
pool change), P7 (independent, small), then P4, P9, and P5/P6 only on measurement. P1 + P2 + P3 bring the m45 estimate from
5.5 ms to about 2.0 to 2.5 ms of native work on the engine thread, under the 3.6 ms aim.

## 10. Limits and open questions

- Located after the first writing (§2): ER's per-view scene render is called by the scene list's render-all 141ADA8D0 from
  `RendMan::140CC8850` in the `AdhocDraw` row, on the main thread. Still open: the shipped
  default of ER's threaded-render switch (`config+0xE2` -> `g+0x1E4` -> `g+0x11D`) and of the deferred record pass
  (`DAT_1447F59B9`), and whether effect records fall into that pass, are also open. Settled by the Ghidra pass: ER's UI is
  redrawn every frame from the retained trees (§3); ER's effect callbacks read live node state at recording time, inline
  in the frame with the default data (§4).
- The workers' readings are assembly-level; every claim a design relies on is to be re-read in the decompiler first (the
  points marked checked were read by Opus in the assembly).
- One profile run per build (run 89) and one split run (run 93) at m45; the walk and Archdragon Peak are not profiled here.
- No DS3 change was tried; the gains in §9 are estimates.
