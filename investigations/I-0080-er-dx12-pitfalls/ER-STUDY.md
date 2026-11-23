# I-0080: the D3D12 pitfalls, Elden Ring's treatment and ours (study for the audit)

Study thread of 2026-10-21 (Opus, with Sonnet 5.5 decompilation workers), prepared while I-0079 « Saccades en mouvement »
tested; the « audit des pièges DX12 » workstream starts from it. Rule: D-0195, with Thomas's nuance relayed by the coordinator
on 2026-10-21: copy ER's architecture **without its known defects**.

## How to read this document

- **ER-DEC**: read in the ER 2.7.1 decompilation (`eldenring_2.7.1.0_25080141_1A354710.exe`, read-only headless Ghidra 12.1.3
  on the `er-mirror-*` copies, `-readOnly -noanalysis`). Addresses are ER absolute VAs, never DS3 facts. **checked** marks a
  claim whose decompile or assembly Opus read itself; the rest is a worker's reading, non-canonical until a design relies on
  it (then check it first). Earlier studies are cited by name (study716, ER-DEEPENING-897, D3D12-DESIGN-REVIEW E1-E12,
  I-0072 ER-PIPELINE-STUDY, I-0073 ER-STUDY (merged into main on 2026-10-21), I-0076 ER-STUDY).
- **DEDUCED**: an inference (from the code read, from D3D12 rules, or from measurements elsewhere); never a decompiled fact.
- **PUBLIC**: a public report about the shipped ER, not checked here.
- **OURS**: the native code at `main` 4741f6f2 (file:line), I-0079 « Saccades en mouvement » included (merged on
  2026-10-21, K-0462).
- The workers' C/ASM exports and query logs stay in the session scratchpad (`scratchpad/er-pitfalls/<topic>/`), not kept as
  evidence (D-0142); a design that relies on an address re-exports it.

## Summary

| Pitfall | ER (decompiled) | ER's defect | Ours today | Lead (section 8) |
|---|---|---|---|---|
| Pipelines on the fly | cache per device, shipped precompile list into a library; a miss compiles on the recording thread under the cache mutex | the on-the-fly stutter (PUBLIC) and the mutex held while compiling | per-PC precompile list, one builder per name, lock not held while compiling; I-0079: no compile in the walking slow frames | A1, A2 |
| Barriers | D3D11 emulation: state derived from bindings, resting states, immediate enter/restore, a UAV barrier per dispatch | redundant round trips, conservative UAV barriers, no split or aliasing | frame graph with per-subresource states (D-0171); about 24 raw barriers outside it | B1, B2 |
| Sync, frames in flight | ring of 6 tokens, lag 2, render worker; Present then Signal | two busy waits on the main thread (lag wait, focused limiter), infinite waits | 2 slots, ticket retention, 3 buffers and a waitable object, a timer pacer; a few full drains; 5 s timeout then silent exit | C1, C2, C3 |
| VRAM budget, leaks | placed pools without cap; budget read once at init; residency wrappers with unknown callers; out of memory returned as a code | no budget awareness | budget logged only; pools since I-0079; no reaction above the budget; exit heap fault open | D1, D2, D3 |
| Descriptors | one shared visible heap, per-draw whole-table rebuild with one copy call per descriptor, D3D11 root signature | per-descriptor copies, SRV cache off, silent exhaustion | global heap, kept draws reuse tables; same D3D11 root signature; 22 heap switches across stages | E1, E2 |
| Uploads | creation through a COPY list waited per resource; per-frame ring on the frame lists | one fence and wait per creation | pooled ring and staging, no wait; all on the DIRECT queue ahead of the frame | F1 |
| Driver dependencies | FL 12_0, adapter 0, debug by config, no DRED, AGS static, NVAPI optional | no device-removed handling, adapter 0, AMD DLL required | debug layer on unless a flag; high-performance adapter at FL 11_0; no DRED, no minidump, no names | G1, G2, G3 |

## 1. Pipelines compiled on the fly

