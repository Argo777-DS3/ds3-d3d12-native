# The fog distance byte image+0x4961ABD (static reading, 2026-10-19)

Input to Thomas's decision 3 of the wave-1 closure ([WAVE1-CLOSURE](WAVE1-CLOSURE.md) §3): correct it before wave 2, or keep it
as a reserve. Static reading only (the repository's `decompiled/` exports); nothing changed in the module.

## Observations

- **Live** (Reference2311 / Native2312, m45, read in game): the byte is 1 in the original, 0 natively; image+0x4961ABC is 1 in
  both (the native module sets it since the optimisation review).
- **Writer** ([D573B0](../../decompiled/DS3-1152-F-00D573B0.c), the original's GX scene set-up, called once from D59F14,
  `.xrefs`): after B5E00 and 236F0, `DAT_144961abd = 1` **unconditionally**, then, when the scene root [144961B50] exists,
  `DAT_144961abc = 1` and eight root fields: +0x47F0 = 0.43 (0x3EDC28F6), +0x47F4 = +0x47F8 = 2.0, +0x4844 = 0.12,
  +0x4848 = 0.34, +0x484C = 0.5 (a 64-bit store, +0x4850 = 0), +0x4854 = +0x4858 = 0.001; just before, +0x4E18 = 1 and
  +0x47A8/+0x47C8/+0x47D0 from its argument. Natively D573B0 never runs (it is a fail-fast hook, GX-DEPENDENCIES §C); the
  module reproduces only +0x4961ABC.
- **Reader** ([14E900](../../decompiled/DS3-1152-F-0014E900.c), the cbFog record builder): the distance factor is
  `1 / ±p[5]` (p[5] floored at 0.1, sign from byte p+7), **replaced by p[6] when the byte is set**; it then feeds the
  exponential term `pow(2, -h * factor)` with the view height h, and the record's word 3.
- **Native callers of 14E900**: the deferred fog arrays (`07_scene_lighting.inc` 541) and the forward rows' cbFog
  (`07_scene_lighting.inc` 607) call the engine's 14E900 directly, so they take the `1 / p[5]` branch in every zone; the effect
  producers (`world/effect_producers.cpp` 376) and the oracle path (`light_producers.cpp` 617) pass the byte explicitly.

## Interpretation

- The byte is not an m45 setting: the original sets it once for the whole session, so its p[6] branch is the original's fog in
  **every zone** with fog, not only at m45 (the comment of `light_producers.cpp` 535, « when the byte is set (m45 Snowfield) »,
  read it from the one capture that showed it). INFERRED: zones where p[6] and `1 / p[5]` differ show a different fog falloff
  natively; m45, where the difference is largest (p[6] = 0.01), is the visible one.
- (Superseded by the correction below: the native banks already hold 0.01 in word 6 at m45, so the byte alone was enough.)
  The earlier reading was that a run-time writer of word 6 had to be reproduced first.

## The plan before Thomas's decision (superseded by the correction below)

The plan read the writer of word 6 first, then set the byte and D573B0's root fields. Not needed: see below. (The inventory's
other writers D6E840/D6E9B0 touch 4961BC5 and 4961AB0, not 4961ABD: the I-0066 globals scan lists only D573B0 among the CPU
writers of 4961ABD, plus 14E900 and one unidentified GX-range function reading or writing it; live, the byte stays 1 natively
after the correction.)

## Correction (Thomas, 2026-10-19 14:46: « corrige le brouillard »)

The run-time word 6 needed no new producer: natively the fog sets the native banks build already hold the engine's value
(m45: `native_fog_set` f[6] = 0.01, f[5] = 100, f[7] = 0). Only the byte was missing, and with it at 0, 14E900 took
`1 / p[5]` negated (byte +0x1C is 0): **the height-fog factor ran with the opposite sign**. The module now sets
+0x4961ABD = 1 with +0x4961ABC in `PublishNativeSceneRoot` (D573B0's root fields are read only by the old renderer's 24740 in the `decompiled/` export, INFERRED not
read natively, and stay unreproduced). The factor also feeds the record's words 0xB and 0xE.

Witness (fog build, Native2313-2319 against the cleanup build's Native2286-2293 and the original's Reference2246-2256):
- the m45 cbFog record (`native_light_blocks` fog0) goes from {0.9211, 0.01667, 0.01113, **-0.01**} to
  {0.9211, 0.01667, 0.02496, **+0.01**}, the branch the original takes with the same set (its own record was not read in these runs);
- the seven witness zones' pictures are unchanged (every region within 0.01 of the cleanup build): at these views the height
  term weighs little. The correction is a fidelity fix of the record, not a visible change at the witnesses.
