# I-0097 - DS3 DX12 Vague 2: the stutter (Saccades)

Capability: C3. Owner: Opus 5.5. Thomas's order (2026-10-23, ROADMAP): after memory and VRAM (I-0096), the stutter. Plan and
protocol from the read-only study [I-0095](../I-0095-stutter-study/STUDY.md) (§7, §8).

## Question

Can the native renderer's slow frames on the replayed Firelink path, the traversal stutter included, be brought back to the
original's level (1 % low about 50 frames/s against the native's about 36; frames over 25 ms 14 against about 93), measured on
what reaches the screen?

## Scope

Included:

- Step 0 (I-0095 item 0): the « frames held two refreshes » count made unbiased (it sampled a frame only when DXGI PresentCount
  advanced by exactly one, `native/renderer/scene_pass.cpp`), its gate threshold recalibrated; a per-frame dump of the motion
  segments; PresentMon on both renderers if Thomas allows its elevation.
- Item 1: the reload windows: the engine thread no longer waiting on the native texture creations inside DS3's joined loading
  tasks, after ER (the engine thread polls; a texture is published only when complete; a late texture delays its model, D-0197).
- Items 2 to 4 of the I-0095 plan as the measurements justify them (the crossing frame's first admission, the 56-60 ms frames,
  the kept-draw expiry period).

Excluded: the Agility SDK (no stutter gain expected, I-0095 §6), a copy queue, the exit heap fault (next workstream), first
pipelines on a new PC (I-0095 item 5) unless cheap.

## Inputs

- Claims and results: I-0079, I-0081 (the motion stutter tranches), I-0083 (the gate), I-0095 (the study), D-0197, D-0202, D-0207.
- Gate runs under `evidence/I-0066/artifacts/` (i83-*, i9x-final-*, i94-cal-*), the original reference `i83-ref-original`.
- Tools: `scripts/Invoke-ReplayGate.ps1`, `scripts/Get-ReplayMeasure.ps1`, `native/frame/Get-SampleProfile.ps1`,
  `investigations/I-0079-motion-stutter/Run-StutterSeries.ps1`; PIX and Nsight Systems are installed; PresentMon (FrameView's copy).

## Hypotheses

- H1: the held count misses most frames presented late (I-0095 §4); an unbiased count reads more of them.
- H2: the 25-34 ms band follows DS3's reload of the same map part every cycle, and the native excess there is the engine thread
  joining DS3 loading tasks that now carry the native creations (I-0095 §4, INFERRED).
- H3: the crossing frame's extra 3-4 ms are the native first admission joined by the engine thread (I-0095 §3).

## Expected evidence

Per build about 10 measure runs of the gate path, builds alternated in one series (I-0095 §8); a difference is called only beyond
2 standard errors. Lows from per-frame data. Profiles of the reload windows (the engine thread's time in DS3's join, the native
time inside the joined tasks).

## Exit conditions

- Success: frames over 25 ms and the 1 % low at or near the original's on the gate path, the gate passed, thresholds recalibrated.
- Refuted: a hypothesis's change gives no gain beyond noise; reported as no gain, the change removed or kept only if neutral and simpler.
- Insufficient information: the measurement cannot separate the builds; recorded as such.
- Stagnation: no new discriminating evidence after 3 cycles.
- Budget exhausted: close the tranche and replan under `.project/WORKFLOW.md`.

## Safety and human interaction

Live runs under D-0116/D-0117: announced at least 5 minutes ahead with the PC clock, offline, hash-checked restore, nothing
installed in the game folder. PresentMon needs an elevated session: asked of Thomas (2026-10-23). « je joue » pauses everything.
No dev hook in production; no visible console window (D-0199).