**ER-DEC** (I-0072 ER-PIPELINE-STUDY, checked there): one pipeline cache per GX device, 128-bit key from pooled state ids;
a main map read without lock and a pending map behind the cache mutex, merged each frame with generation-based eviction
(141B011A0); a miss is **synchronous on the recording thread, under the cache mutex** (141B01AE0): library `LoadPipeline`
by name, else `CreatePipelineState`; a failure is a negative entry and the draw is skipped. The boot step
`STEP_Initialize_forPipelineStateCache` (140B1F5A0) replays a shipped precompile list (`shader:/PipelineStateCache.dat`,
magic `PSC`, built offline from capture JSON) into the `ID3D12PipelineLibrary` (`%TEMP%\FS<hash>.plf`, checked against the
adapter) and releases each handle: it warms the library and the driver, it does not fill the maps. The pacing worker adds:
the eviction is a table of three tiers `{minimum count, generations of age, scan budget}` at cache +0x4EC, written once at
renderer init (1419F18D0 from 140B1B2A0; the values themselves not recovered); the miss compile 141B01AE0 is reached only
through an adapter (1419F1890) and a vtable, and no background compile thread was found.

**ER defect**:
- PUBLIC: Digital Foundry (patch 1.02 analysis, 2022) reported stutters of up to about 250 ms when new effects, enemies or
  areas appear, lessening once content had been seen: the on-the-fly compilation.
- DEDUCED from the decompiled path: a description absent from the shipped list compiles on the recording thread while that
  thread holds the cache mutex, so every other recording thread that misses waits behind it.

**OURS**: `PipelineStore` (`native/renderer/backend.cpp:1492`, `Acquire`) names a pipeline by its whole description, lets one
thread build a name while the others wait **for that name only**, and never holds the cache lock while the driver works
(`backend.cpp:1508-1525`); the precompile list is written on this PC and replayed at boot on four workers
(`backend.cpp:1541`, I-0072). Already better than ER on the lock. Left:
- a pipeline first met on a PC still compiles synchronously on the requesting thread at its first frame (I-0072 RESULT);
- about 30 `CreateGraphicsPipelineState` sites outside the store (the stages: AA, AO, display, glow, grading, post draws, sun
  shadow, tone map, 17 in `scene_pass.cpp`, the texture ingress) and HLSL compiled at run time by `D3DCompile` for our own
  shaders (`scene_pass.cpp:403`, `:424`, `:1499`, `:1514`, `game_texture_ingress.cpp:115`): at resource creation, outside
  the cache and its precompile (ROADMAP already lists them for the precompile screen).
- I-0079 measured it: no slow frame of the Firelink walk, sprint or camera turn held a pipeline or shader compiled on
  the frame path (its H1 refuted, after a warm start). The pitfall is the first encounter on a new PC or after a driver
  update, and our own pipelines outside the cache.

## 2. Barriers and resource states

**ER-DEC** (barriers worker, building on D3D12-DESIGN-REVIEW E8 / ER K-0020; checked by Opus where marked): ER **emulates
D3D11's hazard tracking** on D3D12 through a state tracker per device context (0x318 bytes, constructor 1419E2470, at
context +0x25C8):
- **No state is stored per resource.** The current state is derived at each barrier site (1419E5410, checked): the bound DSV
  gives DEPTH_WRITE, one of the 8 bound render targets RENDER_TARGET, one of the 8 bound UAVs UNORDERED_ACCESS, anything else
  its **resting state**, the union of the read states its creation bind flags allow (141ACA000; a write-only resource rests in
  its write state; upload buffers in GENERIC_READ, readback in COPY_DEST). Resources are created in that resting state
  (141E91A90 under 141E928E0), not in COMMON as the earlier note said.
- Barriers are emitted **at once, when the engine binds or copies**: setting render targets moves the old set back to its
  read state and the new set to RENDER_TARGET in two `ResourceBarrier` calls, without comparing the sets (1419E42F0,
  1419E43C0); a UAV bind, a clear, a copy (1419E30D0, 1419E3240) and a buffer update (1419E4ED0) each enter their state and
  restore the resting state after; a dispatch puts one UAV barrier per written UAV slot before it (1419E4AD0); each list ends
  by unbinding everything back to resting states (1419E4490), so parallel chunk lists start and end in the same states and
  only their submission order matters.
