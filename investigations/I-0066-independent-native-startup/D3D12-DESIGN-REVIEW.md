# D3D12 design review of the merged native milestones (2026-10-11)

Static review requested by Thomas on 2026-10-11 ("revoir tous les jalons précédents
avec ceci en tête"), relayed by the coordinator with two governing points: D-0157
(changing the engine architecture is authorized, to use the control D3D12 gives
instead of copying D3D11's opacity) and "take inspiration from Elden Ring".
Scope: every native milestone merged into `main`, from the startup sequence to
the menus workstream, plus a compatibility check of the menus work in progress
(branch `claude/ui-filters`, F1 not merged). No native code was changed and no
live run was made. Method: reading the production native module (the sources
`frame/Build-GameBootstrap.ps1` compiles), the merged milestone records, and the
read-only ER evidence (`C:/Users/User/Desktop/ER Decompilation`, study716 and
its deepening897). ER addresses below are provenance of ER observations only:
nothing here is a DS3 offset, layout or proof (D-0060).

Status of the statements: **observed** = read in the DS3 native source or in a
DS3 run record; **ER static** = read in ER exports (static, not live); the
"gain" column is an expected effect, not a measurement, unless a run is cited.

## 1. Verdict

The merged work is correct in its data and its ownership (genuine engine
producers, original DXBC programmes, family rules, explicit fences, no D3D11),
but its **execution model still copies D3D11's immediate-context habits**:

1. **The CPU waits for the GPU several times per frame.** One allocator and one
   list; every submission is followed by `Complete` and every `Present` by
   `Drain` (`renderer/scene_pass.cpp:1644`, `:1664-1665`). Before the main frame,
   the Bink frame (`:1532`), each menu offscreen view (`:1419`) and the UI
   admission (`:1827-1830`, a `Drain` then an upload submission with `Complete`)
   each add their own full wait. No frame is ever in flight.
2. **Every draw is rebuilt every frame.** `PrepareNativeModelDraw` runs per draw
   per frame (`frame/game_bootstrap.cpp:5291`): PSO lookup through a byte-string
   key, 2x142 resource descriptors copied from a null template plus 2x16
   samplers per draw, every constant block copied and given a new CBV, samplers
   recreated with `CreateSampler` per draw (`renderer/model_draw.cpp:277-299`).
   Recording then re-sets targets, viewport, root signature, descriptor heaps,
   four tables and PSO for each draw (`renderer/gbuffer_encoder.h:40-61`), and
   the shader-visible heaps change every 64 draws (`model_draw.cpp:122`).
3. **The root signature emulates D3D11 slots**: per stage a table of 14 CBVs +
   128 SRVs and a table of 16 samplers (`model_draw.cpp:14`, `:177-190`),
   filled from null templates whether the programme reads them or not.
4. **Memory is D3D11-like**: only committed resources; FLVER geometry stays in
   the UPLOAD heap for its whole life (`game_bootstrap.cpp:5556-5571`), so the GPU
   fetches vertices from system memory; dynamic streams (physical/FaceGen
   streams `:1708`, cloth `:6969`) get a new committed UPLOAD resource per
   version; every texture gets its own committed upload buffer, its own
   descriptor heap and a full `Drain` (`frame/game_texture_ingress.cpp:61-65`);
   retiring a model storage also drains the GPU (`game_bootstrap.cpp:5530`).
5. **Resource states are conventions, not data**: transitions are written by
   hand, one `ResourceBarrier` call per transition, with rules such as "left
   shader-readable between frames". A frame graph exists (`renderer/frame_plan.h`:
   passes, read/write declarations, RAW/WAR/WAW checks, lifetimes) but only the
   fixture path uses it (`scene_pass.cpp:1909-1912`); the in-game path does not.

Measured cost. Reference shape, steady state (STATE, Native1135, development
build): collect 41 + prepare 32 + render/UI 29 ms per frame, 7.3 fps. One more
detailed window (run i66-native-1254, `startup.jsonl`, `native_present_pacing`
frame 3481, debug layer on) is **not** steady state: it is the load-in
transition (slowest frame 2112.9 ms, 25 UI-only frames then 95 world frames,
627 textures made resident in the window). Its averages use two denominators
(`game_bootstrap.cpp:595-672`): collect/prepare/render per presented frame
(120), recording, main-list GPU wait and the prepare breakdown per world frame
(95). Per world frame: prepare about 71 ms, of which validation 1.9, resources
13.1 and draw-object preparation 53.4 ms; recording 12.6 ms for 1062 draws;
main-list GPU wait 19.6 ms. Both sources show the same shape: the CPU dominates
and the GPU never overlaps it.

What ER shows in the cited static chains (section 2) differs on items 1 and 2:
a frame-lag wait instead of per-frame drains (E2), fence-gated reuse of upload
space (E4) and dirty-range descriptor preparation (E10). ER also keeps a
slot-binding model with dirty tracking, says nothing about root-signature layout
or PSO creation, and its initial-data path also waits per resource (E6); so
items 3 and 4 are D3D12 improvements this review proposes, not ER practice,
except ER's in-place update of retained buffers (E5) and its placed-resource arm
(E7). **No reprise blocks the menus workstream in progress** (section 5). The
proposed plan (section 6) places the reprises after the menus workstream, as the
coordinator's brief sets by default, but **before the large pass families**
(effects E01-E07, YEBIS HAL, shadows, SSAO), because each of those adds passes,
targets and hand-written barriers that the reprises would otherwise have to
rewrite.

## 2. What ER shows (read-only witness)

| # | ER static observation | Source in the ER evidence | What it discriminates for DS3 |
|---|---|---|---|
| E1 | A render worker thread consumes the previous frame's ordered records while the producer fills the other slot; the frame's Present runs after its records on that worker; the next publication waits for the worker. | ER repo K-0013, K-0016, K-0017; [ER-DEEPENING (a)](ER-DEEPENING-897.md) | Inference (K-0016 itself infers no frame-slot semantics; concurrent exclusion unknown): CPU preparation of frame N+1 can overlap recording/submission of frame N. |
| E2 | Ring of 6 fence tokens, one signalled per frame on the queue; at frame end the CPU waits only for the token of frame `e80 - lag`, with lag = 2. | [ER-DEEPENING (b)](ER-DEEPENING-897.md) | Two frames in flight; the CPU waits for an old frame, never for the current one. |
| E3 | Withdrawn resources go into delayed buckets released after the lag-bounded wait (safe for delay >= 1 with lag 2). | [ER-DEEPENING (b)](ER-DEEPENING-897.md), study716 section 5 | Retirement never drains the GPU. |
| E4 | Per-context upload allocators (two selectable, context +0x20/+0x28) over persistently mapped UPLOAD buffers; each block is tagged with queue class and fence value and recycled only when `GetCompletedValue` passes it. | study716 section 3 (`141EAC5C0`, `141EAC840/850`, `141EAB300`) | Uploads without CPU waits. Using one allocator for constants and data is this review's interpretation. |
| E5 | Updates of the CSFg owner's buffers copy from the upload space into the owner's retained buffer (`CopyBufferRegion`), bracketed by state transitions that restore the previous state (a special-mode branch skips them). Update frequency and heap type are not stated; DEFAULT is an inference (copy destination, default heap type in `141E928E0`). | study716 sections 2-3 (`1419E4ED0`, `141E8D570`) | Dynamic geometry can live in retained memory updated in place by a recorded copy. |
| E6 | Initial resource data at creation: a temporary UPLOAD buffer and a temporary COPY-type (3) allocator/list per call, executed on its own queue, signalled on a new fence and waited for by the CPU before release. | ER repo K-0008, K-0009 (`141E928E0`); `evidence/I-0002/EXECUTE-COMMAND-LISTS-PROVENANCE.md` | Load-time uploads go through a separate COPY list and queue, but ER waits per call; batching them is beyond ER. |
| E7 | Buffer creation is committed by default, with an optional placed arm (`CreatePlacedResource` in an engine heap at an offset) when a placement is supplied. | DS3 evidence store `evidence/I-0066/artifacts/er-deep-716/allocation/ER-271-F-01E928E0.c` lines 216-240 (device slots 0xD8/0xE8) | Engine-managed heaps exist in ER; how often they are used is not shown. |
| E8 | One selected source-finalization helper builds up to eight 0x20-byte operation records per group (kinds 0/1/2 mapped to native barrier types transition/aliasing/UAV; before/after states carried in the record, the wrapper supplying the native resource) and issues one `ResourceBarrier` per group. Not proven to be the gateway of all renderer work. | ER repo K-0020; ER `evidence/I-0008/RECORDING-OPERATION-AND-EXPERIMENT.md` and `evidence/I-0008/artifacts/recording-slice/ER-271-F-01E8EB10.c` | Barrier states as engine data and batched barriers exist in ER; an aliasing translation exists, its use is not shown. |
| E9 | A list-reset function sets two descriptor heaps (list slot 0xE0) and a compute root signature (slot 0xE8) once per list; its slots come from a constructor creating type-2 (COMPUTE) allocators, each reset after a fence check. No caller is established (ER K-0010 SUPPORTED, K-0011) and calling the slots a frame context is provisional. | ER repo K-0010, K-0011, `evidence/I-0002/ALLOCATOR-LIST-REUSE.md`, `evidence/I-0002/artifacts/execute-inventory/ER-271-F-01EA5BC0.c` | Heaps set once per (compute) list in ER; not a witness of graphics draws or frame slots. |
| E10 | Before each draw, only dirty descriptor ranges are prepared; a CBV is built from the transient upload pointer, otherwise the resource's existing CBV is copied. | study716 section 4 (`141ACD170`, `141E9BE20`) | Per-draw work proportional to what changed. |
| E11 | The CSFg vertex-stream patch owns its native buffers and selection binds the owner's buffer; the material/animation collector copies current parameter bytes into transient upload space at each preparation and records the associated resource in the stage tables. | study716 sections 2 and 4 | Draw inputs come from engine owners, not from an external scan; constants stay transient. |
| E12 | Present, then `Signal` on the shared queue fence, then `GetCurrentBackBufferIndex`. | ER repo K-0004 | Presentation is a fenced event in the same ownership domain. |

ER shows nothing, in the evidence available, about PSO precreation or a pipeline
library, root CBVs versus tables, bindless indexing, enhanced barriers, async
compute use per frame, or `ExecuteIndirect`. Where the plan uses them it says
"beyond ER" and asks for a measurement first.

## 3. Cross-cutting reprises

Each reprise names the ER observation it follows. Cost: S about 5 cycles or
fewer, M 5-15, L more than 15 (estimates).

| Id | Reprise | ER | Expected gain | Risk | Cost |
|---|---|---|---|---|---|
| R1 | **Frames in flight**: one allocator per frame slot, a fence token per frame, the CPU waits only for frame N-2 (lag 2); Present then Signal; no `Complete`/`Drain` in the frame path. | E2, E12; allocator per slot follows the E9 pattern (compute context only) | High: the main-list GPU wait (19.6 ms per world frame in the load-in window) overlaps the CPU instead of adding to it. | Medium-high: every "drained each frame" assumption becomes a lifetime bug if missed (pooled chunks, rings, readbacks). | M |
| R2 | **One upload arena with fence tokens** for constants, UI geometry, dynamic streams, Bink planes, light batches; replaces the per-draw constant chunks recycled after `Complete`, the per-texture upload buffers and per-version UPLOAD resources. | E4 | Medium; prerequisite of R1. | Low-medium. | M |
| R3 | **Token retirement**: withdrawn models, textures, streams and targets go into a retirement list keyed by the last frame token that could use them; no `Drain` on retirement. | E3 | Medium (removes stalls at loads and area changes); prerequisite of R1. | Medium (identity races with withdrawal; DIRECT-CONNECTION's version rules apply). | S-M |
| R4 | **Geometry in DEFAULT memory**: FLVER storage copied once into a default buffer; dynamic streams (physical/FaceGen, cloth) owner-retained default buffers updated by a recorded copy from the arena with transitions. | E5, E6, E11 | High on GPU bandwidth (vertex fetch from video memory, no allocation per simulated frame). | Low. | S-M |
| R5 | **Batched ingress**: textures and storages uploaded by one load-time copy list per batch (own allocator, own fence), CPU descriptors from a pooled CPU descriptor allocator instead of one heap object per texture. | E6 for the separate COPY list; batching beyond ER | Medium (loading time, no frame stall per texture). | Low. | M |
| R6 | **Global descriptor heaps**: one shader-visible CBV/SRV heap and one sampler heap, set once per list; per-frame linear allocation with fence retirement; per-draw tables written only for the registers the programme's RDEF uses (no null template fill, root signature 1.0 volatile semantics, to confirm under the debug layer); samplers from the existing sampler bank created once at stable indices. | E10; E9 shows heaps set once per list for compute lists only | High on CPU (part of the 53 ms per world frame of draw preparation and of the 12.6 ms recording in the load-in window). | Low-medium (validation layer must stay at 0 errors). | M |
| R7 | **Model root signature sized on use**: table ranges sized to the largest register any admitted programme reads, instead of 14/128/16; optionally hot per-draw blocks as root CBVs (beyond ER, measure first). | E9 (heaps); root CBVs beyond ER | Low-medium. | Low. | S |
| R8 | **Persistent draws**: one prepared draw per (instance, mesh, pass, programme variant) created at admission and kept while its owner and its inputs keep their versions; per frame only changed constants are written (pose palette once per model, not per mesh). R8b, later: the engine-side owners publish their changes (dirty) instead of a native scan reading engine memory every frame (the 41-99 ms collect). | E10, E11 | High on CPU (prepare 32 ms steady, about 71 ms per world frame at load-in; collect 41-99 ms). | Medium (staleness: a missed dirty signal draws old data; needs version checks). R8b high. | L (R8a M, R8b L) |
| R9 | **PSOs created ahead**: at material admission for each pass row the engine selects (57A20 rows are known at load), hashed key instead of a byte string, optional `ID3D12PipelineLibrary` disk cache. | beyond ER | Medium (no first-use hitch; key cost per draw). | Low. | S-M |
| R10 | **Engine-visible resource states and an in-game frame graph**: each native resource carries its current state; passes declare reads/writes (reuse `frame_plan.h`, which already computes dependencies and lifetimes); the graph emits batched barriers and first-use clears. Replaces the "left readable between frames" conventions. | E8 | Medium now, high for the coming families (effects, YEBIS, shadows, SSAO add many passes). | Medium (order changes must keep the original pass order, NATIVE-PASS-ORDER). | M-L |
| R11 | **Transient targets aliased**: G-buffer, light targets, SSS temporary, velocity, offscreen view sets, UI filter targets placed in heaps by lifetime (from R10), with aliasing barriers. | E7, E8 (placed arm and aliasing translation exist; their use is not shown) | Medium on memory (today each distinct offscreen size keeps a full target set until the device closes). | Medium (aliasing errors are silent; needs first-use discard/clear discipline). | M |
| R12 | **One frame list**: menu offscreen views, the Bink conversion and the UI uploads recorded as passes of the frame instead of separate submissions with waits. | E1 (ordered records), E2 | Medium (removes up to one wait per view per frame). | Low-medium. | S-M (after R1/R2) |
| R13 | **Render worker thread** as in ER: the engine thread publishes the frame's ordered records; a worker records, submits and presents; the next publication waits for the worker. | E1 | High when CPU-bound (overlaps collect/prepare with recording). | High (threading of engine reads). | L |
| R14 | **Presentation**: three back buffers and a frame-latency waitable object, tearing flag when allowed, back-buffer format and size from the display mode (I-0068 DISPLAY-SWAPCHAIN-NATIVE); today two buffers, `Present(0,0)`, fixed R8G8B8A8 (`renderer/backend.cpp:393-395`). | E12 | Medium (pacing, latency). | Low. | S |
| R15 | **Diagnostics out of the frame path**: the backbuffer readback and hash every 120 frames (`scene_pass.cpp:1633-1663`), velocity readbacks every 30 frames, per-frame info-queue scans behind a development switch. | none | Low but immediate; needed before measuring R1. | Low. | S |
| R16 | **Other queues**: a COPY queue for streaming (ER uses a temporary COPY list per creation, E6; asynchronous streaming beyond ER); async compute for SSAO and GPU particles (E07) once they exist (beyond ER for per-frame use). | E6; compute beyond ER | Low-medium, later. | Medium. | M |

## 4. Milestone by milestone

Each row: what is already right, what to take up again (with the reprise ids),
the expected gain, risk and cost of that milestone's share.

### M1 - Startup, device, presentation (I-0038 bootstrap reused, I-0066 startup: logos, title, cursor)

- **Already right**: one owned device and direct queue; system DXGI resolved by
  path; explicit fence tickets (`DeviceQueues::Execute/Complete`); fault
  quarantine that keeps unproven GPU work alive; sizes as inputs (D-0151); no
  D3D11 import.
- **Take up again**: `Execute` + `Complete` + `Present` + `Drain` per startup
  frame (`scene_pass.cpp:1692-1695`); two FLIP_DISCARD buffers, `Present(0,0)`,
  fixed format. -> R1, R14, R15.
- **Gain** medium (startup already runs at 60 fps; the gain is the shared
  model). **Risk** low. **Cost** S once R1 exists.

### M2 - Native UI primitive pass (startup to menus, I-0066 UI ingress, 1130-1132 pools)

- **Already right**: genuine programmes and constants; one cached root
  signature; pooled UI upload chunks; menu targets sampled directly
  (`PrimitiveTexture::external`, no copy).
- **Take up again**: every frame `AdmitPrimitiveWork` drains the GPU, creates
  two shader-visible heaps sized to the packet, and submits its uploads with a
  wait (`scene_pass.cpp:1827-1830`, `primitive_pass.cpp:99-102`); it then sets
  its own heaps in the frame list. -> R2, R6, R12.
- **Gain** medium (about 3 ms CPU per admission in the load-in window: produce
  0.28, drain 0.65, build 2.08, wait 0.04 ms; plus two GPU waits). **Risk** low. **Cost** S-M.

### M3 - Resource ingress: textures, FLVER storage, shaders (I-0066 STARTUP-RESOURCES, TEXTURE-FAMILIES, 1025-1045)

- **Already right**: genuine CPU loading and decoding (DCX/TPF/DDS); original
  SM5.0 DXBC used as is (no recompilation, the register layout is DS3 data);
  owner incarnation and version identity; explicit retirement contract.
- **Take up again**: per texture a committed default resource, a committed
  upload buffer, its own descriptor heap and a full `Drain`
  (`game_texture_ingress.cpp:61-65`, also FaceGen and offscreen targets
  `:124-150`, `:178-195`); FLVER storage kept in the UPLOAD heap
  (`game_bootstrap.cpp:5598` through the helper at `:5556`); retirement drains
  (`:5530`). -> R3, R4, R5.
- **Gain** high (geometry fetched from video memory; no frame stall per
  streamed texture). **Risk** low. **Cost** M.

### M4 - First world draw and model draw preparation (Native1045, 1056-1069, caches 1121-1135)

- **Already right**: selection follows the engine (`D079C0 -> 59040 ->
  57A20` rows); rules by family (D-0161); bindings validated against RDEF; one
  shared model root signature and PSO cache since 1121.
- **Take up again**: draw objects rebuilt every frame with full descriptor and
  sampler fills and new CBVs; full state rebind per draw; heap change every 64
  draws; D3D11 slot emulation. -> R6, R7, R8, R9.
- **Gain** high (prepare is 32 ms per frame in the Native1135 steady state; the
  load-in window spends about 53 ms per world frame in draw-object preparation
  and 12.6 ms recording 1062 draws). **Risk** medium. **Cost** M-L.

### M5 - Deferred lighting and point lights (Native1050, 1055)

- **Already right**: genuine programmes, engine CPU builders for cbLight/cbFog,
  tile batches as 1A1DA0, persistent constant blocks.
- **Take up again**: one root signature and heap per pass, heap switches between
  passes, single transitions, the point-light upload ring reused only because
  the frame drained. -> R2, R6, R10.
- **Gain** medium. **Risk** low. **Cost** S (absorbed by R6/R10).

### M6 - Characters: skinning, materials, FaceGen, forward 0x29, SSS, IBL (Native1061-1153)

- **Already right**: native producers by family (cbMatDyn, palettes,
  generated words); FaceGen composed natively into its own target; SSS stage
  and env-map regions natively; forward pass after lighting.
- **Take up again**: full pose palettes copied per draw (per mesh) each frame;
  physical/FaceGen streams, the DS3 counterpart of ER's CSFg patch, create a
  new committed UPLOAD resource per version (`game_bootstrap.cpp:1708`);
  FaceGen target created with a drain. -> R2, R4, R8.
- **Gain** medium-high (allocation churn, bandwidth). **Risk** medium. **Cost** M.

### M7 - Menu offscreen views: Status, portraits, character creation (Native1165-1182)

- **Already right**: the same pipeline as the main view at the view's own size
  (sizes as data); the UI samples the view target directly; genuine CPU scenes.
- **Take up again**: the renderer swaps its member targets for each view
  (`SwapTargetSet`, a bound-state pattern); one full target set per distinct
  size kept until the device closes; an RTV heap created per view per frame;
  each view submitted and drained on its own (`scene_pass.cpp:1354-1419`).
  -> R10, R11, R12.
- **Gain** medium (memory, waits). **Risk** low-medium. **Cost** M.

### M8 - NPCs and cloth (Native1184-1185)

- **Already right**: engine cloth set with CPU-only buffer objects; Havok copies
  uploaded natively per simulated frame; binding through instance -> CI -> set.
- **Take up again**: a new committed UPLOAD resource per simulated frame and
  stream (`game_bootstrap.cpp:6969`). ER's CSFg patch keeps owner buffers and
  records a copy (E5, E11). -> R2, R4.
- **Gain** medium. **Risk** low. **Cost** S-M.

### M9 - Velocity (Native1186-1192, D-0162)

- **Already right**: engine Velo rows, previous pose versions, native
  motion-vector resolve at render size (upscaler-ready).
- **Take up again**: witness readbacks every 30 frames in the frame path;
  targets kept in fixed states by convention. -> R10, R15.
- **Gain** low. **Risk** low. **Cost** S.

### M10 - NPC families C1-C5 (Native1193-1217)

- **Already right**: per-slice index width, mesh bound, model union, scene-colour
  copy at the forward pass (C4) as the original.
- **Take up again**: only the cross-cutting items (the C4 copy becomes a graph
  pass with its own state edges under R10). **Gain** low. **Cost** S.

### M11 - Phantoms (Native1218-1224) and foliage F1

- **Already right**: pure family and producer work (alternate set selection,
  rows 0x21/0x22/0x29, instance alpha, Grass words written as zero).
- **Take up again**: nothing specific to D3D12.

### M12 - Input latch, quality object, movie skip (Native1225-1256)

- No D3D12 mechanics. Note only: the latch became necessary because native
  frames last 125-200 ms; R1/R6/R8 reduce that pressure without removing the
  latch, which stays correct.

### M13 - Bink movie player (Native1250-1252)

- **Already right**: CPU decode kept; the game's own YCbCr programmes; R8 planes.
- **Take up again**: its own submission and drain per movie frame
  (`scene_pass.cpp:1532`); the upload buffer reused only because of the drain.
  -> R2, R12.
- **Gain** low-medium. **Risk** low. **Cost** S.

### M14 - Quit and shutdown (Native1258-1262)

- **Already right**: clean unload and exit 0 as the original; quarantine when
  completion is unproven.
- **Take up again**: shutdown relies on `Drain` plus destructors. With frames in
  flight it needs an explicit order: wait for the last token, run the
  retirement list, then release (ER: delayed buckets after the final wait). -> R1, R3.
- **Cost** S, inside R1/R3.

## 5. Menus workstream in progress (claude/ui-filters, F1 not merged)

**Not blocked by any reprise.** F1 adds packet targets drawn then sampled,
pooled by size (R8G8B8A8 + D32S8), transitions in the encoder, first-use
clear, every target back in PIXEL_SHADER_RESOURCE at the end, retired with its
build; it also creates RTV/DSV descriptor heaps at each build, that is once per
UI admission (per frame), which R6 absorbs. This follows the current conventions
and stays compatible with R10/R11 if F2-F7 keep three habits:

1. each intermediate target declared in the packet with identity and size (F1
   does it); its format, first-use clear and final state are today fixed
   encoder conventions, so they should stay uniform or become packet fields,
   so that the frame graph can absorb the declarations unchanged;
2. no new submission or `Drain` inside the UI path (filter passes stay in the
   frame list);
3. the target pool stays bounded (F1 keeps at most 32 free targets).

Default placement therefore stands: ROADMAP after the menus workstream.

## 6. Ordered plan

Proposed order (placement in ROADMAP belongs to the menus pilot): dependencies
first, then gain per risk. Phases A-C are proposed right after the menus
workstream and before the effects/YEBIS/shadows/SSAO workstreams.

| Phase | Content | Why here | Exit check |
|---|---|---|---|
| A - measure | R15, then a release-like measurement (debug layer off) of collect / prepare / record / GPU | Gains must be measured against a clean baseline. | Pacing rows with and without the debug layer on the same witness. |
| B - memory and uploads | R4, R2, R3, R5 | Prerequisites of frames in flight; ER E4-E6, E3. | 0 debug errors; no `Drain` in the ingress, retirement and stream paths (the per-frame `Complete` and `Drain` stay until R1; coordinator, 2026-10-11); witness image identical. |
| C - descriptors and draws | R6, R7, R9, R8a | Largest CPU cost; ER E10 (E9 for compute lists only). | Prepare and record times divided; identical images on the Native1254 and Native1191 witnesses. |
| D - frames in flight | R1, R12, R14, shutdown order (M14) | Needs B and C (everything the GPU reads is fenced). | Two frames in flight, GPU wait overlapped; clean quit as Native1262. |
| E - states and memory | R10, R11 | Before the new pass families add hand-written barriers. | Frame graph drives the in-game frame; aliasing with 0 debug errors. |
| F - engine side | R13, R8b | Largest change to engine contracts (D-0157); after the simpler gains. | Engine owners publish changes; worker presents; parity witnesses. |
| G - optional | R16, root CBVs (R7), pipeline library (R9), other beyond-ER options | Only if A's measurements justify them. | Measured gain. |

Preparation of phases A to C: [D3D12-PHASES-A-C-PREP](D3D12-PHASES-A-C-PREP.md); phases D to G: [D3D12-PHASES-D-G-PREP](D3D12-PHASES-D-G-PREP.md) (files and functions per reprise, dependencies, commit order, measurements, live witnesses). It refines the exit check of phase B (no `Drain` in the ingress, retirement and stream paths; the per-frame waits stay until R1).

**Consistency with the family designs already written under D-0157.** I-0068
part AU ([SHADOWS-NATIVE](../I-0068-static-yebis-ffx-lights-spec/SHADOWS-NATIVE.md)
section 12) and part AV
([EFFECTS-D3D12-DESIGN-NATIVE](../I-0068-static-yebis-ffx-lights-spec/EFFECTS-D3D12-DESIGN-NATIVE.md)
section 2) already assume this infrastructure: a per-frame arena retired by
fence with one arena per frame in flight (R1, R2), `FramePlan` passes driving
the encoding (R10), prewarmed PSOs (R9), root CBVs into the arena for their own
root signatures (the R7 option), a placed shadow atlas (R11) and a
record/encode split (R13). Phases B-E give those families one shared
implementation instead of one per family; their own root signatures stay
per family where their register sets differ from the model one.

Each phase keeps the existing witnesses (Native1191 velocity, Native1254
in-game, Native1262 quit, Status/portraits views) as non-regression, and the
generality rule (D-0161) is unaffected: the reprises change execution, not the
family rules.

## 7. Limits

- Static review: no reprise was implemented, measured or run live.
- The cited timings come from development runs with the validation layer on;
  they show proportions, not release performance. The i66-native-1254 window is
  a load-in transition, not steady state.
- ER observations are static and bounded to the cited chains (study716,
  deepening897, ER repository K-0004/K-0008-K-0011/K-0013-K-0020); the
  non-threaded ER path and per-frame use of non-direct queues are not covered.
- The D3D12 options marked "beyond ER" have no ER witness in the evidence and
  need a measurement before adoption.

## 8. Pre-merge review

A fresh Opus 5.5 agent at high effort reviewed the branch diff on 2026-10-11.
It confirmed every DS3 `file:line` citation and behavioural claim and the D3D12
slot readings. Its corrections were applied: the ER comparison bounded to what
the cited chains show (section 1 last paragraph); E6 (one temporary COPY list
and a CPU wait per creation, not batching); E9 (compute-type list context, no
established caller); E8 (one selected helper, states carried in the record,
aliasing use not shown); the measured window qualified as a load-in transition
with its two denominators; minor wording in E1, E4, E5, E11, R1, sections 5-7.

One ordered plan that places phases A to G and the seven family designs together (waves, shared post root signature, arena, FramePlan, common witnesses, risks): [D3D12-MASTER-PLAN](../I-0068-static-yebis-ffx-lights-spec/D3D12-MASTER-PLAN.md).

The native `collect` step (104 ms of the 166 ms frame at phase A) is studied in [COLLECT-NATIVE-REPRISES](../I-0068-static-yebis-ffx-lights-spec/COLLECT-NATIVE-REPRISES.md) (causes and reprises C0-C10, workstream K of the master plan); the original side is a separate note of the menus static thread.

Preparation of the collect workstream K (C0-C7): [COLLECT-K-PREP](COLLECT-K-PREP.md).

Preparation of wave 1 of the master plan (effects door/lights/wind, glow 1-3, shadow producers, tessellation parse, SSAO parameters, YEBIS skeleton with tone map and gamma, Scaleform F8a/F9): [WAVE1-PREP](WAVE1-PREP.md).

Ready-to-implement envelope of the model draws (target sets, hull/domain, depth-only, GX04 flat fallback): [MODEL-DRAW-ENVELOPE-READY](MODEL-DRAW-ENVELOPE-READY.md).

Ready-to-implement native colour path and gamma pass (independent of YEBIS): [GAMMA-COLOUR-PATH-READY](GAMMA-COLOUR-PATH-READY.md).

Ready-to-implement glow of wave 1 (forward second target, inputs, resources, gate, the ten passes): [GLOW-WAVE1-READY](GLOW-WAVE1-READY.md).

Ready-to-implement SSAO parameters of wave 1 (layer record, view record, quality gate, programmes, resources): [SSAO-WAVE1-READY](SSAO-WAVE1-READY.md).

Ready-to-implement YEBIS and tone map of wave 1 (bank blocks, converters, recorder host, validation ladder): [YEBIS-WAVE1-READY](YEBIS-WAVE1-READY.md). Ready-to-implement effects of wave 1 by family (system, door, lights, force fields and wind, light-shaft and grading feeds): [FFX-WAVE1-READY](FFX-WAVE1-READY.md). Ready-to-implement shadows of wave 1 (cascade state and fit, atlas, constants, local-light shadow items, filter plan): [SHADOWS-WAVE1-READY](SHADOWS-WAVE1-READY.md). Ready-to-implement water, frame copy position, reflection view and tessellation of wave 1: [WATER-TESS-WAVE1-READY](WATER-TESS-WAVE1-READY.md). The remaining YEBIS passes (depth of field, motion blur, feedback blur, auto exposure, history) and the full YB-4b: [YEBIS-SECOND-TRANCHE-READY](YEBIS-SECOND-TRANCHE-READY.md). The temporal upscaler (extent split, jitter, vectors, reactive mask, history reset): [UPSCALER-DLSS-READY](UPSCALER-DLSS-READY.md).
