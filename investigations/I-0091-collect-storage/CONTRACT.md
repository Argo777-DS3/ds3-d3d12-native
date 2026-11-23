# I-0091 - Stockage de la collecte

Capability: C3 (Wave 2, performance: the engine thread, bottleneck 2 of [I-0088 BOTTLENECKS](../I-0088-engine-thread-120-c/BOTTLENECKS.md))

## Question

After I-0090 the frame is bound by the engine thread (m45 unlocked 125 frames/s: DS3's work 3.8 ms, the collect 4.1 ms,
profile `i90-07-prof-s12`). In that profile the engine thread spends 5.45 % of its samples (about 0.43 ms a frame) in the
value-initialisation of the main scene's model places (`vector<NativeSelectedModel>::resize`, 1,686 models of 1,320 bytes,
`09_model_admission.inc:1118`, memset 4.44 %) and 3.96 % (0.31 ms) in that of the pass rows' places
(`vector<PreparedMesh>::resize`, about 2,975 rows of 576 bytes, `10_options_and_passes.inc:440`), then assigns every place
again from the workers' slots and chunks; the general heap is 5.8 % of the thread (the shadow walk's per-frame hash map, the
constant packing). Each collect allocates its two large arrays fresh (about 4 MB a frame) and the release of the previous
scene frees them: INFERRED from the memset's cost (2.2 MB of warm memory would cost a few hundredths of a millisecond) that
most of it is the first touch of new pages. Which changes, made after Elden Ring's per-frame memory, take this storage cost
off the engine thread, and how much of the collect and of the frame do they give back, measured unlocked, standing and in
motion?

## Scope

Included, in this order, each measured before and after:

1. **The collect's frame storage** (ER-STUDY §5: `GXBS::GXLinearAllocator`, a frame arena kept and reset at each scene render,
   141B452B0; per-context chunked bump allocators without lock, 141AD1FC0 -> 141C4ACD0; no general heap call on the
   entity-draw path): the scene's per-frame arrays (the models, the pass rows) and the pass rows' per-chunk scratch storage take
   their memory from storage kept between frames (blocks returned by whichever thread releases the scene, reused by the next
   collect), never returned to the system each frame. A/B: `no-frame-store.flag`.
2. **The places constructed by the workers** (ER: the entity draw jobs write their records in their own context's memory; no
   array is zero-filled first): an admitted model is built in its candidate's slot and move-constructed once into its place,
   a pass row moved once from its worker's chunk into its place; no value-initialised array, no second assignment. A/B: the
   step-1 build against the step-2 build in one series.
3. The engine thread's remaining per-frame heap on the collect's path named by the profile after steps 1 and 2 (the shadow
   walk's per-frame map at least), removed where it is per frame and the gain measurable; then the engine thread re-profiled
   and the next bottleneck named for the re-ranking (items 3 to 5 of BOTTLENECKS).

Excluded: the pass rows and admission without per-frame rebuild (ER-STUDY P5, beyond ER, needs its invalidation proven), the
effects' update (bottleneck 3), the reflection view's reuse between frames (4), DS3's steps (5), the render worker.

ER's defects not copied: the frame arena's spin lock per chunk refill (ours takes a lock a few times a frame, per block, never
per record); ER rebuilds every entity's draw data each frame without a change test (ours keeps its records, I-0070, I-0074).

## Inputs

[I-0088 BOTTLENECKS](../I-0088-engine-thread-120-c/BOTTLENECKS.md), [I-0090 RESULT](../I-0090-render-worker-frame-memory/RESULT.md)
and its runs `i90-01..08` (`Run-I90Series.ps1`, `Get-I90Summary.ps1`), the profile `i90-07-prof-s12` (with `sample_symbolize`'s
callers of `RtlAllocateHeap`, `memset`, `RtlFreeHeap` on the engine thread); I-0086 ER-STUDY §5 and §8 (branch
`claude/er-engine-thread-study`, 243bf355; 141B452B0 re-read by Opus in the disassembly for this contract: a virtual reset of
the allocator at +8, the cursor words +0x1E0/+0x1E8 cleared); `native/frame/game_bootstrap/05_selected_scene.inc`,
`09_model_admission.inc` (`AdmitNativeDeferredCandidates`), `10_options_and_passes.inc` (`PrepareNativeSelectedPasses`),
`12_scene_collect.inc` (`CollectNativeSelectedScene`, `CollectNativeShadowCasters`).

## Hypotheses

- H1: the two arrays' cost is mostly the first touch of fresh pages: storage kept between frames takes the two resizes from
  0.74 ms to under 0.2 ms of the engine thread at m45.
- H2: constructing the places once (no value-initialisation, one move) takes the rest of the two resizes off the engine thread
  (under 0.05 ms).
- H3: the collect falls by 0.5 to 0.8 ms at m45 and the frame gains about 5 frames/s there (p50 and p99 down), the Firelink
  walk too.

## Expected evidence

The engine thread's stamps (DS3's work, the collect, the end of frame) and the frame and present p50/p99 (`Get-I90Summary.ps1`),
unlocked on the tearing chain (`original-unlock.flag`) at m45 and on the Firelink walk, measure mode without the debug layer; the
frame store's counters (blocks reused and fresh, bytes held); the sampler before and after (the two resizes, `memset`,
`RtlAllocateHeap` on the engine thread); the guards (`record-revalidation.flag`: the serial walk, the pass rows' parallel check,
the prepare shadow: 0 mismatch) and the debug layer (0 error); pictures equal to the build without the change; the replayed path
gate with its memory leg at the merge (D-0202, D-0204).

## Exit conditions

- Success: the two resizes and the collect's per-frame heap measurably down on the engine thread, the collect and the frame
  faster at m45 and on the walk (p50 and p99), no visual difference, guards 0, validation 0, the memory leg flat; Opus step and
  final reviews; the gate.
- Partial: a measured gain short of H3; the rest replanned in RESULT.
- Refuted: a change that measures no gain is not kept (Optimise, do not stack).
- Stagnation: no new discriminating evidence after 3 cycles.
- Budget exhausted: close the tranche and replan.

## Safety and human interaction

Live runs under D-0116/D-0117, announced 5 minutes before with the PC clock, offline, the Shebang zone saves (Alpha
untouched), measure mode without the debug layer for measurements; one live test at a time on the shared PC; builds into this
thread's own folder, hidden (D-0199). Thomas works at night (2026-10-22 about 19:30 to 07:00) and plays only on Friday
2026-10-23 in the evening: nothing waits for him, deviations go to STATE.