- Correction of I-0073 §3: the context's +0xD0 is inside the viewport array (count +0xC8, viewports from +0xCC, read by
  141ACB6D0, checked), not a pending-barrier array; the draw flush emits barriers only through the render-target bind.
- One `ResourceBarrier` per builder call (141E8EB10, uncapped), no merging or deduplication; some passes switch the tracker
  to an explicit mode (1419E29B0 / 1419E36F0, tracker +0x30C) and record their own batches (141AC1D30 and about a dozen
  others).
- Views transition one subresource (mip + slice x mips), copies all; no split barriers, no aliasing barrier ever built, no
  `DiscardResource` called (its wrapper 141E8CFC0 has no caller), no enhanced barriers (no `ID3D12GraphicsCommandList7`
  IID in the image); no hazard check for a texture sampled while still a render target or a UAV (pass order is trusted).

**ER defects** (ER-DEC structure, the cost DEDUCED): redundant round trips (enter and restore around every copy, clear and
update; the render-target sets transitioned even when unchanged); a conservative UAV barrier before every dispatch, which can
serialise independent dispatches; no split barriers or aliasing; single-subresource transitions for array or volume views.

**OURS**: the frame graph drives the barriers by default (D-0171; `hand-barriers.flag` opt-out,
`native/frame/game_bootstrap/20_startup_phases.inc:82-84`): `renderer/frame_graph.h` derives every subresource's state per
use, one batch before and after each pass, transients back to their home state, first-use clear/discard actions. Legacy
`ResourceBarrier` only: no enhanced barriers, no split barriers, no `DiscardResource`; UAV barriers in the graph and the
effect stage. About 24 raw `ResourceBarrier` calls remain outside the graph (texture ingress, texture upload, scene storage,
display, effect, glare, grading, tone map, primitive pass, scene pass, backend).

## 3. CPU/GPU synchronisation and frames in flight

**ER-DEC** (ER-DEEPENING-897, D3D12-DESIGN-REVIEW E1-E2, E12, I-0073 §3): a render worker executes the frame's ordered
records then presents; the next publication waits for the worker; a ring of 6 fence tokens, the CPU waits at frame end only
for token `e80 - 2` (lag 2, 1419EE770 -> 141AFC9A0), skipped for the first 6 frames, a diagnostic flag 1447F56FC may cut the
spin short; per-thread list wrappers keep N allocators (N = frames in flight) and only the first begin of a new frame index
waits for that slot's fence (141E8E5B0, `WaitForSingleObjectEx(INFINITE)` per the descriptor worker); Present, then Signal,
then `GetCurrentBackBufferIndex`; no tearing flag, no frame-latency waitable object (I-0076 ER-STUDY). The pacing worker
adds (checked by Opus where marked):
- **The frame-lag wait spins**: 141AFC9A0 polls the fence's `GetCompletedValue` with `Sleep(0)` between polls (checked), no
  event, no bound unless a diagnostic flag is set; on the main thread, before the publication to the render worker.
- **The frame limiter spins too** (140E84830, checked): a QPC-timed limiter with targets 1/20 to 1/360 s chosen by an
  option mode (1/60 the usual one); when the window has focus it loops on `QueryPerformanceCounter` alone until the
  deadline; without focus it sleeps until about 5 ms before it, then spins. `timeBeginPeriod(1)` at start; no waitable
  timer.
- The allocator ring's reuse (141E8E5B0) blocks in `WaitForSingleObjectEx(INFINITE)` when its slot's fence has not passed;
  that fence is signalled at Present (141E9F630). The main thread's wait for the previous render job (141AF5360) loops on a
  30 s `WaitForSingleObject` while pumping messages.
- The swap chain is `FLIP_DISCARD`, windowed at creation, its buffer count a caller's value between 1 and 15 (not
  recovered); Present's sync interval below 5 comes from the presentation settings (writer not recovered); an occluded
  status counts as success; any other failure is mapped to a code and dropped (141AFCB60 does not test it).

