<!-- Copied from the Sonnet 5.5 study's scratchpad report (2026-10-20 18:48 Paris). Checked by Opus: 141B4CF40 (the entity visit: frustum classify unless the inside counter +0x34 >= 1, then the collect byte +0x13 appends {entity, level, aux} through 141B4CA40, else draws at once) and 141B4CD50 (the GXEntityDrawParallelJob over the gathered 16-byte list, min(records, pool) tasks), read in the exports. The exports themselves stayed in the session scratchpad (not evidence). -->

# ER 2.7.1 static study for I-0077: scene traversal, entity draw and work pool

Read-only, headless Ghidra 12.1.3 on `er-mirror-i0070-a/b/c` (analysis as stored, `-noanalysis -readOnly`).
All addresses are ER absolute VAs. Tags: **OBSERVED** = read in the decompile/asm/data; **INFERRED** = my reading.
Class names come from RTTI (`.?AV...@@` type descriptors found in `.rdata`, vtable located through COL -> vtable). Names of
methods/fields are my labels (INFERRED) unless they are RTTI class names.
Exports (C + ASM of every function cited, query logs): `exports/` (584 files). Script used: `scripts/ERQ.java`.

## 0. Class map (all OBSERVED, RTTI)

| Class (RTTI) | type descriptor | vtable | notes |
|---|---|---|---|
| `GXSG::GXLayerTraverseJob` | 143D318C8 | 142F249E8 | slot0 141ADB640 (deleting dtor), slot1 **141ADB710 = Run** |
| `GXSG::GXEntityDrawParallelJob` | 143D32EF0 | 143036A80 | slot0 141B4C950 (dtor), slot1 **141B4CBE0 = Run** (ticket loop) |
| `GXSG::GXDrawSceneJob` | 143D33508 | 143038C88 | slot1 **141B66030 = Run** (ticket loop over sub-scenes) |
| `GXSG::GXEntityDrawRegister` | 143D32E28 | 143036A70 | holds a copy of the collected entity records |
| `GXSG::GXSgTraverser` / `GXSgCullTraverser` / `GXSgEntityCollectTraverser` | 143D32EC0 / 143D32E90 / 143D32E58 | 14305B998 / 143059770 / **1430369F0** | base / frustum+distance culler / "collect" traverser (ECT) |
| `GXSG::GXSgWorldRenderer` | 143D33570 | 143038DD8 | a traverser too (non-collecting path), ctor 141B67000/141B67080 |
| `GXSG::GXSgWorld` | 143D2E058 | 142F1B420 | vt+8 = **141ADB860** = traverse all layers |
| `GXSG::GXSgLayerFlat` / `GXSgLayerDynamicTree` | 143D318F8 / 143D31AC0 | 142F24A78 / 142F25D78 | layers; DynamicTree vt+8 = 141AEA010 |
| `GXSG::GXSgNumTree` / `GXSgNumTreeNode` / `GXSgNumTreeRootNode` | 143D31A98 / 143D372F8 / 143D37328 | 142F25B30 / 143056C98 / 143056CE8 | AABB tree; node accept vt+0x28 = 141C52720 |
| `GXSG::GXSgCompositeSceneBase` | 143D2CB30 | 143038C08 | vt+0x38 = **141B660D0** (render a composite for one view) |
| `GXSR::GXSgCompositeScene` / `GXSgDrawScene` | 143D2CB10 / 143D2CBA8 | 142F0E840 / 142F0E8C0 | slot0 of GXSgDrawScene = **141A0DE50** (collect for one view) |
| `GXSR::GXSgShadowSceneImpl`, `GXSgReflectSceneImpl`, `GXSgBaseSceneImpl`, `GXSgPaintSceneImpl`, `GXSgEnvMapSceneImpl`, `GXSgVisibilityTestSceneImpl`, `GXSgSLODCaptureSceneImpl` | 143D2CBD8.. | 142F0E8D8, 142F0E920, 142F0E968, 142F0E9B0, 142F0E9F8, 142F0EA40 | per-pass scene kinds |
| `GXBS::GXDrawSceneManager` | (RTTI 143035CC0 vtable) | 143035CC0 | ctor 141B44D90; owns draw-record merge: 141B456E0 / 141B457A0 |
| `GXBS::Tasks::GXWorkerThread` | | 143057718 | pool worker (0x528 bytes each), thread proc 141C568A0 |
| `GXBS::GXRenderThread` | | 142F27140 | render thread, loop 141AF5180, ctor 141AF4DF0 |
| `GXBS::GXCommandRunner` / `GXCommandRunnerJob` / `GXSR::GXRenderCommandRunner` | | 143033DD0 / 143033E08 / 143034E30 | parallel command-buffer recording (run 141B3D940 / plan 141B3E890) |
| `CS::CSDrawStep` (FD4 step) | 143CF3B50 | 142B67DD0 | per-frame entry: +0xC0 = 140B15E80, +0xC8 = 140B16840 |
| `CS::CSModelDispEntity` / `CSDispEntity` (entities) | | 142B6C2C8 / 142B6C0B8 | entity vt+8 = Draw (140B31550 for model entity), vt+0x18 returns `this+0x40` (stub 140B305E0 = `lea rax,[rcx+0x40]; ret`, the cull-info passed to the traverser) |

