# I-0080 result: ER study of the D3D12 pitfalls (audit preparation)

## Conclusion

Complete as a study (2026-10-21): [ER-STUDY](ER-STUDY.md) covers the seven pitfalls of the ROADMAP's « audit des pièges
DX12 » (pipelines compiled on the fly, barriers, CPU/GPU synchronisation and frames in flight, video memory budget and leaks,
descriptors, uploads, driver dependencies), each with Elden Ring's treatment read in its decompilation, ER's own defects, the
native code's state at `main` 4741f6f2, and a lead with its measurable check (section 8). H1 holds in part (ER treats frames
in flight, upload lifetime and the pipeline library), H2 holds: ER keeps known defects the native backend must not copy (the
synchronous pipeline compile under the cache mutex, D3D11-style barrier round trips, busy waits on the main thread, no
budget awareness, per-descriptor copies, no device-removed handling, a static AMD library).

## Evidence

- New ER decompilation (Sonnet 5.5 workers, headless Ghidra read-only on `er-mirror-i0070-a/b/c`, `er-mirror-i0072-a/b`,
  only between the I-0079 live tests): barriers, descriptors, device and driver, pacing. Opus read the decompile of the
  claims marked **checked** in ER-STUDY (1419E5410, 141ACB6D0, 141E9BF90, 141EA2320, 141ACD200, 141EA2730, 1419E9DB2,
  141AFC9A0, 141F0C8F0, 140E84830). Exports kept in the session scratchpad only (D-0142).
- Earlier studies reused: study716, ER-DEEPENING-897, D3D12-DESIGN-REVIEW, I-0072, I-0073, I-0076, I-0079 ER-STUDY.
- PUBLIC: Digital Foundry's patch 1.02 analysis of ER's PC stutter, through secondary reports.

## Uncertainty / unresolved questions

- ER's residency wrappers (141EA2BA0, 141EA1880) exist but their callers were not found; what the engine does with an
  out-of-memory code was not traced.
- Not recovered: the runtime sizes of ER's descriptor heaps and heap pools, ER's swap-chain buffer count and V-sync writer,
  the pipeline-cache eviction values, the writer of ER's debug byte.
- Corrections: I-0073 ER-STUDY §3 (ER's context +0xD0 is in the viewport array, not a pending-barrier array) and I-0079 ER-STUDY §4
  (ER has MakeResident, Evict and SetResidencyPriority wrappers).

## Artifacts produced or modified

- `investigations/I-0080-er-dx12-pitfalls/` (CONTRACT, ER-STUDY, RESULT, status).

## Suggested next step

The « audit des pièges DX12 » workstream (ROADMAP, after the minidumps and the replayed path) starts from ER-STUDY section 8.
