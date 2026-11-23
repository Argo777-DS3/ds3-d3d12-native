# D3D12 review, phases A to C: preparation of the nine reprises (static, 2026-10-11)

Request (coordinator, 2026-10-11): prepare phases A to C of [D3D12-DESIGN-REVIEW](D3D12-DESIGN-REVIEW.md) (main `bbe48c5c`, reprises R1 to R16), which Thomas authorised for after the menus workstream. For each reprise of A to C: the native files and functions touched, the dependencies between reprises, the safest commit order, what to measure before and after (phase A), and the live witnesses. No native code was changed, nothing was launched. Written by the static thread (I-0068); filed under I-0066 because the reprises change the production native module.

Phases A to C hold nine reprises: **A** R15; **B** R4, R2, R3, R5; **C** R6, R7, R9, R8a. R1, R12, R14 (phase D) and later phases are only named where A to C must leave them a clean base.

Labels: **observed** = read in the native source at `bbe48c5c` (line numbers are that tree's); **review** = a statement of D3D12-DESIGN-REVIEW; **proposed** = this note's design; **open** = needs a measurement or a read first.

## 0. Answer first

1. **Order of commits** (details in section 4): R15, then the measurement baseline; B1 retirement tokens (R3 core, behaviour-neutral); B2 upload arena (R2); B3 batched ingress and pooled CPU descriptors (R5); B4 static geometry in DEFAULT memory (R4a); B5 dynamic streams (R4b); B6 removal of the ingress and retirement `Drain` calls one by one; then C1 pre-created PSOs (R9), C2 global descriptor heaps with used-registers-only tables (R6), C3 root signature sized on use (R7), C4 persistent draws (R8a). Every slice is behaviour-neutral or switchable by a flag file, so each can be built, run against the same witnesses and reverted alone.
2. **Three facts of the code change the review's plan**, none contradicts it:
   - **A single mutex serialises everything that touches the queue.** `gpu_submissions` (`game_bootstrap.cpp:296`) is held for the whole render frame (`Clear`, `:621`) and by every texture, storage and stream upload (`TextureIngress` takes it by reference, `:297`; `:5526`, `:5595`, `:1707`, `:6969`, `:9059`). Loader threads therefore wait for the frame, and `DeviceQueues` (`next_`, `submitted_`, `pending_`) is not thread-safe by itself. Phase B must put ingress on its own copy list and fence (R5) before it removes any wait, or the lock stays the bottleneck.
   - **The pools the review asks for already exist in a first form** (1121): constant chunks of 8 MB and 64-draw descriptor chunks in `model_draw.cpp:121-170`, upload chunks in `primitive_pass.cpp`. They return to the pool when the last holder is destroyed, which is a refcount, not a fence. R2 and R3 therefore change the *return policy* and extend the pools to the other users; they do not start from nothing.
   - **Phase A needs almost no new code.** A release-like run exists (`-NoDebugLayer` of `Run-StartupTrace.ps1`, flag `no-debug-layer.flag`, `game_bootstrap.cpp:8926`), and the pacing row `native_present_pacing` already splits collect, prepare, render, record, GPU wait, present, the five phases of the UI admission and the three phases of draw preparation. Phase A adds only a counter set (section 3.2) and R15.
3. **Accepted precision (coordinator, 2026-10-11) on the review's exit check for phase B** (section 6 of the review: "no `Drain` left outside resize/shutdown"): the per-frame `Complete` at `scene_pass.cpp:1644` and `Drain` at `:1665` stay until R1 in phase D, so the phase B exit reads **no `Drain` in the ingress, retirement and stream paths** (accepted); the frame path keeps its two waits until phase D.
4. **Two checks to make in phase A before phase C is designed**: the adapter's `ResourceBindingTier` (it decides whether R6 may leave table slots unwritten, section 2.6) and the number of distinct root-signature shapes the admitted programmes need (R7).

## 1. What the code does today (observed)

| Fact | Where |
|---|---|
| One direct queue, one fence; `Execute` signals a ticket, `Complete` waits on it, `Drain` signals a new ticket and waits | `renderer/backend.cpp:179-201`, `backend.h` `DeviceQueues` |
| The in-game frame: reset the one allocator, record all passes, `Execute` + `Complete` (`:1644`), optional readback, `Present(0,0)`, `Drain` (`:1665`), `Diagnostics()` | `scene_pass.cpp:1545-1670` (`RenderNativeModels`) |
| Other submissions with their own waits: menu offscreen view (`:1419`), Bink frame (`:1532`), UI admission (`:1827`, `:1830`), target recreation (`:1372`, `:1576`, `:1890`), scene retirement (`:1755`, `:1800`), primitive withdrawal (`:1841`) | `scene_pass.cpp` |
| Draws live in `native_model_inflight_` until after the `Drain`; `RenderNativeModels` refuses a frame while it is non-empty | `scene_pass.cpp:1550`, `:1581-1583`, `:1665` |
| Per draw per frame: PSO lookup by a byte string, 2 x 142 descriptors copied from a null template, 2 x 16 samplers copied, every constant block copied into a pooled chunk and given a CBV, samplers recreated with `CreateSampler`; one 64-draw table chunk per 64 draws | `renderer/model_draw.cpp:259-303` |
| Recording per draw: targets, viewport, scissor, root signature, descriptor heaps, 4 tables, PSO, topology, vertex and index buffers | `renderer/gbuffer_encoder.h:40-61` |
| Root signature: per stage one table of 14 CBV + 128 SRV, one table of 16 samplers; hull, domain and geometry denied | `model_draw.cpp:14`, `:177-190` |
| FLVER storage kept in an UPLOAD committed buffer for its whole life; retirement drains | `game_bootstrap.cpp:5556-5571`, `:5598`, `:5526` |
| Physical and FaceGen streams, and cloth streams, get a new UPLOAD resource per version or per simulated frame | `game_bootstrap.cpp:1707-1708`, `:6969` |
| Per texture: a committed DEFAULT resource, a committed UPLOAD buffer, a one-descriptor heap (`NativeTextureRecord::view`), a command allocator and list, and a `Drain` | `frame/game_texture_ingress.cpp:44-66`, `content/gpu_texture_upload.cpp` |
| Same pattern for FaceGen composition, menu offscreen targets (`Drain` at `:150`, `:195`) and structured buffers (UPLOAD, no wait) | `game_texture_ingress.cpp:69-242` |
| Buffers rewritten in place every frame and safe only because the frame drained: lighting constants x4, environment scene and constants, SSS constants, velocity constants, the point-light ring | `scene_pass.cpp:864`, `:1034-1075`, `:1088`, `:1148-1149`, `:1177`, `:1274` |
| Diagnostics in the frame path: backbuffer readback and CPU hash every 120th frame or on request (`:1633-1663`), velocity readback every 30th frame (`:1611`, `:1646`, `ReadNativeVelocity` `:1300`), `Diagnostics()` every frame (a scan only with the debug layer, `:773`), one trace row per frame (`game_bootstrap.cpp:647`), velocity pictures every 90th frame (`:626-631`) | as cited |
| Existing frame graph, used only by the fixture path | `renderer/frame_plan.h`, `frame_plan.cpp` (`CompileFramePlan`) |

## 2. The nine reprises

Each card: **touches** (native files and functions), **depends on / enables**, **slices**, **pitfalls found in the code**.

### 2.1 R15 - diagnostics out of the frame path (phase A)

- **Touches**: `scene_pass.cpp` `RenderNativeModels` only (`sample_pixels` `:1633`, the readback block `:1634-1663`, `EncodeNativeVelocity(... serial%30==0 ...)` `:1611`, `ReadNativeVelocity` `:1646`, `Diagnostics()` `:1665`); `game_bootstrap.cpp` `Clear()` (the per-frame `Log(row)` `:647`, velocity picture trigger `:626-631`, `ProbeWorldUpdateHooks` every 300 frames `:650`).
- **Do not touch**: the fixture path `Render()` (`scene_pass.cpp:2084-2114`) and `backend.cpp` `RenderFrame`. The camera and content hosts require `pixel_hash` and `changed_pixels` every frame (`native/camera/Test-Ipc.ps1`, `Verify-Live.ps1`, `native/content/neutral_test.cpp`).
- **Depends on / enables**: nothing / the clean baseline for every other measurement.
- **Slices**: (a) a *witness switch* read once at start, like `no-debug-layer.flag`: the readbacks, the hash and the per-frame trace row run only when the switch is present, at the same cadence as today; without it the in-game path does neither. The runner places the flag for witness runs, so the existing pilots and the velocity witness (Native1191) keep working unchanged. (b) the per-frame trace row becomes one row per pacing window plus rows on change.
- **Pitfalls**: the pilots read `startup.jsonl` rows; any row removed must be checked against `Pilot-NativeWitness.ps1` (it matches `native_present_pacing` with `content_draws`), `Run-StartupTrace.ps1` and the `native_model_draw_submitted` consumers. The review's expected gain is "low but immediate": the readback is 3.7 MB per 120 frames, the velocity pass roughly one mapped 1280 x 720 half-float image per 30 frames with a CPU loop over every pixel; the per-frame `WriteFile` of a 500-byte row is the larger part at 60 fps. Measure before deciding what to cut (section 3).

### 2.2 R3 - retirement tokens (phase B, first slice)

- **Touches**: `renderer/backend.h/.cpp` (`DeviceQueues`: a retirement list keyed by ticket, collected in `Complete`); consumers one at a time: `DestroyModelStorage` `game_bootstrap.cpp:5526` (the only `Drain` of a storage retirement), model streams `:1577`, `:1586`, `scene_pass.cpp` `RetireSceneResources` `:1781`, `WithdrawSceneWork` `:1797`, `AdmitPrimitiveWork` `:1827`, `WithdrawPrimitiveWork` `:1841`, `Targets`/`DepthTarget` recreation `:1372`, `:1576`, `:1890`; `TextureIngress::Retire` (`game_texture_ingress.cpp:234`, today only `records_.erase`).
- **Depends on / enables**: nothing / R2, R4, R5, and phase D (R1).
- **Slices**: B1 adds `RetireAfter(ticket, owner)` and `CollectCompleted()` to `DeviceQueues` with **no consumer**; with every frame still drained the behaviour is identical. A unit test in `renderer/ownership_test.cpp` (retire, collect before and after completion, device removal, close). Then one consumer per commit, each replacing a `Drain` by `RetireAfter(last_ticket_that_could_use_it, resource)`.
- **Pitfalls**: the *last ticket that could use it* must be a real fact: a model storage is used by draws held in `native_model_inflight_` (released after `:1665`) and by the stream versions that reference it; with phase B alone (frames still drained) the ticket of the current frame is enough. Identity races with withdrawal follow DIRECT-CONNECTION's version rules (review R3). `DeviceQueues::Close` already drains: keep it, and add "collect everything" after it.

### 2.3 R2 - one upload arena with fence tokens (phase B)

- **Touches**: `model_draw.cpp` (`ConstantChunk` pool `:121-157`, `PooledConstants`, the constant copy `:286-297`); `primitive_pass.cpp` (`PooledUploads`, `suballocate` `:87-95`, buffer uploads `:20`, `:47`); `scene_pass.cpp` point-light ring (`native_point_ring_` `:1034-1075`, grown by `NewBuffer`), movie upload (`RenderNativeMovie` `:1491`), the per-frame in-place buffers listed in section 1; `game_bootstrap.cpp` stream and storage uploads (`UploadNativeModelBytes` `:5556`, callers `:1708`, `:5598`, `:6969`); `content/gpu_texture_upload.cpp` (staging).
- **Depends on / enables**: R3 (token return) / R4 dynamic, R5, R6 persistent tables, phase D.
- **Slices**: B2a an `UploadArena` (persistently mapped UPLOAD buffers, 256-byte alignment, a return list keyed by ticket) used first by the model constant chunks: the chunk goes back to the free list when its last holder is gone **and** its ticket is complete; with every frame drained this equals today. B2b the primitive pass and the point-light ring. B2c the per-frame in-place buffers: for each of the lighting, environment, SSS and velocity constant buffers, allocate from the arena (or keep one buffer per frame slot) so that phase D can overlap frames; today they are safe only because of `:1665`. B2d movie and staging.
- **Pitfalls**: `ConstantChunk` is shared by all draws of a frame and by a *second* kind of holder (`pooled_` in each `OwnedNativeModelDraw`); the return policy must keep both. The 8 MB chunk limit (`constant_chunk_bytes`) and the "allocation > chunk" path in `primitive_pass.cpp:88` (an unpooled upload) are the two special cases to keep.

### 2.4 R5 - batched ingress and pooled CPU descriptors (phase B)

- **Touches**: `frame/game_texture_ingress.cpp` (`AdmitOwned` `:44-66`, `AdmitComposedFace`, `AdmitOffscreenTarget`, `AdmitStructuredZero`, `Close` `:238`), `game_texture_ingress.h` (`NativeTextureRecord::view` is a `ComPtr<ID3D12DescriptorHeap>` read in about twenty places: `game_bootstrap.cpp:2907`, `:2925`, `:3653`, `:3685`, `:5263-5264`, `:610-613`, `:8619-8632`, `:8776-8827`, `game_ui_bindings.cpp:25`, `game_ui_images.cpp:57`); `content/gpu_texture_upload.cpp` (`RecordNativeTextureUpload`); `scene_pass.cpp:638` (the fixture `Textures()`), and the UI session upload callers (`game_ui_session.cpp`, which take `gpu_submissions`).
- **Depends on / enables**: R3 (token), R2 (staging from the arena) / R4 (the same copy list), the removal of the ingress `Drain`s.
- **Slices**: B3a an *ingress copy context*: its own allocator and list, filled by `Admit*` under a short lock (the CPU-side decode stays outside), executed on the direct queue for now (a COPY queue is R16, later) with its own ticket; `upload_fence` becomes that ticket and `resident` turns true when the ticket completes, observed by the next frame's collect instead of by a wait in `Admit*`. B3b the pooled CPU descriptor allocator: one non-shader-visible heap with a free list; `NativeTextureRecord` gains `D3D12_CPU_DESCRIPTOR_HANDLE cpu_view` next to `view`; the ~20 readers migrate to a helper, then `view` is dropped; handles return by token. B3c batching: several `Admit*` calls between two submissions share one list.
- **Pitfalls**: **residency becomes asynchronous** (a texture that was resident when `Admit*` returned is now resident one or more frames later). The prepare step already treats a non-resident texture as `missing_textures` and skips the draw (`game_bootstrap.cpp:5263`), so a draw can appear a frame late; the witness must show no *persistent* gap (section 5). `AdmitComposedFace` and `AdmitOffscreenTarget` render into a target the *same frame's* menu views then sample: they need the graph or an explicit wait (keep their `Drain` until R10/R12 if the ordering cannot be proven). Thread safety: ingress runs on loader threads; the copy context needs its own lock, distinct from the frame lock `gpu_submissions` (fact 2 of section 0).

### 2.5 R4 - geometry in DEFAULT memory (phase B)

- **Touches**: `game_bootstrap.cpp` `UploadNativeModelBytes` `:5556` (the only buffer factory), its callers `AdmitModelStorage` `:5598` (static FLVER storage), the physical and FaceGen stream update `:1707-1708`, the cloth upload `NativeClothVertexUpload` `:6969`; `renderer/model_geometry.h` `BindModelGeometry` (reads `GetGPUVirtualAddress` and `GetDesc`, both valid for a DEFAULT buffer); `model_draw.cpp` `ValidateGeometry` `:58-77` (bounds by `GetDesc().Width`, unchanged); the structures `NativeModelStorage` `:333` and `NativeClothStream` `:359`.
- **Depends on / enables**: R5 (copy context) for the static part, R2 (arena source) and R3 (retire the superseded version) for the dynamic part / the GPU-side benefit of everything else.
- **Slices**: B4 (static): `AdmitModelStorage` creates a DEFAULT buffer and records the copy from an arena range through the ingress context; the buffer is promoted by its first use as a vertex or index source (D3D12 allows implicit promotion from COMMON), so no per-draw transition is needed; the CPU bytes stay (the palette validation reads `model.geometry->bytes` and the stream bytes, `game_bootstrap.cpp:4795-4810`). B5 (dynamic): one **owner-retained DEFAULT buffer per stream** updated by a copy recorded at the start of the frame's list (before the draws), with the arena as the source. Because there is one queue, the in-place update is ordered after the previous frame's reads without any extra fence; the *source* range goes back by token. The immutable "one resource per version" semantics of `NativeClothStream`/model streams (a draw keeps the version it bound until its submission completes) is replaced by "the draw reads the buffer as updated at the start of its frame": with frames still drained this is equivalent; with frames in flight (phase D) each stream needs a ring of buffers, one per frame slot (note it in B5's commit message).
- **Pitfalls**: the stream's *generation* is compared by draws (`ModelGeometryBinding::stream_generation`) and by `ValidateModelPaletteIndices` through `candidate.resource.Get()==mesh.geometry.vertex_resource.Get()` (`game_bootstrap.cpp:4799-4802`): an in-place update keeps the resource identity, so the generation must still advance on every content change, or the CPU copy used for the palette check goes stale. A flag file (`legacy-upload-geometry.flag`) selecting the old UPLOAD path lets one build measure both.

### 2.6 R6 - global descriptor heaps and used-registers-only tables (phase C)

- **Touches**: `model_draw.cpp` (`PooledTables` `:158-170`, `BindDevice` null templates `:171-206`, `Prepare` `:277-303`), `gbuffer_encoder.h` `BindGBufferDraw` (heaps and tables), `scene_pass.cpp` `RenderNativeModels` (set the heaps once before the first model draw), `primitive_pass.cpp` (its two heaps per admission, `:100-102`), `scene_pass.cpp` lighting and composition passes that set their own heaps (`:1620` and the lighting passes).
- **Depends on / enables**: R2 and R3 (linear allocation retired by token) / R7, R8a, phase D.
- **Slices**: C2a a **frame-linear shader-visible CBV/SRV heap and one sampler heap**, allocated per frame and retired by ticket; draws allocate ranges from it and the model path sets the heaps once; the old `TableChunk` pool is deleted. C2b **write only the registers the programme declares**: `ValidateBindings` already computes, per stage, the declared constants, SRVs and samplers (`model_draw.cpp:89-108`), so `Prepare` can write those descriptors and skip the `CopyDescriptorsSimple(284 + 32)` template fill. C2c samplers created once at stable indices from the sampler bank (`scene->sampler_bank->descriptors[bank]`, `game_bootstrap.cpp:5248`) instead of `CreateSampler` per draw (`model_draw.cpp:299`).
- **Open (phase A)**: skipping the null fill is legal on every tier only if the bound table's unused descriptors are never read; on Resource Binding Tier 1 all descriptors of a bound table must be valid. Read `D3D12_FEATURE_D3D12_OPTIONS.ResourceBindingTier` of the test adapter and of the user's adapter in phase A; if it is Tier 1, C2b keeps a small null prefix or waits for R7.
- **Pitfalls**: shader-visible heap size limit (1,000,000 CBV/SRV/UAV descriptors on Tier 1/2, sampler heap 2048); the current 64-draw chunk exists because of the sampler limit (`model_draw.cpp:122`): a linear sampler allocation per frame must be capped or reset per pass. The UI and lighting passes call `SetDescriptorHeaps` with their own heaps; changing heaps inside a list flushes the GPU's descriptor cache (review R6), so the global heap must also hold their ranges.

### 2.7 R7 - model root signature sized on use (phase C)

- **Touches**: `model_draw.cpp` `BindDevice` (`cbv_count=14`, `srv_count=128`, `sampler_count=16`, the 4 parameters), `Prepare` (root and PSO key), `ValidateBindings` (the bounds `slot<cbv_count`...), `gbuffer_encoder.h` (`tables.size()<=4`).
- **Depends on / enables**: R6 (the table layout is allocated by R6) and R9 (the root signature identity is part of the PSO key) / R8a.
- **Slices**: C3a measure the largest register each admitted programme reads per stage (the information is in the RDEF interface the draw already inspects) and the number of distinct shapes; C3b one root signature per shape, cached by (cbv, srv, sampler) triple per stage; the PSO key already takes the root signature through `root_` and gains the shape; C3c optional: the hot constant blocks as root CBVs (beyond ER, section 2.9 of the review, measure first).
- **Pitfalls**: root signatures multiply PSOs, so R7 must follow R9's keyed cache; hull, domain and geometry stay denied until the tessellated families (TESS-SHADOW-FORWARD-NATIVE) add their own signature.

### 2.8 R9 - PSOs created ahead (phase C)

- **Touches**: `model_draw.cpp` `Prepare` (PSO built at first use under `caches.lock`, `:259-276`; the byte-string key `:259-266`); `game_bootstrap.cpp` where the per-material pass rows are known (`prepared->recipes[...] = SelectModelPrograms(...)` `:3299`, `PrepareNativeModelFrame` `:4734`); `renderer/scene_pass.cpp` `ModelTargetFormats`/`ModelDepthFormat` (the target formats are static).
- **Depends on / enables**: nothing in B, but it should precede R7 / the first-use hitch disappears, R8a gets a key it can hold.
- **Slices**: C1a a hashed key (a 64-bit combination of the verified program entries, the element list, the fixed-function states and the formats) replaces the string; C1b a `PrewarmModelPipeline(device, input-shape)` called at material admission for each recipe; C1c creation off the cache lock: today `CreateGraphicsPipelineState` runs while `caches.lock` (a recursive mutex shared by all draw preparation) is held, so one new PSO stalls every thread preparing a draw; C1d (optional) an `ID3D12PipelineLibrary` disk cache.
- **Pitfalls**: the PSO needs the *input layout* and the *render target set*, both known at admission; the blend and depth states come from the MTD policy of the recipe (`game_bootstrap.cpp:5190-5240`), which must be evaluated at admission as well. Anything not known there stays a first-use creation (counted, logged).

### 2.9 R8a - persistent draws (phase C, last)

- **Touches**: `game_bootstrap.cpp` `PrepareNativeModelFrame` (the per-mesh loop `:4734-5310`; the three witness phases `native_prep_ticks[0..2]` mark its validation, resource and constant binding, and `PrepareNativeModelDraw` parts), `NativeSelectedScene::prepared_meshes` and the collect step that builds it (`AcquireNativeSelectedScene`), `model_draw.h/.cpp` (`OwnedNativeModelDraw` split into an immutable part, the shape and its descriptors, and a per-frame part, the constants), `scene_pass.cpp` (`native_model_inflight_` becomes a list of persistent draws plus the frame's constant ranges).
- **Depends on / enables**: R2 (constants ranges per frame), R6 (stable descriptor ranges), R7/R9 (stable root and PSO) / R8b, phase D.
- **Slices**: C4a key a draw by (instance, mesh, pass row, programme variant) and keep it while its owner and every input keep their versions (texture record generation, storage and stream generation, pose generation, scene generation); C4b per frame write only the constant blocks whose producer output changed (compare a 64-bit hash of each block, an existing pattern for the cloth fingerprint `:6969`), and write the pose palette **once per model** instead of per mesh; C4c a switch (`rebuild-draws.flag`) to run the old per-frame rebuild for an A/B run.
- **Pitfalls**: the review rates staleness as the risk: a missed version check draws old data. Cheapest mitigation: every persistent draw stores the versions it was built from and the frame compares them; a mismatch rebuilds that draw and logs one row. Persistent *descriptors* need the SRV owners (`NativeModelSrvBinding::owner`) to be kept as long as the draw; texture records retired by R3 must not be recycled while a persistent draw still references their CPU descriptor (R5's allocator returns handles by token, and the draw's owner reference delays the retirement request).

## 3. Phase A: what to measure

### 3.1 Runs

| Run | Setting | Why |
|---|---|---|
| M0 | current build, debug layer on (as today) | continuity with the numbers of the review (Native1135, i66-native-1254) |
| M1 | same, `-NoDebugLayer` | the release-like baseline; the validation layer inflates every D3D12 call |
| M2 | M1 plus R15 | what R15 alone gains |
| M3 (after B) | M1 build with phase B | gain of the memory and upload work |
| M4 (after C) | M1 build with phase C | gain of the descriptor and draw work |

Every run: the same save and slot, the same teleport or start position (`Run-PilotPair.ps1 -Slot ... -Teleport ...`), the same quality preset (`-QualityPreset`, QL3) and motion blur setting, the same resolution; **discard the first three in-game windows** (the load-in transition is not steady state, review section 1); take at least six steady windows of 120 frames per run, and two runs per setting. Compare medians; treat a change smaller than the spread between the two runs of one setting as noise.

Three scenes: (S1) the High Wall idle frame (the world path: collect, prepare, record, GPU), (S2) the title and the Status menu with portraits (UI admission and the menu offscreen views), (S3) a load or teleport window (retirement, ingress). S1 carries the verdict for B and C; S2 and S3 expose the waits that S1 hides.

### 3.2 Counters (the only new code of phase A, with R15)

The pacing row already holds: fps, `slowest_ms`, `collect_ms`, `prepare_ms`, `render_ms`, `record_ms`, `gpu_wait_ms`, `present_ms`, the five UI admission phases (produce, drain, build, wait, diagnostics) and the three prepare phases (validation, resources, draw preparation). Add, to the same row and per window:

| Counter | Where it is counted | Before | Expected after |
|---|---|---|---|
| `drains`, `completes` per frame | `DeviceQueues::Drain/Complete` | about 3 to 5 | B: ingress and retirement gone; D: 0 in the frame |
| `committed_buffers_created` per frame | `UploadNativeModelBytes`, `ConstantUpload`, `PooledConstants`, `suballocate` large path, `NewBuffer` | stream and cloth versions each frame, growing rings | B: about 0 in steady state |
| `upload_bytes` per frame | arena or each upload site | not known | reported, bounds the arena size |
| `descriptors_copied`, `samplers_created` per frame | `Prepare` (`CopyDescriptorsSimple`, `CreateSampler`) | about 316 copies and a sampler count per draw | C: close to the registers actually used |
| `heap_switches`, `set_pipeline`, `set_root` per frame | `BindGBufferDraw` | per draw | C: heaps once per list |
| `pso_created` per frame | `Prepare` | first use | C: 0 after warm-up |
| `persistent_draw_rebuilds` | R8a | n/a | reported |
| GPU time of the main list | two timestamp queries around the model passes (resolve to the readback) | not known | the denominator for "the GPU does not overlap" |
| process private bytes, DXGI local video memory | `GetProcessMemoryInfo`, `IDXGIAdapter3::QueryVideoMemoryInfo` once per window | not known | B: lower committed upload bytes |
| `ResourceBindingTier`, adapter name | once at start (log) | not known | decides R6 |

### 3.3 Exit of phase A

A table of M0, M1, M2 medians for S1, S2, S3; the R15 verdict (keep, narrow or drop each item by its measured cost); the Tier and the root-signature shape count; and the counter baselines. Phases B and C compare against M1.

## 4. Dependencies and the safest order

```
R15 -> [measure M1/M2]
R3 core ----+----> R2 arena -----+----> R4 dynamic ----> remove Drains (B6)
            |                    |
            +----> R5 ingress ---+----> R4 static
R9 (hash, prewarm, off-lock) ---> R6 (global heaps, used registers) ---> R7 (shapes) ---> R8a (persistent draws)
                                  ^ needs R2 (arena) and R3 (tokens)
```

Commit sequence (each commit builds, passes the control suites of section 5, and is reversible alone):

| # | Commit | Reprise | Behaviour change | Flag |
|---|---|---|---|---|
| 1 | witness switch, counters, tier log | R15 + measurement | none by default for pilots (flag placed by runner) | `witness.flag` |
| 2 | **M0, M1, M2 runs** | A | none | |
| 3 | `DeviceQueues::RetireAfter/CollectCompleted` + unit test | R3 core | none | |
| 4 | `UploadArena`; model constants on it; token return | R2a | none | |
| 5 | primitive pass and point-light ring on the arena | R2b | none | |
| 6 | per-frame in-place buffers on the arena or slots | R2c | none | |
| 7 | ingress copy context, `upload_fence` = its ticket, async residency | R5a | residency one frame later | `sync-ingress.flag` |
| 8 | pooled CPU descriptors, `cpu_view` accessor, readers migrated | R5b | none | |
| 9 | static FLVER storage in DEFAULT | R4a | none (image identical) | `legacy-upload-geometry.flag` |
| 10 | dynamic streams in DEFAULT, in-place update | R4b | none (image identical) | same |
| 11 | retire storage, streams and scene resources by token, one site per commit | R3 consumers | stalls disappear | |
| 12 | **M3 run** | B | | |
| 13 | hashed PSO key, prewarm at admission, creation off the cache lock | R9 | no first-use hitch | |
| 14 | frame-linear global heaps, used registers only, stable samplers | R6 | none (image identical) | `legacy-tables.flag` |
| 15 | root signature shapes | R7 | none | |
| 16 | persistent draws with version checks | R8a | none (image identical) | `rebuild-draws.flag` |
| 17 | **M4 run** | C | | |

Why this order: tokens first because they change nothing while frames are drained, so every later slice has a place to return its memory; the arena before ingress and geometry because both need staging space; ingress before geometry because the static copy needs a copy list; PSOs before the root signature work because the signature identity is part of the key; persistent draws last because they hold descriptors, constants ranges, PSOs and geometry, which every earlier slice must have made stable.

## 5. Live witnesses and checks

Non-regression, on every commit of B and C (each run once with the debug layer on, 600 frames, **0 errors**, then once off for timing):

| Witness | What it exercises | Pass condition |
|---|---|---|
| Native1254 in-game (High Wall, `Pilot-NativeWitness.ps1`) | world draws, lighting, forward pass, UI overlay | `content_draws` equal; the idle frame's pixel hash equal to the previous commit's within the documented dynamic content (compare `changed_pixels` and the saved `native-ingame.jpg`); no new `native_selected_constant_unavailable` or `missing_textures` rows |
| Title and menus (`Pilot-TitleContinue.ps1`, menus pilots) | UI admission, primitive pass, R2b, R6 | same `primitive_draw_count`; pacing UI admission phases not worse |
| Status screen and portraits (`Pilot-StatusScreen.ps1`, `Pilot-StatusCycle.ps1`) | menu offscreen views, per-view submissions, FaceGen composition (R5 residency) | the views drawn as before; no persistent gap in the portrait |
| Character creation (`Pilot-CharMakeWitness.ps1`) | FaceGen streams, composed face, offscreen targets | face drawn, physical stream updated (R4b) |
| NPC and cloth witnesses (Native1184-1185, an NPC area) | cloth streams (`NativeClothVertexUpload`), R4b | the cloth moves; no stale vertices (compare the stream fingerprint rows) |
| Velocity Native1191 | readbacks of R15 | the velocity rows and pictures still produced with the witness flag |
| Bink movie (Native1250-1252) | movie upload (R2d) | movie frames unchanged |
| Area change or bonfire warp, then back | model storage, texture, stream retirement under load (R3, R4, R5) | no fault, no leak (private bytes return to baseline within a few windows), no stall at retirement |
| Quit, Native1262 | shutdown order | exit 0, as the original |

Per reprise, the specific live rows:

| Reprise | Row | Read | Expected |
|---|---|---|---|
| R15 | RA1 | rows per second in `startup.jsonl` with and without the flag | one pacing row per window without it |
| R15 | RA2 | `native_present_pacing` M1 against M2 | `present_ms` and `record_ms` not worse |
| R3 | RB1 | `drains` per frame in the retirement paths during an area change | 0 in storage, stream and scene retirement |
| R2 | RB2 | `committed_buffers_created` per frame in steady state | about 0 (arena reuse) |
| R2 | RB3 | arena high-water mark over a load and a fight | below the arena size; no growth in a steady window |
| R5 | RB4 | time from `Admit*` to `resident`; `missing_textures` per frame during a load | resident within a few frames; no persistent missing texture |
| R5 | RB5 | CPU descriptor heap occupancy | returns to baseline after retirement |
| R4 | RB6 | frame time of a world frame with the flag on and off | the DEFAULT path not slower; GPU time of the main list lower or equal |
| R4 | RB7 | cloth and physical stream generation, stream fingerprints | advance on every simulated change |
| R9 | RC1 | `pso_created` per frame in steady state; the first-use hitch (slowest frame) after a new area | 0 after warm-up |
| R6 | RC2 | `descriptors_copied` and `samplers_created` per frame, `heap_switches` | divided; heaps set once per list |
| R6 | RC3 | debug layer under `legacy-tables.flag` off | 0 errors with unwritten slots (only if the Tier allows) |
| R7 | RC4 | number of root signatures and their shapes | a small set; no PSO count explosion |
| R8a | RC5 | `persistent_draw_rebuilds` per frame in an idle frame | about 0; every version change rebuilds exactly one draw |
| R8a | RC6 | `prepare_ms` and `record_ms` M4 against M1 | divided (the review's target) |

## 6. Risks specific to this code

1. **The frame lock.** Any slice that makes ingress asynchronous but leaves `gpu_submissions` around the whole frame gains nothing and can deadlock if a loader holds its own lock while waiting on a ticket that only the frame can complete. Fix the order of locks in B3a (`gpu_submissions` is acquired first, the copy context's lock second, never the reverse) and write it in the code.
2. **Hidden drain assumptions** are not only the `Drain` calls: the per-frame in-place buffers of section 1 and the `native_model_inflight_` guard (`:1550`) rely on the frame having completed. Phase B must remove or convert the buffers (R2c) before phase D relaxes the guard.
3. **Residency timing** (R5) changes what the first frames of a load look like, and the existing refusal rows (`missing_textures`) will be non-zero for a few frames. The witness criterion is "no persistent gap", not "no row".
4. **Image identity.** Frames are not bit-reproducible across runs (animation, cloth, time-dependent effects). Use a still scene (idle player, no moving NPC in view) and compare `changed_pixels` and a saved capture at equal serials modulo 120, and accept differences only in documented dynamic content; any other difference is a bug of the slice.
5. **Thread ownership of `ModelDrawCaches`.** One recursive mutex guards pipelines, programs, pools and templates; R9 and R6 both change what is held under it. Keep a single owner per structure and add a test that two threads preparing draws concurrently produce the same PSO.
6. **Beyond-ER items stay behind measurements**: root CBVs (R7c), pipeline library (R9d) and any COPY queue (R16) are only started if section 3's numbers justify them.

## 7. Limits

Static reading of the tree at `bbe48c5c`; no build, no run, no measurement. Line numbers are that tree's and will move. The review's gains are expectations until phase A's table exists. ER evidence is the review's section 2 and is not extended here.

## 8. Phase A results (2026-10-11, Native1271-1276, menus pilot thread)

Code (commits `1feb726a`, `0d1ec78d`): R15 as a **measurement switch** (`measure.flag`, runner `-Measure`, pair runner `-Measure -NoDebugLayer`):
no periodic backbuffer or velocity readback, no velocity pictures, one frame row per pacing window plus rows where the frame
kind changes; explicit pixel or velocity files still read back. The default (no flag) keeps every witness, so the existing
pilots are unchanged (the inverse of section 2.1's `witness.flag`, same effect, safer default). Counters in the pacing row:
`queue_execute_drain_complete_per_frame`, `memory_private_local_mb`, `measure`; rows `native_adapter` (tier) and
`native_model_root_shapes` (R7 C3a). `Get-PacingMedians.ps1` prints the medians (first three in-game windows dropped).

**Runner defect found and fixed (1274):** `Run-PilotPair.ps1` passed its switches to the job as `[bool]$x`, which in an
argument list is the string `"[bool]False"`, true: every pair run since the switch existed was launched with `-MotionBlur`
(Thomas's own setting is HIGH, so no image changed), and 1271-1273 all ran with `-Measure -NoDebugLayer`. They are kept
as three repeats of the M2 setting (spread of the medians: fps 5.72-5.79; with 1276, 5.72-6.02).

S1, High Wall idle (Thomas's save, Argo, MotionBlur HIGH, 1280x720; medians of 3-5 steady windows of 120 frames):

| Run | Setting | fps | collect | prepare (validate / resources / draws) | render (record / GPU wait / present) | UI build | queue per frame (execute / drain / complete) | private / local MB |
|---|---|---|---|---|---|---|---|---|
| M0 1274 | debug layer, witnesses | 4.26 | 106.5 | 76.6 (1.9 / 13.4 / 58.1) | 37.2 (13.0 / 19.8 / 0.9) | 1.9 | 2 / 2 / 4 | 8541 / 1183 |
| M1 1275 | no debug layer, witnesses | 5.94 | 103.8 | 26.4 (1.9 / 12.4 / 9.3) | 23.7 (0.7 / 19.5 / 0.8) | 1.6 | 2 / 2 / 4 | 8457 / 1182 |
| M2 1276 | no debug layer, R15 | 6.02 | 103.6 | 25.9 (1.9 / 12.2 / 9.2) | 23.2 (0.6 / 19.4 / 0.3) | 1.6 | 2 / 2 / 4 | 8456 / 1181 |
| 1271-1273 | = M2 (runner defect) | 5.72-5.79 | 108-110 | 26.9 | 23.6 (0.6 / 19.4 / 0.4) | 1.7 | 2 / 2 / 4 | 8448-8463 / 1181 |

Reading:
- **The validation layer costs 1.7 fps** (M0 against M1), almost all of it in draw preparation (58 against 9 ms) and
  recording (13 against 0.7 ms): every later comparison is against M1, as planned.
- **R15 alone is within the run-to-run spread** (fps +0.08 against a 5 % spread); only `present_ms` drops measurably
  (0.76 to 0.34: the readback copy and the per-frame row). Verdict: **keep** (no cost, quieter traces), no further cut.
- **The frame is dominated by `collect` (104 ms of about 166)**, the native selection on the engine's scene update
  (`CollectNativeSelectedScene` + `CollectNativeOffscreenScenes`), which phases B and C do not touch; the render thread
  (prepare 26 + render 23, of which 19.5 is the wait for the GPU) is the part B to D address. The GPU wait is the real
  GPU time of the frame list (one queue, waited at once): R1 (phase D) can overlap it; R4 and R6 can shorten it.
  **Consequence for the plan:** after C, the next large gain is the collect step (phase F, R13 engine side, or a
  dedicated reading of `CollectNativeSelectedScene`), not phase D; to be measured with the same rows.
- **Queue:** 2 submissions, 2 `Drain` and 4 CPU waits per in-game frame (the frame's `Complete` + `Drain`, the UI
  admission's pair). Phase B's target is the waits outside the frame (none in S1's steady state: ingress and retirement
  only appear in S3).
- **Memory:** 8.45 GB private (to be explained in B: the CPU copies kept with every geometry and texture, R4/R5), 1.18 GB
  local video memory.
- **Adapter:** NVIDIA GeForce RTX 4070 Ti SUPER, `ResourceBindingTier` **3**, tiled tier 4, root signature 1.1: R6 may
  leave unused table slots unwritten (Tier 3 does not require every descriptor of a bound table to be valid).
- **Root shapes (R7):** **23 distinct** in S1; vertex stage at most 13 CBVs and no SRV or sampler; pixel stage at most
  10 CBVs, 63 SRVs, 11 samplers (against the fixed 14 / 128 / 16 per stage): one signature {VS 13 CBV; PS 10 CBV,
  64 SRV, 16 samplers} would cover S1, or a few shapes by SRV count.

Not done in phase A: S2 (title and Status) and S3 (load or warp) windows, two runs per setting, the GPU timestamp pair,
`committed_buffers_created`, `upload_bytes`, `descriptors_copied`, `pso_created` (they belong to the slices that change
those paths, B and C, and are counted there).

**Follow-up (part BE):** the dominant `collect` step has its own study and reprises, [COLLECT-NATIVE-REPRISES](../I-0068-static-yebis-ffx-lights-spec/COLLECT-NATIVE-REPRISES.md), placed as workstream K of the [master plan](../I-0068-static-yebis-ffx-lights-spec/D3D12-MASTER-PLAN.md).

## 9. Phase B results (2026-10-11, Native1277-1285, D3D12 pilot thread)

Commits (branch `claude/priceless-kowalevski-8bcbcb`, each Opus-reviewed before its merge): B1 `RetireAfter`/`CollectCompleted`
(menus thread), B2a/B2b chunks and UI targets by ticket (menus thread), **B2c** `5d4bb60e` per-submission `UploadArena`
for the lighting, point-light, env-region, SSS and velocity constants; **B3a** `546dd432` ingress copy context
(`DeviceQueues::RecordIngress`/`Commit`/`SubmitIngress`: loader threads record into pooled lists, the next submission
carries every committed list ahead of its own list in one `ExecuteCommandLists`, lists return by ticket); texture,
FaceGen and offscreen-target admissions no longer take the frame lock nor wait; **B3b** `fbf61a6c` pooled CPU descriptors
(`CpuDescriptor` lease) instead of one heap per texture; **B4** `2c725e9b` static FLVER storage in DEFAULT memory filled on
the ingress context (`legacy-upload-geometry.flag`, runners `-LegacyGeometry`, keeps the former path for an A/B run);
**B6** `2c725e9b`, `964d1d20`, `cdbdb154`: storage and model-stream retirement by ticket, UI admission/withdrawal without
Drain or wait (old build retired by ticket, new uploads on the ingress context), cloth and physical stream uploads off
the frame lock. Review fixes `c2f00d09`, `cac80975`.

Witnesses, debug layer, same protocol as Native1278 (title, Load Game, High Wall, Esc, F, walk; 1283 adds the Equipment
page): Native1279 (B2c), 1280 (B3a), 1281 (B4), 1282 (UI), 1283 (B3b): exit 0, 0 debug errors, the same refusal rows and
draw counts as Native1278, title and Load Game identical to the pixel, in-game and menu captures equal within JPEG noise
(mean difference about 1 of 255 on the game area, the same as between two runs of one build). Equipment page equal to
Native1270.

S1 medians (High Wall idle, Thomas's save, MotionBlur HIGH, 1280x720, no debug layer, measure switch):

| Run | fps | collect | prepare | render (record / GPU wait / present) | queue per frame (execute / drain / complete) | private / local MB |
|---|---|---|---|---|---|---|
| M1 1275 | 5.94 | 103.8 | 26.4 | 23.7 (0.7 / 19.5 / 0.8) | 2 / 2 / 4 | 8457 / 1182 |
| M2 1276 | 6.02 | 103.6 | 25.9 | 23.2 (0.6 / 19.4 / 0.3) | 2 / 2 / 4 | 8456 / 1181 |
| **M3 1285** | **6.57** | 106.1 | 26.5 | **5.4 (0.7 / 1.4 / 0.8)** | **2 / 1 / 2** | 8709 / 1799 |

Reading:
- **The GPU time of the frame fell from 19.4 to 1.4 ms** (RB6): it was the model geometry read from UPLOAD memory over the
  bus. Render 23.2 to 5.4 ms; fps +9 % only, because collect (106 ms) is now about 75 % of the frame (workstream K).
- **Queue:** in game, only the frame's own `Complete` and `Drain` remain (phase D). During the load window (Native1279 vs
  1280) the CPU waits went from about 20 to 2 per frame and that window from 9.6 to 15.6 fps (RB1, RB4: texture
  admissions no longer drain).
- **Memory:** local video memory +618 MB (the geometry now in VRAM, as expected); process private bytes +250 MB, not
  explained yet (staging buffers are released by ticket; to be read with the ingress list count and a heap snapshot).
- **Upload arena:** one 1 MB chunk for the whole run, about 270 KB per world frame (RB2/RB3 for the constants).
- **CPU descriptors:** about 3,430 slots in game, back to 414 after unload (RB5).

Phase B exit check ("no `Drain` in the ingress, retirement and stream paths"): **met for the game path**. Ingress
(textures, FaceGen, offscreen targets, model storage, UI uploads), retirement (storage, streams, UI builds) and streams
(cloth, physical) neither drain nor wait. Remaining `Drain` calls: the frame's own (phase D), the menu offscreen view and
Bink movie submissions (frame-like, phase D), shutdown and resize, and the depth-target recreation on a stencil-clear
change (`scene_pass.cpp` 1362, 1566, 1880): it rewrites shader-visible descriptors in place (env, lighting and velocity
views) and stays until the frame-linear heaps of R6 (phase C). Not in the game module: `gpu_texture_packages` and the
`native_service` scene-work path.

Deferred, with reason: **B5** (dynamic streams in DEFAULT, in-place update): with one version per resource the draws read
what they bound; an in-place update needs one buffer per frame slot (phase D) to keep that, so the per-version UPLOAD
resources stay; their creation is now off the frame lock. **B2d** (movie upload on the arena): the movie submission still
drains (phase D). Invariant recorded for B5 and R8a: retirement by ticket covers only submitted work; every consumer of a
retired resource must hold its own reference while recording (draws, ingress lists do).

## 10. Phase C results and closure (2026-10-11, Native1286-1288, D3D12 pilot thread)

Commits: **C1-C2** `9d464a9e` (merged `b79e4b0b`, Opus-reviewed): R9 C1c, a new model pipeline is created without the draw-cache
lock (first insert wins, `pso_created` counted); R6, on Resource Binding Tier 3 every model draw takes one range of a global
shader-visible CBV/SRV heap (32 chunks of 8192 descriptors returned by ticket, slack for the full root extent), sized per stage
to its highest bound register and written only where bound (no null-template copy), and its sampler stage tables come from a
content cache in one global sampler heap (128 tables, created once per device); per-draw tables remain the fallback
(`legacy-tables.flag`, runners `-LegacyTables`). Measurement split `ec142719` (pacing field
`model_prepare_programs_validate_pipeline_tables_ms`).

Witness Native1286 (debug layer, protocol of 1283 with the Equipment page): exit 0, 0 errors in 4,617 frames, every model draw
on the global heaps (0 fallback), captures equal to Native1283. M4 Native1287 (S1, as M3): fps 6.73 (M3 6.57), render 5.0 ms,
GPU wait 1.2 ms, prepare 25.7 ms (validate 1.8 / resources 11.7 / draws 9.4), `pso_created` 56 for the run, no fallback.
Native1288 split of the 9.5 ms of model-draw preparation per frame: programs 1.2, validation 1.1, pipeline key and lookup
1.8, **constant writes and descriptors 5.0** (the constant blocks, pose palettes padded to 256 rows, copied into the upload
chunks); the 11.8 ms "resources" phase before it is the per-frame production of those blocks
(`PrepareReadShaderRegister` and its producers in `PrepareNativeModelFrame`).

Closure of the two remaining reprises (D-0164):
- **R7 (root signature sized on use) is not needed on this hardware**: with Tier 3, R6 already writes only the bound slots and
  leaves the rest of each table unread; shrinking the root signature changes neither the descriptor writes nor the GPU work.
  It returns if a Tier 1/2 adapter must be supported, together with the volatile-range flag noted by the C2 review (a root 1.1
  signature would default to static descriptors, which partially written tables violate).
- **R8a (persistent draws) is folded into workstream K.** The measured headroom of keeping a prepared draw across frames is the
  programmes, validation and pipeline part (about 4 ms); the 5 ms of constant writes and the 11.8 ms of constant production
  remain per frame because their inputs are engine memory read every frame (group, animation and pose blocks). The costly part
  is therefore the production itself: the layout analysis that `PrepareReadShaderRegister` redoes for every draw (the original
  never redoes it, COLLECT-ORIGINAL-STATIC) and one pose palette written per mesh instead of per model. COLLECT-K-PREP and the
  K implementation sheet already plan those pure-function caches (P1-P6) with the digest witness, which is the safer route than
  versioned persistent draws (the review's staleness risk) before frames overlap (phase D).
- **R9 prewarm and hashed key (C1a/C1b)** stay open as low value: 56 pipelines per run, all in the first frames.

Phase A-C summary (S1, no debug layer): fps 5.94 (M1) to 6.73 (M4), GPU time of the frame 19.5 to 1.2 ms, render 23.7 to
5.0 ms, CPU waits per frame 4 to 2 (the frame's own, phase D); collect (104-106 ms) is now about 70 % of the frame.