Only three job classes exist in namespace GXSG: GXLayerTraverseJob, GXEntityDrawParallelJob, GXDrawSceneJob (OBSERVED from the
RTTI string listing of `GXSG@@`). There is **no** node-level, subtree-level or entity-range traversal job in ER.

Global renderer object: `_DAT_1447F33E0` ("g"). Work pool at g+0x5C8, pool size (workers incl. caller) at g+0x5D0, current
draw-record double-buffer index at g+0x708, render thread at g+0x6D0 (OBSERVED uses; names INFERRED).

## 1. GXLayerTraverseJob

**RTTI/vtable/run (OBSERVED):** vtable 142F249E8; `[0]=141ADB640`, `[1]=141ADB710` (Run).

**Creator (OBSERVED):** the only caller of the job constructor `141ADB560` (X-refs: only `141ADB860` + a .pdata entry) is
`141ADB860` = `GXSgWorld` vt+8, i.e. "traverse the world with traverser T":

```
for layer in world.layers[ world+0x18 .. world+0x20 )          // vector of 8-byte layer pointers
    if ( T->vt+0x40() /*canClone, byte T+0x12*/  &&  taskSystemEnabled() /*145517B90, DAT_143C294FD*/ )
        job = new(0x28) GXLayerTraverseJob(layer, T, p4);       // 141ADB560
        pool.push(g+0x5C8, job, workerIndex 0)                  // 141AF3030
    else
        layer->vt+8(layer, T);                                 // inline, the traverser itself
```

**Unit of one job: one layer** (a whole `GXSgLayerFlat` or `GXSgLayerDynamicTree`; OBSERVED). Number of jobs = number of layers of
the world, per traversed view (not per node, not per entity range, no adaptive split, no minimum size; OBSERVED).
Layer population (INFERRED from the creators): the world builders create **one DynamicTree layer** (`141AE9890`, sole caller
`140CCA7E0`) plus one Flat layer (`141ADBCA0`, callers 140BB8A70/140CC7370/140CCA7E0/140D8ED70). So a world yields about 1-2 layer
jobs per view, and the big tree layer is a single job: the tree walk itself is NOT parallel inside a layer (INFERRED from the
single-creator evidence + absence of node jobs). The parallelism of traversal is across layers, sub-scenes (Q5) and views.

