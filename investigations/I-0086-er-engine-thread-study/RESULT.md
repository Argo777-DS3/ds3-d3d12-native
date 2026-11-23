# I-0086 result: ER study of the engine thread and the frame preparation (third tranche preparation)

## Conclusion

Complete as a study (2026-10-22, not merged: a study branch for the third tranche « fil moteur vers 120 images/s »):
[ER-STUDY](ER-STUDY.md) describes how Elden Ring 2.7.1 organises its main thread and its frame preparation (its FD4 frame
timeline decoded from the image, the draw rows, the rendering-parameter update, the UI, the effects, the per-entity draw, the
per-view constants, the frame memory, the limiter and the waits), ER's defects on that path, the native engine thread's
code map and measured costs at m45, and a prioritised plan of measurable gains (P0 to P9). H1 holds in part: ER's main thread
hands the UI's display to its render thread and runs the effects and the entity draw on its pool with per-thread memory;
but it also keeps work serial on the main thread (the 28 rendering-parameter updates) and it rebuilds every draw every frame.
H2 holds: ER spins (the focused limiter on QPC, the frame-lag wait and the effects' barriers on `Sleep(0)`, the main thread's
row barriers), joins pool-wide, holds Scaleform's global lock over the whole UI draw, and leaves a parallel parameter update
compiled but off.

The plan's estimate: the UI's display on the render worker (P1, about 1.3 ms), the collect's independent parts as concurrent
tasks with per-group joins (P2, about 1.5 to 2 ms) and the frame memory without the general heap (P3, about 0.6 to 1.0 ms)
bring the m45 native work on the engine thread from 5.5 ms to about 2.0 to 2.5 ms, under the 3.6 ms that 120 frames/s
leaves beside DS3's own update; P0 (measure the engine thread unlocked) comes first.

## Evidence

- ER data and targeted disassembly (the step and row tables, the `CSDrawStep` bindings, the UI closure queueing, the render
  thread's list player, the effects' fork-join, the per-thread allocator, all checked by Opus) and five Sonnet 5.5 workers'
  assembly readings, then their Ghidra pass (read-only headless Ghidra 12.1.3 on `er-mirror-i0070-a/b/c` and
  `er-mirror-i0072-a/b`, only while no game ran). Scripts, tables and exports in the session scratchpad only (D-0142).
- DS3's tables decoded by the same script, reproducing ENGINE-STEP-THREAD-MAP; MENUS-MAP for DS3's UI path.
- Measurements on record: I-0078 runs 89 (sample profile), 93 (splits), original-04/05; I-0077, I-0081, I-0084.
- The native code at `main` 95aec11c, mapped by a Sonnet reader and checked where the plan relies on it.

## Uncertainty / unresolved questions

- Located: ER's per-view scene render is called on the main thread in the `AdhocDraw` row, by `RendMan::140CC8850` through
  the scene list's render-all 141ADA8D0 (checked, ER-STUDY §2), while the UI's Advance and Capture run on their worker.
- Open: the shipped default of ER's threaded-render switch (`config+0xE2`) and its deferred record pass (`DAT_1447F59B9`).
- The shipped default of ER's threaded-render switch (`config+0xE2`) and the deferred record pass (`DAT_1447F59B9`).
- Every gain in the plan is an estimate from the measurements; one profile and one split run at m45.

## Artifacts produced or modified

- `investigations/I-0086-er-engine-thread-study/` (CONTRACT, ER-STUDY, RESULT, status); the investigations index row.

## Suggested next step

The third tranche of the engine thread (ROADMAP, after the night endurance and its leak follow-up) starts from ER-STUDY §9:
P0 (the engine thread unlocked, measured), then P1.
