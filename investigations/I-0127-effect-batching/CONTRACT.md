# I-0127 - Les draws d'effets regroupés façon D3D12

Capability: C3. Owner: E, the open-gaps pilot (Thomas, 2026-11-07: « Ce sera pour E / R c'est uniquement pour le RT/PT »).
Scope written ahead by the study thread: [STUDY-EFFECT-BATCHING](STUDY-EFFECT-BATCHING.md) (copied from I-0104, D-0214);
FINISH-LINE line 8 bis.

## Question

Can the native effect stage (`native/renderer/effect_stage.cpp`) record the engine's effect statements the D3D12 way (state set
only when it changes, descriptor tables kept by content, one shader-visible heap, runs of draws recorded together) with a
render-thread or GPU gain above the runs' spread at the heaviest effect scenes, the pictures and the draw order unchanged, and
without recreating any effect?

## Scope

Included:

- Our D3D12 recording of the effect statements: the classic draws, the GPU-particle statements, the copies, their descriptors,
  root bindings, pipelines and heaps; the counters and timers that measure it (behind `effect-measure.flag`).
- Elden Ring's mechanism for the same job first (D-0195): its GX context's dirty-state filtering (`141ACD170`, `1419E3700`,
  `141ACB6D0`), read and stated before the design.
- The measure before and after (Thomas's goal: « avec mesure avant/après »), in one announced slot through the coordinator.

Excluded:

- The FXR renderers, the effects' data, the engine's GX statements and the original's DXBC programmes: unchanged, no effect
  recreated, no programme rewritten (so no bindless index inside a programme: t0..t4 and b0..b6 stay fixed registers).
- Merging draws into one, instancing, the engine's CPU work in the FXR renderers and the GX hooks.
- The raster lighting the RT/PT replaces (D-0250), the RT pilot's passes.

## Inputs

- Claims and records: I-0104 (the native effect method, DESIGN, ER-EFFECTS-STUDY), I-0113 tranches 15 to 23 (the 0x29 rows'
  interleave, pass 0x31's layer, CopyRect), I-0120 D1 and D2 (the effect records' lifetimes, skipped views), I-0071 and I-0073
  ER-STUDY (ER's dirty masks and lazy state).
- Sources: `native/renderer/effect_stage.{h,cpp}`, `native/renderer/scene_pass_frame.cpp` (the effect block),
  `native/frame/game_bootstrap/21_effects.inc` (EffectFrameBuilder), `native/frame/effect_draw.cpp`.
- Tools: the stage's own counters and timers (QueryPerformanceCounter on the render thread, D3D12 timestamp queries on the
  direct queue), the native sampler, PresentMon, a PIX event list of one frame before and after (the order), the pair sheets.

## Hypotheses

- H1: a large share of the stage's calls repeat the previous draw's state (the same pipeline, targets, sampler, topology, texture
  tuple), so filtering them (ER's way) and keeping texture tables by content removes most of the calls.
- H2: the stage's render-thread time at the heaviest scenes is small (the study's estimate: a tenth of a millisecond order); the
  gain may then be within the noise (the study's §4 « nothing to win » exit).
- H3: the GPU cost of the effects is dominated by their fill, not by state changes; a GPU gain, if any, comes from fewer pipeline
  and heap switches.

## Expected evidence

Per window of frames at each scene, before and after, from the same build options and the same screen: the stage's render-thread
time (mean, 99th percentile, maximum), its GPU time, the draws per frame (mean, maximum), the calls per frame by kind and how many
repeat the previous state; the frame time (PresentMon); one frame's PIX event list (the same draws in the same order) and the
pictures compared.

## Exit conditions

- Success: the stage's render-thread time (or its GPU time) down by more than the runs' spread at the heaviest scene; pictures and
  order unchanged; no debug-layer error; the start check passed after the merge.
- Refuted / nothing to win: the first measure shows the stage under about 0.2 ms of render-thread time and no GPU idle it causes at
  the heaviest scene: stop with the numbers, Thomas decides (a gain within the noise is reported as no gain).
- Insufficient information: the counters or timers do not attach (fixed before the series, WORKFLOW « Live game rule »).
- Stagnation: two cycles without a measurable change.
- Budget: 6 cycles (tranche 1: the measure and steps A and B; tranche 2: step C; step D only if the measure after C asks).

## Safety and human interaction

Live runs only in a slot the coordinator gives and announces (WORKFLOW « Live game rule »), offline, hash-checked restore, the PC
free of builds and Ghidra during a measuring run. No change to Windows, NVIDIA or security settings. PIX timing captures need
administrator rights (Thomas's): not planned; the GPU time comes from the module's own timestamp queries.