**Run (141ADB710, OBSERVED):**
```
bVar1 = DAT_143C294FF && DAT_143C29502                      // both default 1 (data at 143C294F8: 00 80 00 00 01 01 01 01 01 01 01 01)
alloc = bVar1 && DAT_143C29503 ? &(currentThreadCtx()+0x30)  /*141B44460, per-thread allocator*/ : _DAT_1447F3718
clone = bVar1 ? job.T->vt+0x60(job.T, alloc)  /* clone made ON THE WORKER, inside the job */
              : job.clone  /* made at job creation (141ADB560) by the submitting thread */
clone->vt+0x68()                 // begin  (141B4CB00: creates the collector, stores worker thread id in context +0x69C)
layer->vt+8(layer, clone)        // traverse the layer
clone->vt+0x70()                 // finish (141B4CCB0: submits the collected entities as GXEntityDrawParallelJob)
delete clone; alloc->free(clone); delete job
```

**Traverser state (OBSERVED):** a fresh **clone per job**. `ECT::vt+0x60 = 141B4CB80` allocates 0xB0 bytes and calls ctor
`141B4C680(self, alloc, parent, parent+0x90)` -> `GXSgCullTraverser` copy ctor `141C693F0(self, alloc, parent)`, which copies the
parent's cull parameters (parent+0x20/+0x28/+0x30/+0x34, two 16-byte bound vectors at +0x40/+0x50) and **copies the parent's
bounds-stack vector** (`141C69360` -> `141C69250(begin,end)` of parent+0x68/+0x70) into the clone's own stack (self+0x60..+0x80,
allocator self+0x80). It also takes the parent's flags (+0x10 via vt+0x30, +0x12 = 1, +0x18 cull-strategy pointer) and creates its
own collector (`141B4C5D0`, 0x28 bytes, at +0xA0). Because every job starts at a **layer root**, there is never an ancestor bound or
"fully inside" state to carry: the question does not arise in ER (no job below the root). The ancestors' state of a tree walk is the
traverser's own stack (push by vt+0x18, pop by vt+0x28), kept per clone.

## 2. Where the outputs go, merge, ordering

1. **Collected entity records (traversal output).** The DynamicTree layer sets the clone's "collect" byte (traverser+0x13 = 1)
   when it can clone (`141AEA010`, instruction `141AEA02D mov byte [rbx+0x13],1`; OBSERVED). The per-entity visit
   (`ECT::vt+0x50 = 141B4CF40`) then appends a **16-byte record {entity*, int level(+8), int aux(+12)}** to the clone's private
   vector (`141B4CA40`, collector at +0xA0). No shared queue, no atomic: each job writes only its own vector (OBSERVED).
2. At the end of the job, `141B4CCB0` calls `141B4CD50(collector, ctx, drawCtxMgr)`: if the vector holds >= 1 record it copies it
   into a `GXEntityDrawRegister` (0x28 bytes, `141B4C480`), picks `n = min(records, pool size g+0x5D0)` and builds a
   `GXEntityDrawParallelJob` (0xA8 bytes, ctor `141B4C510`) with `n` pooled per-worker draw contexts (`141B67460`, atomic ticket
   `141EBC020(pool+0x20)`, contexts grown under a lock `141B675A0`), enqueues it (`141A74190`, **non-blocking**) and registers it in
   the draw manager's pending list (`141B46050`, vector at manager+0x178..0x188 under a critical section).
3. **Draw records (entity Draw output).** Each draw context holds thread-local record arrays (the 128-byte object built by
   `141AD1D00`: two small arrays at +1/+7 and a main array at +0xE with count at +0x7C). Records are **0x18 bytes**: u32 key at +0,
   u64 payload at +8, byte (pass/view) at +0x10. When a context is destroyed (`141AD1DC0`), it stable-sorts its own array
   (`141AD1B30`, only if count > 1) and **appends {ptr,count} to the manager's list (`141B454A0`, lock `manager+0x80`)**.
