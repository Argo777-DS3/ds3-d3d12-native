# The effect draws grouped the D3D12 way: the scope of a workstream (study thread S, 2026-11-07)

Thomas (2026-11-07, thread « Corriger la saccade des effets 0x2D »): « c'est l'intérêt de DX12, surtout maîtrisé, on gagne en
perfs, c'est plus moderne et fluide »; « J'espère juste qu'il faudra pas recréer la centaine d'effets »; placed after gap 22 and
the silent-calls audit (« Après l'écart 22 et l'audit, S pourra préparer en amont »). The workstream groups the effect draws in our
D3D12 code (fewer pipeline and binding changes, recording by runs instead of one GX draw at a time), measured before and after.
**Not in it:** the FXR renderers, the effects' data, the engine's GX statements and the original's programmes (DXBC) stay as they
are; no effect is recreated. This is a scope, written ahead for the pilot who opens it (it copies this file into its investigation).

## 1. Where the effects are drawn today (`native/renderer/effect_stage.cpp`, `main` `080ba94d`, read)

The engine's FXR renderers run unchanged on the native effect context (I-0104, `frame/effect_draw.cpp`); each GX draw becomes one
`NativeEffectDraw` (programmes, layout, bank states, five texture slots, five constant ranges of the frame's one arena). The stage
encodes them in order, **one D3D12 draw each**, with for every classic draw: five `CopyDescriptorsSimple` into a fresh region of the
shader-visible ring (`views_per_slot` 4096: 819 draws a frame slot), two descriptor tables (t0..t4, the sampler), five root CBVs,
`OMSetRenderTargets`, `SetPipelineState`, the topology, the vertex view, the index view: about 16 calls a draw, **none filtered
when it repeats the previous draw's**. The pipelines are cached by key (19 to 21 in a run). The GPU particles (E07) take the
compute and particle roots and `ExecuteIndirect` already.

**How many** (the journals' `native_effect_stage` rows, every 1,800 frames; OBSERVED): about 65 to 71 classic draws a frame on
average over a window at the Firelink fire and the Irithyll bridge (`i113-t22-fire2-series`, `i113-t22-bridge-native`) and in
the pyromancy casts at Archdragon Peak (`i113-t15-pyro-native`); a burst in a cast is higher than its window's mean (not read
per frame). So **the CPU cost is likely small** at these scenes (some 1,100 calls a frame, INFERRED a tenth of a millisecond
order, connaissance générale on D3D12 call costs): the measure decides, and the workstream stops after it if there is nothing
to win (§4).

**Elden Ring** (first rule, D-0195; [Deep study716](../I-0066-independent-native-startup/ER-ENGINE-D3D12-DEEP-STUDY-716.md),
Report710, a model draw's path): ER's GX context under D3D12 also binds per draw: its preparation `141ACD170` prepares the
**dirty** descriptors and binds a GPU descriptor table at the root index (`01E8F3B0`) before each `DrawIndexedInstanced`. So the first step below is ER's
own mechanism (dirty-state filtering), which the native stage lacks; the later steps go beyond ER. ER's effect path itself was not
read for this point (I-0104 read its billboards to the device context): a Sonnet ER read of `141ACD170`'s filtering comes first.

## 2. What can be grouped without touching a programme

The original's DXBC programmes read their constants from fixed registers (VS b0, DS b0, PS b0, b1, b6) and their textures from
t0..t4: a bindless index or a per-draw structure in one buffer would need the programmes rewritten (158 entries per stage in the
shader set, I-0068), which is out. What stays possible:

| Step | What | Gain expected | Risk |
|---|---|---|---|
| A | **Dirty-state filtering** (ER's way): skip `SetPipelineState`, the topology, the targets, the sampler table and each root CBV when equal to the previous draw's; reuse the previous texture table when the five views are the same | the redundant calls (the same programme, states and targets run in long series in key order: their share not measured) | low: state is per command list, restored after `before_draw`'s rows (the stage already rebinds after them) |
| B | **Persistent texture tables**: one descriptor table per distinct five-view tuple, kept by content (an admitted texture's view lives as long as its record), instead of five copies a draw | the descriptor copies and ring room | low to medium: a tuple's lifetime follows its records' (I-0120 D1's retire rules) |
| C | **Runs by indirect commands**: consecutive draws with the same pipeline and the same texture table recorded as one `ExecuteIndirect` whose command signature changes the five root CBVs and the vertex and index views per draw (allowed arguments in D3D12); the arguments written into the arena with the constants | one call per run instead of one per draw | medium: order is kept (a run never crosses a pipeline, a table, a `before_draw` row or a copy); the per-draw arguments cost upload bytes; a debug-layer and PIX check per run |
| D | **Recording off the render thread** (the effect stage's commands on a worker list), if A to C leave its time on the render thread's critical path | the render thread's frame time | medium: the list's order against the forward rows (`before_draw`) |

Not in the scope: merging draws into one (each FXR draw has its own constants and blend order), instancing (the programmes do not
read an instance id), the engine's own CPU work in the FXR renderers and the GX hooks (the original's, kept).

## 3. Measures, before and after (WORKFLOW « Live game rule », an announced slot through the coordinator)

- **Where:** the heaviest effect scenes known: a cast series (`alpha-allspells-pyro`: Seething Chaos, Flame Fan, Fire Orb,
  Profaned Flame), the Firelink fire, the Irithyll bridge (gap 22's), a zone with snow (m37) or ash.
- **What:** the effect stage's CPU time on the render thread (the native sampler, `sample-profile.flag`, or Nsight Systems' CPU
  ranges), its GPU time (PIX timing, Thomas's rights for a timing capture), the frame's total; the draws per frame, the calls per
  frame (a counter in the stage), and the frames over budget on the replayed path (Invoke-ReplayGate on request, D-0227).
- **Rules:** the PC free of builds and Ghidra (a measuring run), one tool trial before the series, the ASUS at 60 Hz or the OLED
  at 280 Hz named and never mixed, three render sizes for the control (1280x720, 1920x1080, an arbitrary window), and the picture
  compared with the run before (the same draws in the same order: a PIX event list of one frame before and after).

## 4. Exit conditions

- **Success:** the effect stage's render-thread time down by a margin above the runs' spread at the heaviest scene, the pictures
  and the PIX order unchanged, no debug-layer error, the start check passed.
- **Nothing to win:** if the first measure shows the stage under about 0.2 ms of render-thread time and no GPU idle it causes at
  the heaviest scene, the workstream stops there, with the numbers, and Thomas decides (a gain within the noise is reported as no
  gain, WORKFLOW).
- **Budget:** about one workstream: A and B in one tranche, C in a second, D only if the measure after C asks for it.

## 5. Its place

After gap 22 (E's tranche 23) and the silent-calls audit ([I-0113 STUDY-SILENT-CALLS](../I-0113-open-gaps/STUDY-SILENT-CALLS.md)),
by Thomas's order of 2026-11-07; written in [FINISH-LINE](../../.project/FINISH-LINE.md) (line 8 bis) and the ROADMAP's wave 2.
Which pilot carries it (E's effects or R's renderer) is the coordinator's and Thomas's choice.
