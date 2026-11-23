# D3D12 R13 and R14: the separate render thread and the pacing, ready to implement (static)

Request (coordinator, 2026-10-11): write the sheet ready to implement for the separate render thread (R13) and the pacing (R14), building on the [phase D sheet](D3D12-PHASE-D-IMPLEMENTATION-SHEET.md). It covers the data shared between the engine thread and the render thread and their passage through immutable versions; the `Execute`/`Complete` race found in the phase D review; what the original does (job `143D61260`, GX task queue `+0x2C8`) and what ER does; the expected gain given that collect takes about 105 ms; the commits under flags, the tests and the witnesses. Static reading only: nothing was built, run or measured here.

Tree read: branch `claude/friendly-meitner-c21c20` at `fd0c672c` (= `main`; native), `decompiled/DS3-1152-F-<RVA>` exports (`.c` and `.asm.txt`) and the Ghidra mirror for the original. Labels: **STATIC** (read in code or data), **INFERRED**, **OPEN**, **proposed**. Line numbers are that tree's. ER is architectural evidence only ([ER-DEEPENING-897](ER-DEEPENING-897.md), [review section 2](D3D12-DESIGN-REVIEW.md)). The sheet was reviewed by an independent Opus 5.5 pass (two blocking findings, six important ones); all are applied below. The review also corrected one of this sheet's own first claims: the original's wait is **30 ms slices, not a 30 s timeout** (K-0245), and the sheet now says so.

## 0. Answer first

