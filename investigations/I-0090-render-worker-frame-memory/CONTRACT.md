# I-0090 - Mémoire de trame du fil de rendu

Capability: C3 (Wave 2, performance: the render worker, bottleneck 1 of [I-0088 BOTTLENECKS](../I-0088-engine-thread-120-c/BOTTLENECKS.md))

## Question

Unlocked at 125 frames/s (m45) and 157 (the Firelink walk) the render worker is the frame's limit: busy 97 % and 92 %, a fifth
of it the general heap, the frame ring's retirement about 1 ms (`slot_release_ms` 0.95 at m45, `i88-12`), the reflection view
1.2 ms, the GPU never waited for. The profile (`i88-20`, `i88-21`) shows where the heap goes: of the main view's 2,975 rows at
m45, 252 a frame are rebuilt in full because their model's inputs changed (an animated model's pose, its engine animation
block, a volatile model's new mesh set; 346 of 1,351 on the walk), each a complete `PrepareNativeModelDraw` (programmes,
validation, pipeline, sampler tables, a descriptor range, SRV copies) and, a frame later, a complete destruction at the
retirement (`OwnedNativeModelDraw` 10.3 % of the worker). Which changes, made after Elden Ring's per-frame memory without its
defects, take the per-frame heap and the per-object destruction off the render worker, and how much of the worker and of the
frame do they give back, measured unlocked, standing and in motion?

## Scope

Included, in this order, each measured before and after:

1. **The draw's persistent state** (ER: the material and pipeline state are persistent; only the per-frame packet is rebuilt):
   a row rebuilt because its model changed takes its kept draw's prepared state (pipeline, root signature, descriptor tables,
   their per-slot copies and the C4-2a encode record) when everything that state was made from is unchanged (programme set,
   sampler bank, geometry, fixed-function state, targets, SRVs, samplers, the constant bindings' registers and sizes); only its
   constant payloads are new. A shadow check under `prepare-shadow.flag` prepares every Nth such draw in full and compares.
2. **The frame's per-frame memory** (ER-STUDY §5: a linear frame arena reset per scene render, per-thread bump chunks, no general
   heap call on the draw path): what the prepared frame owns for one frame (its lists, the rebuilt rows' payloads and draw
   objects) taken from storage owned by the frame slot and reused when the slot comes back, and the retirement's remaining
   per-object work off the render worker's path; the exact form chosen from the profile after item 1.
3. The worker re-profiled; the next bottleneck named for the re-ranking (items 2 to 5 of BOTTLENECKS).

Excluded: the collect's storage on the engine thread (bottleneck 2, a workstream of its own), the effects' update (3), the
reflection view's reuse between frames (4), DS3's steps (5); the GPU side (bindless, queues): the GPU is not in the path at
1280x720.

## Inputs

[I-0088 BOTTLENECKS](../I-0088-engine-thread-120-c/BOTTLENECKS.md) and its runs `i88-12`, `i88-13` (stamps, pacing),
`i88-20`, `i88-21` (sample profiles); I-0086 ER-STUDY §5 and §8 (`investigations/I-0086-er-engine-thread-study/ER-STUDY.md` on
the study branch `claude/er-engine-thread-study`, 243bf355, not merged): `GXBS::GXLinearAllocator` reset per scene render, per
draw context chunked bump allocators without lock, no CRT allocation on the entity-draw path, and its defects (the arena's
spin lock per chunk refill; per-entity draw data rebuilt without any change test); C4-2 kept draws (PERSISTENT-RECORDS-DESIGN 2,
I-0070), C4-2a encode record, I-0074/I-0075 (record and prepare in chunks); `native/renderer/model_draw.*`,
`native/renderer/backend.*` (`FrameRing`), `native/frame/game_bootstrap/13_model_frame.inc`.

## Hypotheses

- H1: most of the 252 (m45) and 346 (walk) rebuilt rows a frame change only their constant payloads (the bone palettes, the
  instance and model blocks); their prepared state equals their kept draw's, so reusing it removes their `Prepare` (about
  7 µs a row on the pool) and most of their destruction at the retirement.
- H2: after H1 the retirement's remaining cost is the per-frame objects' release (draw objects, payload vectors, the frame
  input's lists, the volatile models' mesh sets); storage owned by the frame slot and the release off the worker's path take
  the retirement under 0.2 ms.
- H3: the worker's busy share falls under 85 % at m45 and the frame moves to the engine thread's bound (about 130 frames/s at
  m45), the walk above 157.

## Expected evidence

The kept-draw outcomes (`native_persistent_draws`, a new `state_reused` count), the prepare split (`native_prepare_split`),
`slot_release_ms` and `worker_job_ms` (`native_frame_audit`), the frame and present p50/p99 (`Get-I88Summary.ps1`), unlocked on
the tearing chain (`original-unlock.flag`) at m45 and on the Firelink walk, measure mode without the debug layer; the sampler
before and after (the worker's heap and retirement shares); the guards (`record-revalidation.flag`, `prepare-shadow.flag`: 0
mismatch) and the debug layer (0 error); pictures equal to the build without the change; the replayed path gate with its memory
leg at the merge (D-0202, D-0204).

## Exit conditions

- Success: the worker's heap and retirement shares measurably down (target: 1.5 ms of the worker at m45), the frame faster at
  m45 and on the walk (p50 and p99), no visual difference, guards 0, validation 0; Opus step and final reviews; the gate.
- Partial: a measured gain short of the target; the rest replanned in RESULT.
- Refuted: a change that measures no gain is not kept (Optimise, do not stack).
- Stagnation: no new discriminating evidence after 3 cycles.
- Budget exhausted: close the tranche and replan.

## Safety and human interaction

Live runs under D-0116/D-0117, announced 5 minutes before with the PC clock, offline, the Shebang zone saves (Alpha
untouched), measure mode without the debug layer for measurements; one live test at a time on the shared PC; builds into this
thread's own folder, hidden (D-0199). Thomas sleeps by day and plays Thursday evening: nothing waits for him, deviations go to
STATE.

## Budget

8 cycles.
