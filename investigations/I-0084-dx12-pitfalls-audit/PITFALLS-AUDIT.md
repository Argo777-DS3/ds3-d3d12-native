# I-0084: the D3D12 pitfalls audit, lead by lead

Base: `main` 68b392d4 (native tree 862785aa). ER's treatment and defect come from [I-0080 ER-STUDY](../I-0080-er-dx12-pitfalls/ER-STUDY.md)
(ER 2.7.1 decompiled, read-only); "ours" was re-read in the code for this audit (a static inventory of every pipeline,
barrier, wait, descriptor heap, resource and device call of `native/`, checked by Opus where a verdict relies on it). Live runs
are under `evidence/I-0066/artifacts/<run>/` (D-0116, announced in the thread): the replayed Firelink path (I-0083) on main's
build, `a84-base-gbv` (debug layer with GPU-based validation), `a84-base-cold` (no pipeline library or list),
`a84-base-sample` (the sampler), then the fixes' runs below. Verdicts: **solved** (with its evidence), **fixed here**,
**measured, kept** (why), **deferred** (to a named later workstream, why).

## Summary

| Lead | Pitfall | Verdict | Evidence |
|---|---|---|---|
| A1 | Pipelines compiled in play | measured, kept for now: precompile screen (ROADMAP) | a84-base-cold: 156 pipelines compiled on the frame path with no library, all at the first world frame (one 181 ms frame), none during the path; warm runs: 0 |
| A2 | Our own pipelines outside the cache | **fixed here** | the 21 direct creation sites of the game path (about 40 pipelines) and their 16 root signatures now go through `PipelineStore`, which had 6 callers |
| A2b | A pass compiling its HLSL again per view | **fixed here** | the composition pipeline was rebuilt (D3DCompile twice) for every view context and resize |
| B1 | Missing barriers | solved at Firelink | a84-base-gbv: GPU-based validation, the whole path, 0 error, 0 warning |
| B2 | Superfluous barriers | measured, kept | 131 graph barriers and 8 raw a frame at Firelink; no per-dispatch UAV barrier of ER's kind outside the effect stage |
| C1 | Full drains in play | solved | drains by site 0 in play at Firelink (i83-final-m1/m2/v1, a84-base-*) and over 19 zone travels (i81-19-zones-d) |
| C2 | Hang and device removed | solved by I-0082 | K-0464, D-0201 |
| C3 | Busy waits | ours solved; the game's own limiter spins | owner thread: 31 % of samples in `RtlQueryPerformanceCounter` inside DS3's limiter 0x140EEDBE9..C65 (as in the original, I-0078); our waits are events and timers |
| D1 | VRAM budget ignored | **fixed here** | budget read each window, a reaction above it, the targets raised in residency priority; runs below |
| D2 | Leaks | counters in place for the night test | private, local and non-local memory with budgets, streamed heaps, staging, descriptors, retirements per window |
| D3 | Transients not aliased | deferred (D3D12-DESIGN-REVIEW R11) | targets 123 MB main view + 18 MB menu views at Firelink |
| E1 | Descriptor heap switches | measured, kept until bindless | 20 `SetDescriptorHeaps` sites on the game path (stage-owned heaps) |
| E2 | D3D11 root signature, per-descriptor copies | deferred to bindless (ROADMAP) | kept draws reuse their tables, so copies happen at prepare only |
| F1 | Uploads on the frame's queue | deferred (copy queue, ROADMAP loading and threading) | ingress lists ride ahead of the frame list on DIRECT |
| G1 | Debug layer required | solved by I-0082 | D-0201 |
| G2 | Adapter and features | solved | high-performance adapter, FL 11_0, binding tier read; no vendor library |
| G3 | Vendor independence | **fixed here** for WARP; AMD and Intel testers before a public release | a84-fix3-warp: menus, load and the lit world on Microsoft's software adapter, no fault (about 11 s a frame) |

## A. Pipelines