1. **The original has exactly this architecture, and ER has it too (STATIC for the original, INFERRED for ER's frame choice).** The game thread records command *records* into one of two slots (`ctx+0x398 + idx*8`, index `ctx+0x3B8` toggled at each frame end, `FUN_14000A7B0`); at frame end it drains the GX task queue (`ctx+0x2C8`), then **posts the finished slot to a `GXRenderThread`** (`FUN_1400D6270`); the thread (`FUN_1400D63E0`) replays the slot onto the D3D11 immediate context (`FUN_14000B3C0` then `FUN_14000AA90`) and then calls the present callback `FUN_1400063D0` -> `FUN_1400D5CB0` -> pacing `FUN_1400D5E30` -> `Present`. **The job queue has depth 1**: the post first waits for the thread's previous job (`FUN_1400D6300`: an *unbounded* wait for the end of the slot replay, then idle-event waits in **30 ms slices** with a window-message pump between slices; no global timeout, no fault; K-0245). The engine clock is written around the post by the `EndFrame` path, not after `Present` (this contradicts the "after Present" assumption of the phase D sheet R8; section 8.3 settles it). ER's worker is the same shape.
2. **Native cut, in two steps.** F1: `Render*` and `Present` on a worker, `Clear()`'s prepare stays on the engine thread. F2: prepare moves too, after the R8b snapshots (S1 to S4 of [R8B-ENGINE-READS-STATIC](R8B-ENGINE-READS-STATIC.md)). Everything that crosses is an **immutable job** (`shared_ptr<const>` and values); everything else is thread-owned (section 3). The job is **the whole frame from the first worker commit** (movie, menu views, UI, main pass): phase D's single list (cD3) forbids a half-frame split (section 9).
3. **The gain is bounded by collect (section 7), and the figures are ceilings.** Frame 148.6 ms (M4): collect about 105, prepare 25.7, render 5.0 (wait 1.2), the rest 12.9. F1 hides at most 5.0 ms (-3.4 %, 6.73 to at most 6.96 fps). F2 hides at most prepare + render, 30.7 ms (-20.7 %, to at most 8.48 fps). Beyond that only collect matters (about +0.5 fps per 10 ms at today's frame time, +0.8 at 118 ms). R13 is a prerequisite and a ceiling-raiser, not the cure for the 105 ms.
4. **The `Execute`/`Complete` race is real and reachable only with the worker (section 5).** `ExecuteCommandLists`, `next_++` and `Signal` are three unordered steps; `Complete` waits on one auto-reset event shared by all waiters and throws "invalid completion" when woken for another waiter's value. Rule: one submitting/waiting thread (the worker), plus hardening of `DeviceQueues` (submission mutex covering the ingress swap, a per-wait event, a re-wait loop) with the lock order written in section 5.2.
5. **R14 is the original's QPC pacer on the worker (section 6).** Interval 1 with a 59/60 Hz refresh goes to DXGI, any other valid non-zero interval is paced by a 1/60 s QPC deadline and presented with 0, interval 0 passes through. At today's frame times (over 100 ms) the pacer does nothing visible. Phase D's cD9 already owns the present-interval/tearing rules; this sheet adds the pacer on top without a second flag (section 6.2).
6. **Commits (section 9):** cR0 counters and thread ids; cR1 queue hardening (= phase D's cD-F); cR2 job-safe renderer state, inline; cR3 worker with the whole-frame job, depth-1 queue, fault channel, quiesce, behind `render-thread.flag`; cR4 pacer; cR5 prepare on the worker behind `prepare-thread.flag` after R8b S1 to S4; cR6 M8 run and defaults. Requires phase D cD1 to cD4 (slot ring, view contexts, single list, per-slot state) and, for F1, the dynamic-stream variant of section 3.4.

## 1. What the original does (STATIC unless marked)

### 1.1 Frame end and the render thread

| Step | Function (export `DS3-1152-F-<RVA>`) | What it does |
|---|---|---|
| game side frame end | `FUN_140D56930` (`00D56930`, the `EndFrame` slot the native hooks) | calls `FUN_140EEE190` (QPC into `clock+0x40`), then `FUN_14000A7B0(GX ctx = [144961AB0])`, then tail-jumps to `FUN_140EEE160` (QPC into `clock+0x48`): **both clock writes are around the post, not around `Present`** (exports `00EEE190`, `00EEE160` added) |
| drain and finalize | `FUN_14000A7B0` (`0000A7B0`) | when `ctx+0x3E1 == 0`: `FUN_1400D1D30(ctx+0x2C8)` drains the GX task queue (section 1.2). When `ctx+0x80 == 0` it executes inline (`FUN_14000A440`: drain, `FUN_1400D6300` wait for the thread, finalize, toggle, then `14000AE10`, `1400CC580` when set, `14000B4E0`). When `ctx+0x80 != 0` (threaded path): writes the end-of-frame record into the current slot, calls `FUN_1400D6270(thread = [ctx+0x380], callback = [ctx+0xA8])` (values loaded from the context, not addresses), toggles `[ctx+0x3B8] ^= 1` and resets the other slot's arenas (`+0x20..+0x98`), calls its object's `vt+0x70`, swaps `ctx+0xBB0/+0xBB4` |
| post | `FUN_1400D6270` (`000D6270`) | **first `FUN_1400D6300`** (below), then stores the slot at `thread+0x18` and the present callback at `thread+0x20` (callback only when `[thread+0x58]+0x82` is set), copies two bytes into globals (only when `[ctx]+0xB60 != 0`; calling them "settings" is an interpretation), resets the idle event (`thread+0x48`), `SetEvent` |
| wait for the previous job | `FUN_1400D6300` (`000D6300`) | `thread+0x48` is an idle event (`DLPlainConditionSignal`). If it is not set (`vt+0x18` tests it with a zero timeout): **(a)** wait on `thread+0x38` with `-1` (INFINITE, no message pump): that event is "slot replay finished", reset then raised around `FUN_14000B3C0` in `D63E0`; **(b)** then wait on the idle event with input `0x7530` which `FUN_1417F1BD0` divides by 1000 before `WaitForSingleObject`, i.e. **30 ms** (K-0245, CONFIRMED: "30 milliseconds at the Windows call, not 30 seconds"); while it times out: `PeekMessageW(PM_NOREMOVE)`, then `GetMessageW`/`TranslateMessage`/`DispatchMessageW` while messages are pending, then wait another 30 ms slice. **No global timeout and no fault path.** Consequence: the game thread pumps window messages only during the render thread's present/pacing phase (after the replay), once per 30 ms |
| thread routine | `FUN_1400D63E0` (`000D63E0`) | loop until `thread+0x10`: (sleeps 40 ms when `DAT_144961AB8`); if `thread+0x18` is set -> reset then raise `thread+0x38` around `FUN_14000B3C0(ctx, slot)` (appends the end-of-frame record, flushes the slot, `FUN_14000AA90(ctx, ctx+0x10, 0)` replays the records onto the D3D11 backend) and clears it; branch `+0x11` writes `[+0x58]+0x50` into `DAT_144731BD0`, `ctx+0x110` and each output's `+8` before the callback; if `thread+0x20` is set -> calls the callback's slot 0 with `([thread+0x58]+0x40, +0x48, +0x4C)` = (interval, flags, third argument) **read at callback time on the render thread** and clears it; sets the idle event; waits for the next job |
| present callback | `FUN_1400063D0` (`000063D0`, vtable slot, data ref `14496B7B0`) | for each output record of `[ctx+8]` (stride 0x148): optional gamma draw (`FUN_14192CBD0`), then `FUN_1400D5CB0` |
| present with pacing | `FUN_1400D5CB0` (`000D5CB0`) -> `FUN_1400D5E30` (`000D5E30`) -> `FUN_142139260` -> `FUN_14192CF00` -> `Present(interval, 0)` | section 6.1; K-0009 ([I-0004 RESULT](../I-0004-present-sync-timing/RESULT.md)) |

Consequences (STATIC): two recording slots and one render thread give **two frames in flight with a job queue of depth 1**: the game thread records frame N+1 into slot B while the thread replays slot A and presents (with its pacing sleep); the game thread blocks at the post of N+1 in `FUN_1400D6300`, first until the replay of N ended, then until the whole of N (including `Present` and the pacing wait) ended. The slot reset after the post touches only the slot just freed. The writer of `ctx+0x80` (the threaded flag) and the type of the object at `thread+0x58` are **OPEN**.

### 1.2 The GX task queue and job `143D61260`

- `ctx+0x2C8` (`[[144961AB0]+0x2C8]`) is a lock-protected CPU task queue of `GXBS::Tasks::GXInternalForTask` wrappers (K-0114): `FUN_14020E720` pushes under two locks (`+0x70`, `+0x38`). `FUN_1400D1D30` (`000D1D30`) is the **helper/drain loop**: it evaluates the queued wrapper's virtual `+8` as a predicate, pops it under the lock when false, and executes it through the wrapper's slot 0 (with the worker index `[queue+0xA8]`), repeating until the queue is empty; `FUN_1400D1C40` (`000D1C40`) releases a counted semaphore (at most `workers - 1` wake-ups, not one per task) and wakes the pool through `FUN_1418D79B0`; `FUN_1400D1EF0` (`000D1EF0`) is a **busy wait** for the workers to return (it does not help; `D1D30` helps). Callers of the drain: `14000A440`, `14000BC30`, `14000A7B0`, `140024740`, `14013F120`, `140128810`, `1402046B0`, `14024D000`, `14025B870`, `00208180`. The thread creation of the pool consumers is **OPEN**.
- **Job `143D61260` = `GXSG::GXDrawSceneForTask`** (K-0110, I-0020): built on the stack by `GXSgCompositeScene` slot `+0x38` (`FUN_1402046B0`, vtable `143D4A108`) when `DAT_14473CE5B` and `DAT_14473CE59` are both set: fields scene, context range, manager (`vt+0x68`); a pooled wrapper (`FUN_1400B6480`) carries `+0x10 = 0`, `+0x14 = number of sub-scenes`, `+0x18 = 1`; the wrapper is pushed (`FUN_14020E720`), the semaphore released (`FUN_1400D1C40`) and the composite scene waits (`FUN_1400D1EF0`, or helps through `FUN_1400D1D30` when `CE5C`); each task runs `FUN_140204240` over a range `[begin, end)` of sub-scenes, calling the composite scene's virtual `+0x48` (K-0110). Otherwise (`CE5B == 0 || CE59 == 0`) the same sub-scenes run serially in the calling thread. The values in the image are all `01`; `CE59` is recomputed each frame (`FUN_140023790`), so the **parallel path is the shipped default** (COLLECT-ORIGINAL-STATIC section 0.4).
- What the tasks produce: 24-byte command records appended to per-scene stream blocks (K-0129) that are transferred, under guards, to the shared `GXDrawSceneManager` (`FUN_140128240`, `FUN_140128810`); `FUN_140024740` drains the same queue, consolidates the manager lists and dispatches records into the active slot. **The parallelism is across pass scenes on the recording side; the render thread only replays records.** The native build runs none of this (one synchronous walk on the owner thread).

### 1.3 Original, native today, proposal

| Aspect | Original | Native today | Proposal |
|---|---|---|---|
| Recording | pass scenes in parallel on a task pool, records into slot A/B | one owner thread: collect, prepare, record | collect and prepare on the engine thread (F1) or worker (F2); record on the worker |
| Replay and `Present` | `GXRenderThread` | owner thread, inside `Clear()` | `RenderWorker` |
| Handoff | slot pointer + present callback, depth-1 queue, idle events, message pump in 30 ms slices | none | `FrameJob`, depth-1 queue, idle event, pumped wait (section 8) |
| Pacing | callback on the render thread (`D5E30`) | `Present(0,0)` | `FramePacer` on the worker |
| Frame clock | `clock+0x40`/`+0x48` written around the post by `D56930` | `EndFrame` writes `clock+0x48` after `Clear()` returns, after `Present` (`game_bootstrap.cpp:7424-7425`; `EEE190` runs before, `:7419`) | after the post (as the original), with a dt witness (section 8.3) |
| Fault | render thread errors are D3D11 errors / asserts; the wait never faults | `Refuse` -> `ExitProcess(0xD3126606)`; `EndFrame` catch -> `ExitProcess(0xD3126607)` | worker catch -> fault channel -> `EndFrame` ends in the same exit; a native wait deadline is **proposed**, not original |

## 2. What ER does (architectural reference)

| ER observation | Take | Do not take |
|---|---|---|
| E1 (K-0013, K-0016, K-0017): the frame end hands ordered records to a worker (`141AF5060`: waits for the previous job through an idle event with a message pump, stores the records and, when presentation is enabled, the callback, signals); the worker loop processes the records then calls the callback's slot 0 with the presentation settings and clears it; threaded mode only (`renderer+0x11C`). Which frame's records (`[(renderer+0x708)^1]`) is **INFERRED** in ER-DEEPENING | The shape: depth-1 job queue, wait with message pump, presentation callback on the worker, records owned by the job until replayed | The non-threaded branch is not evidenced; K-0016 itself infers no frame-slot semantics |
| E2 + (b): a ring of 6 fence tokens; the frame end waits for the token of frame `N - lag` (lag 2) *before* publishing; the token is queued in the frame's records and signalled by the worker | The wait order: bound the lag before handing over (phase D slots); `slot_wait_ms` | The 6-token ring itself |
| E3: delayed destruction after the lag-bounded wait | The invariant, already expressed by tickets | A fixed delay |
| E12: `Present`, `Signal`, `GetCurrentBackBufferIndex` | The worker does the three in this order | |
| D-0157: engine owners produce data; the backend turns it into native records; ordered records carry it to the thread that submits (METHOD-710, DEEP-STUDY-716 section 6 is the inferred design) | The R8b Level 2 shape (section 4.3) | An ER value is never used as a DS3 value |

The original and ER agree on the shape: the native design re-uses the engine family's own structure, with D3D12 tickets in place of D3D11 immediate-context replay.

## 3. The native threads and the data between them

### 3.1 Today (STATIC at `fd0c672c`)

The owner thread (the `EndFrame` hook thread) does everything in `Clear()` (`game_bootstrap.cpp:647-736`, under `gpu_submissions` from `:651`): `AcquireNativeSelectedScene`, `PublishNativeSceneCopy` (calls `renderer.SceneColourCopy()`, which creates a GPU resource and touches a renderer member), `PrepareNativeModelFrame` (eight engine reads and engine calls, [R8B sections 2 and 4](R8B-ENGINE-READS-STATIC.md)), `RenderNativeMovieFrame`, `RenderNativeOffscreenViews` (prepare + render per view), `AdmitPrimitiveWork` (rebuilds the member `primitives_`, `primitive_depth_`, `primitive_generation_`), the main render and the present; the receipt check and its log (`:674-688`) follow. Collect runs on the same thread inside the replaced `FUN_140E006A0` (`UpdateNativeSceneCpu`; R8B-2 settles the thread). Other threads: loaders (`RecordIngress`/`Commit` only), the UI sessions, engine task threads that call the stream and cloth hooks, the menu thread (selection fallback only). `Log` is mutex-protected (`:519`); `Refuse` calls `ExitProcess` from any thread (`:567`). Whether the window's thread is the owner thread is **INFERRED** (the `native_window` row logs both ids; cR0 reads them).

### 3.2 Classes of data

| Class | Content | Rule |
|---|---|---|
| **J: job (immutable, crosses)** | `NativeModelFrameInput` (serial, camera, `clear_stencil`, `draws`/`forward_draws`/`velocity_draws` as `shared_ptr<const OwnedNativeModelDraw>`, `lighting` and `velocity` as `shared_ptr<const>`, primitive generation and stencil flags); `NativeMovieFrame` by value + its target record; one `NativeModelFrameInput` + target record + background + `pixel_file` per menu view; the UI pass as an immutable snapshot; B5 stream copies; witness requests (sample backbuffer, velocity, pixel/velocity file paths by value); the scene-colour copy resource pointer; the receipt-return cell | Built on the engine thread, never mutated after `Post`; every pointer inside is a `shared_ptr` that keeps its owner alive until the worker's slot retires (L1 of the phase D sheet) |
| **E: engine-owned (owner thread only)** | the engine's memory (model groups, animations, wind blocks, materials), `NativeSelectedScene`, collect, selection candidates, the UI sessions, `native_ui_frame`, texture registry mutation | The worker never reads it. Until R8b S1 to S4, prepare is in this class (F1); afterwards prepare only reads J-class snapshots (F2) |
| **W: worker-owned** | every `SceneRenderer` member that is not in S: slot ring, target sets, pipelines and PSO maps, lighting/velocity/SSS/environment resources, `frame_uploads_`, readbacks, `last_serial_`, `startup_serial_`, `scene_ready_`, `incarnation_`, `target_generation_`, `faulted_`, `open_`, `native_light_constants_`, descriptor heaps | Touched only by the worker after cR3; the owner thread reaches it by posting a job, by reading an atomic mirror (X7), or by the quiesce protocol |
| **S: shared mutable (synchronised)** | `DeviceQueues` (committed ingress list, retirement list, fence, counters), the draw caches (`ModelDrawCaches` under `caches.lock`, PSO creation off-lock), the texture registry and records (`NativeTextureRecord.resident/resource`), `native_scene_copy` record (`native_scene_copy_lock`), the journal, pacing counters, `control->` counters, the witness channel (new) | Already locked or atomic, or made so in cR1/cR2 |

### 3.3 Members of `SceneRenderer` and bootstrap state that the engine thread touches today

| Id | Member / method | Engine-thread use | Problem with a worker | Fix (commit) |
|---|---|---|---|---|
| X1 | `primitives_`, `primitive_depth_`, `primitive_dsv_`, `primitive_generation_`, `primitive_texture_generation_`, `primitive_stencil_initialized_` (`AdmitPrimitiveWork` `:1811`, `WithdrawPrimitiveWork` `:1837`, `RetirePrimitives` `:1804-1810`; `primitive_stencil_initialized_` written at `:1802` and `:1840`) | Rebuilt every UI frame | The worker encodes `primitives_.Encode` (`:1619`, `:1677`, `:2048`) while the next admission moves and rebuilds the same object: a data race | The UI pass becomes an immutable `shared_ptr<const PrimitiveFrame>` built by the admission (own pass object, its depth target and heaps) and carried by the job; the stencil-initialised flag moves into the worker (cR2). The superseded pass is retired by the **job that held it** (the job becomes the owner; `RetireWhenSubmittedWorkCompletes` keyed on `LastSubmitted` would release it while the job is still queued, unsubmitted) |
| X2 | `SceneColourCopy()` and `scene_colour_copy_` (`:1324-1333`, called by `PublishNativeSceneCopy`) | Creates the resource on the engine thread, compares its size with `presenter_.width/height` | The worker reads `scene_colour_copy_` when recording the forward pass (`:1588`) and resets it at `:2153` | The engine thread creates/replaces the resource (device calls are free-threaded; size read from the X7 mirror) and puts the pointer in the job (`job.scene_copy`); the worker uses the job's pointer only; the member is removed (cR2) |
| X3 | `velocity_file`, `velocity_witness`, `witness_diagnostics`, `primitive_admission_ms` (public members) | Written/read by `Clear()` around the render | Plain members read/written from two threads | A mutex-protected `WitnessChannel` (requests in, completed results out) and atomics for the admission timings (cR2) |
| X4 | `Resize`, `Close`, `Start`, depth recreation, `ReadStartupView`, `Faulted` | Bootstrap/close on the owner thread | Touch worker members | Quiesce protocol (section 5.3): the owner stops the worker (flush, idle) then runs them; `ReadStartupView` reads the X7 mirror |
| X5 | `DeviceQueues::Drain/Complete` from `Close()` (`game_bootstrap.cpp:793`, `:1646`), `game_texture_ingress.cpp:231` | Owner thread at shutdown | A drain while the worker submits | Same quiesce, then the drain (cR3) |
| X6 | `Diagnostics()` (info queue read-and-clear) | UI admission calls it (`:1830`); the worker at slot retirement | Two consumers clear each other's messages | Worker-only; the admission's call is replaced by a flag the worker reads (cR2). **This replaces the phase D sheet's G4 ("the UI-admission call stays")** |
| X7 | `AdmitPrimitiveWork` reads (`scene_pass.cpp:1812-1816`) `last_serial_` (`:1659`), `startup_serial_`, `scene_ready_`, `incarnation_`, `faulted_`, `open_`, `presenter_.width/height` | Admission on the engine thread decides from worker-written state | Plain reads of members the worker writes | A small `WorkerMirror` of atomics (last completed serial, ready, incarnation, faulted, extent) published by the worker at the end of each job; the admission reads the mirror (cR2) |
| X8 | `SubmitIngress()` in the admission (`:1829`) | Submits ingress lists from the engine thread | Second submitter | Removed by phase D cD3 (single list carries the ingress swap); until then it is a cR1 caller under `submit_mutex_` |
| X9 | `native_prep_ticks` and the other `static` counters in `Clear()` (`game_bootstrap.cpp:602` and neighbours) | Plain statics | Two writers under F2 | `std::atomic` or worker-local copies (cR0/cR2) |
| X10 | Receipt check and journal of `Clear()` (`:674-688`) | Consumes the receipt of the frame just rendered | With a worker the receipt that returns is the one of job N-1 | The receipt-return cell in the job: the check and the log consume the **previous** job's receipt, the serial is carried (cR3); the `Clear()` row is renamed accordingly |

### 3.4 Constraints between phase D items and R13

- **B5 (dynamic streams) is invalid as written in the phase D sheet once prepare and record are on different threads.** Phase D section 5.2 (B5a arena-bound range allocated at prepare; B5b "in `Clear()` allocate an arena range, `memcpy`, record `CopyBufferRegion`, `RecordIngress()`") both allocate from `frame_uploads_` on the prepare thread, which violates `UploadArena`'s "one recording thread" (`backend.h:125`) and rule L3 as soon as record is on the worker. **This sheet amends phase D section 5.2** (including the writer of `copied_generation`): under F1 the dynamic stream copy is recorded and its arena range allocated **by the worker** from the job's stream versions (`StreamCopy`, immutable bytes); the prepare thread only publishes the version. Until that variant (B5w) is implemented, cR3 requires `legacy-dynamic-streams.flag` (the phase D legacy path) to be present. B5a with prepare-time ranges remains valid only for F2 where prepare and record share the worker. Test T-D8 gains a "prepare thread != record thread" variant (T-R8).
- **The phase D ring and the single list are prerequisites** (cD1 to cD4): the worker records one list per frame into the slot it gets from the ring; the arena rule L3 holds on the worker.
- **`gpu_submissions`** protects CPU structures only after cD4; the worker never takes it. Prepare (engine thread) keeps taking it where it does today. `Post` is never called under it (section 8.1).

## 4. Immutable versions

### 4.1 The job

```cpp
struct FrameJob {                       // proposed; built on the engine thread, immutable after Post
    uint64_t serial{};
    std::shared_ptr<const NativeModelFrameInput> models;            // null for startup/clear frames
    uint64_t primitive_generation{}; bool clear_stencil{};          // startup frames without models (RenderStartupPrimitives)
    std::optional<NativeMovieFrame> movie;  std::shared_ptr<const TextureRecord> movie_target;
    std::vector<OffscreenViewJob> views;                            // frame + target record + background + pixel_file
    std::shared_ptr<const PrimitiveFrame> ui;                       // X1
    Microsoft::WRL::ComPtr<ID3D12Resource> scene_copy;              // X2
    std::vector<StreamCopy> stream_copies;                          // B5w: owner, version bytes (shared_ptr<const vector>), generation
    WitnessRequests witness;                                        // sample flags and paths by value
    uint32_t interval{}, present_flags{};                           // R14: proposed; the original reads them at callback time (6.2)
    std::shared_ptr<ReceiptCell> receipt;                           // X10
};
```

The mutation of `model_frame->primitive_generation` in `Clear()` (`:666`) must precede the freezing of the job.

### 4.2 Rules (V1 to V6)

- V1: nothing in a job is written after `Post`; the worker never calls back into engine memory; every engine pointer in the old `NativeSelectedModel` that prepare dereferences (`model.group`, `model.animation`, the material pointer) is replaced by the Level 1 values of R8b before prepare may run on the worker.
- V2: a job holds a reference to everything the recorded list reads (draws -> geometry binding resources, storage, stream versions or buffers, descriptor owners, texture records, ingress `kept` entries); the slot keeps the job until its ticket (phase D L1).
- V3: versions are monotonic per owner (`generation`); an owner never mutates a published version; a newer version replaces the pointer under the owner's lock (R8b Level 2 form, engine hooks publish `shared_ptr<const>` records).
- V4: the engine retiring a model while a job is queued cannot free memory the worker needs: V2 plus L2 (`RetireWhenSubmittedWorkCompletes` for owners released from other threads) plus, for objects the job itself holds, release by the job (X1).
- V5: frame-to-frame state lives on the worker (previous pose rows for velocity are CPU matrices carried in the job, not worker memory; the scene-colour copy pointer is per job).
- V6: the guard (R8b F6, permanent in the diagnostic build) builds the draw inputs twice, from live reads and from the snapshot, and compares the produced constant bytes; it runs on the engine thread before the post.

### 4.3 What prepare still reads from the engine (the gate of F2)

R8b shows eight direct reads (E1 to E8) and further engine touches in the prepare call tree: the forward-light key block and four engine functions per draw, the gparam bank fallbacks (`FUN_140032770`, `FUN_140038E70`), the programme repository walk (`[image+0x4792BF8]`, `[image+0x4792D58]`), the environment-region engine call (`FUN_1400416A0`). **F2 may not start until R8b S1 to S4 have landed and row R8B-6 reports zero engine calls and zero repository walks in prepare.** F1 needs none of them: prepare stays on the engine thread. R8b S1 to S4 move work into collect (they build the snapshots there), and F6 repeats the prepare: F2's gain is a ceiling.

## 5. The `Execute`/`Complete` race

### 5.1 Mechanism (STATIC, `backend.cpp`)

- `SubmitWithIngress` (`:254-283`): swaps `committed_` under `ingress_lock_`, `ExecuteCommandLists`, then `ticket=next_++`, then `Signal(fence_, ticket)`, then publishes `submitted_`. Two submitting threads can obtain tickets in one order and `Signal` in the other (the fence value would go backwards), `submitted_` can be published out of order, and the ingress lists swapped by the later caller can execute before the earlier caller's frame list, breaking the "ingress lists precede the frame list in commit order" contract.
- `Complete` (`:290-301`): one `completion_` auto-reset event shared by all waiters. A thread woken by another waiter's value finds `GetCompletedValue() < value` and **throws "invalid completion"** (`:298`) instead of waiting again. The other waiter **may** then time out after 5 s (the surviving `SetEventOnCompletion` registration of the thread that threw can still wake it when its own value is reached; the timeout is only hit when that takes more than 5 s). `pending_` is a plain bool written by both.
- `Drain` (`:284-289`) is the same sequence with its own ticket.
- Today only the owner thread submits and waits in the game module ([phase D sheet section 0.3](D3D12-PHASE-D-IMPLEMENTATION-SHEET.md)); the race becomes reachable when the worker submits while the owner thread still calls `Drain` at shutdown or `SubmitIngress`.

### 5.2 Rule and hardening (cR1 = the phase D sheet's cD-F, one commit)

1. **Affinity rule:** after cR3, `Execute`, `SubmitIngress`, `Drain`, `Complete`/`Wait`, `CollectCompleted`, `Diagnostics` run on the worker only; the owner thread reaches them through a job or the quiesce protocol. Debug builds assert the thread id.
2. **Hardening (defence in depth, needed by the quiesce and by tests):**
   - one `submit_mutex_` held across [ingress swap, `ExecuteCommandLists`, ticket assignment, `Signal`, `submitted_` publish, the `RetireAfter` loop] and across `Drain`'s ticket and signal;
   - `Complete` takes a per-call event from a small free list (guarded by a leaf lock), `ResetEvent` before reuse, and after a wake-up **re-checks and waits again until its own deadline** instead of throwing; an event whose wait expired is neither recycled nor closed (its registration is still pending; same treatment as `completion_` in quarantine);
   - `pending_` becomes atomic.
3. **Lock order (the review checked there is no cycle):** `submit_mutex_` then `ingress_lock_`; `submit_mutex_` then `retire_lock_`; `Commit` takes `ingress_lock_` only; `QuarantineToProcessExit` takes `retire_lock_` then `ingress_lock_` and nothing takes the reverse; `caches.lock` then `retire_lock_` through deleters; `CollectCompleted` releases owners **outside** `retire_lock_`. **`Complete`/`Wait` is never called under `submit_mutex_`.** The worker never takes `gpu_submissions`.

### 5.3 Quiesce protocol (cR3)

`RenderWorker::Quiesce()` (owner thread, **never under `gpu_submissions`, `model_storage_lock`, `model_stream_lock` or any other native lock**): post nothing new; wait for the worker's idle event; then the owner may run `Drain`, `Resize`, `Close`, depth recreation, `ReadStartupView`; `Resume()` after. `Close()` starts with `Quiesce()` before it takes any lock (today `:792-797` takes `gpu_submissions`, `model_storage_lock`, then `Drain`; `CloseNativeModelStreams` `:1645-1646` takes `model_stream_lock` then `gpu_submissions`, so it also quiesces first). The global lock order including the idle wait is written once in the header of `render_worker.h`: *idle wait before any native lock*. `Close` sets the stop flag (the original's `thread+0x10`), wakes the worker, joins. The native quiesce wait carries a **proposed** deadline (30 s, then fault): the original's wait has none (section 1.1), so a hang there is a native decision, logged as such.

## 6. R14: pacing

### 6.1 The original's pacer (K-0009, STATIC)

`FUN_1400D5CB0(output, interval, flags, X)` (valid intervals 1 to 4; larger asserts in `FUN_1400D5DA0`): on the first call it presents at once and anchors `QPC`; later, when `DAT_14473D0D8` and `output+8` are set and `interval != 0`, a predicate (`FUN_1400D5DA0`, plus the DXGI-sync discriminator `output+0x0C`) chooses between **direct DXGI synchronisation** and **manual pacing** (I-0004: with `state+0x08` non-zero, DXGI sync is accepted only for interval 1 when the descriptor-derived refresh value `state+0x0C` is 59 or 60; any other valid non-zero interval selects the manual branch; interval 0 passes through). Manual branch (`FUN_1400D5E30`, plus a call to `FUN_141928190` in `D5CB0`): `deadline = anchor + interval * freq / 60`; if `now < deadline` and the remaining time exceeds 17,000 us, `Sleep(ceil(remaining_us / 1000))` for the coarse part; then a pure spin on `QPC` until the deadline (`FUN_1417E8070(0)` returns at once); the final `QPC` becomes the next anchor, the cumulative interval counter advances, the function returns 0 and `Present(0, 0)` is called. Otherwise it returns the interval unchanged and `Present(interval, 0)` runs. `DXGI_STATUS_OCCLUDED` is accepted silently. Not established: runtime activation, the sole-limiter status, the source and writer of the interval (`[thread+0x58]+0x40`, OPEN).

### 6.2 Native `FramePacer` (cR4)

- A pure class with an injectable clock: `Pace(interval, refresh_hz, mode) -> present_interval`. Same maths as above (`60` as the divisor, `17,000 us` threshold, `Sleep(ceil)`, spin tail, anchor update). Runs on the worker immediately before `Present`.
- Interval source: the original reads it at callback time on the render thread. Reading it at post time into `FrameJob.interval` is a **proposed** deviation (one frame older at most); until the object is identified (live row RP-1) the default is the current behaviour, `Present(0,0)`, so nothing changes without the pacer.
- **Ownership with phase D:** the present interval, the tearing flag and the waitable object belong to phase D's cD8/cD9 (`legacy-present.flag`, presence = new behaviour until inversion). The pacer is part of that same present path and uses **that flag**, not a second one: `FramePacer` is enabled by cD9's new behaviour when the pacing mode is selected, and its absence is the cD9 legacy path. When the pacer returns 0 on a tearing-capable chain the `ALLOW_TEARING` flag is used; the waitable object bounds `interval == 0` with no pacing; the pacer never runs with exclusive full screen without an interval (DISPLAY-SWAPCHAIN-NATIVE section 5).
- Where it blocks: the worker. The engine thread blocks only at the next post, as in the original.

### 6.3 Latency

The worker adds frames between the engine's input latch and `Present`. With `slots = 2` (phase D) and a depth-1 job queue the worst case is **two frames** more than the single-thread build (one queued job, one in flight), not one; the original has the same structure (two slots, depth-1 queue). New counter `input_to_present_ms`; the exit check is "not worse than the original's by more than the structural frames at the same frame time".

## 7. Expected gain

Baseline (M4, S1, [PREP section 10](D3D12-PHASES-A-C-PREP.md)): 6.73 fps = 148.6 ms; `collect` 104 to 106; `prepare` 25.7 (validate 1.8, resources 11.7, draws 9.4); `render` 5.0 (M3 split: record 0.7, wait 1.4, present 0.8; the rest is movie, menu views and UI admission); the remainder 12.9 ms is engine work outside the native phases. K1 (Native1289): collect 110 to 121 ms in that run, of which `MeshBlocks` 68 to 75, `MeshTextures` 14, `MeshPrograms` 8.4, `ModelAdmission` 5.4, `Teardown` 4.4.

All gains below are **ceilings, computed from the M3/M4 tables, not measured for a worker**:

| Stage | Engine-thread time (ceiling) | fps (ceiling) | Change | Why it is a ceiling |
|---|---|---|---|---|
| today | 148.6 | 6.73 | | |
| F1: render + present on the worker | 143.6 | 6.96 | -3.4 % time | the UI admission build (inside the `render` slice) and the job construction stay on the engine thread; the 1.2 to 1.4 ms GPU wait is already removed by phase D (cD6), so on a post-phase-D base F1 hides only about 3.7 ms (-2.5 %) |
| F2: prepare also on the worker | 117.9 | 8.48 | -20.7 % time | R8b S1 to S4 move work into collect and F6 repeats the prepare on the engine thread |
| F2 and collect -10 ms (K caches, parallel mesh blocks) | 107.9 | 9.27 | +0.79 fps for those 10 ms | |
| F2 and collect halved (about -52 ms) | 65.9 | 15.2 | the only route to a large gain | |

The marginal value of 10 ms of collect is about +0.5 fps at 148.6 ms, +0.8 fps at 118 ms, +1.0 fps at 108 ms: it grows as the frame shrinks. The worker's own load in F2 is 30.7 ms per frame against 117.9 on the engine thread: the headroom is about 87 ms. M8 measures all of it.

**Where the 105 ms can go (beyond R13).** The original's own answer is parallel recording across pass scenes on a task pool with a helping drain (section 1.2), which the native build does not run. The K caches (P1 to P6, [K-COLLECT-IMPLEMENTATION-SHEET](K-COLLECT-IMPLEMENTATION-SHEET.md)) remove repeated pure work; the `MeshBlocks` loop (68 to 75 ms: the shader-register blocks of every resolved recipe for both stages) is **INFERRED** to be independent per mesh once the visitor has admitted the meshes (to be proven by K1 before use), so a task pool with the original's pattern (push tasks, release a counted semaphore, the producer helps, a barrier before the post) is the natural second step. It needs the K plans' state to be per-scene or immutable and no engine writer running during collect (R8B-1). It is a separate sheet once K1 and the caches are measured.

## 8. The worker protocol

### 8.1 Lifecycle and queue

- Created at `Start` when `render-thread.flag` exists; normal priority; name "ds3-native-render"; stop flag + event; joined at `Close` after `Quiesce`. The class lives in its own translation unit `native/renderer/render_worker.h/.cpp` (linkable by the test harness, section 10.1).
- **Queue:** one job slot, one idle event (manual-reset like the original's `thread+0x48`), one "replay finished" event (the original's `thread+0x38`), one wake event. `Post(job)`: wait for the worker's previous job. Two wait shapes, choose one and say so in the code: **(W1, original-faithful)** wait unbounded for "replay finished", then idle in 30 ms slices with the message pump between slices; **(W2, recommended)** `MsgWaitForMultipleObjects` on the idle event with `QS_ALLINPUT`, dispatching messages whenever they arrive, plus the **proposed** 30 s deadline. W2 is recommended because the native owner thread is also the window's thread (INFERRED) and a pump that runs only after the replay can starve window messages for a whole replay; W1 is the fallback if RT-6 shows re-entrancy trouble with W2.
- **Re-entrancy rule:** pumped messages may re-enter the engine's window procedure from inside `EndFrame`; therefore `Post` and `Quiesce` are never called under a native lock (`gpu_submissions` is held by `Clear()` from `:651`: the post is the last statement of `Clear()` after the lock scope ends, before the clock write). Test T-R9 and row RT-6.

### 8.2 Worker loop and fault channel

- **Loop:** take the job, `FrameRing::Begin`, record the single list (movie, views, UI, main), `Execute`, arena `Retire`, `FramePacer`, `Present`, `Signal` -> slot ticket, deferred retirement of the slots that completed, publish completed witness results and the receipt, update the X7 mirror, set idle.
- **Fault channel:** the worker's catch stores a `std::exception_ptr` and sets a fault flag; the owner thread tests it at `BeginFrame`, at `Post` and at `EndFrame`. `EndFrame`'s catch only handles `std::exception`, so the rethrow is wrapped (`std::runtime_error` carrying the message) to reach `ExitProcess(0xD3126607)`; `BeginFrame` is `noexcept`, so there a set flag exits through `Refuse` (`0xD3126606`), never a throw. `Refuse` called on the worker exits the process directly (as today). A thrown job leaves frames in flight: the worker marks the ring faulted and the destructor quarantines it (phase D section 3.6).
- **Startup, title and clear frames** use the same job with `models == null` and the `primitive_generation`/stencil flags of section 4.1.
- **DXGI and the window thread:** `Present` runs on the worker; `ResizeBuffers` and full-screen changes stay quiesced on the owner thread.

### 8.3 Frame clock

The original writes `clock+0x40` (`EEE190`, before the post) and `clock+0x48` (`EEE160`, after the post returns) in `D56930` (STATIC, exports added). The native `EndFrame` today writes `clock+0x48` after `Clear()`, i.e. after `Present`. **This sheet replaces the phase D sheet's R8 ("the clock must still be written after `Present` returns"), §3.2 step 5 and PHASES-D-G-PREP 2.7 Pitfalls ("must be preserved")**: with a worker the native `Clear()` returns after the post; the clock is then written exactly where the original writes it (after the post), which moves the engine's measured dt from "frame including `Present`" to "frame including the post wait" (the same quantity as the original, since the original's post wait absorbs the previous frame's present/pacing). It is a change to the engine's dt and needs a witness: row RT-9 (animation speed and the dt distribution, worker against single-thread build, and against an original run if one is available). Until RT-9 passes, the decision on the clock order is open and recorded as such.

## 9. Commits under flags

Flags are presence files next to the module, placed by the runner (convention of phases B and C). Each commit builds alone. Equivalences with other documents: cR1 = phase D cD-F; T-R2 = T-D2 extended to threads; PHASES-D-G-PREP 2.7 slices F1 to F4 and R8B S5 ("R13 F4") map to cR3, cR3/cR5, cR5, cR6 respectively; this sheet's F1/F2 are the two thread cuts, not those slices.

| Commit | Content | Files | Behaviour | Flag | Tests |
|---|---|---|---|---|---|
| cR0 | Counters and rows: thread ids at collect, prepare, record, window (settles R8B-2 and the window-thread point), `owner_frame_ms`, planned `worker_busy_ms`, `queue_wait_ms`, `input_to_present_ms`; `native_threads` row; X9 atomics | `game_bootstrap.cpp`, pacing row | none | | short run |
| cR1 | `DeviceQueues` hardening (5.2): submit mutex including the ingress swap and the retire loop, per-wait events, re-wait loop, atomic `pending_`, lock-order comment | `backend.h/.cpp` | none | | T-R2 |
| cR2 | Job-safe renderer: X1 (UI snapshot object, job-held retirement), X2 (scene copy in the job), X3 (`WitnessChannel`), X6, X7 (`WorkerMirror`), X10 (receipt cell); `FrameJob` built and executed **inline** on the owner thread | `scene_pass.*`, `primitive_pass.*`, `game_bootstrap.cpp` | none (inline) | | T-R5, A-C and D witnesses |
| cR3 | `RenderWorker` (own unit): depth-1 queue, pumped wait (W2), fault channel, quiesce/resume/stop (before any native lock), `Clear()` posts a **whole-frame** `FrameJob` (movie, menu views, UI, main pass in the one phase D list, so no engine-thread recording exists besides the job); `Post` after the lock scope; clock after the post; B5w (or `legacy-dynamic-streams.flag` required); receipt check on the previous job | `scene_pass.*`, `render_worker.*`, `game_bootstrap.cpp` (`Clear`, `EndFrame`, `Close`) | render and present on the worker | `render-thread.flag` (presence = worker; default off) | T-R1, T-R3, T-R4, T-R8, T-R9, RT-1 to RT-6, RT-9 |
| cR4 | `FramePacer` on the worker; interval from the engine settings; uses cD9's flag (6.2) | `scene_pass.*`, new `frame_pacer.h/.cpp` | pacing | cD9 `legacy-present.flag` | T-R6, RP-1 to RP-3 |
| cR5 | Prepare on the worker (F2): R8b S1 to S4 present, guard F6 clean, `PrepareNativeModelFrame` reads job-class snapshots only; B5a becomes valid | `game_bootstrap.cpp` | CPU off the engine thread | `prepare-thread.flag` (needs `render-thread.flag`) | T-R7, RT-7, R8B-1, R8B-6 |
| cR6 | **M8 run** (owner-thread busy time against worker busy time, S1 to S5, with and without the debug layer); defaults inverted if the witnesses pass (`single-thread.flag` = legacy) | runners | defaults | | exit checks |

Order rationale: queue hardening and the shared-state fixes change nothing and are testable with the existing single thread; the worker (cR3) takes the whole frame because phase D's single list (cD3) is a prerequisite and a half-frame split (movie and views on the engine thread, the rest on the worker) would put two recording threads on one allocator, arena and queue; pacing last among the render commits because it needs the interval source and the worker; prepare on the worker last because it depends on the R8b snapshots.

## 10. Tests and witnesses

### 10.1 Host/WARP tests (`native/frame/Build-GameBootstrap.ps1` harness: MSVC `/O2 /MT`, no TSan; new units added to the link lines `:162-165`)

| Id | Content |
|---|---|
| T-R1 | `RenderWorker` (own unit, linkable) with a fake job body: depth-1 semantics (the second `Post` blocks until the first completed), order preserved over 10,000 jobs, the pumped wait dispatches a posted window message, stop joins. |
| T-R2 | Queue hardening: 2 to 4 threads call `Execute` and `Wait` concurrently for 10,000 submissions with a forced overlap (queue wait on a CPU-signalled fence, realistic on WARP) and a test hook that records the order of `Signal` calls: tickets strictly increasing in `Signal` order, the fence value never decreases, ingress lists precede the frame list of the same caller's ticket, no wrong wake-up, no "invalid completion". **Negative control:** the same test against the current `DeviceQueues` reproduces "invalid completion" or a backwards fence value, proving the test can see the race. |
| T-R3 | Fault channel: an exception in the job body reaches the owner at the next `Post`/`EndFrame` (and, from `BeginFrame`, exits through the `noexcept`-safe path), the ring is quarantined, nothing in flight is released; the proposed deadline (shortened in test) is a fault. |
| T-R4 | Quiesce: `Drain`, `Resize`, `Close` run only while the worker is idle; a job posted during `Quiesce` is refused or queued after `Resume`; `Close` quiesces before taking any native lock. |
| T-R5 | Snapshot and affinity: the UI snapshot is not modified by the next admission while the worker encodes it (admissions at full speed against a worker that encodes slowly, with generation tags checked on both sides); the scene-copy pointer in the job is used, not the member; **thread-affinity assertions on the public `SceneRenderer` and `DeviceQueues` methods** (debug builds) stand in for the missing sanitiser. |
| T-R6 | `FramePacer`: injected clock; intervals 0, 1 (59/60 Hz DXGI path and other refresh), 2, 3, 4; the coarse sleep only above 17 ms, spin tail, anchor update, cumulative counter, deadline arithmetic against hand-computed values; late frames do not accumulate debt beyond the anchor reset. |
| T-R7 | Equivalence: for a recorded `NativeSelectedScene`, the draw inputs produced on the worker from snapshots equal those produced from live reads (the F6 comparison as a test over fixtures); zero engine calls (counted) during prepare on the worker. |
| T-R8 | B5 under F1: prepare thread != record thread; the stream copy is allocated and recorded by the worker, the version is immutable, `copied_generation` written by the worker only, no cross-thread `UploadArena` use (affinity assertion). |
| T-R9 | Pump re-entrancy: a posted window message whose handler calls back into the bootstrap (a `WM_SIZE`-like request that wants a native lock) while `Post` waits does not deadlock (the post is outside the locks); the same test run with the post deliberately placed under `gpu_submissions` deadlocks or asserts, proving the rule. |

### 10.2 Live witnesses (D-0116 protocol, announced; never from the static thread)

| Row | Action | Expected |
|---|---|---|
| RT-1 | `render-thread.flag` against off, S1 and S4, 600 frames | identical image on a still scene; `owner_frame_ms` down by about the render time; no device removal |
| RT-2 | injected present failure and an injected exception in a job | single exit with `0xD3126607` (or `0xD3126606` from `Refuse`); process ends cleanly |
| RT-3 | Status with portraits, character creation, Bink then title, with the whole-frame job | same frames as the single-thread build; views and movie in list order |
| RT-4 | quit in game, quit during a load, quit during a menu view, area change | exit 0, no hang, no use after release (debug layer) |
| RT-5 | foliage with wind, a character with cloth, a phantom, a gparam transition, a dynamic-stream model | equal to the single-thread build (the F6 witnesses of R8b) |
| RT-6 | thread ids and window behaviour: the window stays responsive while the post waits (W2); no re-entrant `WM_SIZE` fault; whether the window's thread is the owner thread | log rows; settles the message-pump re-entrancy and the INFERRED window-thread point |
| RT-7 | cR5 with R8B-1 / R8B-6 | zero differences, zero engine calls in prepare on the worker |
| RT-9 | frame clock: animation speed and the `dt` distribution (mean, p95, max) with the clock written after the post, worker against single-thread build | same animation speed; `dt` distribution explained (section 8.3); decision recorded |
| RP-1 | Read the engine settings object at the original's `thread+0x58` equivalent in an original run and in a native-bootstrap run: interval, flags, the byte copied to `DAT_144731BD0` | identifies the type and the writer; confirms the native read and the callback-time vs post-time deviation |
| RP-2 | Pacing with the original's interval on a short-frame scene (title or a menu if it runs faster than 60 fps) | present cadence follows the interval; no change when frames are longer |
| RP-3 | Input latency (`input_to_present_ms`) worker against single thread | within the two structural frames of 6.3 |
| RT-8 | soak, 30 minutes across areas, menus, a fight | stable private bytes, descriptor counts, `queue_wait_ms` and `slot_wait_ms` |

## 11. Risks and open points

1. **Hidden shared state.** Section 3.3 lists what was found (ten items after review); the sanitiser is not available in the harness, so the net for what was not found is the affinity assertions of T-R5 and the generation tags, plus RT-8. Static locals in `Clear()` (pacing, counters) are single-threaded today and become two-threaded (X9).
2. **Re-entrancy of the pumped wait** on the owner thread (the engine's window procedure called from inside the frame end): the original does it, but only after the replay and in 30 ms slices; the native W2 pumps earlier. Safety is OPEN (T-R9, RT-6); W1 is the fallback.
3. **R8b is the gate of F2** and is the largest change to engine contracts (D-0157); the permanent comparison F6 is not optional.
4. **Latency** grows by up to two frames (6.3); at the current frame times that is large in absolute terms, which the exit check measures.
5. **Engine dt change** (8.3): the clock moves from after `Present` to after the post; RT-9 decides.
6. **OPEN:** the writer of `ctx+0x80`, the type and writer of the settings object at `thread+0x58` and the interval's source; the thread creation and size of the GX task pool; the thread of collect (R8B-2); the window thread; whether DXGI is happy with `Present` on a non-window thread under the debug layer (T-R2 and RT-1 under the layer).
7. **Not designed here:** the parallel scene pool (section 7), COPY/compute queues (phase G).

## 12. Evidence and limits

Original: `decompiled/DS3-1152-F-` `0000A7B0`, `0000A440`, `0000B3C0`, `0000AA90`, `000D6270`, `000D6300`, `000D63E0`, `000D1C40`, `000D1D30`, `000D1EF0`, `0020E720`, `002046B0`, `00204240`, `00D56930`, `00EEE160`, `00EEE190` (both added by this sheet), `000063D0`, `000D5CB0`, `000D5DA0`, `000D5E30`, `017E8070`, `017F1BD0`, K-0009, K-0110 to K-0115, K-0129, K-0245, COLLECT-ORIGINAL-STATIC. Native: `native/renderer/{backend.h,backend.cpp,scene_pass.h,scene_pass.cpp,model_frame.h,primitive_pass.cpp}`, `native/frame/game_bootstrap.cpp`. Documents: D3D12-PHASE-D-IMPLEMENTATION-SHEET, D3D12-PHASES-D-G-PREP (2.7, 2.8), R8B-ENGINE-READS-STATIC, K-COLLECT-IMPLEMENTATION-SHEET, COLLECT-ORIGINAL-STATIC, I-0004 RESULT, DISPLAY-SWAPCHAIN-NATIVE, ER-DEEPENING-897, D3D12-DESIGN-REVIEW. No build, run or measurement; gains are computed from the M3/M4 tables and the K1 split and are ceilings.

## Amendments by the phase F sheet

The [phase F sheet](D3D12-PHASE-F-SHARED-STATE-SHEET.md) adds the census of the locks, thread-locals and globals the cut touches and amends this sheet: a pre-flight commit cF.1 to cF.7 before cR2 (cF.4, cF.5 and cF.7 condition F1; cF.1 to cF.3 and cF.6 condition F2); the lock order is one chain plus an exception to remove (the menu-view fallback collect runs under `gpu_submissions` and takes `model_stream_lock`, the reverse of `CloseNativeModelStreams`); process exit terminates the other threads first, so the worker object and the renderer are detached or leaked and nothing run at detach may take a lock the worker can hold; two more sync points (the pending movie frame and the offscreen scene list); the thread-local `native_offscreen_preparing` and the witness file writes leave prepare before F2.

## Amendments by the engine step and thread map

[ENGINE-STEP-THREAD-MAP](ENGINE-STEP-THREAD-MAP.md): the engine frame is a table of 88 rows with six lanes; the collect and the render step run in the `DrawStep` row (79), after which rows 80 to 87 (`DrawBegin`, `GameSceneDraw`, `AdhocDraw`, `DrawEnd`, `Flip`, `DelayDeleteStep`, `RecordHeapStats`, `FrameEnd`) still run on the same frame. A prepare that runs on the render worker (F2) therefore overlaps those rows and the **next** frame's update rows, whose character steps (`g63`) run on the owner thread and on pool threads and rewrite the model state that prepare reads: R8b S1 to S4 must snapshot at collect, exactly as written here, and no engine object may be read from the worker. `DelayDeleteStep` (row 85) is the engine's delayed-deletion path, so retirement hooks must not free what the worker still holds. cR0 gains the passive rows TM-1 to TM-5 of the map.

## Implementation log

**cR1, cR0 and cF.4 (2026-10-12, pilot thread, commits `4835c1d1`, `50b56eb2` and the review fixes `c452ac90`, Native1357-1360).**

cR1 (= phase D cD-F), what changed (5.2): one `submit_mutex_` over the ingress swap, `ExecuteCommandLists`, the ticket, the `Signal`, the submitted ticket and the retirement of the ingress lists, and over the ticket and signal of `Drain` and `Signal()`; each wait takes its own event from a pool (`ResetEvent`, registration, a re-wait loop to the 5 s deadline; a failed wait's event is never reused; a fence already passed takes no event); the dead `pending_` flag is gone; `LastSubmitted()` is documented as a lower bound under concurrency (safe through rule L2 only). T-R2 (`frame_ring_test`): four threads submit and wait 1,500 times each behind a gated queue, a fifth commits and submits ingress lists, a sixth signals and drains, a sampler checks that the completed value never goes back; no wait throws. Not built: the Signal-order hook and the negative control against the former queue (the sampler and the deadlines are the detectors).

cR0, what changed (thread map TM-1, TM-4, TM-6): a lock-free census of the threads that call the engine hooks the worker will share data with, a `native_thread_census` row every 600th frame against the owner thread, and `native_retirement_caller` rows for the first four engine retirements of each kind (the backend's own replacement calls excluded, the caller RVA only inside the game image). Not done: TM-2, TM-3, TM-5 (Ez task step ids, captured stacks at the frame hooks, `ChrIns` task counts), `worker_busy_ms`, `queue_wait_ms`, `input_to_present_ms` (they need the worker).

Observed (Native1358, Firelink, frame 7201; owner thread 57952):

| Hook | Threads (calls) |
|---|---|
| model storage admission, CPU geometry | one non-owner thread (627) |
| model storage retirement | that thread (1,254) and the owner (70); destruction on the owner |
| encoded texture admission and retirement | five threads, none the owner, plus the owner for 361 retirements |
| pose initialisation, pose retirement | owner only |
| pose updates (single, public) | the owner and three other threads (pool) |
| physical stream initialisation and retirement | owner; updates on the owner and one other thread |
| cloth uploads | eight threads (the owner among them) |
| cloth set release | owner |
| Scaleform task dispatch (TM-4) | **one thread, not the owner** (the Scaleform step) |
| scene CPU update (collect) | owner |

Consequences for cR2 and cR3: every hook that publishes data the render path reads already runs off the owner thread somewhere (poses, cloth, textures, storage), so the snapshot and lock rules of the phase F sheet are confirmed necessary; the retirement hooks the delay-delete audit asked about run on the owner (poses, streams, cloth) or on the loader thread (storage, textures), never in a render job so far (`job_in_flight` false by construction before cR3).

cF.4, what changed: the module's lock order (phase F sheet 3.3) written once next to `gpu_submissions`, and `CloseNativeModelStreams` no longer holds `model_stream_lock` while it takes `gpu_submissions` (PILOT D-2: the flag under the stream lock, the drain under the submission lock, the map emptied after). Not done: the debug "held level" assertion helper, which goes with `render_worker.h` (cR3).

Live: Native1357 (High Wall, portraits, Equipment), Native1358 (Firelink, cloth NPCs, phantom), Native1359 (Bink, character creation): 0 debug errors, 0 shadow mismatches; exit 0 except the known character-creation close fault. Native1360 (fixed build, High Wall): exit 0, 0 debug errors, 0 shadow mismatches; the engine callers of the retirements are now distinct (storage RVA 0xA041E on a loader thread; physical streams 0x51B91C and 0xD3F7D5, poses 0xD17AB9, all on the owner).

Opus review (cR1 + cR0): no blocker; fixed: the retirement rows polluted by the backend's own calls, the cache-line sharing of the census, the dead flag, the event taken for a passed fence, the T-R2 drain thread and sampler guard; noted: T-R2 weaker than 10.1 (above).

**cF.7, cF.5 and cR2 (2026-10-12, pilot thread, commits `5d789071`, `2da9df6d` and the follow-up below).**

cF.7 and cF.5 (phase F sheet): the renderer is a leaked `*new SceneRenderer` (process exit ends the other threads before static destructors, PILOT D-10) and every module exit goes through one `ExitNative` seam; `scripts/Test-ThreadAffinity.ps1` checks the named render and prepare functions against the reviewed allow-list `scripts/thread-affinity-allow.json` (41 entries) and reports the S-1 prepare reads F2 must remove (7 with the collects below: the hair witness dumps, the journal path of the movie and view witnesses, `native_offscreen_preparing`).

cR2, what changed (section 4.1, X1, X2, X7, X10 of 3.3):

- **The frame is one job.** `NativeFrameJob` (`scene_pass.h`): the serial, the world frame (or none), the movie frame with its target and the target's record, the menu views in order (prepared frame, target, record, background, witness file), the UI build, its generation and the stencil flag, the scene-colour copy. `Clear()` builds it on the engine thread (`CollectNativeMovieFrame`, `CollectNativeOffscreenViews`, the admission) and `SceneRenderer::ExecuteFrame` records movie, views, then the world frame or the UI/startup frame into the frame's single list, inline. A movie or view failure keeps its own row and refusal (`NativeFrameStageError`).
- **X1.** `AdmitPrimitiveWork` returns the build as a `shared_ptr<const PrimitivePass>`; the job holds it and the frame slot keeps it until the frame's ticket; the superseded build is retired by ticket and lives while any slot holds it. The admission no longer creates the UI depth or clears the stencil flag: the frame creates the persistent depth (D-12) and checks the stencil by build generation (`primitive_stencil_generation_`). The former entries (`RenderNativeModels`, `RenderStartupPrimitives`, work mode, the reference control) pass the current build explicitly.
- **X2.** The forward pass uses the job's scene copy, not the member.
- **X7.** `faulted_` is atomic and the admission reads the last and startup serials from release-stored mirrors published at each write; the other members it reads (`open_`, `scene_ready_`, `incarnation_`, the extent) change only in owner operations that cR3 quiesces.
- **X3, X6** were done by cD4; **X10** (the receipt of the previous job) needs the worker (cR3).
- **T-R5, inline part** (`startup_primitive_test`): an admitted build held by a job is untouched by the next admission; a build of another serial, a wrong generation and a new build whose stencil the frame does not initialize are refused without poisoning the renderer; the job draws the build it holds.

Not done: the affinity assertions of T-R5 (they need the worker's thread id, cR3).

Opus review (cF.7, cF.5, cR2): no blocker. Fixed: a world frame without a UI build while one is current is refused again, as before cR2 (the first form drew it without UI, silently); the job's world frame is `const` and complete before it enters the job, and `ExecuteFrame` checks its UI fields against the job's; a non-standard exception from the movie or a view keeps its refusal; a failure creating the UI depth marks the renderer faulted; `Test-ThreadAffinity.ps1` runs from `Test-Project.ps1`, scans class members (`NativeGparamRecords>Build`, one more counter reviewed, 42 entries) and the `ExitNative` comment no longer claims a setter. Noted for cR3 (the next pilot): `primitive_uploads_pending_` (admission and frame), `primitive_stencil_initialized_` (also written by `WithdrawPrimitiveWork`), `scene_colour_copy_` (still a member written by `SceneColourCopy()` on the engine thread), the extent (X4 quiesce), `WaitFrameSlot()` still in `Clear()`, the job-held texture records released on the worker; the affinity scope should add `PrimitivePass::Encode`, `PrimitiveDepthTarget`, `NativeModelTargets`, `DepthTarget`, `Readbacks`, `TakeReleased`, `UploadArena::Allocate` and `FrameRing` (no statics found in them); `primitive_reference.cpp` is built only by the I-0038 scripts (its edits are not compile-checked).

Live: Native1361 (High Wall, cR2 as committed `2da9df6d`), Native1362 (Firelink) and Native1363 (Bink and character creation) on a build that also carried part of the follow-up: exit 0, 0 debug errors, 0 shadow mismatches, the same hair witness refusals as Native1357. Native1364 (High Wall) and Native1365 (Bink and character creation) on the final build: exit 0, 0 debug errors, 0 shadow mismatches.

cR3 prototype (not merged, branch named in STATE): `native/renderer/render_worker.{h,cpp}` (depth-1 queue, W2 pumped wait with the 30 s deadline, fault channel, quiesce/resume/stop) and `native/frame/render_worker_test.cpp` (T-R1 and the worker parts of T-R3 and T-R4, PASS when built alone); not yet in `Build-GameBootstrap.ps1` nor wired into `Clear()`.

**cR3 (2026-10-12, pilot thread, branch `claude/jolly-tu-563224`, from the prototype `600aa339`; Native1366-1373).**

What changed (sections 5.3, 8):

- **The worker.** `render-thread.flag` (runner `-RenderThread`) creates the `RenderWorker` at `Start`, after the renderer, and logs `native_render_thread` (worker and owner thread ids, `W2`). Without the flag nothing changes: the job runs inline in `Clear()` with the same lock scope as cR2. The worker is leaked like the renderer (D-10).
- **`Clear()`.** It builds the job under `gpu_submissions` (prepare, scene copy, movie, menu views, UI admission), then posts it after the lock scope (rule 0 of the lock order). The post waits for the previous job. The previous job's outcome (receipt, `lit`, velocity counts) is then checked and logged by `FinishNativeFrame` under the submission lock, as inline (X10). The job waits for its slot on the worker (`WaitFrameSlot` moved off the owner thread). The clock is written after the post, the original's order (8.3).
- **Faults.** On the worker, a movie or view failure keeps its own refusal. Any other exception is logged as `native_render_job_failure` with its serial and message, then kept. The owner rethrows it at the start of its next `Clear()`; `BeginFrame` (noexcept) logs it and refuses with `native_render_worker_faulted`.
- **Close and views.** `Close()` quiesces the worker before any native lock, rethrows a kept fault, finishes the last outcome and joins the worker. `ReadStartupView` runs with the worker quiesced (X4). `NativeSfxStartup` catches a throwing `Close()` (its hook is noexcept).
- **Shared state.** The renderer refuses `ExecuteFrame` and `WaitFrameSlot` off the render thread, and `AdmitPrimitiveWork` and `SceneColourCopy` on it (T-R5; `SetRenderThread`, one load per call). The slot, release, waitable and present counters that the owner thread's pacing row takes are atomic, and so is `primitive_uploads_pending_`. The 1186 velocity picture path travels in the job (it was a request the previous job could take).
- **Rows.** `native_frame_audit` gains `queue_wait_ms` (the owner's post wait per frame), `worker_job_ms` (the worker's job, slot wait included) and `pumped_messages`. `render_ms` stays the owner thread's time after prepare in `Clear()`: inline the build and the job, as before; with the worker the build only. The census's `job_in_flight` is true only while a worker job runs.
- **Tests.** `render_worker_test` is now built and run by `Build-GameBootstrap.ps1`. It covers T-R1, the worker parts of T-R3/T-R4, T-R9 (a re-entered handler takes a module lock the post does not hold, and finds it held when the post is made under it) and a quit request kept across the pumped wait. `startup_primitive_test` adds the worker part of T-R5: a job built on the owner runs on the worker and is refused on the owner; the admission is refused on the worker; neither faults the renderer. The `Test-ThreadAffinity.ps1` scope gains the worker's functions (`RunNativeFrameJob`, `WaitFrameSlot`, `TakeReleased`, `Readbacks`, the depth and target builders, `PrimitivePass::Encode`, `UploadArena::Allocate`, `FrameRing` Begin/Retire/End, `DeviceQueues::Execute`, the `RenderWorker` loop and waits): no new static.
- **Runners.** `Run-StartupTrace.ps1 -RenderThread`, passed by `Run-PilotPair.ps1` and `Run-PrepareGate.ps1` (series suffix `-rt`, composed with `-slots2`).

**Process-exit fault found on the way (Native1367 and Native1370, both modes).** Measurement runs ended with `0xC0000005` at the game's `0xF14C52`, after the window closed. That address is the engine's abort handler (`mov [0], 0xDEAD`), reached from CRT `abort` (`141FD0EAC`), from a pure virtual call in the engine's free at `0x2B7B80`. The caller was a static destructor of this module (frames resolved against the DLL: the CRT exit table, then the deleter of a parsed FXR buffer held by the latest deferred SFX request's static store). At process termination the engine's heaps may already be destroyed when this module's statics are. Fix: `DllMain` sets `native_process_terminating` on `DLL_PROCESS_DETACH` with a non-null reserved argument, and the releases into engine memory then skip themselves (the parsed FXR deleter, `FreeModelPrefix`, the CPU image owners of `1BB6D00`); the process reclaims them. Whether this hazard is new since cF.7/cR2 or was latent is not established: the nine earlier measurement runs (Native1314-1356) exited 0, and no measurement run was played between Native1356 and Native1367. Native1371/1372 (both modes, fixed build): exit 0.

Live (D-0116, offline, Thomas's save and configuration restored by hash, every restoration `ok`):

| Run | Content | Result |
|---|---|---|
| Native1366 gate-rt | High Wall, debug layer, MotionBlur HIGH, in-game taps, Equipment page, both shadows | exit 0, 0 debug errors, 0 mismatches over 87 shadow rows; 1,062 draws; physical stream retirements on the owner thread while a job is in flight (held by the job, L2) |
| Native1367 time-rt, Native1370 time | measurement, worker and inline | the exit fault at `0xF14C52` (above); frames otherwise normal |
| Native1368 firelink-rt | teleport to Firelink (area streaming, cloth NPCs), white phantom, camera turns | exit 0, 0 debug errors, 0 mismatches over 165 rows |
| Native1369 charmake-rt | Bink opening movie, character creation | movie frames 240 and 2400 identical to the pixel with Native1365 (inline); the creation view matches by eye (the idle animation differs between any two runs, inline included); the known close fault at `0xD12759` (Native1317) |
| Native1371 time-rt, Native1372 time | measurement on the fixed build, worker and inline | exit 0 both |
| Native1373 gate-rt | High Wall protocol on the fixed build | exit 0, 0 debug errors, 0 mismatches over 89 rows; velocity pictures named by their own frame (`velocity-3690.rgba`) |

Measurement at S1 (High Wall, no debug layer, medians of 120-frame windows, same build): inline Native1372 17.56 frames/s, `render_ms` 5.06; worker Native1371 18.13 frames/s, owner build 2.10 ms, worker job 2.45 ms, owner post wait 0.55 ms per frame; collect 26.6 and prepare 15.5 ms unchanged. The pair Native1367/1370 (before the fix) gave 18.96 against 17.53. F1 hides the record and the present (about 3 ms), as section 7 predicted; it stays infrastructure behind its flag (D-11).

Not done (rows of 10.2 still open): RT-2 (an injected present failure and job exception), RT-4 beyond the window close (quit to title, quit during a load or a menu view, area change by the menu), RT-6 window behaviour beyond the pump counter (Native1366: 3,462 messages dispatched by the pumped wait, no fault), RT-8 soak, **RT-9 (the engine `dt` with the clock after the post): open, so the clock order is not yet decided**. B5w is not needed: since cD5 the dynamic streams are immutable pooled versions, published on the engine threads and held by the draws, never an arena range at prepare.

Opus review (fresh agent, the whole diff): no blocker. Fixed: a worker fault surfaced under the UI admission's refusal (now rethrown first, with its own row); `render_ms` keeps the inline meaning; the velocity picture travels in the job; `render_worker_test` is run by the build; after a quit request the pump keeps processing sent messages (DXGI on the worker may send to the window); `job_in_flight` is false inline; the noexcept SFX hook catches `Close()`; the gate suffixes compose; the negative control of T-R9 holds the lock on another thread. Noted: `RetirePrimitives` can submit ingress lists from the owner thread while the worker submits (safe through the cR1 submit mutex; commented).

Second Opus review (the fix commit `2bf29e8e`): no blocker. Applied: the inline `render_ms` stops before the job is released, as before cR3. Noted: `worker_job_ms` can be skewed by about one job per 120-frame window (its two counters are exchanged separately); `GameUiImageOwner`'s release (`0x1BB6D00`, `game_ui_image_owner.cpp`) has no process-termination guard, safe while the UI sessions stay leaked; the sent-message branch after a quit request has no test.

### cR5: the world frame prepared on the render worker (2026-10-12, pilot thread "Thread de rendu et prepare")

Pre-flight, behaviour-neutral (phase F sheet section 5): cF.1 `TextureWitness` takes the view kind as a parameter instead of the thread-local; cF.2 `native_prep_ticks` atomic, every prepare static checked atomic (allow-list notes updated); cF.3 the one-time hair programme dumps (`std::ofstream`, `control->journal`) removed from prepare; cF.6 `AcquireNativeLightingPrograms` no longer holds its static mutex across `FindLoadedProgramme`. Prepare reduction cP6 (R8b S1 to S3): the group's first 0x190 bytes and the animation flags and block (`NativeSelectedModel::EngineBytes`) and the default row 4929B20 (`default_param_row`) are copied at collect; cP7 (S4): the deferred lighting input (`ProduceNativeSceneLighting`) and each model's forward cbLight/cbFog (`CollectNativeForwardLights`) are built at collect (main and menu views). `PrepareNativeModelFrame` reads no engine memory and calls no engine function (static check S-1: none left in prepare).

cR5: under `prepare-thread.flag` (runner `-PrepareThread`, with `-RenderThread`), `Clear()` hands the worker a closure over the published (immutable) scene, the serial, the job's UI fields and the outcome; `RunNativeFrameJob` prepares the world frame before the slot wait and gives it to `ExecuteFrame(job, prepared)`. Menu views stay prepared on the engine thread, so prepare may run on both threads at once (its caches are locked, its statics atomic, its call-local tables per call). Without the flag, nothing changes.

| Run | Prepare | S1 frames/s | Engine thread (to update + update + UI + Clear, ms) | Note |
|---|---|---|---|---|
| Native1408 | inline | 39.1 | 2.9 + 5.6 + 5.6 + 11.3 | same build |
| Native1407 | worker | **58.5** | 3.6 + 5.4 + 5.8 + 2.1 | prepare 9.9 and record 2.7 ms on the worker |

Witness Native1406 (High Wall protocol, debug layer, both shadows, worker prepare): exit 0, 0 debug errors, 0 collect-shadow mismatches over 143 scenes, 0 prepare-shadow mismatches, restoration ok.

Witnesses with the worker prepare (debug layer, both shadows): Native1409 (Cemetery of Ash, teleport to Firelink Shrine, white phantom, camera turns): exit 0, 0 debug errors, 0 collect-shadow mismatches over 314 scenes; Native1410 (New Game, Bink, character creation): 0 debug errors, the known character-creation close fault at `0xD12759` (as Native1250/Reference1250); restorations ok.

Opus review (fresh agent): no blocker in the code. Applied: the guard the R13 plan asks before cR5 (RT-7, R8B-1): under prepare-shadow, `CheckNativeEngineBytes` reads the group bytes, the animation flags and block and the default row again in `Clear()` and compares them with the collect's copy (`native_engine_bytes_check`); Native1411 (High Wall protocol, worker prepare): 505,467 model checks over 4,200 frames, 0 differences, 0 debug errors, 0 shadow mismatches; the menu-view prepare no longer sets the thread-local (cF.1 complete); the scene lighting is built only when the scene has prepared rows; profile labels; the runner refuses `-PrepareThread` without `-RenderThread`. Noted: under the flag `prepare_ms` is the worker's time and `worker_job_ms` includes it (compare `-pt` series together); prepare on both threads serialises on `ModelDrawCaches::lock` (menu views only); the texture ingress publishes a record before its view (a pre-existing race with loader threads, out of scope). Not yet default: cR6 decides the defaults.

### cR6: the defaults (2026-10-12, D-0168)

The render worker and the worker prepare are the default; `inline-render.flag` (`-InlineRender`) and `inline-prepare.flag` (`-InlinePrepare`) keep the inline paths for A/B; `-RenderThread` and `-PrepareThread` are accepted and change nothing. M-run on the default build without any flag: Native1413 (S1, `-Measure -NoDebugLayer`) 59.1 frames/s, collect 5.2 ms and prepare 9.4 ms (on the worker), engine thread 3.9 + 5.2 + 5.6 + 2.0 ms; Native1412 (High Wall witness, debug layer, both shadows): exit 0, 0 debug errors, 0 collect-shadow mismatches over 145 scenes, 0 prepare-shadow mismatches, restoration ok.

### cR4 with cD9: the present pacer (2026-10-12, pilot thread, after Thomas saw stutter in a live run)

Measured first (new rows `native_frame_times`, the engine thread's frame intervals, and `native_present_times`, the intervals between presents, per 120-frame window: p50, p99, maximum, mean, deviation, frames above 17.5, 25 and 34 ms). Native1468 (S1, no debug layer, before the pacer): engine thread p50 16.67 ms, p99 19-21 ms, deviation 0.8-1.6 ms, 0 frames above 25 ms: the game's own limiter holds 60 frames/s with a jitter of a few milliseconds, and `Present(0,0)` passed it to the screen. **`FramePacer`** (`renderer/frame_pacer.*`): the original's maths (6.1) with interval 1 (the engine object of the original's interval is not read yet: RP-1 open): `Present(1)` at a 59/60 Hz output, otherwise a deadline of 1/60 s after the last present, a high-resolution waitable timer for the coarse part and a spin for the last millisecond (instead of the original's `Sleep(ceil)` above 17 ms), then `Present(0)`; a late frame presents at once and re-anchors; the first frame presents at once. It runs where `Present` is called (the render worker by default) through `SceneRenderer::PresentPaced`; the refresh rate is read at `Start` from the window's display (`EnumDisplaySettings`, logged in the time rows). Default on; `legacy-present.flag` (runner `-DisplayFlags legacy-present`) presents at once as before. Test T-R6 (`frame_pacer_test`, a fake clock and 30 frames on the system clock). **A/B on the same build:** Native1469 (paced) present intervals p99 16.7-17.0 ms, deviation 0.02-0.13 ms, engine thread deviation 0.05-0.45 ms; Native1470 (legacy) present p99 20-29 ms, deviation 1.5-3.1 ms; both 60.0 frames/s, record 2.6/2.5 ms; the display runs at 120 Hz, so this A/B took the hand-paced branch (1/60 s spin, `Present(0)`): `present_call_ms` (the call alone) 0.19 against 0.18 ms; `present_ms` of the pacing row (8.3 against 0.5 ms) and `worker_job_ms` include the pacer's hold, and the engine thread now waits 4-6 ms in its post (the pacer is the binding limiter). **Opus review: no blocker; applied:** an output whose refresh is a whole multiple of 60 Hz (120, 180, 240) goes to DXGI as `Present(interval x multiple)` (a labelled deviation from the original's interval 1 at 59/60 Hz only: a hand-paced `Present(0)` into the compositor drifts against the 8.3 ms grid), the refresh rate re-read every 120th present, a display-side measure (`GetFrameStatistics`: refreshes between presented frames, row `native_present_refreshes`), the host test widened (intervals 3-4, the clamp, unknown refresh, a tolerant system-clock bound). Open: RP-1 (the original's interval source), RP-3 (input latency with the sync wait), the waitable-present option with the pacer, exclusive full screen (D-0170's resolution and mode work).
