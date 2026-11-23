# I-0093 result: the reflection view

## Conclusion

Complete (2026-10-23, gate i93-final PASS): the water reflection view carries its models' instance block tables from one collect to the next, as the
main view does (C7-1c), in a store of its own. Unlocked at m45 the reflection's collect falls from 8.0 % to 4.9 % of the engine
thread's samples (`i92-09-prof-pool` against `i93-07-prof-carry`), the collect from 3.62 to 3.39 ms a frame, the frame from 140.0 to
144.0 frames/s (p50 7.08 to 6.89 ms, p99 8.17 to 7.96; two runs each, `main` 6ec1532e against the change in one series); the
reflection's picture is the same (`reflection-640x360-200.rgba`, mean absolute difference 1e-5 of the half-float channels, against
2e-5 between two earlier builds that did not touch it).

Before: every reflected model (35 at m45) planned and packed its instance blocks again every frame (`PlanNativeProgramTables`
3.2 %, the constant packing 2.0 %, the heap 1.3 % of the engine thread), because the main view's carry was main-view only: the
reflection's instance records differ from the main view's (the mirrored clip plane), so it could not share them. Now the admission
takes its carry from `native_reflection_instances` while preparing the reflection (the same entry type and test: the same model,
fade count and both instance records' bytes and written masks), swept every 120 collects and cleared with the reflection's tables
(a camera change of the carried tables, a K-shadow mismatch, `ResetNativeSelectedScene`); the model records stay the main view's
(the reflection only borrows them, `main_view`). ER: its view constants are re-uploaded only when changed (ER-STUDY §5); its planar
reflection was not studied (this step extends an existing carry, no new mechanism).

## Measurements (one process a run, measure mode without the debug layer, unlocked on the tearing chain)

| Run | build | frames/s | frame p50 / p99 ms | DS3 / collect ms | worker job ms |
|---|---|---|---|---|---|
| i93-01 m45 | main | 139.5 | 7.10 / 8.18 | 3.47 / 3.65 | 6.18 |
| i93-03 m45 | main | 140.6 | 7.05 / 8.16 | 3.47 / 3.59 | 6.17 |
| i93-02 m45 | carry | 143.9 | 6.89 / 8.01 | 3.52 / 3.38 | 6.10 |
| i93-04 m45 | carry | 144.1 | 6.89 / 7.92 | 3.48 / 3.40 | 6.14 |
| i93-05 walk | main | 191.3 | 5.19 / 6.00 | 2.63 / 2.52 | 4.11 |
| i93-06 walk | carry | 225.3 | 4.27 / 5.82 | 2.47 / 1.57 | 4.04 |

Observations:
1. The reflection scene is the same: 35 models, 36 rows at m45 in both builds (`native_reflection_view`).
2. The walk's medians vary between runs more than the gain (I-0091 observation 4); its p99 6.00 to 5.82.
3. Pictures: the m45 standstill main against carry 0.59 and 0.63, two main runs 0.38 (`Compare-I91Pictures.ps1`, no cell over 120);
   the reflection target itself equal (above).
4. Guards on the Firelink walk (`i93-09-check-carry`): the serial walk check 0 in 64, the pass rows' parallel check 0 in 64, engine
   bytes 0 differences over 5,760 checks, the kept draws' shadow 0 mismatch in 20,105; the debug layer at m45 (`i93-08`) 0 D3D12
   error, no fault.

## Reviews

Opus review of the diff: no High; M1 the reflection never runs the K shadow, so its carry could only be invalidated by its own (never set) mismatch: a main-view K mismatch now drops the reflection's tables and instance carry too (the reflection's tables are otherwise covered by the pure-function argument: the instance blocks come only from the programme interfaces and the instance record bytes, the clip plane included); L1 the reflection store is now cleared on every collect that publishes no reflection (an early return, a failure); L2 two stale comments made exact; L3 `kprofile::InstanceTablesCarried` now counts the reflection's reuses too (a comparison of that counter with runs before I-0093 shows the rise).

## Gate

`i93-final` on the final tree (commit d750c684's `native/`), the first with the eight-visit memory leg (D-0206): PASS. Frames over 34 ms at the frame end 11.5, over 25 ms 98, held two refreshes 14.5, game steps over 34 ms 1; pictures 0.54 and 0.64; memory 12.4 MB a visit (the titles' Theil-Sen slope).

## Evidence

Runs `evidence/I-0066/artifacts/i93-01..09-*` (`Run-I93Series.ps1`, `evidence/I-0093/artifacts/series-1.log`), build
`evidence/I-0093/artifacts/i93a-build`, the profile `i93-07-prof-carry/sample-profile-report.txt`.

## Uncertainty / unresolved questions

- One evening, one PC, two runs a configuration at m45, one on the walk.

## Artifacts produced or modified

`native/frame/game_bootstrap/05_selected_scene.inc`, `09_model_admission.inc`, `12_scene_collect.inc`;
`investigations/I-0093-reflection-view/` (CONTRACT, RESULT, `Run-I93Series.ps1`).

## Suggested next step

BOTTLENECKS item 5 (DS3's own steps under the native renderer: the observers on the update path, P9), after a re-rank by
`i93-07-prof-carry` (m45 unlocked 6.9 ms): the collect 47.7 % of the engine thread (the admission 22 %, the main walk 8.5 %, the pass
rows 8.1 %, the reflection 4.9 %), the effects' update 3.9 %, DS3's own work 3.5 ms. Then, in Thomas's order (2026-10-23): the
memory and VRAM workstream, the stutter, the exit fault.
