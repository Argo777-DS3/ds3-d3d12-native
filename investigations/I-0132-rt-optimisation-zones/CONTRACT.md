# I-0132 - R11 of the RT/PT plan: the optimisation and the zones' tour in PT (L'optimisation et le tour des zones en PT)

Capability: C3 (RT-PT-PLAN §7, §8 R11 and its « Chantier d'éclairage »; D-0235, D-0237, D-0249, D-0256, D-0257, D-0258, D-0259)

## Question

Where does the traced frame's GPU time go on the merged tree (R10, `6eb9cf4d` and after), what can be taken off it without
changing the picture's truth (D-0249), and what do the zones never looked at natively show under the rays (faults, wrong data,
the light sources Thomas will want reworked), the water of the High Wall and the Undead Settlement among them?

## Scope

Included:

- R11a: the measure of every traced pass on the current tree (R8a's zones, R9's probes and fill shadow included), at three sizes,
  the RT and PT levels; the optimisations that keep the estimator (no picture change by construction): Russian roulette past a
  path's first vertex, R9d's fill shadow folded into the GI pass, the duplicated reads and passes the optimisation pass finds.
- R11b: one structural optimisation chosen from R11a's numbers (R11-PLAN §3), proposed to Thomas when it changes the picture.
- R11c: the zones' tour in PT (R11-PLAN §4): each zone never looked at natively, the High Wall's and the Undead Settlement's water,
  pictures shown to Thomas, the faults and the wrong data recorded and fixed by family (D-0150, D-0161); the local-light noise
  ReSTIR DI would answer looked for (I-0128); `rt-fill-shadow-all`'s retry lapsed with D-0262 (no fill light left).
- R11d: the light sources' rework at Thomas's eye (RT-PT-PLAN §8 « Chantier d'éclairage »), by family or by map table, once the
  tour's pictures are seen.
- The open points of I-0111, I-0114, I-0117, I-0118, I-0119 and I-0125, closed on 2026-11-09 with Thomas's yes (R11-PLAN §6).

Excluded:

- The settings menu and the title screen (R12, D-0258); frame generation's screen-mode witness (R12's precondition, I-0131).
  Amendment (2026-11-10, recorded at the closure 2026-11-21): R12 then ran in this investigation by its own plan
  ([R12-PLAN](R12-PLAN.md), ROADMAP workstream 2); the investigation closed with it on 2026-11-21.
- D-0255's change-driven collect and rays' walk (Thomas 2026-11-07 18:13: not now); a GPU-driven raster (D-0255).
- The effects' look and the open gaps (E's, D-0250); YEBIS's replacement (RT-PT-PLAN §8, after the PT).
- A new program or SDK on Thomas's PC (Nsight Graphics): proposed to Thomas with the command, never installed by the pilot
  (WORKFLOW « Tools »); the Agility SDK was allowed by Thomas on 2026-11-09 (R11-PLAN §5).

## Inputs

- [R11-PLAN](R11-PLAN.md) (this folder); RT-PT-PLAN §6 to §8; the RESULTs of I-0126, I-0128 (R8a's zones, R8c's table), I-0129,
  I-0131; the six closed investigations' RESULTs.
- Tools: `rt-gpu-measure.flag` with `investigations/I-0128-rt-path-tracing/Get-R8Gpu.ps1`, Run-R7aGi.ps1 `-Measure`; PIX for one
  frame's events; Nsight Systems when a CPU/GPU timeline is needed; ffmpeg signalstats and FLIP for the pictures; clangd; the
  zone saves and Alpha's travel copies (WORKFLOW « Live game rule »).

## Hypotheses

- H1: the PT level's second path vertex (about 7 ms each for the GI and the reflections at 1920x1080 DLAA, R8c) is mostly spent on
  paths whose throughput is small (DS3's dark albedos), so Russian roulette takes most of it off, the mean unchanged.
- H2: the passes that rebuild the pixel's surface and read the G-buffer apart (the GI, R9d's fill shadow, the reflections) share
  that work in one pass for a measurable saving.
- H3: the unseen zones hold material families, lights or instances the rays read wrong (a hole, a light leak, a black surface),
  found only by looking (RT-PT-PLAN §12 risks 2 and 4).

## Expected evidence

- The zones' GPU times before and after each change, the same tree with the change behind a flag, the same view and size; the GI's
  mean against the earlier estimator (Compare-GiMean.py) where the estimator changes its variance.
- The tour: per zone a picture of the RT and PT levels at its bonfire (and the water's views), the journal's faults and skipped
  rows; the families met by the control views where a surface looks wrong.

## Exit conditions

- Success: R11a and R11c merged, reviewed, the start check passed; R11b and R11d done or replanned with Thomas's choice recorded.
- Refuted: H1 (the roulette saves under 1 ms) or H2 (no saving): recorded, the change not merged (optimise, do not stack).
- Insufficient information: a zone the saves cannot reach is named, with what is needed (a save, Thomas's run).
- Stagnation: two failed local fixes on one fault open an RCA.
- Budget exhausted: close the tranche and replan under `.project/WORKFLOW.md`.

## Safety and human interaction

Live runs on the ASUS under D-0116/D-0117 (the OLED only with Thomas there), each slot asked of the coordinator; offline, saves
restored by hash; Alpha never modified (its travel copies kept as new hash-recorded test saves). Thomas's judgement: the tour's
pictures and the light sources' rework (D-0237), any change that alters the picture.
