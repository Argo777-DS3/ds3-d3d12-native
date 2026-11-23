# I-0073 ER study: texture loading, the copy queue, the remaining draws (2026-10-20)

Elden Ring 2.7.1 (`eldenring_2.7.1.0_25080141_1A354710.exe`), static only: read-only headless Ghidra 12.1.3 at below-normal
priority on copies of the analysed project (`C:/Users/User/Tools/DS3-Decompilation/er-mirror-i0070-{a,b,c}`,
`er-mirror-i0072-a`), no ER launch. Three Sonnet 5.5 studies (L loading, Q queues, R recording, D-0195); their C/ASM exports
stay in the session scratchpad (`er-L/out`, `er-Q/out`, `er-R/out`, Opus's own in `er-V/out`), not kept as evidence. Addresses
are ER absolute VAs, never DS3 facts. Status: OBSERVED (read in the code), INFERRED (a reading). **Checked by Opus** marks the
claims whose decompile or assembly Opus read itself. D3D12 calls are named by their vtable slot confirmed in the assembly.

## 1. Texture loading and decoding off the engine thread

- **The job system** (OBSERVED, L): every thread is created by one wrapper (141ED8760: `_beginthreadex`, then the
  affinity mask, and a priority from a table of {-15, -2, -1, 0, 1, 2, 15}). Pools of `CSEzWorkThread` (constructor 140E81A30:
  4 to 6 slots from the process affinity mask, one affinity mask per slot by mode) each hold a FIFO of jobs under a critical
  section; posting either queues only (140E82980) or queues and wakes (140E828B0); a wake is a message to the thread, whose
  loop (140E84090) runs jobs until the queue is empty. A job's constructor increments a completion group and its destructor
  decrements it and sets the group's event at zero; the poll 140E83200 is `WaitForSingleObject(event, 0)` (**checked by
  Opus**), the blocking join is 140E83030. Seven global pools (140E83600: high-priority clusters, low-priority all-core) and,
  owned by the file system `CSFileImp` (+0x40, built by 14023C4F0, **checked by Opus**), two loading pools named
  `LoadProcess_S` (one thread) and `LoadProcess_M` (3 to 5 threads), both at idle priority (-15) per L.
- **File read and DCX**: overlapped `ReadFile` (DLIO, 141FC3900) and a per-file state machine that decodes the DCX payload
  one chunk of at most 0x80000 bytes per step (14265F8E0, 14265F4C0; Oodle `OodleLZ_Decompress` behind 142407A60 and its
  siblings), stepped by `CSFileStep` with at most 0x7F iterations a call (OBSERVED, L). Which thread steps it is not settled.
- **The TPF processor is asynchronous and polled** (**checked by Opus**, 14023B5A0 and 14023B0C0): `TpfFileCap` makes a
  `CSLoadProcessor_Tpf` (14023AE30) whose update moves a state at +0x68: 1 (data ready) counts the textures of the TPF,
  allocates their entries and posts one `CSLoadProcessTask_Tpf` job to `LoadProcess_M` with a wake (`[[CSFileImp+0x40]+8]`,
  140E828B0); 2 polls the group without blocking (140E83200) and, once signalled, finishes (14023B330) and moves to 3. The
  engine side never waits.
- **On the worker**: the task (14023C2C0) builds a `GXCGTextureBuilder_TPF` per texture and posts one create job per texture
  (140B7EA70, vtable 142B79358) to the same pool and group; each create job (140B7EB20) switches on the source kind (2D or
  cube), builds the GX texture, stores the byte count at +0x58, the authored constant for formats 0x46-0x48 and 0x4C-0x4E,
  and the counted handle at +0x40 (**checked by Opus**); underneath, the D3D12 resource is created and filled by 141E928E0
  (section 2), which blocks **that worker** until the copy queue's fence passes. Decoding, resource creation and upload all
  run on `LoadProcess_M` workers; the texture is complete before anything can see it.
