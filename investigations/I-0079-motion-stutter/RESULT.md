# I-0079 result: the stutter while moving (first tranche)

Complete (2026-10-21). The native renderer's frames over 34 ms while walking at Firelink fell from 79 to 9-11 in 80 s (the original
1 to 3 on the same path), over 25 ms from 264 to 67 (the original 11); sprinting 97 to 11 (original 3) and 336 to 66 (original
11); the camera turn 1. No slow frame held a pipeline or shader compiled on the frame path (H1 refuted). The causes were three
native costs met while the world streams, each removed. A residual remains (frames of 25-60 ms about every 8 s, and more
frames of 17-25 ms than the original): the next tranche.

## Method

The same Firelink path (Eta's save at the shrine bonfire, walk Z 4 s / S 4 s, sprint with Space, camera turn L) natively and in
the original, 80 s of motion after a 40 s settle, measure mode without the debug layer (`Run-Stutter.ps1`, `Run-StutterSeries.ps1`;
read with `Get-Stutter.ps1`, which keeps the frames whose tick lies inside the motion marks). PresentMon (NVIDIA FrameView's
1.9) is on the PC but its ETW session needs elevation this thread does not have: both renderers are timed in process, the
native at its frame end (`native_frame_times`, `native_slow_frame`), the original at its present pacer D5CB0 (`original-probe.flag`,
`original_slow_frame` rows added here, no other hook). Every slow frame classed by the sampler's frame marks
(`Get-SampleProfile.ps1 -SlowFrames -SlowMs 34`; the sampler now ends at a chosen mark and writes its samples).

## Causes (run i79-04, the sampler in the slow frames)

1. **Streamed resource creation (most slow frames).** The engine thread waited in DS3's task join (0x1423C1DA4 / 23C21BB
   polling 0x141799430) while a loading thread ran `AdmitEncodedTextureFamily`: 30 % of that thread's samples in
   `RecordNativeTextureUpload`, a committed DEFAULT texture and a committed UPLOAD buffer per texture, each creation waiting on
   the paging fence (`NtGdiDdDDIWaitForSynchronizationObjectFromCpu`, `CreateAllocation`), then a clear of the whole upload range
   before the copy; the static model buffers the same (two committed buffers each). About 68 textures a second while walking,
   1.1 ms each.
