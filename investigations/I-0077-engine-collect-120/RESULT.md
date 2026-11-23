# I-0077 result: the engine thread's collect after Elden Ring's layer traversal and entity jobs (first tranche)

Complete (2026-10-20, first tranche). Runs `evidence/I-0066/artifacts/i74-native-53..72-*` (the I-0074 series runner, new
scene `m45split`: m45 in measure mode with the collect split and the frame stamps), builds `evidence/I-0077/artifacts/i77{a..f}-build`.
ER reference: [ER-STUDY](ER-STUDY.md) (Sonnet 5.5, read-only Ghidra on ER 2.7.1; 141B4CF40 and 141B4CD50 checked by Opus).

## What ER does (the design's reference)

One `GXLayerTraverseJob` per layer (a world has one dynamic tree and one flat layer, so the tree walk is one job, serial
inside); the traverser clone only tests visibility and gathers each entity into a 16-byte list (141B4CF40, the collect byte
+0x13); a `GXEntityDrawParallelJob` then runs the entities' draw work by ticket (141B4CD50); per-thread records are joined
and sorted (141B457A0). The whole traversal is synchronous on the draw-step thread helped by the pool; the frame pipeline's cut
is recording and submission (the render thread), as in our render worker. Views are walked one after another.

## Conclusion

H1 confirmed: the admission's global locks and model moves were a measured part of the collect. The pool workers, sampled for
the first time (`WorkPool::worker_hook`), each spent about a third of their admission time in `mtx_do_lock` (run 53): the model
records and carried instance tables now sit in 64 shards by entity (`NativeEntityShards`), a record is taken out and put back
in its slot (no node freed and allocated each collect), the material registry lock is shared by its readers (every reused
record compared its 0x4F8 material bytes per mesh under the global lock), the engine-bytes cache has 64 hashed stripes, and
each candidate has its own admission slot (no growing per-worker lists), the models placed on the pool.

H2 confirmed: the main walk runs C1FB0's layer loop itself, one traverser per layer on the pool; in ER's manner the traversers
only run the tests that need the bounds stack (a map piece's and a draw entity's frustum classification) and gather the
entity; the rest of `AdmitNativeSelectedObject` runs for the gathered entities on the pool in chunks, joined in walk order. At
m45 the world has two layers (the dynamic tree: 1,970 visits, 1,683 gathered; a flat layer: 1), as in ER. The serial walk,
redone and compared every 64th collect under `record-revalidation.flag`, gave the same candidates in the same order.

H3 confirmed in part: the reflection walk takes the same split (its view flags set on the workers, `NativeViewFlags`) and
its scene and view tables are carried while its camera is unchanged; its frame is prepared on the render worker before the
frame slot wait (`NativeOffscreenViewJob::prepare`), no longer in `Clear()` on the engine thread. The pass rows are placed on
the pool.

| m45, measure mode, medians of 75 windows | Before (run 54, main + the sampler hook) | After (run 70, `i77f`) |
|---|---|---|
| collect (`UpdateNativeSceneCpu`) | 5.76 ms | 4.81 ms |
| UI prepare | 1.37 ms | 1.35 ms |
| `Clear()` (end of frame) | 0.85 ms | 0.49 ms |
| the engine thread's native work | 7.98 ms | 6.65 ms |
| ModelAdmission CPU, all threads | 11.1 ms | 7.3 ms |
| frames/s, slowest frame | 60, 18.4 ms | 60, 18.4 ms |

Guards: the Firelink walk with `record-revalidation` (runs 62, 68, 72): 0 walk-check, record and pass-row mismatches. D3D12
validation at m45 (runs 59, 65, 69): 0 errors, 0 faults. Pictures: m45 0.98-1.01 per region against main (run 50), as main
against itself. The reflection view kept its outcomes (58 objects, 35 models, 36 prepared rows at m45).

Pre-merge review (Opus 5.5, D-0176): no High finding. M1 (the traversers' bounds stacks could grow from the engine heap on
several workers at once): each stack reserved for 64 levels on the engine thread with 234620's own reserve 19C1D0, checked
(empty, capacity) and a growth logged (none in runs 69-72). M2 (engine functions now on workers): read statically, `06F310`,
`64F000` and `619800` only read, `D03050` writes only its output (its callees DFD950 and D03B60 not read), `C2630` writes only
its traverser's +0x13, `208180` spawns no node job with byte +0x11 zero; the rest (the dynamic tree layer's walk, the culler's
vt+0x60, D043C0, D04350, D03430, E35EF0) INFERRED safe (DS3's own deferred layer tasks run the same walk on GX workers), backed
by the guards and the A/B `no-parallel-walk.flag`. Low, applied: L1 the reflection target's written flag applied by the
engine thread, L2 the reflection prepare before the slot wait, L3 a second reset increment, L4 the guard's diagnostic counts
restored, L5 a reflection walk failure drops the reflection, L6 the world entry check kept. Recorded: the serial default
construction of the models and rows before their parallel placement (about 0.3 ms each at m45).

## Uncertainty / open

- One run per scene and build; m45 is a standing scene (the camera carry holds); the walk and the admission scale with the
  visible entities, so the gain should hold while moving, not measured here.
- The engine thread is still far from 120 frames/s: of 16.7 ms at the 60 cap, the game's own update is about 4.7 ms and our
  work 6.65 ms. What remains, by size (run 71's profile, the engine thread at 16.7 ms a frame): the admission (2.1 ms with
  its placement; each reused model about 6 µs of CPU on every worker: its inputs, instance records, record check, engine
  bytes), the UI prepare (1.4 ms, Scaleform display and our primitive production on the engine thread), the pass rows (1.1 ms,
  rebuilt for every reused model), the reflection walk (0.5 ms), the main walk and expansion (0.5 ms), the effects (record
  0.55 ms, system update 0.5 ms).
- Next tranche: persistent per-model pass rows and instance records (ER's per-entity data), the UI production off the engine
  thread, the effects' update on the pool.
