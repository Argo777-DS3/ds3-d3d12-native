# I-0093 - Vue du reflet

Capability: C3 (Wave 2, performance: the engine thread, bottleneck 4 of [I-0088 BOTTLENECKS](../I-0088-engine-thread-120-c/BOTTLENECKS.md))

## Question

After I-0092 the frame is bound by the engine thread (m45 unlocked 139 frames/s, 7.14 ms). The water's reflection view's collect
(`CollectNativeReflectionScene`) is 8.0 % of the engine thread at m45 (`i92-09-prof-pool`, about 0.57 ms): its admission 4.9 %, of
which the programme tables' planning 3.2 % (the constant packing 2.0 %, the heap 1.3 %), its lighting 0.9 %, its walk 0.9 %, its pass
rows 0.5 %. The main view carries each model's instance block table from one collect to the next while its instance records are the
same bytes (C7-1c, `native_block_carry.instances`); the reflection view does not (`carry_instances` is main-view only), so every
reflected model packs its instance blocks again every frame, though its instance records (the mirrored clip plane included) only
change when the model or the water plane moves. Which part of the reflection's collect can be kept between frames, and how much of
the engine thread does that give back?

## ER

ER-STUDY §5: a view's constant block is re-uploaded only when it changed (the pass-enter snapshot skipped while the previous slice is
current); its per-entity draw data are rebuilt each frame without a change test (a defect not copied: ours keeps what did not
change). ER's planar reflection itself was not studied (BOTTLENECKS item 4: none studied); this step adds no new mechanism, it
extends the main view's existing carry (C7-1c) to the reflection view.

## Scope

1. The reflection view's instance block tables carried from its previous collect, per entity, while the model and its instance
   records are the same bytes, in a carry of its own (the reflection's records differ from the main view's: the clip plane), swept
   and cleared with the reflection's tables; the model records stay the main view's (borrowed, never stored back).
2. Measured at m45 and on the walk (unlocked, measure mode); the next part named from the profile (its lighting, its walk).

Excluded: the reflection's render on the worker (about 0.15 ms), its rows kept between frames (prepare), ER's planar reflection.

## Expected evidence

The engine thread's collect and the reflection's share in the sampler before and after; frames/s, p50/p99; the reflection's
picture (`reflection-*.rgba` of the debug run, the water at m45) equal; guards, the debug layer, the gate.

## Exit conditions

Success: the reflection's collect measurably down at m45, same pictures, validation 0, the gate. Refuted: no gain, not kept.
Budget 6.

## Safety and human interaction

As I-0092 (announced live runs, offline, Shebang saves, hidden builds; one thread at a time, Thomas 2026-10-22 21:59).