**ER defects** (ER-DEC, the consequence DEDUCED): two busy waits on the main thread (the focused limiter on QPC, the frame
lag on `Sleep(0)`), which burn a core and, at 60 frames/s, compete with the job pools; no waitable object, so the latency
is held by the fence ring and the spin; infinite waits with no hang detection by default.

**OURS**:
- `FrameRing` (`backend.cpp:791-870`): two slots by default (`20_startup_phases.inc:98`, at most 3, `backend.h:393`), an
  allocator and list per slot plus extra chunk lists; `Begin` waits for that slot's ticket, then releases what the slot kept:
  retention per resource by submission ticket (`RetireAfter`, `backend.cpp:567`), the invariant ER-DEEPENING-897 (b) states.
- Three back buffers and a frame-latency waitable object, maximum latency = slots (`20_startup_phases.inc:104`,
  `backend.cpp:896-907`), waited at `scene_pass.cpp:1719` (bounded 100 ms); a present pacer on a waitable timer
  (`renderer/frame_pacer.cpp:21`), interval 1. Better than ER on latency.
- Full GPU drains left on the game path: the depth target recreated when the stencil clear changes (`scene_pass.cpp:1906`,
  `:2283`), the Bink movie planes (`:2053`), resize and close.
- **One DIRECT queue** (`backend.cpp:402`); textures and streams recorded on DIRECT ingress lists are put ahead of the next
  frame's list in the same `ExecuteCommandLists` (`SubmitWithIngress`, `backend.cpp:485`). DEDUCED: a burst of arrivals
  lengthens that frame's GPU work (ER puts creation uploads on its COPY queue, I-0073 §2).
- A fence wait gives up after 5 s (`backend.cpp:550-553`): "native completion timeout" faults the renderer, which ends in
  `Refuse` -> `ExitNative(0xD3126606)` (`02_frame_lifecycle.inc:38`): a GPU hang closes the game silently.

## 4. Video memory budget and leaks

**ER-DEC** (earlier): creation committed by default, placed when a heap block is supplied (141E928E0, E7); withdrawn
resources released by the frame-delayed buckets (141AF4380 / 141AF4850 / 141AF4580, ER-DEEPENING-897 (b)); a texture
holder's swap pins the old texture two frames (141A97400, I-0073 §1); upload ring blocks recycled by fence (141EAC850).
I-0079 ER-STUDY §2 and §4 (its worker, partly checked there): textures and static buffers of 8 MB or less placed in pooled
DEFAULT heaps of 32 to 256 MB (`GXBS::GXPhysicalMemoryHeap`, first-fit over a linked list, 141B04090 grows by one heap when
no block fits, **no cap and no budget check**), larger and render-target resources committed; a heap range freed at once
when its resource dies (any delay comes from the frame-delayed release of the last reference); empty heaps trimmed to one
per list. The VRAM worker of this study adds (checked by Opus where marked):
- `IDXGIAdapter3::QueryVideoMemoryInfo` (+0x70, wrapper 141EA1560) is called **once**, at the heap pool manager's
  initialisation (141B04480: local and non-local; the non-local result only decides whether the non-local pool lists
  exist); the pool sizes are constants or configuration (128, 128, 32, 256 MB; 8 MB placed bound), not derived from the
  budget; no budget-change event, no re-query found; a `SetVideoMemoryReservation` wrapper (141EA1600) has no caller found.
- **Residency wrappers exist** (correcting I-0079 ER-STUDY §4, which read those offsets as command-list calls): 141EA2BA0
  calls device +0x110 `MakeResident` or +0x118 `Evict` on a list of pageables (checked in its assembly, 141EA2D03 and
  141EA2D0B) and 141EA1880 / 141EA1A80 call +0x170 `SetResidencyPriority` (checked, 141EA1A09); their callers are reached
  only through vtables and were not found, so what drives them is unknown; no `EnqueueMakeResident`.
