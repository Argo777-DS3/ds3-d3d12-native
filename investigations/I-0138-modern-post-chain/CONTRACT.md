# I-0138 - La chaîne de post-traitement moderne à la place de YEBIS (ROADMAP chantier 11)

Capability: C3

ROADMAP workstream 11 (phase 2, « the modern picture »), piloted by Fable 5.2 at high effort (Thomas, 2026-11-10; D-0273).
Decisions that frame it: D-0262 (YEBIS entirely replaced by a modern technique), D-0178 and D-0273 (two colorimetries: the
original's remade in the new chain, and a new one), D-0263 and D-0264 (one automatic exposure target for the whole game, half
strength by default), D-0266 (the settings rows; no chromatic aberration that YEBIS does not have), D-0195 (Elden Ring's
architecture first, without its known defects), D-0274 (NVIDIA cards only).

## Question

What does the post-processing chain that replaces YEBIS consist of, pass by pass, so that nothing DS3 asks of YEBIS (by area
and in the cutscenes) is lost, the chain follows Elden Ring's architecture where ER has one, and it ends in an SDR output
equal to today's for the original colorimetry and in an HDR10 output?

## Scope

Included:

- Step 1 (this tranche, read-only): what YEBIS does in DS3, by area and in the cutscenes; what Elden Ring 2.7.1 does instead;
  the native chain as it stands; a plan put to Thomas ([PLAN](PLAN.md)).
- Steps 2 to 6 of ROADMAP workstream 11, each a later tranche opened on Thomas's go: the chain in linear HDR (exposure, tone
  map, bloom, depth of field, motion blur, the grading in two colorimetries), HDR10, the blue-noise dither, the settings
  rows, YEBIS and its refusals removed.

Excluded:

- The lighting itself, the physical light sources and the traced volumetric fog (workstream 12); the textures (workstream 13).
- The effect families of workstream 5 (the FXR light shafts 10003 are not YEBIS's screen-space shafts).
- The menu views' GX tone map (I-0113 tranche 27), the TAA and DLSS (I-0132, I-0111), unless the new chain's formats force a
  change there, named when it does.
- Any build, live run or measure in step 1.

## Inputs

- Canonical claims: K-0410, K-0412, K-0433 (the YEBIS chain observed), K-0448 (auto exposure never on in reachable play),
  K-0449, K-0450 (our YEBIS passes by default).
- Artifacts and sources: [TONEMAP-SKIN](../I-0067-static-menus-subsystems-map/TONEMAP-SKIN.md) §1, §5, §6, §15 to §17;
  I-0068's [YEBIS-NATIVE](../I-0068-static-yebis-ffx-lights-spec/YEBIS-NATIVE.md) and
  [YEBIS-TECHNIQUES](../I-0068-static-yebis-ffx-lights-spec/YEBIS-TECHNIQUES.md);
  [YEBIS-SECOND-TRANCHE-READY](../I-0066-independent-native-startup/YEBIS-SECOND-TRANCHE-READY.md);
  [W10-NATIVE-PASSES](../I-0066-independent-native-startup/W10-NATIVE-PASSES.md);
  [STUDY-YEBIS-REFUSALS](../I-0113-open-gaps/STUDY-YEBIS-REFUSALS.md);
  [REMO-CUTSCENES-NATIVE](../I-0069-static-menu-audio-map/REMO-CUTSCENES-NATIVE.md);
  [ER-TAA-STUDY](../I-0132-rt-optimisation-zones/ER-TAA-STUDY.md); the 168 GPARAM dumps
  `evidence/I-0066/queries/gparam-all/`; the native tree (`native/world/yebis_*`, `native/renderer/*_stage.*`,
  `scene_pass_frame.cpp`).
- Tools: the GPARAM dumps read by Sonnet workers; headless Ghidra 12.1.3 on the Elden Ring mirrors (`ghidra-research`
  workers, idle priority); no game.

## Hypotheses

- H1: Elden Ring no longer runs the YEBIS library: FromSoftware replaced it by GX filters of its own, whose order and
  parameter model the native chain can follow.
- H2: DS3's data use a small part of YEBIS (a fixed exposure by area, one filmic curve, a colour matrix, a vignette, a 16³
  LUT, a bloom, screen-space shafts in a few areas, a depth of field, a motion blur), the cutscenes adding animated values
  of the same parameters; the rest of the library (star, ghost, anamorphic flare, lens distortion, heat shimmer, auto focus)
  is never asked by the shipped data.

## Expected evidence

- H1: the strings, RTTI classes and call order of ER's post filters, read in its executable.
- H2: a census of every post-processing GPARAM group over the 168 files, world areas and cutscene files apart, with the
  inheritance rule applied.

## Exit conditions

- Success (step 1): the study and the plan are in the repository and Thomas has the plan.
- Refuted: a hypothesis above is false; the plan says so and follows what was read.
- Insufficient information: a point the plan depends on could not be read statically; it is named with the run or capture
  that would settle it.
- Stagnation: no new discriminating evidence after 2 cycles on one point.
- Budget exhausted: close the tranche and replan under `.project/WORKFLOW.md`; renew a bounded budget automatically within unchanged authorized scope and existing protected boundaries.

## Safety and human interaction

Step 1 launches no game and builds nothing. The later steps need live runs under WORKFLOW « Live game rule », and the HDR10
step needs the OLED on with Thomas present and Windows' HDR setting (his to switch; since 2026-11-14 a thread may switch it
where its own session rules allow, WORKFLOW « Thomas's PC and tools »).
