# I-0083 result: the replayed path and its blocking gate

Complete (2026-10-21). One recorded Firelink path (walk, sprint, camera turn) is replayed from one process at every merge into
`main`; the gate built on it refused a deliberately slowed build and accepts the current one; `Test-Project` refuses `main`
whose native code has not passed it. The original was measured on the same path at the native's engine point for the first
time: its game step has far fewer frames over 25 ms than the native's (10 against 44-49 on the walk) and as many over 34 ms
(2 against 0-3).

## What was built

- **The path, recorded once** ([scripts/replay/firelink.path.json](../../scripts/replay/firelink.path.json)): Eta's Shebang save at
  the Firelink shrine bonfire, 40 s settle, then I-0079's walk (Z 4 s, S 4 s, a 10 s cycle, 8 cycles), the sprint (Space held,
  4 cycles) and the camera turn (L 5 s, 2 cycles): 132 s of motion, as data (times from the motion start).
- **The replayer** ([Invoke-ReplayPath.ps1](../../scripts/Invoke-ReplayPath.ps1)): one process, every event at its time on a
  high-resolution clock (the I-0079 pilots started one PowerShell per hold); the foreground checked every 20 ms, all keys
  released and the replay stopped when the game leaves it; the segment marks kept in memory until the end. Lateness on the gate
  runs: mean 0.7 ms, every event under 3 ms except the first (14 ms, the first call of the input path, inside the trimmed 2 s).
- **One run** ([Invoke-ReplayRun.ps1](../../scripts/Invoke-ReplayRun.ps1)): the I-0079 runner's structure (Run-StartupTrace job,
  the title passed, the standstill picture with the game's client rectangle), kinds Measure, Validation, Reference.
- **The reader** ([Get-ReplayMeasure.ps1](../../scripts/Get-ReplayMeasure.ps1)): per segment, I-0081's points (frames over
  17/25/34 ms at the native frame end or the original's pacer, the game step at the world update gate, frames held two refreshes
  or more), the frame coverage, the faults (crash reports, `native_failure`, `refusal`, `native_device_removed`), the replay
  status, the restoration. Checked against `Get-StutterSummary.ps1` on i81-27 (identical: 866/53/11, 696/42/3, 29 held).
- **The original's game step** (`04_reference_witnesses.inc`, `22_setup.inc`): the probe of I-0078 also hooks the world update
  gate 8FA3E0 and times it with the native's own code (`RecordGameStep`, factored out unchanged), nothing else read or written.
- **The gate** ([Invoke-ReplayGate.ps1](../../scripts/Invoke-ReplayGate.ps1)) and its record
  [REPLAY-GATE.json](../../.project/REPLAY-GATE.json): it builds the committed `native/` tree, runs two measure runs and one
  validation run; FAIL on a fault, on the mean of the two measure runs over a threshold, or on a standstill picture departing
  from the reference; INCONCLUSIVE on an environment problem (replay incomplete, restoration missed, fewer than three quarters of a segment's frame windows, a run not of the gate's build, thresholds calibrated on another path, a
  picture missing or taken behind another window); PASS writes the record naming the tree. Experiments never write it, and a
  calibration never certifies itself. [Test-Project.ps1](../../scripts/Test-Project.ps1) refuses `main` (or a branch under
  `-MergeGate`) whose `native/` tree is not the recorded one; WORKFLOW's merge procedure runs the gate.

## Observations (path totals over 132 s of motion unless a segment is named; measure mode, no debug layer)