- Out of memory: every creation failure (committed, placed, heap, upload buffer) is mapped to a code (`E_OUTOFMEMORY` -> 9,
  141EAD2D0) and returned at once: no retry, no cache flush, no smaller fallback (141E928E0, 141E9E530, 141B04090); what the
  engine above does with code 9 was not traced. No live-object report, no VRAM counters.

**ER defect** (ER-DEC structure, the consequence DEDUCED): no budget awareness: the heap pools grow without limit and the
OS demotes or evicts silently when the card is full (a small card pays in stutter, not in a message).

**OURS**:
- `QueryVideoMemoryInfo` only for the log (`02_frame_lifecycle.inc:132`, current usage once per pacing window): no budget,
  no budget-change event, no residency call, no reaction when usage passes the budget.
- Since I-0079 (`DeviceQueues::CreateStreamed`): streamed textures and static buffers of 8 MB or less placed in pooled
  DEFAULT heaps (64 MB `ALLOW_ONLY_NON_RT_DS_TEXTURES` and `ALLOW_ONLY_BUFFERS`, the 4 KB-placed small textures in 32 MB
  heaps of their own, best fit), a range reused only once the queue passes the ticket of its release, a spare empty heap
  released; everything else (render targets, the stages' resources, large textures) still committed (about 75
  `CreateCommittedResource` sites). The 4 KB small placement is granted by the texture's tile count, not its byte size (the
  rule I-0079 met).
- The frame graph's placement planner exists but the transients are still committed (D3D12-DESIGN-REVIEW R11): each
  offscreen size keeps its own target set.
- Retirement is by ticket; an object whose retirement could not be recorded is kept to process exit on purpose
  (`backend.cpp:1184`, `UploadArena::Retire`). No live-object report at exit; the process-exit heap fault 0xC0000374 is
  still open (STATE). The night endurance test Thomas validated (zone travel loop, 1-2 h) is the measurement this needs.

## 5. Descriptors and their CPU cost

**ER-DEC** (descriptor worker; points marked checked read by Opus in `141e9bf90` and `141ea2320`):
- Heaps (141E9FD30): one shared shader-visible CBV/SRV/UAV heap (context +0x60), a shader-visible sampler heap per
  command-list wrapper (141E8C530), CPU-only DSV, RTV, CBV/SRV/UAV and sampler heaps with a locked bitmap allocator
  (141EB1CC0 / 141EB1E00, freed at once, no fence deferral), null descriptors built at start. Sizes come from a runtime
  configuration (not recovered).
- Shader-visible allocation: a first-fit free list under a lock with an infinite wait (141EB1620), fronted by per-wrapper
  size-class pools (141E8F930) that take chunks; a fence-stamped deferred collector runs at list begin (141EACBC0). A full
  heap returns handle -1, which is then bound without a wait, a growth or a log (141E9B930, 141E9C270, 141E8F3B0).
- Per draw (141ACE220): a 64-bit dirty mask per root table; a dirty table is rebuilt whole from per-stage slot arrays,
  trimmed after its last bound slot, holes filled with the null descriptor, **one `CopyDescriptorsSimple(1, ...)` per
  descriptor** (checked in 141E9BF90: a loop calling device +0xC0 with count 1, the null SRV for empty slots);
  `CopyDescriptors` is never called; a CBV with a transient pointer gets a fresh `CreateConstantBufferView` per slot
  (141E9BE20). Sampler tables are hashed and reused (141ACDF70); an SRV table cache exists but is off (flag byte
  0x1447F5631 = 0 in the image, its only reader 141ACD200 checked; no writer found); CBV and UAV tables are never cached.
- Samplers are created once per sampler state (141E9DD60), copied per table.
- Root signatures (141EA2320 graphics, checked; 141EA2120 compute): **the D3D11 slot model**: 16 tables, per stage (VS, HS,
  DS, GS, PS) SRV x128, CBV x14, sampler x16, plus UAV x8 for all; no root constants, no root CBVs; flags 0x41.
- `SetDescriptorHeaps` only at list begin (141E8E72F, 141EA5D3E) and after `ClearState` (141E8CD2C), always the same pair.

