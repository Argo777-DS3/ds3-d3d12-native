# I-0077 - V2: the engine thread's collect toward 120 frames/s, after Elden Ring's layer traversal and entity jobs (D-0195)

Capability: C3, wave 2. Opened 2026-10-20 17:48 Paris from the I-0076 checkpoint (STATE « Next action »), in a fresh thread.

## Question

Which parts of the engine thread's work per frame (m45: collect 5.8 ms, UI prepare 1.4 ms, the end of frame 0.9 ms beside the
game's own update) can move off the engine thread or stop being repeated, after Elden Ring's scene traversal (layer
traversal jobs, entity Draw jobs, per-thread outputs joined in order), so that the engine thread's frame approaches the
8.3 ms of 120 frames/s?

## Known inputs

- I-0071 ER-STUDY A (ER's five fork-join levels: sub-scenes, `GXLayerTraverseJob`, `GXEntityDrawParallelJob`, instance
  records, recording chunks; per-thread records concatenated and sorted), I-0074 and I-0075 results (the admission and
  recording pools), COLLECT-ORIGINAL-STATIC (DS3's C1FB0 per-layer tasks with traverser clones, the node walk 208180).
- The pre-study: i74-native-39 and the i74-native-25 profile (STATE).
- This workstream's first runs: i74-native-53 (m45, the sampler on the engine thread, the render worker and, new, every pool
  worker) and i74-native-54 (m45, the collect split and stamps).

## Hypotheses

- H1: the admission's global locks (the model records, the carried instance tables) and its model moves are a measured part
  of the collect; per-entity shards, records kept in place and one slot per candidate remove them.
- H2: the main walk's entity filter (serial on the engine thread) runs per layer on the pool, as ER's layer traversal jobs
  and DS3's own deferred layer tasks, with the same candidates in the same order.
- H3: the reflection walk and the pass rows' merge can leave the engine thread's critical path without changing the frame.

## Discriminating evidence

m45 collect split and stamps before and after in measure mode (no debug layer), the sample profile, the record revalidation
and prepare-shadow guards on the Firelink walk (0 mismatch), a D3D12 validation run, pictures at m45 and Firelink.

## Scope and exit

In: the main collect (`CollectNativeSelectedScene`), its walk and admission, the pass rows, the reflection walk; the UI and
end-of-frame costs are measured and only moved if the ER study and the profile show a clean seam. Out: the game's own update,
the render worker. Success: a measured shorter engine-thread frame at m45, 0 guard mismatches, 0 validation messages, the
same pictures; reviewed and merged. Budget: 12 cycles.

## Safety and human interaction

Live runs under D-0117 with the D-0116 conditions, each announced in the thread 5 minutes before (PC clock), no other heavy
work (Ghidra) during a measurement.
