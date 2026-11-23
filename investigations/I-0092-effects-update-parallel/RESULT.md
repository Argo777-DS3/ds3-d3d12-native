# I-0092 result: the effects' update in parallel

## Conclusion

Complete (2026-10-23, gate i92-final-b PASS): the effects' node refills and particle items run on the admission pool in DS3's own ranges of 10, the
engine thread helping. Unlocked at m45 the effects' update falls from 6.1 % to 3.8 % of the engine thread's samples
(`i91-18-prof-s123` against `i92-09-prof-pool`), DS3's own work before the collect from 3.68 to 3.47 ms a frame (the SFX step is in
it), the frame 136.7 to 139.0 frames/s (p50 7.25 to 7.14 ms, p99 8.36 to 8.20, the mean of two runs each); on the Firelink walk
DS3's work 3.07 to 2.53 ms (two runs each), the walk's frames/s medians varying as in I-0091 (serial 202 and 175, pool 190 and
228; p99 6.40-6.44 against 5.62-5.96). H1 is supported (the parallel parts left the thread; the library update, DS3's slot 3
inside the manager, 2.2 %, stays serial), H2 too (same pictures).

The change, after DS3's single-threaded branch of the scene control's slot 8 (decompiled for this investigation, CONTRACT): DS3
itself hands its update list (`FUN_140296FF0`) and its node refills (`FUN_14025B870`) to ranged GX jobs of `min(count, 10)` items
run by the GX workers with the caller helping (INFERRED: the generic job's run slot was not read; FFX-DRAW §2 saw `296FF0` on a GX job thread, and the original's multi-threaded branch gives each GX worker its own update context) (`FUN_1400B6480`, `FUN_1400D1C40`, `FUN_1400D1D30`); the particle item
(`FUN_141ADC100` -> `FUN_141ADD510`) reads only its own node and draws from a per-thread generator (`FUN_141ACADB0` -> TLS slot
`FUN_141ACB000`). Our transcription ran both on one thread in list order; `effect_system::RunRanges` now takes them by tickets of
10 on `AdmissionPool()` (free in DS3's SFX step: the collect uses it later in the DrawStep row), joined before the passes are
destroyed. ER (ER-STUDY §4) spreads its effects' update over the GX pool the same way; its barrier polling with `Sleep(0)` is not
copied (the pool's condition-variable join). The per-item kind counts (a diagnostic: a map lookup an item each frame) are now taken
only for the update whose row is logged. A/B: `serial-effects.flag`.

## Measurements (build `i92a`, one process a run, measure mode without the debug layer, unlocked on the tearing chain)

| Run | effects | frames/s | frame p50 / p99 ms | DS3 / collect ms | worker job ms |
|---|---|---|---|---|---|
| i92-01 m45 | serial | 136.9 | 7.24 / 8.37 | 3.67 / 3.58 | 6.07 |
| i92-05 m45 | serial | 136.5 | 7.25 / 8.34 | 3.68 / 3.58 | 6.10 |
| i92-02 m45 | pool | 138.1 | 7.18 / 8.27 | 3.49 / 3.69 | 6.32 |
| i92-06 m45 | pool | 139.9 | 7.09 / 8.12 | 3.45 / 3.64 | 6.10 |
| i92-03 walk | serial | 202.1 | 4.81 / 6.40 | 2.97 / 1.79 | 4.01 |
| i92-07 walk | serial | 174.7 | 5.67 / 6.44 | 3.17 / 2.50 | 4.09 |
| i92-04 walk | pool | 190.0 | 5.22 / 5.96 | 2.61 / 2.53 | 4.13 |
| i92-08 walk | pool | 227.5 | 4.32 / 5.62 | 2.45 / 1.77 | 4.07 |

Observations:
1. The counts are the same work: `native_effect_update` m45 nodes refilled and particle items run every update in both modes
   (2.93 and 2.95 million items over the run, 1.80 and 1.81 million refills).
2. The m45 collect is 0.08 ms higher with the pool (3.58 to 3.66, two runs each): INFERRED the pool's threads, woken in the SFX
   step, not yet idle or their caches warm for the admission; within the frame's gain (-0.11 ms p50).
3. The walk: as in I-0091, the median mixes a standing settle and the walk; split by segment the collect is the same in the two
   modes (settle 1.64-1.69 ms), DS3's work is not split by segment.
4. Pictures (`Compare-I91Pictures.ps1`, m45 standstill with the bonfire): serial against pool 0.34 and 0.40, two serial runs 0.40,
   no cell over 120.
5. The debug layer at m45 with the pool (`i92-10`): 0 D3D12 error, no fault.

## The exit heap fault (Thomas, 2026-10-22 23:36, « essayer de revenir au menu principal puis de quitter normalement »)

`i92-11-quit-normal` (the launcher's build of `main` bbf22ff1, m45 save, `exit-dump.flag`): in game Pause > System > Quit Game
(« Save the game and return to title menu ») > Yes, then at the title menu Quit Game (screenshots `q1` to `q12` beside the trace):
the game ends by itself (exit code 0) and the exit heap fault `0xC0000374` is raised all the same, this time with a minidump
(`crashes/startup-crash1.dmp`, 828 KB; the report could not open the faulting thread, gone with the process). The bounded runs end
by the module's stop and `TerminateProcess` after 3 s; the gate's memory leg and the endurance close the window's close box. I-0082
§5 saw the same fault in the Reference mode (the original renderer with our observer). Not analysed here (no debugger on the PC;
the dump is kept for that analysis).

## Reviews

Opus review of the diff: no High; M1 the claim that DS3 runs these bodies concurrently rests on the ranged jobs and the multi-threaded branch's per-worker contexts, not on the job's run slot (marked INFERRED here and in CONTRACT); M2 the particle generator's TLS index is allocated unsynchronised at its first use (`FUN_141ACB000`): now taken once on the engine thread before any range runs on the pool; lows applied (`ItemKinds`' comment, `<cstdint>` in `work_pool.h`, the runner's comment); left: the pool used from more than one range (10 items), kept as measured (L1), and the A/B flag also gating the kind counts (L3: the counts' saving shows only against `main`).

## Gate

Replayed path gate on the final tree (commit b1f971eb's `native/`): `i92-final` FAIL on the memory leg alone (+48 MB a visit against 40; the titles after the four visits 7,335, 7,594, 7,712, 7,690 MB; frames over 25 ms 94.5, over 34 ms 13, pictures equal). The leg was then run again on its own: I-0092 -12.5 and +53.5, `main` without I-0092 +47.5 MB a visit; then eight visits each (`i92-mem8-main`, `i92-mem8-pool`): both builds' title memory toggles between about 7,600 and 7,710 MB (`main` 7,623, 7,600, 7,602, 7,617, 7,609, 7,704, 7,697; I-0092 7,617, 7,604, 7,600, 7,709, 7,609, 7,708, 7,708), a state of about 100 MB present or absent at the measurement, no continued growth, the same in the two builds; the frame store held about 20 MB in every leg. Thomas (2026-10-23 01:24, « Je suis votre recommandation »): the gate run again, `i92-final-b` PASS: frames over 34 ms at the frame end 12, over 25 ms 92, held 15, game steps over 34 ms 1.5, pictures 0.64 and 0.60, memory -7.5 MB a visit. The four-visit leg's two-point estimate reads the 100 MB state as ±50 MB a visit: a robust leg follows (Thomas, 2026-10-23 01:51).

## Memory at the title against the original (Thomas, 2026-10-23 01:01, « faudrait regarder l'original »)

`i92-mem-original` (the bare original, D3D11 without our module, `Run-Endurance.ps1 -Original -OnlyZones 'Beta m31' -Visits 4`; the same counter as the gate's memory leg: the process's private bytes sampled from outside every 5 s, the median of each title stay, timed from the tour's log): the title before the first load 5,886 MB (native 5,863), after visits 1 to 3 6,487, 6,437, 6,427 MB (native 7,342, 7,623, 7,600), in m31 7,222 to 7,565 MB (native 8,050 to 8,300). After a zone visit the native backend keeps about 1.1 to 1.2 GB more than the original at the title and about 0.8 GB more in the zone; a candidate workstream. The gate's memory leg is now the eight-visit robust slope (D-0206).

## Evidence

Runs `evidence/I-0066/artifacts/i92-01..11-*` (`Run-I92Series.ps1`, `evidence/I-0092/artifacts/series-1.log`,
`quit-normal.log`), build `evidence/I-0092/artifacts/i92a-build`, the DS3 decompilation exports `evidence/I-0092/artifacts/ghidra-ffx*`
(headless Ghidra on the read-only mirror `ghidra-mirror-i0066`), the profile `i92-09-prof-pool/sample-profile-report.txt`.

## Uncertainty / unresolved questions

- One evening, one PC, two runs a configuration; the walk's medians do not separate the modes (observation 3).
- The particles' random draws now come from several threads' generators (as in the original, whose GX workers run them); the
  sequences differ from the serial order, which no picture comparison can show at a standstill.

## Artifacts produced or modified

`native/frame/effect_system.cpp`, `native/frame/effect_system.h`, `native/frame/game_bootstrap/21_effects.inc`, the runner's flag
(`Run-StartupTrace.ps1`); `investigations/I-0092-effects-update-parallel/` (CONTRACT, RESULT, `Run-I92Series.ps1`).

## Suggested next step

Re-rank by `i92-09-prof-pool` (m45 unlocked, 7.14 ms): the collect 50 % of the engine thread (the model admission about 23 %, the
reflection's collect 8 %, the main walk 8 %, the pass rows 7 %), DS3's own work (its steps, the effects' library update 2.2 %). The
reflection view (BOTTLENECKS item 4) and the admission's reuse path are the next candidates; the exit heap fault's dump is a
separate, small investigation.
