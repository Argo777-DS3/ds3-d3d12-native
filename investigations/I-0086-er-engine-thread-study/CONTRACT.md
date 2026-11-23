# I-0086 - DS3 DX12 Vague 2: ER study of the engine thread and the frame preparation (third tranche preparation)

Capability: C3

## Question

How does Elden Ring 2.7.1 organise its main thread and the preparation of a frame (the frame timeline and its task lanes,
the draw step, the scene collect, the UI, the effects, the per-entity and per-view data, the hand-off to the render thread,
the waits and the limiter), which of ER's choices are defects not to copy (I-0080), and what does that imply, measured
against the native engine thread of today, for the third tranche « fil moteur vers 120 images/s » (ROADMAP, I-0078 « open »)?
The aim of that tranche is about 3.6 ms of native work a frame on the engine thread (120 frames/s leaves 8.3 ms, of which
DS3's own update takes about 4.7 ms at m45, I-0078), against 5.52 ms at m45 today.

## Scope

Included:

- Read-only static decompilation of ER 2.7.1 (headless Ghidra on the `er-mirror-*` copies, Sonnet 5.5 workers, Opus checks
  the sources a conclusion relies on), reuse of the earlier ER studies (I-0071, I-0073, I-0077, I-0079, I-0080 ER-STUDY).
- A static description of the native engine thread (`native/`, `main` at 95aec11c) and of DS3's frame timeline
  (ENGINE-STEP-THREAD-MAP), with the measurements already on record (I-0077, I-0078, I-0081, I-0084).
- A prioritised plan of measurable gains for the third tranche, each with its check.

Excluded:

- Any change to the native code, any live run or build (the PC belongs to the night endurance thread I-0085), any merge into
  `main`.
- The third tranche itself.

## Inputs

- Canonical claims: K-0458 to K-0466; D-0168, D-0187, D-0195, D-0196, D-0200.
- Artifacts and sources: `C:/Users/User/Desktop/ER Decompilation` (read-only), the ER Ghidra mirrors, the studies above.
- Tools: Ghidra 12.1.3 headless (`-readOnly -noanalysis`, hidden, below-normal priority, only while no game runs).

## Hypotheses

- H1: ER's main thread does less per frame than DS3's native engine thread because ER moves the per-entity draw work, the
  parameter updates, the effects and the UI's drawing to job pools or to the render thread, and keeps only the traversal's
  start and the joins on the main thread.
- H2: ER keeps defects on that path that the native design must not copy (busy waits of the limiter and the frame lag, a
  pool-wide join, rebuilding per-draw data every frame where DS3's native can keep it).

## Expected evidence

Per topic: the ER functions (absolute VAs, decompile or assembly) marked as read, kept apart from what is deduced; the native
code's file and line; the measurement already on record that sizes the gain.

## Exit conditions

- Success: one study document (ER-STUDY.md) covering the timeline, the draw step, the UI, the effects and the per-frame data,
  decompiled facts and deductions kept apart, with the third tranche's prioritised plan.
- Insufficient information: a topic whose ER treatment is not found statically is recorded as not found, with what was
  searched.
- Stagnation: no new discriminating evidence after 3 cycles on one topic.
- Budget exhausted: close the study and replan under `.project/WORKFLOW.md`.

## Safety and human interaction

No live-game launch, no build, no irreversible change. Ghidra and the workers run only while no game process runs, and are
stopped before each test the endurance thread announces, until its « PC libre ».