4. **Merge (141B457A0, OBSERVED):** sums the counts, allocates `count*0x18` from the current frame arena
   (`(manager+0x58)->vt+0x48`), `memcpy`s each per-thread array in list order, then runs `141AD1A20` = an MSVC-style **stable merge
   sort** (insertion sort for runs <= 0x20, then merge with a temp buffer) with comparator `(byte@+0x10, then u32@+0)`. It publishes
   `manager+0x1E0 = array`, `manager+0x1E8 = count`, resets the list, flips `manager+0xA4 ^= 1`.
5. **Determinism (INFERRED):** the final order is a sort by (pass byte, 32-bit key) so it is deterministic whenever keys are
   unique; records with equal (pass,key) keep their concatenation order, which follows the (non-deterministic) order in which
   thread contexts were destroyed/registered. It is **not** ordered by job index.

## 3. Traversal vs GXEntityDrawParallelJob

* **OBSERVED (clonable traverser, i.e. the parallel path):** traversal only **gathers** (cull + level selection) into the 16-byte
  records; the entity Draw runs **afterwards** in the separately enqueued `GXEntityDrawParallelJob` over the gathered list.
  Run (141B4CBE0): `ctx[workerIdx]` (= `job+0x20+idx*8`) gets `thread id` at ctx+0x69C; `for (i = atomicTicket(job+8); i < job.count(+0x10); i = atomicTicket)`:
  record = `{entity, a, b}` from the register's vector (`job+0x18 -> +0x10`, 16 bytes each); `ctx+0x618 = a; ctx+0x61C = b;`
  `entity->vt[1] (entity, ctx)` (Draw). **One ticket per entity** (no batching, no minimum job size).
* **OBSERVED (non-clonable traverser, e.g. the shadow-like views, or task system off):** `traverser+0x13` stays 0 and
  `141B4CF40` calls `entity->vt[1] (entity, ctx)` **directly inside the traversal** (single context `traverser+0x98`).
* **Work per entity**
  - in the traversal (`ECT::vt+0x50 = 141B4CF40`, OBSERVED): (a) optional occlusion/visibility test: `traverser.vt+0x30()` &&
    `entity+0x68` && `141BF3F00`; (b) cull-strategy object `traverser+0x18`: `vt+0x10()` then `vt+0x20(+0x28, entity)`; (c) level
    selection `141C69800` = `cullObj(+0x20)->vt+0x30(entity->vt+0x18())` (0 = rejected; 1 or fixed 2 when `traverser+0x34 > 0`;
    skipped when `DAT_1447F59BA`), and a second value `141C697E0` = `cullObj->vt+0x58(...)` when `cullObj->vt+0x50` is non-zero;
    (d) record or direct Draw. The frustum / distance / AABB tests themselves are in the tree-node accept `141C52720` (mask test
    `node+0xE8`, traverser `vt+0x18` push+test of node bounds `node+0xB0` with distance `node+0xF4`, children `node+0x50..0x58`
    each `vt+0x28`, own entities `node+0x30..0x38` with per-entity mask at `entity->bounds+0x20` and traverser `vt+0x50`, pop `vt+0x28`):
    the same shape as DS3's `FUN_140208180` (OBSERVED).
  - in the entity Draw (`CSModelDispEntity::vt+8 = 140B31550`, big): once-per-frame guard (`this+0x28C == _DAT_143D896FC`), fade/visibility
    flag update from `this+0x220/+0x224/+0x17C`, view-type dependent LOD / alpha (`ctx+0x114` view type 2..0x2C, 0x38), distance
    LOD (`140B32880`), then emission of draw items into the context's per-thread record arrays (INFERRED from the part read; the
    function was only partially read).

## 4. Threads, entry, pipelining

**Per-frame entry (OBSERVED):** the FD4 step class `CS::CSDrawStep` (vtable 142B67DD0):
* `+0xC0 = 140B15E80` (begin) -> `140B1A200` -> **renderer BeginFrame `1419E77F0`** (`g`, args mode flags).
* `+0xC8 = 140B16840` (execute) -> `140B1A5C0` -> **renderer EndFrame `1419E82C0`**.
These run on the game/step thread: `1419EC1F0` (the tail of EndFrame) tests `GetCurrentThreadId() != renderThread(g+0x6D0)+0x14`
before waiting for the render thread, so EndFrame is by construction not on the render thread (OBSERVED).