2. **The kept models' expiry (seven frames of 30 ms per run).** Every 600 prepares the kept models unused since then are erased on
   the render worker; they held the last references to their textures, whose committed allocations were destroyed there
   (`DestroyAllocation2`, `FreeGpuVirtualAddress`: 63 % of the worker's samples in those frames).
3. **The lighting banks reparsed (frames of 45-60 ms).** Each admission or release of one GPARAM file reparsed every file
   (`PublishNativeGparamBanks` -> `Banks::Load`), 2.6 ms on average and up to 6 ms, half of them on the engine thread.

## Corrections

- **Placed resources in pooled heaps** (`DeviceQueues::CreateStreamed`), after Elden Ring's placed arm (ER-STUDY: textures of 8 MB
  or less in 32 MB heaps for the 4 KB-placed and 256 MB heaps for the others, first-fit, committed above): textures in 64 MB
  heaps (`ALLOW_ONLY_NON_RT_DS_TEXTURES`), the 4 KB-placed ones (the D3D12 memory allocator's tile-count rule, one slice) in 32 MB
  heaps of their own, static buffers in 64 MB `ALLOW_ONLY_BUFFERS` heaps, 8 MB placed at most, best fit, heaps created not zeroed
  where the runtime accepts it. A range returns when the runtime destroys the resource (a COM lease in its private data) and is
  reused only once the queue passes the ticket submitted last at that moment; a spare empty heap of each kind is released every
  120 frames, one kept (ER's 141B04700). No D3D12 call runs under a pool lock.
- **Pooled staging** (`DeviceQueues::AllocateStaging`): persistently mapped 16 MB UPLOAD chunks, four pooled, their ranges kept
  by the copy's ingress recording until its submission completes; no clear of the range. This improves on ER, which creates a
  committed upload buffer per creation and waits on its copy (ER-STUDY 3; the WORKFLOW note on ER's defects).
- **GPARAM banks**: a file whose shared bytes are the same object keeps its parsed form (`Banks::Load(dir, previous)`).
- The second cause needed no change of its own: destroying a placed resource frees no allocation.

## Measurements (80 s of motion, measure mode; frames over 17 / 25 / 34 ms, worst)

| Run | Build | Motion | >17 | >25 | >34 | worst ms |
|---|---|---|---|---|---|---|
| i79-02, i79-13 | original | walk | 625, 623 | 11, 11 | 1, 3 | 34.0, 34.7 |
| i79-03 | main (a) | walk | 1236 | 264 | 79 | 63.9 |
| i79-15 | first fix (d) | walk | 1043 | 72 | 11 | 55.9 |
| i79-20 | heaps sorted (e) | walk | 1150 | 67 | 9 | 38.4 |
| i79-18 | original | sprint | 688 | 11 | 3 | 57.7 |
| i79-17 | main (a) | sprint | 1274 | 336 | 97 | 70.9 |
| i79-16, i79-21 | d, e | sprint | 1087, 1091 | 77, 66 | 10, 11 | 41.4, 57.2 |
| i79-19 | d | camera turn | 782 | 2 | 1 | 58.8 |

Texture admissions while walking: 1.1 to 0.54 ms each (run 03 against 10); GPARAM publications 2.6 to 0.37 ms each. The render
worker's recording unchanged (1.2-1.3 ms median). The final build (f, the review fixes): walk i79-23 898 / 71 / 11 frames over 17 / 25 / 34 ms, the worst 37.1 ms; under the debug
layer i79-22 0 validation error, 0 fault; the unit controls pass (Build-GameBootstrap with controls).

Invalid runs, kept for the record: i79-05..08 stopped at their first frames (the 4 KB placement asked for textures too large:
a runtime error, corrected by the tile-count rule); i79-10..14 natively ran with the debug layer on (the runner passed its switch
wrongly; their velocity captures and `baseline.json` show it), so their 7 ms recording is the debug layer's, not the PC's.

## Video memory

At Firelink after the walk the process's local video memory is 2.17-2.20 GB against 1.51 GB on main (run 03): the heaps' free
space after the zone load's peak (28 heaps for about 1.07 GB of live placed resources in i79-15, before the best fit and the
small-texture heaps; the same total after them). It stays level during the walk (no growth); whether it returns after zone
travels is open (the stability workstream's VRAM test, ROADMAP). Corrected by [I-0081](../I-0081-motion-stutter-2/RESULT.md)
(observation 5): main's 1.51 GB was read in the middle of its walk, which reached 2.18 GB at its end; no growth over five zone loads.

## Pre-merge reviews (Opus 5.5, D-0198)

Step review (after the first fix): no High; M1 (a heap created under the pool lock the leases take), M2 (a placed range reused
at once, without the video memory manager's deferral), M3 (16 pooled staging chunks) fixed; L4-L7 fixed (a counted heap, the
pool copied, the stale comment, unused helpers); L8 (sampler start and end in different units) documented in the runner;
L9-L10 (capped diagnostic rows; small buffers taking 64 KB) left as they are. Full review at the merge: no High; M1 (staging
chunks created and released under the staging lock, on the frame path through `CollectCompleted`) and M2 (the fence read
under a pool lock) fixed: chunks created with the lock dropped, oversized ranges in buffers of their own, released chunks
destroyed after the lock, the completed value read before it; L3 (dead branch) reused for the oversized ranges; L4 (a quarantined
pool's ticket pointer) cleared at quarantine; L5 (the not-zeroed flag dropped on any failure) only on `E_INVALIDARG`; L8 (small
placement for arrays) refused as D3D12MA does; L6 (the format-support cache keyed by format only, per process) and L7 (a 64 MB
heap released on the frame owner every 120 frames at most) left as they are, recorded here.

## Uncertainty / open

- One run per build and motion; Firelink only. The residual (9-11 frames over 34 ms, about 67 over 25 ms, and 1,100 over 17 ms
  against the original's 625) is not classed yet: the profile taken after the fix (i79-14) ran with the debug layer.
- The engine still joins DS3's texture fragments (DS3's TPF creator 140D4E100); ER polls instead. D-0197 (Thomas): if the
  residual comes from that join, the creation finishes off the engine's path and a model whose texture is not resident appears a
  few frames later, never with a stand-in.
- Video memory after zone travels (above).

## Artifacts

Runs `evidence/I-0066/artifacts/i79-01..23-*`; builds `evidence/I-0079/artifacts/i79{a..f}-build`; reports
`i79-04-walk-native-prof/sample-profile-slow-report.txt`, `report-prepare.txt`, `report-other.txt`; [ER-STUDY](ER-STUDY.md).
