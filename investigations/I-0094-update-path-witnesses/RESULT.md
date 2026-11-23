# I-0094 result: DS3's work under the native renderer

## Conclusion

Complete (2026-10-23, gate i94-final PASS): the seventeen pass-through witnesses the module kept on DS3's update and UI paths in a native run (they only
logged rows of earlier investigations) are now installed only with `update-observe.flag`; the Reference mode keeps them. The
measured gain is nil at m45: DS3's own work before the collect 3.50 ms with them, 3.48 ms without (two runs each, 144.2 against
144.2 frames/s); on the Firelink walk 2.72 against 2.47 ms (one run each, within the walk's run-to-run spread). H1 (the observers
are a measurable part of DS3's slower functions under the native renderer, ER-STUDY P9) is not supported at m45: what makes DS3's
own work slower under the native renderer than under the original (I-0078, about 0.5 ms) is not these hooks. The change is kept as
the rule « diagnostics run only behind their flag » asks (WORKFLOW, Optimise, do not stack): it costs nothing and leaves the play
path without seventeen detours, two of which took a lock and a map lookup at every call.

Kept installed (functional or measuring): the menu movies' advance (the pilots wait for its `ui_movie_first_advance` rows: review
High 1, taken out of the witness list), the texture bind (the default-bound texture registry), the characters' schedule (the test
tools and the witness moves), the world update gate (the game step measure of the gate).

## Measurements (build `i94a`, one process a run, measure mode without the debug layer, unlocked)

| Run | witnesses | frames/s | frame p50 / p99 ms | DS3 / collect ms |
|---|---|---|---|---|
| i94-01 m45 | on | 143.4 | 6.90 / 8.08 | 3.51 / 3.40 |
| i94-03 m45 | on | 145.0 | 6.83 / 7.88 | 3.48 / 3.36 |
| i94-02 m45 | off | 144.5 | 6.86 / 7.95 | 3.47 / 3.39 |
| i94-04 m45 | off | 143.9 | 6.90 / 8.00 | 3.49 / 3.40 |
| i94-05 walk | on | 193.5 | 5.12 / 5.97 | 2.72 / 2.37 |
| i94-06 walk | off | 198.7 | 5.03 / 6.34 | 2.47 / 2.50 |

Pictures (m45 standstill) on against off 0.58 and 0.26, two runs with them 0.58: the same. The debug layer (`i94-08`) and the menu
check (`i94-09`, the in-game menu and the Status screen) 0 D3D12 error, no fault. The build `i94a` measured still had the movie
advance among the witnesses (fixed after the review; the measured hooks are a superset of the final ones).

## Reviews

Opus review of the diff: High 1, the pilots (`Pilot-LoadGameThomas`, `Pilot-CharMakeWitness`, `Run-AlphaCast`, the test-tool and
launcher tests) wait for the `ui_movie_first_advance` rows only `ObserveUiMovieAdvance` writes: taken out of the witness list. Low 1:
with `ffx-observe.flag` and without `update-observe.flag` the effect observer now hooks the entity update itself (its count kept).
Low 2: the `world_update_probe` row shows the first bytes of hooks that are no longer installed without the flag (no script reads
it; noted here). No other reader of the witnesses' state, rows or original pointers in `native/`, `scripts/` or `investigations/`.

## Gate

`i94-final` on the final tree (commit a47897ed's `native/`): PASS. Frames over 34 ms at the frame end 13, over 25 ms 94, held two
refreshes 14.5, game steps over 34 ms 2.5; pictures 0.58 and 0.55; memory 0 MB a visit (the titles' Theil-Sen slope); no fault in
the five runs (`evidence/I-0094/artifacts/gate-i94-final.log`).

## Evidence

Runs `evidence/I-0066/artifacts/i94-01..09-*` (`Run-I94Series.ps1`, `evidence/I-0094/artifacts/series-1.log`), build
`evidence/I-0094/artifacts/i94a-build`, the profile `i94-07-prof-clean`.

## Uncertainty

One night, one PC; the walk one run each.

## Artifacts produced or modified

`native/frame/game_bootstrap/22_setup.inc`; the runner's flag (`Run-StartupTrace.ps1`); `investigations/I-0094-update-path-witnesses/`.

## Suggested next step

BOTTLENECKS is exhausted (items 1 to 5: I-0090 to I-0094). DS3's own work stays about 3.5 ms at m45 unlocked; I-0078 measured DS3 functions slower under the native renderer than under the original (8F3506, 608467, D58DF4); INFERRED: the pools' threads and the module's memory traffic sharing the caches, not the hooks. Next in Thomas's order (ROADMAP): the memory and VRAM workstream,
then the stutter, then the exit fault.