**Where the traversal runs (OBSERVED + INFERRED):** the composite render `141B660D0` (GXSgCompositeSceneBase vt+0x38, per view) does,
on the calling (draw-step) thread:
1. `vt+0x70` get scene context, build the 128-byte draw context `141AD1D00`; `vt+0x50` prepare (`141A06A90`/`141A06B10` camera, masks);
2. traversal of the sub-scenes: if `thunk 141F235BE` (mode) is false: serial `for child: child->vt+0x40`; else
   **`GXDrawSceneJob`** fork-join via `141A74440(job, nSubScenes, 0, DAT_143C294FE)` (`141B66030` ticket loop -> `141B663D0` -> `child->vt+0x40`);
   inside, `GXSgWorld::vt+8` (`141ADB860`) pushes the layer jobs (Q1) and returns;
3. if `DAT_143C294FF`: **`141B456E0` join**: frees finished clones (manager+0x180) and, if the pending entity-draw jobs list
   (manager+0x150) is non-empty, calls `141AF38A0` on the pool (the caller **runs jobs itself** until the pool drains, see Q6);
4. `vt+0x58` (-> `141A08820`: pool wait, `141A1D190`, **`141B457A0` sort+publish**, flip of the draw-record arena);
5. `vt+0x60` (-> `141A0A510`): record command buffers in parallel (`141B3DA20` -> `141B3DFE0` -> `GXCommandRunnerJob` run `141B3D940`,
   ticket per chunk, plan `141B3E890` up to 0x6C chunks) and submit (`141B3E620`).
   When `DAT_1447F59B9 == 1` (set in init `141A09100` only if the second command list g+0x720[idx] exists), step 5 is **not run
   inline**: `141B660D0` allocates two closure objects and queues them with `141AEC810(commandList = g+0x6E8[g+0x708], 141B66440 / 141B665A0)`;
   the closures run `composite->vt+0x60` and a list swap on the **render thread** later.

**Pipelining (OBSERVED):**
* `GXRenderThread` (RTTI, ctor `141AF4DF0`, created at init through `141AF4FA0`) loops in `141AF5180`: wait event (+0x4B0), if
  `+0x498` (command list) != 0 run `1419ECCA0(renderer, list)` (plays the recorded commands = GPU submission), signal done (+0x4D0).
* EndFrame tail `1419EC1F0` (when `g+0x11C != 0`, "threaded rendering this frame"): waits for the previous frame's render work
  (`141AF5360`, which pumps the Win32 message queue while waiting), **flips `g+0x708 ^= 1`** (`1419EF5C0`), and posts
  `g+0x6E8[idx^1]` to the render thread (`141AF5060`: store list, `SetEvent`). The game thread then returns and starts the next frame.
  `g+0x11C` is copied from `g+0x11D` at BeginFrame (`1419E77F0`, `mov [rsi+0x11C], ...`), i.e. a runtime-switchable option; when 0 everything
  runs on one thread (`1419E82C0` else-branch executes `1419E85F0` inline).
* So ER **is** pipelined, but the pipeline cut is *recording/submission* (steps 5 and the GPU command list), NOT the scene traversal:
  frame N's traversal, entity Draw jobs, join, draw-record sort/publish (steps 1-4) are executed synchronously by the draw-step
  thread (helped by the pool) before EndFrame; the render thread executes frame N's recorded commands while the game thread runs
  frame N+1 (INFERRED from the closure structure and the flip; I did not execute it).
* **Double buffers (OBSERVED):** renderer index `g+0x708` (0/1) selects `g+0x6D8[2]`, `g+0x6E8[2]` (command lists), `g+0x6F8[2]`,
  `g+0x710[2]`, `g+0x720[2]`, `g+0x730[2]`; the draw-record merge toggles `manager+0xA4` (arenas `manager+0x70[2]`);
  recorded chunks/commands are consumed from the other slot (`^1`) by the render thread.
