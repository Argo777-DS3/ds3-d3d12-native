# I-0083 - Parcours rejoué et seuil bloquant

Capability: C3 (Wave 2, stability tools, D-0200 third item)

## Question

Can one recorded path be replayed identically at every workstream, measured at the points the original shares, and refuse a
merge into `main` whose native code makes it worse than the current `main` (frames over 34 ms first, Thomas's metric), without
blocking a merge on run-to-run noise?

## Scope

Included:

- The path's commands written once as data (a file of timed key and mouse events) and one replayer that plays them in a
  single process with a high-resolution clock (the I-0079 pilots start one PowerShell per key hold, so each hold drifts by
  the start-up of a process and a 300 ms foreground wait). The path keeps I-0079's Firelink walk first, so the numbers stay
  comparable, then the sprint and the camera turn.
- The gate: two measure runs (measure mode, no debug layer, checked in each run's `baseline.json`) and one validation run
  (the debug layer: a D3D12 error ends the native run, so the validation run passes when the path completes with no fault);
  the counts at the native frame end (frames over 17/25/34 ms), the game step at the world update gate, the frames held two
  refreshes or more; no fault, no refusal, no crash report; the picture at the standstill before the motion compared with a
  reference picture of `main`.
- The same measurement points as the original (I-0081's lesson): the original's present pacer as in I-0079, plus its game step
  at the world update gate, which no original run has measured yet (I-0081 « open »): the original probe also times that gate
  with the native's own code. The original runs once on the path as the reference shown beside the gate, it does not run at
  every merge.
- The threshold: thresholds calibrated on runs of the current `main` (the spread of the counts), stored in the repository
  with the numbers they come from; lowered when a workstream improves the path, raised only by Thomas.
- The validation hook: a gate record in the repository naming the `native/` tree that passed; `scripts/Test-Project.ps1`
  refuses `main` (or a branch checked with `-MergeGate`) whose `native/` tree has no passing record, and reports it as a
  warning on a work branch; WORKFLOW's merge procedure runs the gate before the merge.

Excluded:

- Changing the renderer's performance (this workstream measures and gates; the engine thread's third tranche is later).
- Other zones and the zone travels (the night endurance test, D-0200 fifth item); input injection inside the game process.

## Inputs

- I-0079 and I-0081 runners and readers (`Run-Stutter.ps1`, `Get-Stutter.ps1`, `Get-StutterSummary.ps1`), their numbers on
  main (walk 58-71 frames over 25 ms and 9-11 over 34 ms before I-0081; 40-56 and 7-11 after), K-0462, K-0463.
- I-0082's crash record (`crashes/` beside the journal, `native_failure`, `refusal`, `native_device_removed` rows), D-0201.
- Eta's Shebang save at the Firelink shrine bonfire (hash in `Run-Stutter.ps1`), the D-0116 live conditions.
- ER (D-0195): the shipped Elden Ring has no replayed path or performance gate in the code studied (I-0080 ER-STUDY lists its
  pipeline precompile list as the only replayed data); ER's known defect here is that its stutter shipped (PUBLIC). The gate
  is the improvement: a regression of the slow frames cannot reach `main` unmeasured.

## Hypotheses

- H1: replayed from one process, the path's run-to-run spread on the same build is smaller than with the per-hold pilots.
- H2: a threshold set from that spread separates the current `main` from a build with a known regression (I-0081's « tried
  and removed » programme warm gave 63-82 frames over 25 ms against 38-46).

## Expected evidence

Four or more measure runs of `main` on the replayed path (spread per metric); one original run with both points; one run of a
deliberately worse build (a regression flag or a kept build of I-0081, not a code change) refused by the gate; the gate's
record checked by `Test-Project.ps1` (passing on the gated tree, failing after a change to `native/`).

## Exit conditions

- Success: the replayer, the gate and its record in place; the thresholds calibrated with their numbers; the regression run
  refused and `main` accepted; Test-Project enforcing the record; WORKFLOW's merge procedure updated.
- Refuted: the spread on the same build is so large that no threshold separates a known regression (then the gate counts on
  more runs, or measures a longer path; recorded).
- Insufficient information: the replay cannot keep the same path (the character drifts from it); recorded with the runs.
- Stagnation: no new discriminating evidence after 3 cycles.
- Budget exhausted: close the tranche and replan under `.project/WORKFLOW.md`.

## Safety and human interaction

Live runs under D-0116/D-0117 through `Run-StartupTrace.ps1` (offline, protected files restored by hash, nothing installed in
the game folder), each series announced in the thread at least 5 minutes before with the PC clock; Eta's test save only, Alpha
untouched. The gate itself becomes a merge step; changing a threshold upward stays Thomas's decision.