- **Publication into the holder**: when the processor completes, `TpfFileCap::OnLoaded` (140226280) registers the textures
  in the texture repository (140B84E70 -> 140B826B0, under the repository's critical section), and 140B84270 swaps each
  `TexResCap`'s holder through 140B7E940 (I-0071 ER-STUDY B). **The swap pins the old texture for two frames** (**checked by
  Opus**, correcting I-0071 ER-STUDY B and study L): when the holder's own slot is used (+0x18 null) and an old inner exists,
  140B7E940 first calls 141A97400, which adds a reference to the old object and pushes it, under the renderer manager's lock
  (`[[1447F33E0]+0x5B8]`), into the delayed-release bucket of policy 2 (141AF4D50), the frame-delayed unreference of
  ER-DEEPENING-897 (b); then it drops the holder's reference and stores the new inner with an atomic increment. A forwarding
  holder (+0x18 set) only reassigns the target slot (140B7EEC0). No lock guards the store itself: readers take the holder's
  current inner at bind time.
- **A synchronous path exists too** (OBSERVED, L): 140B81290 / 140B82A40 post the same create jobs to global pool 0, wake it
  and block their caller in the join 140E83030 before swapping the holders.
- **No per-frame budget** of textures or bytes was found; the throttles are the 0x80000 DCX step, the 0x7F file-step cap,
  the idle priority and the 3-5 threads of `LoadProcess_M`, and an unstudied `CSFileAllocateLimiter` (OBSERVED, L).
- The texture's SRV is not created in the job path (INFERRED, L: at first bind); not checked.

## 2. Queues, the copy queue and fences

- **Three queues, priority and flags 0** (OBSERVED, Q): COPY (type 3) created by the device setup 141E9FD30 (descriptor
  `{3, 0}` written at 141E9FF34, CreateCommandQueue +0x40 into context +0x08, **checked by Opus**), DIRECT (type 0, 141EA3740,
  wrapper at 0x144805320) and COMPUTE (type 2, 141EA6580, wrapper at 0x1448052E8), all three created in that order by
  141E890E0 (**checked by Opus**: calls 141E9FD30, then 141EA3740, then 141EA6580) and published in the device context at
  0x1448050C0 (accessor 141E8AF90). A second compute queue 141E8A730 has no static caller.