* Which step calls composite `vt+0x38` (the view loop) was not located (virtual call through the scene/IDrawScene objects); it
  happens inside the BeginFrame..EndFrame window of CSDrawStep (INFERRED).

## 5. Views / passes

* **OBSERVED:** one traversal **per view**: `141A0DE50` (`GXSgDrawScene` slot0, collect for a view) is called with the view context
  (`view+0x114` = view type) and a 64-bit layer/draw mask; it builds a **new traverser** each call with a mask chosen from the view
  type: types 0x0D..0x1C -> mask `0x2000` and `cVar8=0` (non-clonable, so inline/serial, direct draw); 0x1D..0x2C -> `0x20`, also
  non-clonable; 0x2D -> `0x80`; 0x39 -> `0x100`; everything else (main view etc.) -> `0x01` and clonable (parallel) when the task
  system is on. If an explicit mask is passed it is `(mask & 0x1FFFF | base) & mask`. Entities test their own masks (`entity+0x20`)
  against the traverser mask (`traverser+8`) in the layer/node code (`141ADBF80`, `141C52720`). So the **same world/layers are walked
  once per view/pass with a per-pass mask; there is no union-mask single walk** (OBSERVED shape; the meaning of the masks
  (shadow cascade vs reflection) is INFERRED from the pass-specific scene classes `GXSgShadowSceneImpl`/`GXSgReflectSceneImpl`/...).
* A second view is a second call of the same path (own traverser, own collector/clones, own `GXEntityDrawParallelJob`s) and its own
  `141B456E0` join; views are traversed **one after the other**, not concurrently (join inside `141B660D0`). Concurrency exists
  between sub-scenes of ONE view (GXDrawSceneJob, one ticket per child scene) and between the layer jobs of those children.
* Non-clonable views (types 0x0D..0x2C) are the serial ones; their layers are walked inline by the caller and entities are drawn
  directly during the walk.

## 6. Work pool

* **OBSERVED structure:** pool object at `g+0x5C8` created by `141AF3400(pool, desc{count, ...}, affinities, priorities)` from the
  renderer init `1419E9B60` (line `FUN_141af3400(param_1+0xb9, ...)`). `count` = **N workers including the caller**, computed in
  init: `cores = _DAT_144847384` (processor count, 2 if unavailable) clamped to [2,16]; `N = max(2, min(cores, cfg(g+0x54C)+1))`.
  Worker 0 is the thread that created the pool (id stored at worker+0x14, `141C565B0`); workers 1..N-1 are `GXWorkerThread` objects
  (0x528 bytes each, `141C56270`, thread via `141C56770` -> `141B445E0`/`141ED8760`). N is also `g+0x5D0` (`*(uint*)(g+0x5D0)`).
* **Queues:** each worker has a job queue at `worker+0x498` (lock-free, `141AFD220` pop / `141AFD070` steal, enabled by
  `DAT_143C27EBC` which BeginFrame sets from `g+0x66C`, default data value 1; else a locked queue `141AFCE70`).
  Push = `141C565A0(workerStruct, job)`; **all pushes go to worker 0's queue** (`141AF3030(pool, job, 0)`, `141AF3070` loops the same).
* **Execution:** worker proc `141C568A0`: `loop { 141C56470 (drain); state=2; 141AF3680 (sleep); state=3 }`. `141C56470`: pop own queue,
  else pop worker 0's queue, else steal (`141AFD070`), then `job->vt[1] (job, workerIndex)`. `141AF3680` counts sleepers
  (`141EBC020(pool+0x18)`), sets the "all idle" event `pool+0x88` when everybody sleeps, then `WaitForSingleObject(semaphore, INFINITE)`;
  **no spinning** before the sleep.
