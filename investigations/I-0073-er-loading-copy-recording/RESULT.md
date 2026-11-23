# I-0073 result: ER texture loading, copy queue and remaining draws

Complete (2026-10-20), static study only; first kept on its branch (the coordinator's instruction: the
persistence thread was merging I-0071 point 3b). Detail and addresses: [ER-STUDY](ER-STUDY.md).

## Conclusion

H1 confirmed. Elden Ring decodes, creates and uploads textures on its job system's loading pool (`LoadProcess_M`, 3 to 5
idle-priority workers): one task per TPF, one create job per texture; the TPF processor polls the job group without blocking
and the textures reach their stable holders only complete. Swapping a holder pins the old texture for two frames through the
renderer's frame-delayed release (a correction of I-0071 ER-STUDY B, which had not found it).

H2 confirmed with a nuance: ER owns one COPY queue, but uses it only for synchronous creation uploads: a fresh type-3 list
and a fresh fence per resource, the calling worker blocked until the fence passes; no batching, no persistent copy fence, no
cross-queue wait. Per-frame updates go through the persistently mapped upload ring on the DIRECT or COMPUTE lists.

H3 confirmed: ER plans the sorted draws into chunks from a per-pass table (only the large geometry and shadow passes split,
up to 5 and 14 chunks), records each chunk into its own pooled DIRECT list on up to 16 workers plus the calling thread, with
per-worker allocator rings, upload and descriptor sub-allocators and state applied from pass snapshots, barriers inside the
lists, and the render worker submits the lists in key order, 72 per ExecuteCommandLists.

DS3's original already has ER's texture creation job (`CSGXTextureCreateFragment`, the same body as ER's create job) on its
own job system, but its TPF creator joins blocking (ER's synchronous path). The native backend decodes off the engine thread
already and records uploads on DIRECT ingress lists that run ahead of the next frame. The adaptation proposed: keep DS3's
fragments, make the native work inside them ER's (one-block copy-out, a COPY queue with a copy fence per record and residency
polled on completion, without ER's CPU wait), then point 3c as ER's chunked recording with the prepare merged into each chunk
([ER-STUDY](ER-STUDY.md) section 5).

## Evidence

- ER functions checked by Opus on the decompile or assembly: 141E890E0, 141E9FD30 (COPY queue type 3), 141E928E0, 141E88450,
  141EA4C10, 140E83200, 14023B5A0, 14023B0C0, 14023C4F0, 140B7EB20, 140B7E940, 141A97400, 140B7EEC0, 141AF4D50, 141B3E890,
  141B3D940, 141B3DC10, 141B3DA20, 141E8E5B0, 141AECAE0, 141EBC020/030/070.
- DS3 functions checked by Opus: 140D4A0A0 (`CSGXTextureCreateFragment` Run), 140D49FF0, 140EEBED0 (`CSEzWorkFragment`),
  140D4E100, 140EEB930, 140EEB820, 140EEC0E0; native `TextureIngress`, `DeviceQueues::RecordIngress` and `SubmitWithIngress`.
- The other claims come from three Sonnet 5.5 studies (L, Q, R) and are marked OBSERVED (their reading) in ER-STUDY. Their
  exports stay in the session scratchpad and are not evidence.

## Uncertainty / unresolved questions

- The thread stepping ER's file and DCX state machine, and the caller of DS3's 140D4F470.
- ER's per-draw pipeline calls in its draw objects, the sort comparators, N (frames in flight) and the worker counts.
- Whether the COPY queue's per-list cost needs batching on DS3 (to measure, beyond ER).

## Artifacts produced or modified

- `investigations/I-0073-er-loading-copy-recording/` (CONTRACT, ER-STUDY, RESULT, status).

## Suggested next step

The loading and threading workstream (I-0074) builds on section 5 in the order given there; no DS3 claim here replaces a
measurement on DS3.
