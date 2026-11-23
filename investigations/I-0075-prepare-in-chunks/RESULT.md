# I-0075 result: the render worker's prepare in chunks, after Elden Ring

Complete (2026-10-20). Runs `evidence/I-0066/artifacts/i74-native-28..36-*` (the I-0074 series runner and scenes), builds
`evidence/I-0075/artifacts/i75{a,b}-build`. ER reference: [I-0073](../I-0073-er-loading-copy-recording/ER-STUDY.md) section 3: ER
prepares each draw in the worker that records it, over chunks planned per pass, submitted in order.

## Conclusion

H1 confirmed. `PrepareNativeModelFrame` (main view) runs its rows in chunks of 128 on the recording pool (free while the render
worker prepares: three workers beside it), each chunk with its own frame lists, counters, shared-block table, prepare-shadow
payloads, kept-draw outcomes, deferred kept-draw insertions and erasures, and deferred D2 paint requests; the chunks are merged
in row order (lists appended, the first longest frame block of each kind, the kept-draw changes and the paints applied in
row order). Before the chunks, on the render worker: every row's model version and the repeated rows' occurrences (counted in
row order; they replace the probe of the kept table's `last_used`, which other chunks would now write concurrently). The
keying changed slightly: under the probe a row following a same-key row that kept no draw took occurrence 0 again; now each
repeated row has its own key. While the
chunks run the kept-draw table is only read (each row's entry is its own: the occurrence makes the keys unique). The menu
views, the rebuild and dry-run diagnostics and scenes under 256 rows run as one chunk; `no-prepare-chunks.flag` turns it off.

H2 not tested: the sun-shadow casters stay under the chunk recorder's 256-draw threshold at m45, Archdragon Peak and
Firelink, so a chunked caster recording was never exercised; it was written, then left out of the merge (review M2). The
casters keep one recording on the render worker (their cascade views, viewports and rects now made before it).

| Scene (window) | Before | After | Runs |
|---|---|---|---|
| m45 prepare ms (A/B in one build) | 4.87 (no chunks) | 2.91 | 32 / 28 |
| Archdragon Peak (windows 66-100): frames/s, prepare, record | 59.1, 8.77, 4.20 (I-0074) | 59.3, 4.39, 3.81 | 23 / 29 |

Guards (i74-native-30 and 35, Firelink walk with `record-revalidation` and the prepare shadow, the kept-draw checks running
inside the chunks): 200,784 and 201,153 record comparisons, 20,106 kept-draw shadow checks each, 64 parallel pass-row checks,
0 mismatches, no cache-mismatch row. D3D12 validation: 0 messages over 4,930 and 4,882 m45 frames (runs 31, 36). Pictures:
m45 0.99-1.00 per region against the I-0071 merged build; Firelink 0.99-1.01; Archdragon Peak ground rows 0.98-1.00, the sky
and fog within their run-to-run spread.

Pre-merge review (Opus 5.5, D-0176), applied in build `i75c-build` (checked by runs 37-39: 4,944 m45 frames under validation, 0 message; 197,066 record comparisons, 20,106 kept-draw checks and 64 pass-row checks, 0 mismatches; the split now counts the 2,812 rows of an m45 prepare; m45 prepare 2.91 ms): no High finding.
- Medium, M1: with chunks the measure-mode split (`native_prepare_split`, phase state per thread) counted only the calling
  thread's rows; a worker now opens the row phase when the caller is measured, and the phases' self times and
  `prep_validate_resources_draw_ms` are summed over the threads (they can exceed the prepare's wall time). M2: the chunked
  caster recording, never exercised, left out (above).
- Low, applied: the kept shadow-check turn read under the lock only (L2); the witness counters read atomically (L5); a chunk
  that throws closes its list (L6). Recorded: the occurrences are counted every frame on the render worker (L1, could move to
  the pass rows); each chunk has its own shared-block table, so frame-level blocks are packed once per chunk and fresh draws
  still build one at a time under the model-draw cache lock (L3); some witness counters are per chunk (L4); a chunk carries a
  whole `NativeModelFrameInput` (L7).

## Uncertainty / open

- One run per scene and build. The kept rows of a model whose version changed are still built again every frame (pose,
  animation: about 35 a frame on the Firelink walk); they are now spread over the workers.
- With the prepare at 2.9 ms and the recording at 1.9 ms at m45, the render worker is no longer the limit; the engine thread's
  collect (5.8-6.0 ms) is, with the game's own update.