* **Wake:** `141AF3070`/`141AF3170` call `141F35260 = ReleaseSemaphore(P[4], n)` (semaphore created in `141F34F90`, max N-1). Worker-start
  gate is a `PulseEvent` (`141C56460 -> 141F358B0`). Completion/idle uses Win32 events (`SetEvent/ResetEvent/WaitForSingleObject`).
* **Ticket:** `141EBC020` = `lock xadd` post-increment (OBSERVED, 10-byte function). Used by all three job classes plus the draw-context
  pool and the sleeper count. A parallel job object holds `{ticketCounter (+8), workerCount (+0xC), itemCount (+0x10)}` and is
  pushed as `min(workerCount, g+0x5D0, requestedMax)` task nodes (`141A74190` non-blocking, `141A74440` blocking, `141A741D0`
  with per-thread node pools `*_DAT_1447F5140 + threadIdx*0x58`, node allocator `141A740B0`).
* **Join:** `141AF3170` (arm, records the owner thread id at pool+0x5C), `141AF38A0` / `141AF3750` (wait): the waiting thread **helps**
  (`141C56500` = try pop own/master/steal and run one job, repeated) until `141AF39D0` reports all workers idle. The join is
  **pool-wide** (waits for every queued job, not for one batch).
* **Granularity (OBSERVED):** fixed, no adaptation by size: layer jobs = 1 per layer; draw jobs = `min(records, N)` workers with 1
  ticket per entity; sub-scene jobs = `min(children, N)` with 1 ticket per child; command recording = chunks (plan 141B3E890, up to 0x6C).

## 7. What this implies for DS3 (INFERRED, for the I-0077 collect thread)

* ER's traversal is NOT finely parallel: one job per layer, with a single big tree layer. Its win comes from (a) pushing
  per-entity Draw (the expensive part) into a separate ticketed job over a flat 16-byte record list, and (b) pipelining
  recording+submission to a render thread one frame behind. The same split (cull/gather -> flat list -> parallel per-entity draw
  with per-thread record arrays -> stable sort of 0x18-byte records -> parallel chunk recording) maps directly to the DS3 structure
  `FUN_1400C1FB0` / `FUN_140208180` (identical visitor shape: push, test, children, own entities, pop).
* Per-thread record arrays + local stable sort + locked append + global stable merge sort is the ER answer to ordering.
* A clone-per-job traverser created inside the job on the worker (with a per-thread allocator) avoids shared state; the layer root
  is the only entry point so no ancestor-bounds hand-off is needed.

## Limits (not found / not decoded)

* The caller of `GXSgCompositeSceneBase::vt+0x38` (`141B660D0`) and the exact view loop were not located (virtual dispatch; X-refs
  empty). Its placement between BeginFrame and EndFrame of `CSDrawStep` is inferred.
* The thread that executes `CSDrawStep` is inferred as the main game/step thread (EndFrame is shown not to be the render thread).
* `g+0x11D` (requested threaded-render mode) and `g+0x3A4`/`g+0x720` list creation conditions: writers not traced to their config source;
  default of the render-thread mode at runtime is unknown. `DAT_1447F59B9` (deferred submission) is 0 until `141A09100` sets it.
* `_DAT_144847384` as logical-core count and the cfg field `g+0x54C` (worker cap) are inferred from use, writers not traced.
* Entity Draw (`140B31550`) was read only in part; the full set of draw-item emissions and fade math not decoded. Other entity
  classes (flver/FFX/grass) not examined.
* Tree internals beyond node accept (`141C52720`), insertion (`141C513D0`) and dynamic updates were not decoded; the tree depth
  parameter (`141AE9FB0(10)` in the world builder) is unexplained.
* GXSgCompositeScene child classes' `vt+0x40` implementations and the exact mapping view type -> pass names were not decoded.
* Hot-patched flags: only the static initial values of `143C294F8..143C29503` (all 1) were read; runtime/debug changes unknown.
* No dynamic confirmation (no game run), all findings are static.
