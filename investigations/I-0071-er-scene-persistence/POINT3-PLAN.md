# I-0071 point 3: the per-frame draw build on workers, after ER (plan, 2026-10-20)

Thomas chose the order 1, 2, 3 (2026-10-20 00:43 Paris); point 3 is ER study A ([ER-STUDY](ER-STUDY.md) A) adapted to DS3,
and it carries the loading and threading workstream (the shader-cache thread's handover, STOP-ALLOWED 2026-10-20 03:12).

## Where the CPU goes now (measured)

- m45, I-0070 merged build (i70-native-19, measure mode, `native_collect_sections`): the two world walks with their admissions
  7.7 ms, the pass rows (`PrepareNativeSelectedPasses`) 2.25 ms, the reflection walk 0.7 ms, the rest under 1 ms; then the
  prepare 5.4 ms and the recording 4.2 ms (`Get-PacingMedians`).
- Archdragon Peak standing, 1,087 models (i70-native-20, kprofile on, `native_collect_split`): engine traversal 0.24 ms, entity
  filter 0.86, model admission 5.1 (the reuse path: fade, inputs, record check), mesh blocks 2.6 (`PlanNativeProgramTables` on
  reused meshes), passes 4.4, lights 0.5. The engine's own walk is cheap: the cost is the per-entity native work after it.

## ER's shape to copy (ER-STUDY A)

The traversal only gathers (entity, clip) in deferred mode; a parallel job (`GXEntityDrawParallelJob`, atomic ticket, pooled
per-worker contexts) runs each entity's Draw; per-thread frame allocators hold the records, concatenated and sorted by a key
afterwards; the recording of sorted chunks runs on workers into separate command lists ordered by a chunk key.

## Steps

1. **3a, deferred admission on workers.** The visitor (`AdmitNativeSelectedObject`) keeps gathering candidates during the
   engine walk but no longer admits them; after the walk the candidates are admitted by a worker pool (the engine thread
   takes part and waits), each worker filling its own models and counters, merged in walk order (the same scene as today, so
   pictures and the revalidation guard stay comparable). Prerequisite audit of `AdmitNativeModelCandidateBody`'s shared state:
   the scene's lazy members (`scene_word_ranges`, `system_textures`), counters (`result.*`, `NoteNativeRecordOutcome`), the
   scene and view block tables and `planned_programs` that `PlanNativeProgramTables` fills, `light_keys`, the static witness
   sets (already locked), the engine calls (D24490, D24430) made from workers.
2. **3b, the pass rows per model on workers** (`PrepareNativeSelectedPasses`), merged in model order.
3. **3c, the prepare and the recording in chunks** (ER 141B3E890 / 141B3D940): rows split by pass into chunks recorded into
   their own command lists, executed in chunk order.

Each step is measured at m45, Archdragon Peak and the Firelink walk against the step before (collect, frames/s, p99), with the
record revalidation and prepare shadow guards, and pictures against main.