| Runs | Build | frame end >17/25/34 | game step >17/25/34 | held 2+ |
|---|---|---|---|---|
| i83-cal-m1..m4 | this branch (= main's native code) | over 25: 82, 74, 83, 97; over 34: 16, 12, 13, 14 | over 34: 4, 0, 1, 2 | 51, 33, 38, 50 |
| walk only, m1..m4 | idem | 935/56/12, 902/46/8, 1002/54/9, 907/63/10 | 705/45/3, 693/44/0, 704/46/1, 688/49/2 | 33, 21, 22, 31 |
| i83-ref-original, walk | the original (pacer, gate) | 612/9/3 | 516/10/2 | - |
| i83-ref-original, total | idem | 945/14/3 | 780/15/2 | - |
| i83-reg-inline-m1, m2 | InlineRender (D-0168's worker off) | over 25: 185, 243; over 34: 12, 18 | over 34: 8, 16 | 311, 378 |

1. **Replay spread.** On the same build the walk's frames over 25 ms spread 46-63 (I-0081's per-hold pilots: 40-56 on its final
   build): the replayer removes the pilot's drift but not the spread, which is the game's own (H1 not supported). Two runs per
   gate are therefore judged by their mean against thresholds that carry that spread.
2. **Thresholds** (calibration i83-cal, formula in the record): frames over 34 ms 22, over 25 ms 105, held 62, game steps over
   34 ms 6; the standstill picture 1.64 mean difference and 0.5 % of cells (calibration pictures 0.43-0.55 from the reference;
   the native against the original at Firelink is 2.88, I-0081 shots). With counts this small, a regression of fewer than about
   8 frames over 34 ms in 132 s is not separable from noise with two runs; the frames over 25 ms and the held refreshes carry
   most of the sensitivity.
3. **The regression is refused** (i83-reg-inline, H2 supported): with the render worker off, frames over 25 ms 214 on average
   against 105, held 344 against 62, game steps over 34 ms 12 against 6; its frames over 34 ms (15 on average) stay under 22: a
   regression that moves frames from one refresh to two shows first in the 25 ms and held counts.
4. **The original at the shared point** (I-0081 « open » answered): on the walk the original's game step at the world update gate
   has 10 frames over 25 ms against the native's 44-49, and 2 over 34 ms against 0-3. The native's excess is in the 25-34 ms band
   (one refresh late, not two), the band the engine thread's third tranche targets.
5. **Validation run** (i83-cal-v1, the debug layer): the path completed with no fault (a D3D12 error would have ended it).

## The final gate

i83-final (2026-10-21 19:41-19:52, commit 438b0a35, native tree 862785aa, the calibration's): PASS. Means of the two measure runs:
frames over 34 ms 12.5 (threshold 22), over 25 ms 93 (105), held 48.5 (62), game steps over 34 ms 1 (6); pictures 0.52 and 0.48
(1.64); the validation run complete with no fault. The record in REPLAY-GATE.json; `Test-Project -MergeGate` passes on it.

## Uncertainty / open

- Firelink only, one PC (RTX, 60 Hz); the gate's sensitivity to a small regression in frames over 34 ms is limited (obs. 2).
- The picture check sees the standstill before the motion only: a rendering regression visible only in motion or elsewhere is
  not caught (the D-0181 side-by-side stays the rule for visual workstreams).
- The thresholds are main's current level, not the original's: they block regressions, they do not demand the original's
  numbers; they go down when a workstream recalibrates after an improvement.

## Reviews (Opus 5.5, D-0198)

Step review (after step 1): three High (the run plan unrolled into characters, a run with no frame rows scoring zero, `-Evaluate`
recording a pass for a tree not run), seven Medium and ten Low, all applied except L8 (the held count's window bound, kept
identical to Get-StutterSummary) and L9 (the probe refused if the gate hook fails, accepted: one pinned executable). Second
step review (after the calibration): every earlier finding fixed, no High; applied: a FAIL on the measure thresholds alone is
confirmed by two more measure runs, the four judged by their mean (N1, against the run-to-run and session-to-session spread); a
recalibration lowers a threshold only when its runs are lower beyond twice their combined standard error (N1, no drift to the
luckiest calibration); the window check counts windows at the game's rate (60 at most) with a quarter of slack, so a slow build
fails on its counts instead of stopping as inconclusive (N3; the slowed runs have 37 walk windows of 39); `-AllowRaise` only
with one of Thomas's decisions, and only with `-Calibrate` (N5); the replayer's waits warmed before its clock (N6: the first event stays 14 ms late, the input call itself, inside the trim) and a
stopped replay left `running`, not `complete`; Test-Project outside a repository (N4). Judged, not changed: the frames over
34 ms are the least sensitive count (a regression must add about 60 % before it fails; obs. 2). Final review (whole branch): the
native change is probe-only (no renderer, barrier or D3D11 path touched), no dead or neutralised code, no new console window;
applied: a gate refuses a run name already used and judges only runs of its own build (a reused tag would have re-measured old
runs for a new tree); thresholds calibrated on another path make it inconclusive; each threshold keeps the runs it comes from (a
refused calibration no longer becomes the next one's comparator); the picture thresholds under the same rise rule; a
calibration whose runs then fail is not filed; `-RecordOriginal` refuses a faulted or unrestored run; WORKFLOW merges `main`
into the branch before the gate; the regression's frames over 34 ms corrected (12 and 18). Its High (no passing record yet) is
answered by the final gate (section above).

Optimisation pass (WORKFLOW « Optimise, do not stack »): the only native change runs in reference runs (the probe's gate hook);
the native mode keeps the same game-step timer, moved into a function called from the same place, so its engine-thread
cost is unchanged; nothing is read or computed twice, no diagnostic added.

## Artifacts

Runs `evidence/I-0066/artifacts/i83-*` (i83-01-smoke refused by the path-reading bug, i83-02-smoke, i83-cal-m1..m4, i83-cal-v1,
i83-ref-original, i83-reg-inline-m1..m2, i83-final-m1, m2, v1); gate verdicts `evidence/I-0083/gates/*.json`; the reference picture
`evidence/I-0083/reference/`; builds `evidence/I-0083/artifacts/i83a-build`, `evidence/I-0083/gates/build-<tree>`; series
script and logs `evidence/I-0083/artifacts/series-1.*`, `smoke-*.log`.
