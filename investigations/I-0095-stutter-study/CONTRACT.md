# I-0095 - Étude des saccades (the stutter study, read-only)

Capability: C3

Number: I-0095 given by the coordinator (2026-10-23 06:06 Paris); the memory workstream takes I-0096.

## Question

What are the replayed path's slow frames (over 25 ms and over 34 ms) made of, how much of the gap to the original is noise,
and which corrections, ranked by expected gain, should the stutter workstream (after the memory workstream) try first?

## Scope

Included:

- The existing measurements only (`evidence/`, the gate runs I-0083 to I-0094, the I-0079 and I-0081 profiles).
- The traversal stutter (streaming of zone parts, models, textures, pipelines while moving).
- Elden Ring's loading while moving, from the existing ER studies and the read-only ER project, without its known defects.
- The DirectX 12 Agility SDK leads (GPU upload heaps with Resizable BAR, enhanced barriers): evaluated, nothing promised.
- A measurement protocol on more runs, to be executed later by the workstream's pilot.

Excluded: any change to `native/`, any build, any live run, any Ghidra session (a live run was in progress on the PC), any
merge into `main`.

## Inputs

- Claims: K-0462 (I-0079), K-0463 (I-0081), K-0465 (I-0083), K-0475 (I-0094); decisions D-0197, D-0202, D-0207.
- Artifacts: `evidence/I-0066/artifacts/i83-*`, `i9[0-4]-final-*`, `i94-cal-*`, `i81-07-walk-prof-b` (slow-frame profile).
- Tools: `scripts/Get-ReplayMeasure.ps1`, `investigations/I-0081-motion-stutter-2/Get-StutterSummary.ps1`.

## Hypotheses

- H1: the gate pair means (94.5, 87.5, 99.5 frames over 25 ms) are within run-to-run noise.
- H2: the frames over 34 ms are one per walk cycle, at the area crossing, dominated by work outside the native phases.
- H3: the frames over 25 ms at the native frame end mostly do not reach the screen as a held refresh.

## Exit conditions

- Success: a ranked plan with expected gains (counts and 0.1 % low) and a protocol, each claim with its runs.
- Insufficient information: the plan says which measurement is missing and the protocol includes it.

## Safety and human interaction

No live run, no build, nothing installed. The protocol's runs are for the stutter workstream's pilot, announced under D-0116.