- **The copy queue serves only synchronous creation uploads.** 141E928E0 (resource creation with initial data, buffers and
  textures) creates the destination (committed, or placed when a heap block is supplied) in COMMON and a committed UPLOAD
  buffer, copies the CPU data into it, records a **fresh type-3 allocator and list per call** (CreateCommandAllocator +0x48
  type 3, CreateCommandList +0x60 type 3), CopyBufferRegion (+0x78) or one CopyTextureRegion (+0x80) per subresource from
  the GetCopyableFootprints layout, closes, ExecuteCommandLists (+0x50) on the queue passed as its 16th argument, creates a
  **new fence**, signals it to 1 (+0x70) and blocks the calling thread with SetEventOnCompletion (+0x48) and
  `WaitForSingleObjectEx(INFINITE)` before releasing everything (**checked by Opus**). Its four creator callers (141E88450
  textures, 141E98000 buffers, 141E987F0, 141E9B200) pass `[0x1448050C0]` (device) and `[0x1448050C0 + 8]` (the COPY queue) as
  arguments 15 and 16 (**checked by Opus** in 141E88450's assembly: `[RSP+0x70]`, `[RSP+0x78]`). Staging map and commit
  (141EA3B40, 141EA4130) use the same pattern (OBSERVED, Q).
- **No batching, no persistent copy fence, no cross-queue wait** (OBSERVED, Q): every copy submission has its own fence,
  waited for on the CPU and destroyed; the queue-wrapper Wait functions (141EA4B50, 141EA6C10, queue +0x78) have no code
  reference; the DIRECT queue never waits on a copy fence: the resource is complete before the creator returns. No barrier is
  recorded on copy lists (the COMMON promotion and decay rules carry the states, INFERRED).
- **Per-frame updates do not use the copy queue**: 141E8D570 (and 141E8D2C0, 141E8D020) copy into the persistently mapped
  UPLOAD ring (141E8F770) and record CopyBufferRegion / CopyTextureRegion on the frame's DIRECT or COMPUTE list; the ring
  blocks are tagged `class << 56 | fence value` (141EAC840) after the submission and recycled when the queue fence passes
  them (141EAC850 through 141EAB300 = GetCompletedValue), no CPU wait (OBSERVED, Q; the tagging in 141EA4C10 **checked by
  Opus**). A "queue class" is an index into a 10-fence array at context +0x90, one per DIRECT/COMPUTE wrapper (141EA1780).
- Not found: an asynchronous streaming copy path, a copy context, tiled or reserved resources for textures. The thread that
  calls 141E928E0 for textures is in section 1.

## 3. The remaining draws: prepared and recorded in chunks on workers

Classes (RTTI, OBSERVED, R): `GXSR::GXRenderCommandRunner` (vtable 143034E30: planner +0x10 = 141B3E890, command interpreter
+0x18 = 141B3ED30, pass enter +0x20 = 141B3E6C0), `GXBS::GXCommandRunnerJob` (body 141B3D940), `GXSR::GXRenderExecContext`
(clone 141B3BFA0, pass snapshot apply 141B3C540), the per-thread device context (constructor 141ACA660, deferred variant
141AF7630).

- **Records and sort** (I-0071 ER-STUDY A, extended by R): each 0x18-byte record holds a 32-bit key (bits 31-29 class, 28-23
  pass, 22-16 view, 15-0 sequence), a pointer to a byte-coded command stream and a kind byte; 141B457A0 concatenates the
  per-thread vectors, sorts and publishes them. A command of type -8 in the stream is the pass-begin snapshot: a clone of the
  execution context (render targets, viewport, view constants) stored by `key & 0xFFFF0000` while the stream is produced.
- **Chunk planning** 141B3E890 (**checked by Opus**): consecutive records whose pass and view map to the same entry of a
  static table (141B3E7F0, 43 entries of 0x24 bytes at 143C28E40, OBSERVED, R) form a run; a run is split into
  `clamp(ceil(n / quantum), 1, max)` chunks, at most `n / min_per_chunk`, of near-equal size, one chunk when fewer than two
  workers or two chunks; the chunk table holds at most 0x6C entries (the array size: the remainder becomes the last chunk).
  Only a few entries split (R's table dump: pass 1 views 0 and 6, pass 0x38 views 0x34, 6 and 0x18, up to 5 or 14 chunks with
  10-15 draws per chunk); most passes are one chunk each. Kind-1 records are skipped and run first on the calling thread.
- **Dispatch and join** (OBSERVED, R; the thread clamp **checked by Opus**): 141B3DA20 takes the worker count from the
  renderer (+0x5D0, clamped to 16), plans, sorts the chunk table (141B3D3C0) and queues `min(threads, chunks)` tasks through
  141A741D0; the calling thread joins with 141AF38A0, which pops and runs queued tasks until the group completes, so the
  caller records chunks too.
- **Worker body** 141B3D940 (**checked by Opus**): each worker takes its own per-thread device context
  (`renderer+0x60[worker]`), clones the execution context once, then loops on the atomic ticket 141EBC020 (`lock xadd`) over
  the chunk indices: begin (141ACA950), record the chunk (141B3DC10), end (141ACB540).
- **One command list per chunk** (**checked by Opus**, 141E8E5B0 and 141ACA950 per R): a chunk begin opens a list from the
  thread context's list wrapper: the wrapper keeps a ring of N allocators (N = frames in flight, device +0xF0), and only the
  first begin of a new frame index waits for the DIRECT queue fence of that slot and resets its allocator; the list itself
  is popped from a shared free pool under a lock (created when empty), reset on that allocator and given the two
  shader-visible heaps (SetDescriptorHeaps +0xE0: the shared CBV/SRV/UAV heap and a per-wrapper sampler heap).
- **State in each chunk list is lazy and complete**: on entering a pass/view the chunk applies the pass-begin snapshot
  (141B3E6C0 -> 141B3C540): binding blob, raster block, all dirty bits set, render targets re-emitted, view constants copied
  into a fresh constant slice (OBSERVED, R); before each draw 141ACB6D0 emits only what is dirty (render targets +0x170,
  viewports, scissors, pending barriers +0xD0 (corrected by I-0080 ER-STUDY: +0xD0 is in the viewport array, not a pending-barrier array), vertex/index buffers, root tables); the pipeline and root signature
  (141E8F2E0) reset the cached tables; 141ACD170 copies the CPU staging descriptors into the shader-visible heap and sets only
  the dirty tables (I-0071 A). **Barriers are recorded inside the chunk lists**, from a pending array; no barrier-only list
  was found on the draw path.
- **Per-worker memory** (OBSERVED, R, 141E8C530): per list wrapper two linear upload sub-allocators (constants and data)
  over shared fence-stamped pools, a sub-range allocator over the shared shader-visible heap (per-thread ranges from a locked
  source, not an atomic cursor), the sampler heap. All per-draw constants and descriptor tables are produced in the worker
  that records the draw; nothing per draw is prepared between the sort and the recording.
- **Ordered submission** (**checked by Opus**, 141B3DC10 and 141AECAE0): a recorded chunk hands its closed list to the
  ordered records (141AEC780) with the key `(first key & 0xFFFF0000) | ((start + 4) & 0xFFFF)`; at frame end the render
  worker (141AF5180) sorts the records by key and replays them: lists are gathered into batches of up to 0x48 and submitted
  by **one ExecuteCommandLists per batch in key order** (141AF7620 -> 141EA4C10, then Signal and the ring tagging); callback
  records (types 1-3) flush the pending batch first. Recording threads never submit. The chunk finish behind an obfuscated
  stub (141AF5A60 -> 145CBE9F3) is INFERRED to close the list and move its sub-allocator blocks into the record.

## 4. DS3 against ER

DS3 1.15.2 read in its own code (read-only headless Ghidra on `ghidra-mirror-i0068`, exports `ds3-D/out`) and in the native
source; DS3 addresses are DS3 absolute VAs.

| Point | Elden Ring | DS3 original | DS3 native today |
|---|---|---|---|
| Texture creation job | create job 140B7EB20 (vtable 142B79358) on `LoadProcess_M` | `CSGXTextureCreateFragment` (vtable 142955480, Run 140D4A0A0), a `CSEzWorkFragment` (base vtable 1429B3FF8): **the same body** (kind switch, 2D through the source's +0x50, cube through +0x58, constant for formats 0x46-0x48 / 0x4C-0x4E, bytes at +0x58, counted handle at +0x40) (**checked by Opus**) | the GX creation under the fragment is replaced (`AdmitTexture` 2AF40, `AdmitEncodedTextureFamily`): TPF entry decode and DDS parse in the native code, then `TextureIngress::AdmitOwned`; census Native1358: encoded texture admissions on five threads, none the engine thread (R13-R14 sheet); OPTIMISATION-REVIEW 8.1: never on the owner thread |
| Who waits | TPF processor posts one task and **polls** (14023B5A0); a separate synchronous path joins (140B81290) | the TPF creator 140D4E100 (reached from 140D4F470) posts one fragment per texture (140D49FF0, then 140EEB930 queues under the pool lock, 140EEB820 wakes the threads) and **joins blocking** (140EEC0E0 with -1), under the lock at 144793D28: ER's synchronous path, not its polled processor (**checked by Opus**) | unchanged (engine structure); the native decode and copy-out lengthen that join, on DS3's loading thread, not the engine thread |
| Upload | per texture on the worker: COPY list on the COPY queue, CPU wait on a fresh fence; complete before publication | D3D11 creation with initial data on the worker | recorded on a pooled DIRECT ingress list without a wait; the next frame submission puts every committed ingress list ahead of its own list in one ExecuteCommandLists (`DeviceQueues::SubmitWithIngress`); the record is `resident` once committed |
| Copy-out of the source | the builder hands subresource pointers (141E95FE0) | n/a | rows read with `ReadProcessMemory` one row at a time (`TextureIngress::Admit`), the TPF validated whole and one entry copied (`DecodeCpuTextureContainerEntry`) |
| Old texture | holder swap pins it two frames (141A97400, delayed-release policy 2) | the fragment's +0x40 counted swap | `TextureIngress::Retire` and `DeviceQueues::RetireAfter` (fenced) |
| Draw build | sorted records, chunk plan by pass table, up to 16 workers plus the caller, one list per chunk from a pool, per-worker allocator ring and sub-allocators, state from pass snapshots, barriers in the lists, submission in key order in batches of 72 | D3D11 deferred contexts (not read here) | collect on the engine thread with admission and pass rows on a worker pool (I-0071 points 3a, 3b); `PrepareNativeModelFrame` (5.4 ms at m45) then one frame-slot DIRECT list recorded by the render worker (4.2 ms) (POINT3-PLAN); barriers from the frame graph (D-0171) |

### 4.1 A lead for what holds DS3's engine thread at arrivals (for I-0074)

I-0070/I-0071's counters show no texture decoded on the engine thread, yet frames of 55-150 ms at resource arrivals while the
native collect takes 4-5 ms (I-0074). ER's engine side never waits for textures (the polled processor 14023B5A0); DS3's
creator does, twice over (**checked by Opus**, the thread of the callers is not):

- 140D4E100 takes the critical section at 144793D28 (vtable +0x10 lock, +0x20 unlock; initialised by 140D6EB00) **before**
  posting its fragments and releases it only **after** the blocking join 140EEC0E0: the section is held for the whole
  creation of a TPF's textures, that is for the native decode, copy-out and ingress recording done in the fragments.
- The same section guards 140E49320, a name lookup (`param_3` vtable +0xD8) reached from 140E48980 (callers 140E4E790 and
  140E602F0).
- The creator's entry is the texture repository's create-or-find by name (140D52000 on `[0x144793048]`, then 140D52640 ->
  140D4F470 -> 140D4E100), called directly from engine code at 1404EEAC0, 1404FFCF0, 140500390, 1405004E0 and 14082B130.

So the engine thread stalls for a whole TPF's creation whenever it either calls 140D52000 itself or takes the 144793D28
section in 140E49320 while a loading thread creates a TPF (INFERRED; which thread runs those callers is not read). The
census of R13-R14 (admissions on five non-owner threads) counted the fragments' threads, not the thread waiting in the join.
Measure to settle it: the owner thread's time inside 140D4E100 and inside 140E49320's lock per slow frame (a counter in the
native admission when `GetCurrentThreadId()==owner_thread` is already inside 140D4E100, or a sample profile of the owner
thread in the slow frames). If confirmed, the remedy is ER's: the native work inside the fragments as short as possible
(section 5 items 1 and 2: no per-row copy-out, no work beyond the copy recording), since the join and the lock are the
engine's.

## 5. What adapts to DS3 (proposal for the next workstream, I-0074 and I-0071 point 3c)

1. **Loading: keep the work in DS3's fragments, make the native part ER's.** DS3's loading already has ER's shape (one job per
   texture on a worker pool, the same body); the engine thread neither decodes nor creates textures. What the native backend
   owns is the work inside the fragment, so ER's method applies there: decode, create and upload on the fragment's worker,
   and publish only a complete texture. DS3's blocking join in 140D4E100 and its file-cap flow stay the engine's (ROADMAP:
   DS3 structures that were never thread-safe stay on their own threads); ER's polled processor is not copied into DS3's engine.
   Cheaper copy-out first (ROADMAP "redundant work removed first"): the source rows read in one block per subresource instead
   of one `ReadProcessMemory` per row (FABLE-REVIEW 6).
2. **Copy queue: ER's queue, without ER's CPU wait.** ER proves the separation (a COPY queue owned by the device context,
   type-3 lists, no barriers, COMMON promotion and decay) but blocks each worker per texture. DS3's adaptation: a COPY queue
   next to the DIRECT queue; the fragment's worker records its texture on a pooled COPY list (`IngressRecording` on a type-3
   allocator), submits it on the copy queue under its own small lock and signals a copy fence value stored in the record;
   the record becomes `resident` only when the copy fence has passed that value (polled by the collect through
   `GetCompletedValue`, as ER's ring blocks are), so the frame never draws a texture still being copied and the DIRECT queue
   carries no uploads. Optional beyond ER: a DIRECT `queue->Wait(copy fence)` instead of the poll when a frame must use a
   texture in the same frame. FaceGen compositions and offscreen target clears are graphics work and stay on the DIRECT ingress.
   Batching several textures per copy list is beyond ER and only worth it if the per-list cost shows in the measure.
3. **Old textures**: DS3 already retires through the fenced `RetireAfter`, a stronger rule than ER's two-frame delay; keep it.
4. **The remaining draws (point 3c)**: ER's runner adapted to DS3's prepare and recording.
   - Plan chunks by pass from the sorted rows: split only the large passes (ER splits its main geometry and shadow passes,
     up to 5 and 14 chunks, 10-15 draws minimum per chunk; DS3: the G-buffer and the shadow cascades), one chunk elsewhere,
     a fixed table maximum like ER's 0x6C.
   - Merge prepare into the chunk: ER prepares nothing per draw before recording; each chunk worker writes its constants and
     descriptor tables while it records, from its own upload and descriptor sub-allocators (DS3's `UploadArena` and the
     shader-visible ring become per worker, refilled from a locked source with fence-stamped blocks, as ER's).
   - One DIRECT list per chunk from a shared pool, one allocator ring per worker sized by the frames in flight (D-0187), the
     first begin of a new frame waiting for that slot's fence (ER 141E8E5B0); each chunk list sets its heaps and root signature
     and applies the pass snapshot (render targets, viewport, view constants) before its first draw.
   - Barriers: ER records them inside the chunk lists; DS3's frame graph keeps them at pass boundaries, so the first chunk of
     a pass (or a small list before it) carries the pass's barriers.
   - Submission by the render worker in plan order, the ingress lists first, one ExecuteCommandLists per batch (ER: 72).
   - Workers: DS3's admission pool from point 3a, the render worker joining and recording chunks too (ER's caller does).
5. **Order of work proposed**: (a) the measure I-0074 builds (what holds the engine thread at arrivals, with the lead of 4.1); (b) the copy-out and
   the copy queue (1, 2), measured at the Firelink walk and Archdragon Peak; (c) point 3c (4), measured at m45, Archdragon
   Peak and Firelink against point 3b, with the record revalidation and prepare shadow guards and pictures against main.

## 6. Open points

- The thread that steps ER's file state machine and DCX decode (`CSFileStep`), and the caller of DS3's 140D4F470.
- ER's per-draw pipeline and root-signature calls inside type-3 draw objects, the comparators of the chunk and record sorts,
  the value of N (frames in flight) and of the worker count (renderer +0x54C, +0x5D0).
- The obfuscated chunk-finish stub (141AF5A60 -> 145CBE9F3) and any barrier-only callback between passes.
- Where ER creates a texture's SRV.
