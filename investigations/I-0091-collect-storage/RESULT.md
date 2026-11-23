# I-0091 result: the collect's storage

## Conclusion

Complete (2026-10-22): the collect's per-frame storage is off the engine thread's path. Unlocked at m45 the collect falls from
3.99 to 3.58 ms a frame (-0.41 ms; the base one run, the three steps the mean of two), the frame from 128.4 to 136.8 frames/s (p50 7.73 to
7.25 ms, p99 8.75 to 8.33), DS3's own work unchanged (3.74 to 3.67); the engine thread's `memset` 5.5 % to 0.7 % of its samples,
the two arrays' value-initialisations (5.5 % and 4.0 % in `i90-07`) gone. H1 and H2 are supported, H3 at m45 (+8.4 frames/s
against about 5 expected); on the Firelink walk the runs vary more than the gain (171 to 175-206 frames/s, below).

Three steps, made after Elden Ring's per-frame memory (I-0086 ER-STUDY §5: `GXBS::GXLinearAllocator`, a frame arena kept and reset
at each scene render, 141B452B0 re-read by Opus; per-context chunked bump allocators without lock; an entity's record written once
where it stays) without its defects (the arena's spin lock per chunk refill: ours takes a lock per block, a few a frame; ER
rebuilds every entity's draw data each frame with no change test: ours keeps its records, I-0070, I-0074):

- **Step 1, the collect's frame store** (`native/frame/frame_store.h`): blocks by power-of-two class from 64 KiB kept between
  frames (8 a class, 160 MiB at most), returned by whichever thread releases a scene and taken again by the next collect, so the
  pages a frame writes stay mapped; `frame_store::Array<T>` (a vector's subset over it) holds the scene's models and pass rows.
  A/B `no-frame-store.flag` (every block back to the system). The store is never destroyed (a scene released by a detached
  thread at exit still returns its blocks). Counters: `native_frame_store` every 600 collects.
- **Step 2, the places constructed once**: an admitted model is built in its candidate's slot (a guard empties the slot of a
  refused admission) and move-constructed once, on the pool, into a place nothing initialised before (`Array::Uninitialized`,
  then `Commit`; the moves cannot throw, the array refuses any other change while places are pending); the pass rows are built
  in place in their chunk (`emplace_back`) and moved once into theirs; the chunks and their storage are kept by the collecting
  thread from one main collect to the next. Before: a model was constructed on the stack, moved into its slot, its place in the
  scene value-initialised (1,686 models of 1,320 bytes at m45, by the engine thread alone) and assigned again; a row built as a
  temporary, moved into a chunk created that frame, its place value-initialised (2,975 rows of 576 bytes) and assigned again.
- **Step 3, the rest of the collect's per-frame heap named by the profile**: the shadow walk's map of the admitted models (one
  heap node a model each collect, 1 % of the engine thread) is one open-addressed table (`NativeEntityIndex`, one allocation a
  collect); the constant packing's range and written scratch (`PackShaderConstant`, two allocations a packed block) are kept by
  each thread.

## Measurements (one process a run, measure mode without the debug layer, unlocked on the tearing chain)

Medians over the in-game windows (`Get-I91Summary.ps1`). Builds: `i91a` (step 1, its base with `no-frame-store.flag`),
`i91b` (steps 1-2), `i91d` (steps 1-3), `i91e` (the final tree, after the final review). The engine thread's stamps: DS3's own work before the collect, the collect.

| Run | Build | frames/s | frame p50 / p99 ms | present p50 / p99 | DS3 / collect ms | worker job ms |
|---|---|---|---|---|---|---|
| i91-11 m45 base | a + flag | 128.4 | 7.73 / 8.75 | 7.75 / 9.02 | 3.74 / 3.99 | 6.31 |
| i91-02 m45 s1 | a | 130.6 | 7.58 / 8.77 | 7.60 / 9.07 | 3.67 / 3.93 | 6.23 |
| i91-03 m45 s12 | b | 134.6 | 7.37 / 8.49 | 7.39 / 8.81 | 3.64 / 3.73 | 6.21 |
| i91-12 m45 s12 | b | 135.4 | 7.33 / 8.34 | 7.35 / 8.62 | 3.63 / 3.70 | 6.16 |
| i91-13 m45 s12 | b | 133.3 | 7.42 / 8.70 | 7.44 / 9.14 | 3.70 / 3.74 | 6.20 |
| i91-14 m45 s123 | d | 137.1 | 7.24 / 8.29 | 7.25 / 8.64 | 3.66 / 3.56 | 6.04 |
| i91-17 m45 s123 | d | 136.4 | 7.26 / 8.36 | 7.28 / 8.70 | 3.67 / 3.60 | 6.09 |
| i91-04 walk base | a + flag | 171.3 | 5.79 / 6.64 | 5.78 / 6.83 | 3.23 / 2.54 | 4.21 |
| i91-05 walk s1 | a | 174.4 | 5.57 / 7.19 | 5.58 / 7.36 | 3.02 / 2.62 | 4.40 |
| i91-06 walk s12 | b | 194.5 | 5.04 / 6.58 | 5.06 / 6.95 | 3.08 / 1.95 | 4.34 |
| i91-15 walk s12 | b | 206.0 | 4.83 / 6.63 | 4.79 / 7.05 | 2.92 / 1.94 | 4.10 |
| i91-16 walk s123 | d | 179.2 | 5.50 / 7.22 | 5.49 / 7.36 | 2.97 / 2.58 | 4.29 |
| i91-19 walk s12 | b | 175.1 | 5.65 / 6.52 | 5.66 / 6.64 | 3.12 / 2.50 | 4.16 |
| i91-21 walk s12 | b | 203.1 | 4.88 / 6.38 | 4.88 / 6.62 | 2.91 / 1.92 | 4.12 |
| i91-22 walk s123 | e (final) | 202.5 | 4.88 / 6.06 | 4.84 / 6.46 | 2.92 / 1.89 | 4.06 |

Observations:
1. Step 1: the store serves every large array again (`native_frame_store`: 3.8 blocks a collect asked of the system and given
   back in the base, 0.001 with the store, 1.9 to 3.8 taken again; 1.9 to 4.8 MB held free between frames). The collect -0.06 ms
   at m45, 128.4 to 130.6 frames/s.
2. Step 2: the collect 3.93 to 3.72 ms at m45 (three runs 3.70-3.74), 130.6 to 134.4 frames/s (133.3-135.4). The sample profile
   (`i91-07-prof-s12` against I-0090's `i90-07`, m45, 30,000 samples of the engine thread): `memset` 5.47 % to 0.66 %, the two
   value-initialisations (`vector<NativeSelectedModel>::resize` 5.45 %, `vector<PreparedMesh>::resize` 3.96 %) gone, the models'
   default construction 0.96 % to 0.50 % (now once, in the slot).
3. Step 3: the collect 3.72 to 3.58 ms at m45 (two runs), 134.4 to 136.8 frames/s, p99 8.5 to 8.3. Profile `i91-18-prof-s123`:
   the engine thread's `RtlAllocateHeap` 5.41 % to 4.45 % and `RtlFreeHeap` 1.60 % to 1.08 % (leaf), the shadow walk's heap calls
   (0.94 %) gone, the constant packing's two scratch allocations gone (its output block stays); the collect 50.0 % to 48.5 % of the
   engine thread. What remains of the heap on the collect is the blocks it makes (the planned programmes' tables, the instance
   block table, the lighting's per-frame lists), not storage.
4. The Firelink walk: the median mixes a standing settle (about 430 models, collect 1.7-2.1 ms whatever the build), the walk and
   an end view whose collect depends on where the walk ends (2.6 to 3.7 ms between runs of one build). Steps 1-2 gave 194.5 and
   206 frames/s against 171-174, step 3 179 frames/s with its settle segment 0.3 ms slower and its end 0.3 ms faster than the
   second s12 run. After the final review (M1) two more pairs in one series (`series-4.log`): s12 175.1 and 203.1 frames/s,
   s123 202.5 (p99 6.06 against 6.38 and 6.52; `i91-20`, the other s123 run, refused at launch: `prefix_refused`, the game's
   code bytes not yet the pinned ones when the module checked them, before any of the module's code ran; the next run with the
   same build started). Split by segment, the collect is the same in the two builds (the settle 1.66-1.68 ms, the walk 3.46-3.48,
   the end view 3.59-3.63 for the runs that end at the same view; i91-16's settle 1.97 is that run's alone): the medians differ
   by how many windows each segment gives, not by build. Step 3 is neutral on the walk.
5. Pictures (`Compare-I91Pictures.ps1`, 160x90 cells, mean channel difference 0-255): m45 base against s12 0.35-0.43, s1 against
   s12 0.41, the walk's standstill 0.53, no cell over 120 (the same scene; two runs of one build differ by about 0.36); the walk's
   moving picture differs equally base-s1 and base-s12 (2.7, the walk's timing).
6. Guards on the Firelink walk with steps 1-2 (`i91-08-check-s12`, `record-revalidation`, `prepare-shadow`): the serial walk check
   0 mismatch in 64, the pass rows' parallel check 0 in 64, engine bytes 0 differences over 5,880 checks, kept draws' shadow 0
   mismatch in 20,105; the debug layer at m45 (`i91-09`) and the menu check (`i91-10`, the in-game menu and Status screen) 0 D3D12
   error, no fault in any run.

## Reviews

Step review of steps 1-2 (Opus): no High; M1 the parallel rows' chunks were a function static (now `thread_local` and never
destroyed, reset without allocating), lows applied (the array refuses any change while places are pending; rows built in place;
the redundant rows hint removed; the A/B flag's coverage stated: it does not cover the kept chunks, which build `i91a` does not
have). Final review (Opus, the whole diff): no High; M1 step 3 kept on m45 while its one walk run moved the other way (two more walk pairs run, observation 4: neutral); lows applied (the array's clear refuses pending places and a block with pending places is kept away; the pass sink's Reset beside its fields; the serial rows reserved from the model count; the summary's guard for a run without I-0090's row); left: the shadow walk's index takes one fresh 128 KiB table a collect (negligible, measured nowhere).

## Gate

Replayed path gate `i91-final` on the final tree (commit 13a65a7e, build `evidence/I-0083/gates/build-757205c8e858`): PASS. The means of the two measure runs: frames over 34 ms at the frame end 14.5 (I-0090's gate 13), over 25 ms 87.5 (99.5), held two refreshes 17.5 (17.5), game steps over 34 ms 2 (1.5); pictures 0.54 and 0.63; memory leg -2.5 MB a visit; the validation run clean. The memory leg's quit raised the pre-existing exit heap fault 0xC0000374 (exit code 0, no dump; I-0082 §5, also in I-0090's gate). The original on the same path (I-0083): 14 and 3.

## Evidence

- Runs `evidence/I-0066/artifacts/i91-01..18-*` (`Run-I91Series.ps1`; series logs `evidence/I-0091/artifacts/series-1.log` to
  `series-3.log`; `i91-01` refused at launch, the runner did not know `no-frame-store`, run again as `i91-11`), builds
  `evidence/I-0091/artifacts/i91a-build` (step 1; its unit run stopped at the YEBIS glare controls, whose reference captures were
  missing from the temporary checkout it was built from: an environment gap, the same controls pass in `i91b` and `i91d`),
  `i91b-build`, `i91c-build` (step 3's first half, not measured), `i91d-build`; the profile reports
  `i91-07-prof-s12/sample-profile-report.txt`, `i91-18-prof-s123/sample-profile-report.txt` and the callers of the heap and
  `memset` (`evidence/I-0091/artifacts/i91-07-focus.txt`, `i91-18-focus.txt`).

## Uncertainty / unresolved questions

- One PC (Ryzen 7 5800X3D), a 1280x720 window, one evening; m45 two or three runs a configuration, the walk one or two; the walk's
  medians are not comparable between builds at this resolution of the gain (observation 4). Archdragon Peak not measured.
- Memory: the store keeps at most 8 blocks a class and 160 MiB free; the kept chunks hold the peak of the rows' capacity (about
  2 MB at m45), outside the store's counters. The gate's memory leg judges the whole.

## Artifacts produced or modified

- `native/frame/frame_store.h`, `native/frame/frame_store_test.cpp` (in the build's controls),
  `native/frame/game_bootstrap/05, 09, 10, 12, 20`, `native/frame/game_bootstrap_includes.h`, `native/content/shader_constants.h`,
  `native/frame/Build-GameBootstrap.ps1`; the runner's flag (`Run-StartupTrace.ps1`).
- `investigations/I-0091-collect-storage/` (CONTRACT, RESULT, `Run-I91Series.ps1`, `Get-I91Summary.ps1`, `Compare-I91Pictures.ps1`).

## Suggested next step

Re-rank by the profile `i91-18-prof-s123` (m45, unlocked, 7.26 ms a frame): DS3's own work 3.7 ms; the collect 3.6 ms, of which the
model admission 23 % of the engine thread (its reuse path: the record check, the programme tables' planning, the instances), the
main walk 8 %, the reflection's collect 8 % (BOTTLENECKS item 4), the pass rows 8 %; the effects' update 6 % (item 3). The render
worker's job 6.1 ms leaves the frame bound by the engine thread.
