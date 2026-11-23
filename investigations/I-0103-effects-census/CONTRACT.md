# I-0103 - Recensement des effets

Capability: C3. FINISH-LINE line 3 ([FINISH-LINE](../../.project/FINISH-LINE.md)), opened 2026-10-28 by the pilot (Opus 5.5)
after I-0102. It is the census line 4 (the native effect method, D-0190) and line 5 (the missing effects) start from.

## Question

Which effect families does the whole game (base game and both DLC) use, and for each one, does the native backend draw it,
skip it, or draw it wrong today, with the evidence for each status?

## Scope

Included:

- Every FXR action kind that draws or lights something, across every effect bundle of the game (`commoneffects`, every map's,
  `dlc1`, `dlc2`), counted by FXR and by bundle (zone), from the game's own data.
- The chain from an FXR action kind to the GX appearance and the renderer class the engine builds for it (the GX effect type
  registry and its constructors, static), so each kind lands on a class the native allows, allows under a flag, or skips
  (`native/frame/effect_draw.cpp`'s allowed list).
- The known effect gaps outside the FFX draw list that the finish line counts as « effects »: the decal materials, the burning
  objects' look (Burn MTDs), the blood's fade, the translucent NPC phantom, effect spot lights, the Estus Flask's model in the
  hand, the shared bundles' families seen only off the witness zones (light shafts E08, snow on surfaces, water ripples).
- What the native's journals already counted (skipped renderer classes, dropped draws) where runs recorded them.
- The producers' capture layer (`world/light_producers`, `world/effect_producers`) and the scene renderer's stage observer
  that I-0102 left to this line: their classification (kept for line 4, or removed) with the reason.

Excluded:

- Writing or moving any effect family (lines 4 and 5); spot lights and local shadows of the map (line 6, D-0221); the visual
  defects of line 7 that are not effects.
- Live runs, unless a status cannot be settled statically; any such run is announced first (D-0116, D-0117) and said to
  Thomas (D-0227).

## Inputs

- Canonical claims and notes: [FFX-DRAW](../I-0066-independent-native-startup/FFX-DRAW.md) (E01, E02 classes, E07, the census
  of renderer classes, its « Open »), I-0068's [FFX-FAMILIES-PHASE2](../I-0068-static-yebis-ffx-lights-spec/FFX-FAMILIES-PHASE2.md)
  (families E01-E10 by action, zone weights), [EFFECTS-E03](../I-0068-static-yebis-ffx-lights-spec/EFFECTS-E03-MODEL-PARTICLES-NATIVE.md),
  [EFFECTS-E07](../I-0068-static-yebis-ffx-lights-spec/EFFECTS-E07-GPU-PARTICLES-NATIVE.md),
  [WAVE1-LAST-EFFECTS](../I-0066-independent-native-startup/WAVE1-LAST-EFFECTS.md), STATE « Open gaps », I-0101 RESULT (Burn
  MTDs).
- Artifacts: I-0063's whole-game scan `evidence/I-0063/artifacts/scan/fxr.tsv` (148,615 rows: bundle, FXR, action, count) and
  its families; the native journals under `evidence/I-0066/artifacts/` that ran with `ffx-record.flag`.
- Tools: GhidraMCP on Thomas's Ghidra (the DS3 copy `ghidra-mcp-ds3`, read-only) or headless Ghidra on the I-0066 mirror; the
  ER project read-only only if a census row needs it (line 4 reads ER).

## Hypotheses

- H1: the effect families the game uses are the FXR action kinds I-0063 listed (600-609, 10000, 10003, 10012, 11000), and the
  nine renderer classes the native skips are built by some of them; their weight in the game tells which matter.
- H2: some visible gaps are E03 model particles skipped by default (`ffx-model-particles.flag` off): the Estus Flask in the
  hand, the Ringed City's grass (INFERRED until the FXR and its model are read).
- H3: some families exist only in zones never seen natively (E08 light shafts in m30, m33, m38), so the witness zones never
  showed them missing.

## Expected evidence

The registry table (action kind, constructor, renderer class or classes, the condition choosing among them), read in the
decompiled game at named addresses; per action kind its FXR and bundle counts from `fxr.tsv`; per class its native status from
the allowed list at the census commit; per non-FFX gap its source line. A status is OBSERVED only from a native run that drew or
counted it, STATIC from the code and data, INFERRED otherwise, and is written as such.

## Exit conditions

- Success: `CENSUS.md` lists every family with its weight, its zones, its native status and evidence, ordered for lines 4 and 5;
  the capture layer's classification is written; STATE and FINISH-LINE carry the result; the census is merged into `main`.
- Refuted: H1 refuted if an action kind draws through a path outside the registry: add that path as a family.
- Insufficient information: a family whose class cannot be settled statically is listed as unknown, with the smallest run that
  would settle it (not run without Thomas being told).
- Stagnation: no new discriminating evidence after two cycles.
- Budget exhausted: close the tranche and replan under `.project/WORKFLOW.md`.

## Safety and human interaction

No live run is planned. Ghidra is read-only (no rename, no comment). Nothing in `native/` changes in behaviour; if the capture
layer is removed, it is a separate, reviewed step with the build and unit tests as proof.