**A1, first encounter.** ER compiles a missed pipeline on the recording thread under its cache mutex and ships a precompile
list (PUBLIC: Digital Foundry's stutters on first sight). Ours: `PipelineStore` (backend.cpp `Acquire`) never holds the cache
lock while the driver works and precompiles this PC's list at boot on four workers (I-0072). Measured (`a84-base-cold`,
`no-pipeline-library.flag`, i.e. a first start: neither library nor list): 156 pipelines created on the frame path, 60 of them
by model draws in the first three world frames (presented 1434-1438, the first 181.6 ms), none during the replayed motion; the
path's counts equal the warm runs' (frames over 34 ms 13 against 12-13). Uncertainty: NVIDIA's own driver shader cache stayed
warm (not cleared: it is Thomas's system state), so a truly new PC compiles slower. Verdict: kept; the remedy is the precompile
screen of the ROADMAP (compiling the zone's descriptions before its first frame), not a draw that waits a frame (D-0197).

**A2, our own pipelines.** The inventory found 6 pipeline callers of the store on the game path and 21 direct creation sites; the
stages (AA, AO 7, display, glow 4, grading 2, post draws, sun shadow 3, tone map 2), the native lighting (4 + shadowed sun +
mask composite + SSS 2 + environment 4), velocity 2, the composition passes, the movie and the FaceGen compositor created
theirs directly at their first frame on every start, outside the library and the precompile list. Fixed: those 21 sites
and their 16 root signatures go through `Pipelines().Graphics/Compute/Root` (the store falls back to a direct creation when a
description cannot be encoded, as before). Checked: a new build folder stores 212 pipelines at its first start
(`a84-fix1-m1`, against 156 before the routing) and its second start loads 212 and compiles 2 (`a84-fix1-m2`). The runtime HLSL compiles (`D3DCompile`, 5 small passes) stay: they run once per
process at a stage's creation, never on the frame path, and `d3dcompiler_47.dll` is part of Windows.

**A2b.** `MaterialColorCompositionPipeline` ran at every `NativeModelTargets` (the main view, each menu-view extent, each
resize): two `D3DCompile` and a new pipeline each time, replacing the one frames in flight held. Fixed: built once per device.

## B. Barriers

ER emulates D3D11 hazard tracking (state derived at each bind, enter and restore around every copy, clear and update, a UAV
barrier before every dispatch). Ours: the frame graph derives per-subresource states and batches them per pass (D-0171);
outside it, the hand-written `Transition` helpers skip a resource the graph owns, and stage-local barriers remain in grading
(LUT UAV round trip), glare, tone map, display and the ingress lists.

**B1.** `a84-base-gbv`: the debug layer with GPU-based validation (`native_debug_layer` gpu_validation true) on the whole
replayed path: complete, 0 fault, and the renderer treats any error or warning as a fault (`SceneRenderer::Diagnostics`), so 0
error and 0 warning at Firelink (walk, sprint, turn). The witness zones under GPU-based validation are the night test's.

**B2.** 131 graph barriers and 8 raw a frame (a84-base-*, i83-final-*): the graph's after-pass batches return transients to
their home state (ER's round trip in kind, but batched). No GPU time attributable to barriers is measurable with the current
tools; kept, to revisit with the transient aliasing (D3).

## C. Synchronisation

**C1.** `drains_by_site` sums to 0 over the play windows of every run on the path and over the 19-zone loop of I-0081
(`i81-19-zones-d`): the remaining full drains (stencil-clear change, movie planes, lighting programme change) do not fire in
play; resize, display mode and close drain by design. H3 refuted.

**C3.** The sampler (`a84-base-sample`, owner thread 30,000 samples): 31.05 % of the engine thread in
`RtlQueryPerformanceCounter`, inside DS3's own limiter (0x140EEDBE9..0x140EEDC65, the function at 0x140EEDC60 under the draw
step 0x1423C1DA4), the same spin the original shows (I-0078: 36 %). ER spins the same way (140E84830, I-0080). Ours never spins
except the pacer's last millisecond on a refresh that is not a multiple of 60 Hz (`frame_pacer.cpp:22`). The game's limiter is
the engine thread's third tranche's (it caps the rate the 120 frames/s work lifts, timing object +0x354/+0x358): recorded there.

## D. Video memory

**D1, the budget.** ER reads it once at start and never reacts. Fixed here:
- `NativeMemoryMb` reads the local and non-local usage and budget each pacing window (`local_budget_mb`,
  `nonlocal_usage_budget_mb` in `native_present_pacing`);
- above the local budget, `TrimStreamedMemory(true)` releases every free placed heap and the idle staging chunks (the memory
  the backend holds on its own account), one `native_video_memory_over_budget` row per window while it lasts; what the game
  keeps is never withdrawn (D-0197);
- the frame's targets get `D3D12_RESIDENCY_PRIORITY_HIGH` with each new generation (`NoteTargetBytes`), so a demotion takes
  streamed textures first;
- the check: `video-memory-budget-mb.txt` (announced runs) allocates a ballast at maximum priority that leaves the game that
  much of the budget, standing for a smaller card.
Peak local use at 1280x720: 2.2-2.5 GB at Firelink, 1.8 GB over 19 zones (well under a 4 GB card). Runs: 4 GB left
(`a84-fix1-vram4g`) no window over the budget and the path's counts unchanged (frames over 25/34 ms 75/12, held 40); 1.5 GB left,
less than the game needs (`a84-fix2-vram1500`, after the step review): 67 windows over, no fault, frames over 25 ms 105 and held 62
against about 80 and 45 (textures demoted to system memory), 16 heaps released.

**D2, leaks.** Counters per window: process private bytes, local and non-local usage and budget, streamed heaps, placed and
committed creations, staging chunks, CPU descriptors in use, retirements pending, pipelines. The zone-travel loop's return to
its starting level is the night endurance test's measurement. The process-exit heap fault 0xC0000374 (kept dump
`i82-w1-dred`, the faulting thread already gone, no stack): pre-existing in the original too (WAVE1-CLOSURE), not a native
leak; left to the night test with `exit-dump.flag`.

**D3.** Each view extent keeps its own target set (123 MB main, 18 MB menu views at Firelink); the graph's placement planner
exists (R11). Deferred: worth it only with the 4K and two-monitor coverage (ROADMAP).

## E. Descriptors

**E1.** The model draws use one global shader-visible pair; 20 stage-owned heaps are set on the game path (AA, AO, display,
effects, glow, grading, post draws, sun shadow, tone map, lighting views, velocity, composition, movie, UI pool). ER keeps one
pair a list. Several stages renew a small heap when their inputs change (AO, sun shadow, tone map, grading, mask views). Kept
until bindless binding (ROADMAP), which replaces the stage tables by ranges of the global heap.

**E2.** The D3D11 slot model (14 CBV, 128 SRV, 16 samplers a stage) is ER's too; ER rebuilds a dirty table with one copy per
descriptor for every draw. Ours copies per descriptor only when a draw is prepared (kept draws reuse their tables, I-0070,
I-0074). Deferred to bindless.

## F. Uploads

**F1.** Ingress lists (textures, static buffers) are put ahead of the next frame's list on the DIRECT queue
(`SubmitWithIngress`); ER waits on the CPU per created resource on its COPY queue. A COPY queue with a GPU-side wait is the
ROADMAP's loading and threading item; deferred.

## G. Driver and environment

**G1, C2**: I-0082. **G2**: the first hardware adapter by high-performance preference that opens feature level 11_0
(backend.cpp `Open`), the binding, tiled and heap tiers and root signature 1.1 read and logged (`native_adapter`); formats are
queried where used; a failure ends in I-0082's fault exit with the player's message. **G3**: no vendor library is loaded (ER
links AMD's AGS statically); resources are unnamed except the queue and lists (I-0082, for DRED). `warp.flag` opens the device
on WARP (Microsoft's software adapter), the only other implementation on this PC, with a library of its own and longer
deadlines (completion 120 s, render worker 600 s: its first try, `a84-fix2-warp`, drew the first world frame and then passed the
5 s hang deadline). `a84-fix3-warp`: the menus, the load and the lit world frame after frame, no fault.