**ER defects** (ER-DEC, the cost DEDUCED): the per-descriptor copies (several hundred single-descriptor calls for a fully
dirty draw), the SRV table cache left off, whole-table rebuilds on one changed slot, a CBV re-created per slot per rebuild,
silent heap exhaustion, a locked first-fit allocator for chunks.

**OURS**:
- Model draws: one global shader-visible CBV/SRV/UAV heap, 122 chunks of 8,192 plus the stage tables under the 1,000,000
  bound, and a 2,048-sampler heap (`renderer/model_draw.cpp:205-206`), ranges per draw with merged free ranges (I-0074);
  CPU descriptors from pooled non-visible heaps (`backend.h:115`), copied into tables when a draw is prepared. Kept draws
  reuse their tables across frames (I-0070, I-0074: per-draw tables 188 to 0 a frame at Archdragon Peak), which ER does not
  do (its SRV cache is off).
- The same D3D11-shaped root signature as ER (per stage 14 CBV + 128 SRV + 16 samplers, D3D12-DESIGN-REVIEW item 3).
- The other passes each own small shader-visible heaps (AA, AO, display, effect ring, glow, grading, post draws, sun
  shadow, tone map): 22 `SetDescriptorHeaps` sites. DEDUCED: the frame list switches heaps several times; on some GPUs a
  heap change costs a flush; ER keeps one pair per list. To measure before changing.

## 6. Data uploads

**ER-DEC** (I-0073 §2, study716 §3, E4-E6): three queues (COPY, DIRECT, COMPUTE); creation uploads go through a fresh COPY
allocator and list per call on the COPY queue, waited by the creating worker on a new fence (no batching); per-frame updates
copy into a persistently mapped UPLOAD ring and record the copy on the frame's DIRECT or COMPUTE list, ring blocks tagged
`queue class << 56 | fence value` and recycled when the fence passes, no CPU wait; the CSFg dynamic buffers are updated in
place by `CopyBufferRegion` between restoring transitions.

**ER defect** (ER-DEC, the cost DEDUCED): one COPY submission, one fence and one CPU wait per created resource; no
asynchronous streaming copy path.

**OURS**: `UploadArena` (`backend.cpp:1201`): size classes 1/4/16/64 MB pooled 16/4/2/1 (`backend.h:235-236`), persistently
mapped, retired by ticket, never blocks; above 64 MB a chunk of its own; DEDUCED: unbounded growth in a burst. Dynamic
streams through `UploadVersion`; textures through ingress recordings on the DIRECT queue (section 3). I-0079: a pooled
staging allocator for the streamed resources (persistently mapped UPLOAD chunks reused by ticket; ER creates and waits on
one committed UPLOAD buffer per created resource, I-0079 ER-STUDY §3). `GPU_UPLOAD` heaps (Resizable BAR) unused (ROADMAP: an option later).

## 7. Driver and environment dependencies

**ER-DEC** (device worker; checked by Opus where marked):
- Device: one `D3D12CreateDevice` (d3d12 ordinal 101) in 141EA2730 at **feature level 12_0** (the index 2 stored at
  1419E9DB2, mapped by 141E90FB0; checked) for `ID3D12Device5` (riid 0x1430A5238); adapter 0 only, through
  `CreateDXGIFactory1` and `EnumAdapters1(0)` then a query of `IDXGIAdapter3` (141EAE580): no `IDXGIFactory6` GPU preference,
  no WARP, no software-adapter skip, no fallback level; no `CheckFeatureSupport` found at start; root signature 1.0.
- Debug: the layer is enabled only when a configuration byte is set (141EA2730, checked: `D3D12GetDebugInterface`, ordinal
  102, then `EnableDebugLayer`; a missing layer is tolerated by a null check); the GPU-based validation and synchronized-queue
  flags come from bytes the renderer zeroes (1419E9D99 memset), so they stay off; an InfoQueue deny list of two message ids
  (0x2D1, 0x335). No DRED, no DXGI debug, no Agility SDK. Where the configuration byte is set was not found.
