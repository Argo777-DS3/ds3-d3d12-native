# I-0081 - V2 : saccades, tranche 2

Capability: C3

## Question

What makes the native renderer's remaining slow frames while moving at Firelink (the final I-0079 build, run i79-23: 11 frames
over 34 ms in 78.6 s against the original's 1-3, 71 over 25 ms against 11, 898 over 17 ms against 625), and which native
cost can be removed so that the walk, the sprint and the camera turn come as close to the original as the game's own step allows?

## Scope

Included:

- Classing the slow frames (over 34 ms, then 25-34 ms) by the sampler in measure mode without the debug layer.
- Thomas's lead (2026-10-21): the development instrumentation left on in measure and play mode (the collect timers `kprofile`,
  the journal rows written synchronously under one lock from every thread). The same walk with them off.
- The corrections the classing calls for; D-0197 (a model whose texture is not resident appears a few frames later, never with a
  stand-in) only if DS3's texture join remains a cause.
- The process's local video memory over a loop of zone travels (2.2 GB at Firelink after I-0079 against 1.5 before): growth or
  not, heap free space.
- The Elden Ring mechanism for each part touched (D-0195), without its known defects (I-0080).

Excluded:

- The engine thread toward 120 frames/s (I-0078's third tranche), the minidumps and the pitfalls audit (D-0200's next workstreams).
- Other zones than Firelink for the stutter measurements (the zone travels serve the memory question only).

## Inputs

- Canonical claims: K-0462 (I-0079's first tranche), K-0457, K-0458.
- Artifacts and sources: I-0079 RESULT and runs `i79-02`, `i79-13`, `i79-18` (original), `i79-23` (final native build);
  [I-0079 ER-STUDY](../I-0079-motion-stutter/ER-STUDY.md); [I-0080 ER-STUDY](../I-0080-er-dx12-pitfalls/ER-STUDY.md).
- Tools: `investigations/I-0079-motion-stutter/Run-StutterSeries.ps1`, `Run-Stutter.ps1`, `Get-Stutter.ps1`;
  `native/frame/Get-SampleProfile.ps1 -SlowFrames`.

## Hypotheses

- H1: the slow frames come once per walk cycle, where the path crosses an area boundary (an NPC's models, GPARAM banks and
  environment regions admitted together, rows seen around i79-23's slow frames), and a native admission cost on the engine
  thread pushes the original's own 25-34 ms frame past one refresh.
- H2: the journal (about 300 synchronous rows a second while walking, under one mutex shared by the loading threads, the engine
  thread and the render worker) or the collect timers add enough to the frame path to matter.
- H3: the residual is DS3's own step (the original shows the same frames, shorter), not a native cost.

## Expected evidence

The sampler's samples inside the slow frames (engine thread and render worker), the slow rows' phase times, and walk runs
with the instrumentation off against on; for memory, the streamed-memory rows over a travel loop.

## Exit conditions

- Success: the slow frames classed, the native causes removed or bounded, measured before/after in walk, sprint and turn against
  the original, 0 validation error in a separate debug-layer run, the video memory question answered.
- Refuted: no native cost found in the slow frames (H3): recorded with the evidence.
- Insufficient information: the sampler cannot separate the causes: recorded, with the next instrument.
- Stagnation: no new discriminating evidence after 3 cycles.
- Budget exhausted: close the tranche and replan under `.project/WORKFLOW.md`; renew a bounded budget automatically within unchanged authorized scope and existing protected boundaries.

## Safety and human interaction

Live runs under D-0117/D-0116 (offline, hash-checked restore, nothing installed in the game folder), each announced in the
thread at least 5 minutes ahead with the PC clock. Thomas's authorization to merge when ready was given in advance (relayed
2026-10-21). No irreversible change.
