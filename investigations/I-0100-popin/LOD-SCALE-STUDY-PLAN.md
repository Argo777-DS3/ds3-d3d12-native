# I-0100: the LOD scale after a reload, study plan (option A or B)

Asked by Thomas on 2026-10-26 21:30 (« Ça aurait quel impact ? L'original fait comment ? Et Elden Ring ? »), before he chooses
between option A (the launch LOD scale kept through a session, `lod-launch-scale.flag`, opt-in) and option B (DS3's own
behaviour). A study of its own, in a fresh thread after tranche 4's merge (Thomas, 21:32: « D'accord, on fait ça »). Every claim
not verified is written « hypothèse ».

## What is known (RESULT tranche 4 step 1, step 2 items 7-8)

- DS3's LOD step D1CCE0 compares `|centre - camera| x scale` with the LodParam distances; the scale is the LOD manager's +0x150,
  written by 6EB60 as `tan(fov/2) / tan(ref/2)` (divided by `height / ref height` when the latter is set) from its camera's
  fov +0x140 and its reference fov +0x2D4.
- At a launch: fov 0.750492 rad (43°), reference 0.959931 rad (55°), scale 0.756695. After a death's reload: the reference
  holds the camera's fov, so the scale is 1 and every LOD and draw distance comes 1.32 times nearer (`i100-t4a-al-n1`, `-o1`,
  `i100-t4f-al-n1`); in the original too (its scale read in `i100-t4a-al-o1`; its reference word not read).
- Option A restores the first reference after D1DE70 (`i100-t4g-al-n1`: the scale 0.757 after the death, the LOD mix as at
  a launch). The man-serpent's fade at 23.8 m after a reload would be about 31 m with A.

## Questions and how to answer them

1. **The impact of A** (measured, not estimated): the same replayed path after a death, with and without A, both on the ASUS:
   the objects and draws per frame (`native_current_view_selected`, `popin_native` counts), the LOD mix (`popin_lod` last
   states), the collect's and the render's times (measure mode, the sampler), the frames over 20.4 ms, the private memory and
   the video memory (`Run-Endurance.ps1`'s counters), the models first admitted (`popin_first`) and the streaming's loads.
   A needs a path with a death: the Anor Londo save with `test-death.flag` (no replayed path yet records a death; hypothèse:
   the Settlement path with a death added at its start would serve).
2. **Why DS3 replaces the reference at a reload, intended or forgotten**: who writes the LOD manager's +0x2D4 (and +0x2D0) and
   when, by targeted decompilation of DS3 (the manager's constructor D1D150, the step-0x4F task D1DE70 and its calls 6E750 /
   6E770, the camera build 989E10 / CED670); what the launch value 0.959931 is (a constant, a parameter, the title's camera);
   whether a travel's reload does the same as a death's (one run with a travel); and the original's reference word read in a
   plain Reference run (popin-observe, `popin_lod_scale` now names `ref_fov`).
3. **Elden Ring** (read-only, ER 2.7.1 in `C:/Users/User/Tools/ER-Decompilation/ghidra/ER_2_7_1_25080141_I0001`, headless
   Ghidra on a copy, D-0195): the same LOD manager scale (a `tan(fov/2) / tan(ref/2)` writer beside the LodParam step), what ER
   takes as its reference and whether a reload changes it. ER is not run.

## Output

A short summary for the coordinator (impact, cause, ER), the measures in a RESULT section of I-0100, and a recommendation A or
B for Thomas. Nothing becomes default without his choice.