- Device removed: never asked (`GetDeviceRemovedReason` has no call in the backend); HRESULTs are mapped to codes
  (141EAD2D0: removed 0xD, reset 0xE, hung falls in the generic 0xF) and the presentation loop drops Present's result
  (1419EC620); every fence wait is `WaitForSingleObjectEx(INFINITE)`; the frame-lag wait 141AFC9A0 polls
  `GetCompletedValue` with `Sleep(0)` between polls (checked), its hang message off (thresholds 0 in the image) and, when on,
  ending in a deliberate null write.
- Vendors: a dispatch (141EAC340) installs HDR callbacks per vendor: NVIDIA (`VendorId == 0x10DE`, `nvapi64.dll` loaded
  dynamically, optional), AMD (`0x1002`, `agsInit`, `agsSetDisplayMode` for the HDR mode); **`amd_ags_x64.dll` is a static
  import, required on every vendor** (ER evidence `pe-imports.csv`); no Intel branch, no driver version check, no
  `SetStablePowerState`.
- Crashes: no minidump (dbghelp only inside Havok's stack tracer), no vectored handler, the CRT's filter only; an assertion
  dialog; no PIX markers.

**ER defects** (ER-DEC, the consequence DEDUCED): no device-removed recovery or message (a TDR ends in a hang on an infinite
wait or a crash); a frame-lag wait that spins a core with `Sleep(0)` while GPU-bound; a creation failure unchecked (no message
on a GPU below 12_0); adapter 0 only (a laptop's integrated GPU may be picked); a hard dependency on an AMD DLL.

**OURS**:
- **The D3D12 debug layer is on by default** (`20_startup_phases.inc:44`: `debug_layer=true` unless `no-debug-layer.flag`
  sits next to the module); the launcher places the flag (`tools/Launch-DS3-DX12.ps1:27`). DEDUCED: a copy started without
  the flag on a PC without the Windows Graphics Tools fails at `Check(D3D12GetDebugInterface)` (`backend.cpp:375`) and does
  not start; with them, it runs with the validation cost. GPU-based validation behind `gpu-validation.flag`.
- Adapter: the first hardware adapter by high-performance preference that creates feature level 11_0 (`backend.cpp:383-391`);
  WARP only when asked (`backend.cpp:381`, used by the unit controls); no Agility SDK; no vendor library, no driver version
  check.
- No DRED in production (only the old `world/live_host` diagnostic), no resource names (`SetName`), no PIX markers; a vectored
  handler logs access violations (`22_setup.inc:132`); no minidump (Thomas validated automatic minidumps as a stability tool,
  2026-10-21).
- `d3dcompiler_47.dll` at run time for our own HLSL (section 1).

## 8. Leads for the audit

DEDUCED proposals; each names its measurable check (ROADMAP « audit des pièges DX12 »). Order is the audit's to choose.

- **A1 Pipelines, cold start.** Measure a first start with an empty cache (since D-0241: `native-pipelines.v3.psc` removed by hand; before it `Launch-DS3-DX12-ReloadShaders.cmd`; then the
  Firelink path): count `frame_created` per zone; keep ER's shipped-list idea but improve on it: the misses off the frame
  path (a draw waits a frame rather than blocking the recording thread, in the spirit of D-0197), never a mutex held while
  compiling (already so). Check: zero frame-path compilations after the boot precompile on a cold PC.
- **A2 Our own pipelines into the store.** The stage and composition pipelines and the `D3DCompile` shaders go through
  `PipelineStore` and its precompile (with the precompile screen). Check: `D3DCompile` absent from the frame path, all
  pipelines named in the store's list.
- **B1 Raw barriers.** Move the ~24 raw `ResourceBarrier` sites into the frame graph or document why they stay (ingress
  lists before the frame). Check: GPU-based validation runs at the witness zones with 0 errors and 0 warnings (Thomas: WARP
  and GPU-based validation since only an NVIDIA card is available).
- **B2 Superfluous barriers.** Count transitions per frame by pass and look for enter/restore pairs and per-dispatch UAV
  barriers of the ER kind; consider split barriers only where a measured gap exists. Check: barrier count per frame before
  and after, the same pictures.
- **C1 Full drains left.** The depth target recreation on a stencil-clear change, the Bink planes and resize: replace by
  retirement by ticket where possible. Check: `gpu_audit.drains` zero in play outside resize and close.
- **C2 Hang and device removed.** Unlike ER: on a fence timeout or a removed device, write the reason
  (`GetDeviceRemovedReason`), DRED breadcrumbs and page fault data (enabled in measure mode at low cost), a minidump (the
  first stability tool), and a message to the user instead of a silent exit. Check: a forced TDR (`dxcap -forcetdr` or a
  deliberate long shader in a test build) yields a dump and a log row.
- **C3 Waits without spinning.** Keep our event and timer waits (better than ER's `Sleep(0)` and QPC spins). Check: the
  engine thread's idle time in the sampler is in wait functions, not in a spin.
- **D1 Budget.** Read `QueryVideoMemoryInfo` budget each pacing window and register the budget-change event; above the
  budget, trim the pools' spare heaps and the kept models' expiry first, then lower the streamed mip level (the D-0197 rule
  forbids stand-ins, so the policy acts on what is kept, not on what appears). Check: the run under an artificially limited
  budget of 4 to 6 GB (ROADMAP; DEDUCED means: a test cap in our own pools, since D3D12 offers no budget override).
- **D2 Leaks.** Count live heaps, placed ranges, committed resources and descriptors by kind in the frame row; the endurance
  test's zone-travel loop must return to its starting level. Check: the night test, then the exit heap fault 0xC0000374
  (the debug layer's `ReportLiveDeviceObjects` at close in a test build).
- **D3 Transients.** Use the frame graph's placement planner for the per-size target sets (D3D12-DESIGN-REVIEW R11, ER has
  no aliasing). Check: local usage after visiting the offscreen views (menus, Status, portraits) and returning.
- **E1 Heap switches.** Move the stages' small shader-visible heaps into the global heap (ranges per stage) so a frame list
  sets one pair, as ER does. Check: `SetDescriptorHeaps` calls per frame list, GPU frame time before and after.
- **E2 Root signature.** Both ER and ours carry D3D11's 14/128/16 slots per stage; size the tables on use or move to bindless
  (ROADMAP wave 2), never ER's per-descriptor copies. Check: descriptors copied per frame (CPU prepare time at m45).
- **F1 A copy queue.** Creation and streaming uploads on a COPY queue with its own fence and a GPU-side wait of the DIRECT
  queue (beyond ER, which waits on the CPU per creation). Check: the frame's GPU time no longer grows with arrivals at
  Firelink.
- **G1 Debug layer polarity.** Make the debug layer opt-in (a `debug-layer.flag`) and tolerate its absence as ER does.
  Check: the module starts on a PC without the Graphics Tools and without any flag.
- **G2 Adapter and feature checks.** Keep the high-performance preference (better than ER's adapter 0); check the feature
  level and options we use at start (resource binding tier, root signature 1.1, the formats queried today) and fail with a
  message. Check: WARP and a second adapter, the start log.
- **G3 Vendor independence.** No vendor DLL (unlike ER's static AGS); names on resources (`SetName`) and PIX markers in a
  development build so captures on AMD and Intel are readable. Check: WARP runs of the witness zones; AMD and Intel testers
  before any public release (ROADMAP).

## Resume note

State at 2026-10-21 07:00 Paris (before the PC restart): branch `claude/er-dx12-pitfalls-study`, rebased on `main`
4741f6f2, not merged (a study thread; the audit workstream takes it). Every section has its ER decompilation. Open ER
questions, none blocking the audit: the runtime sizes of ER's descriptor heaps and heap pools (a renderer configuration
built elsewhere), ER's swap-chain buffer count and V-sync writer, the eviction tier values, what sets ER's debug byte, who
calls ER's residency wrappers, what the engine does with an out-of-memory code. The workers' exports were in the session scratchpad (lost at the restart; re-export
from the mirrors if a design needs an address).
