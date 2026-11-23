# I-0071 result: Elden Ring's architecture for scene persistence, adapted to DS3 (points 1 to 3b)

Points 1, 2, 3a and 3b done (2026-10-20, Thomas's order 1, 2, 3 of 00:43 Paris); point 3c (the prepare and the recording in
chunks) follows. The ER map is [ER-STUDY](ER-STUDY.md); the point-3 plan [POINT3-PLAN](POINT3-PLAN.md).

## Conclusion

H1 refuted for ER (no per-instance draw persistence: ER rebuilds its draw records every frame, fast, on parallel jobs, ER-STUDY
A); H2 confirmed (ER binds through `CSGxTexture` holders whose inner is swapped in place, B); H3 confirmed for DS3 (point 1).
DS3 adopts what ER does, on its own data, keeping I-0070's records where they serve (transformed, not thrown away, D-0195):

1. **Distant enemies.** DS3's `CSLoadBalancerImp` has ER's shape and stays at level 0 in the original and natively at
   Archdragon Peak (ER-STUDY "Point 1 result"): not the cause; the native draws what the engine's fade weights say.
2. **Textures through the engine's holders (ER B).** A material binding keeps the holders the lookup D24490 returns
   (`SprjGxTexture`, vtable image+0x29559E8, its inner at +0x10, seeded with SYSTEX_DUMMY's holder until the texture arrives)
   and the inner each held when bound. A kept model whose holder's inner changed is rebound in place in a copy of its mesh set
   (`RebindNativeRecordTextures`, outcome `rebound`), not admitted afresh. I-0070's pending texture choices, their 500 ms
   expiries and holder pairs are removed. The holders of a record are checked in one guarded block per mesh
   (`NativeMeshHoldersStand`; a guarded read per word cost the m45 walks 0.7 ms a collect, i71b).
3. **The per-frame build on workers (ER A, `GXEntityDrawParallelJob`).**
   - 3a: the main walk only gathers its candidates; after the walk they are admitted on a pool (`NativeAdmissionPool`: at most
     seven workers beside the engine thread, two hardware threads left; chunks of 16 by an atomic ticket), each worker keeping
     its models with their candidate's index, merged in walk order (the scene one thread gives). The shared state became safe
     for it: atomic admission counters and record outcomes, the scene tables under a lock, the engine-bytes cache in 16 locked
     stripes, the light-set indices in a registry kept across collects (`NativeLightSetIndex`).
   - 3b: the pass rows (`PrepareNativeSelectedPasses`) by chunks of 32 models on the same pool, each chunk's rows kept per engine
     pass and merged pass by pass in chunk order (the serial order). The census collects run on one thread.
   - The pool rethrows a worker's exception once all have returned and runs a job alone when another thread holds it.

## Evidence

Runs `evidence/I-0066/artifacts/i70-native-NN-*` (measure mode, no debug layer, the I-0070 saves and key paths), builds
`evidence/I-0071/artifacts/{i71b,i71d,i71e}-build`, series logs `evidence/I-0071/series-*.log`; `Get-RecordSeries.ps1`,
`Get-PacingMedians.ps1`. Frames over 25 and 34 ms over the run, frames/s, collect ms (median of the 2 s windows):

| Scene | I-0070 (18-20) | point 2 (37-39) | 3a (40-42) | 3b (47-49) |
|---|---|---|---|---|
| Firelink walk | 71 / 26, 7.04 ms | 69 / 29, 7.34 ms | 82 / 39 (cold pipeline cache), 3.96 ms | 49 / 19, 4.48 ms |
| m45 Ariandel, still | 57.3 f/s, 11.66 ms | 55.3 f/s, 12.29 ms | **60.0 f/s** (the cap), 7.41 ms | 60.0 f/s, 5.67 ms, 10 over 34 ms |
| Archdragon Peak walk | 224 / 87, 8.87 ms | 138 / 42, 9.82 ms | 105 / 43, 7.72 ms | 170 / 43, 7.44 ms |

- m45 collect sections (`native_collect_sections`): the two world walks with their admissions 7.71 ms (I-0070), 8.45 (point
  2), 3.62 (3a); the pass rows 2.25, 2.21, 2.31 (3a), 0.71 (3b).
- Point 2 at Archdragon Peak: texture lookups in the worst collect 1,784 to 150, fresh admissions in the worst window 46.7 to
  16.3 (i70-native-20 / -39).
- The 3a Firelink walk was the first run of a new build folder, so its pipeline library was cold (42 pipelines created at
  arrival, serial 1337); its other slow frames have collects of 4-5 ms (engine-side), as the point-2 run's.

Guards (`record-revalidation.flag`, every record admitted afresh and compared every 8 collects; `-PrepareShadow`, every 30th
kept draw rebuilt and compared), 0 mismatches in every check run:

| Build | Runs | Record comparisons | Kept-draw checks | Holder refusals | Rebound |
|---|---|---|---|---|---|
| point 2 (i71b) | 35, 36 | 0 mismatches | 0 mismatches | 0 | 154, 1,884 |
| 3a (i71d) | 43, 44 | 198,494 + 488,874, 0 mismatches | 20,182 + 2,094, 0 mismatches | 0 | 158, 2,140 |
| 3b (i71e) | 45, 46 | 202,587 + 486,498, 0 mismatches | 20,162 + 1,920, 0 mismatches | 0 | 136, 1,907 |
| review corrections (i71f) | 50, 51 | 103,202 + 436,333, 0 mismatches | 5,467 + 12,491, 0 mismatches | 0 | 155, 2,433; pass-row parallel check 64 + 128, 0 mismatches |

The 3b Archdragon Peak walk has more frames over 25 ms than 3a's (170 against 105) for the same 43 over 34 ms and a lower
collect; the fast-travel arrival and streaming of that walk vary from run to run, and one run each does not separate them.

Pre-merge review (Opus 5.5, D-0176), applied in build `i71f-build`:

- High, H1: the light-set registry could be cleared in the middle of a collect (full at 225 keys, or a scene reset from another
  thread), leaving models admitted before the clear with envData past or onto other sets. Now a slot unseen for 8 main
  collects is freed and taken again by a new key, slots are freed and a reset applied only at the main collect's start
  (`BeginNativeLightKeys`), and a new key with every slot taken gets 0, as the serial code did.
- Medium, M1: the engine functions now also called from workers were read in the decompile (`evidence/I-0071/artifacts/ghidra/workers`): D24490 →
  2FF080 (a DLString through the global allocator 14177A330), D4ED20 → 2356180 (a hash bucket walk, no lock), D24430 → D446D0,
  32A80 (a bucket walk, no allocation), D1760 and D16E0 (string scans): no lock and no thread-local state (OBSERVED); the
  loader thread's inserts race readers as they did with the engine thread alone.
- Medium, M2: a DOL lightmap binding dropped its `_atlas000` candidate when the set object was put first; the bindings now keep
  four holders (`set_first` marks the set object).
- Medium, M3: a binding whose first candidate had no holder was never looked up again while its record was reused; such a
  binding now takes the lookup again (the kept choice, renewed after 500 ms or a registry move) and rebinds on new holders, and
  a kept holder found invalid clears its choice so the fresh admission looks it up again.
- Low: the planned-programme cache per thread keyed by a process-unique scene id (L1); a pool job run from inside a pool job
  runs alone (L2). Not changed: two candidates of the same entity in one walk may now take the record in another order than
  one thread would (L3, a valid outcome either way), and `FlushNativeAdmissionMarks` and kprofile phase times count the caller's
  thread only (L4, diagnostics).
- m45 after the corrections, back to back with the 3b build (i70-native-53 / -54): 3b 7.01 ms and corrected 5.71 ms collect, but a headless Ghidra of another thread ran through run 53 (its prepare and recording slower by the same 20 %); the clean runs agree, 3b 5.67 ms (48) and corrected 5.71 ms (54), 60 frames/s, the m45 picture equal to point 2's within 2 % per region.
- Added guard: with `record-revalidation.flag` every 64th parallel pass-row preparation is made again on one thread and
  compared row by row (`native_pass_rows_parallel_check`).

The point-2 build before its fix (i71-build) refused every reuse as `holder` (a holder holding nothing was taken as invalid:
1,076,939 in i70-native-30); runs 30-34 are void.

## Uncertainty / unresolved questions

- Whether the original shows the distant-enemy disappearances at the same place (not compared live).
- The render worker's prepare (5.4 ms) and recording (4.2-4.7 ms) at m45 are now the longer path for 120 frames/s: point 3c.
- The pool is sized from `hardware_concurrency` (7 workers on Thomas's 5800X3D, 16 threads); the engine's own job threads are
  not coordinated with it.

## Artifacts produced or modified

- `native/frame/game_bootstrap/05_selected_scene.inc`, `08_model_programmes.inc`, `09_model_admission.inc`,
  `10_options_and_passes.inc`, `12_scene_collect.inc`, `01_state.inc`; [ER-STUDY](ER-STUDY.md), [POINT3-PLAN](POINT3-PLAN.md),
  [Watch-LoadBalancer.ps1](Watch-LoadBalancer.ps1).

## Suggested next step

Point 3c after ER 141B3E890 / 141B3D940: the prepare's kept-draw rows checked in chunks on workers and the recording into
chunk command lists executed in chunk order; then loading and threading (texture decoding off the engine thread; that cause was refuted by I-0074, K-0457: no texture was decoded on the engine thread, which waited on DS3's blocking join of the texture fragments).
