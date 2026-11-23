# I-0125 - DS3 DX12 Vague 2: R6 of the RT/PT plan, the traced reflections (Les reflets tracés)

Capability: C3

## Question

Can the reflections of DS3's surfaces, the water's among them, come from rays through the frame's top level instead of the
region cubes (`GXEnvMapRegion*`) and the water's mirror view, with each ray's hit lit by DS3's own data and formulas (its
material, its baked ambient, the light set's directional lights with the sun traced, the local lights of the one list), so that a
reflection shows what is really there (characters, shadows, the sky in the water of Iudex Gundyr's arena: I-0113 gap 18)
(RT-PT-PLAN §8 R6)?

## Scope

Included (the tranches of [R6-PLAN](R6-PLAN.md)):

- R6a: the radiance at a ray's hit (`rt_hit.hlsli`) and its check: the control view's primary rays lit as the hit would be,
  against the raster's lit picture at the pixels where both see the same surface.
- R6b: the deferred surfaces' reflections traced into the specular accumulation (`SpecularAcc`) in place of the cubes'
  sample, the cubes kept for the rays that miss; Ray Reconstruction's specular hit distance.
- R6c: the water's reflection picture traced in place of the mirror view.
- R6d (R6-PLAN §7, merged `75eb6c5e`): the sky in the rays: a reflection ray that meets no surface composes the sky's unlit
  forward pieces as their programmes draw them (the dome, the scrolling zenith, the cloud sheets), every hit fogged along the
  reflected leg.
- R6e: R6d's checks at the witness zones and its final review's Lows. R6f: the raster that R6 replaces (the mirror view's walk and
  draws) removed, Thomas having accepted the traced water (D-0251, 2026-11-06).

Excluded: the GI (R7: the hits' ambient stays DS3's baked one, D-0247), several bounces (R8), the effects and transparents lit by
the traced light (R9), the menu (R10), the emissive surfaces as sampled emitters (R8, D-0237's lead).

## Inputs

- The programmes (the I-0063 cache `C:/Users/User/.codex/I0063-cache/programs`, `fxc /dumpbin` into
  `evidence/I-0125/artifacts/dxbc`): `GXEnvMapRegion`, `GXEnvMapRegion_Pcc`, `GXLightDeferredAO`,
  `GXLightAcc_DDirLightShadowed1`, `GXLightComposite`, `GXLightCmbAlbedo`, `GXFlver_WaterBumpTriInteractionFloatObj_Fwd`;
  the G-buffer programmes' census (I-0119 GBUF-FAMILIES, its disassemblies in `evidence/I-0119/artifacts/census/asm`).
- The rays' scene, materials and lights (I-0117, I-0119, I-0122), the traced sun (I-0118), Ray Reconstruction (I-0123).
- The native lighting chain (`scene_pass_lighting.cpp`), the env regions (`EncodeNativeEnvRegions`), the water's mirror view.

## Hypotheses

- H1: DS3's specular ambient is the region cubes' sample along the mirror direction, times DS3's own environment term
  (F0 + (1 − F0)(1 − N·V)⁵ g²) and the region's colour scale: a traced radiance can take the cube sample's place with the
  rest kept.
- H2: a hit's radiance can be computed from the material table plus per-instance and per-frame data the native already produces
  for the raster (the lightmaps, the SH rows, the light sets, the light list), with no screen-space data.

## Expected evidence

R6a: on primary rays the hit radiance (without the specular ambient) against the raster's lit picture at the agreeing pixels, by
programme family (the R3b method), the differences measured (luminance, FLIP), never judged by eye. R6b and R6c: the pictures at
the witness zones (RT and PT levels, RR on and off), measured against the cubes' pictures; the Iudex Gundyr witness.

## Exit conditions

- Success: R6a to R6d merged with their reviews; the mirror view's removal (R6f) once Thomas accepts.
- Refuted: a hit's lighting cannot be made from the native's data (H2): the gap named, then a narrower R6 (misses and the
  sun alone) put to Thomas.
- Insufficient information: a DS3 input unreadable statically (a lightmap's encoding, a constant's producer): one targeted
  live witness.
- Stagnation: two failed local fixes of the same defect: an RCA (WORKFLOW).
- Budget exhausted: close the tranche and replan under `.project/WORKFLOW.md`.

## Safety and human interaction

Live runs only under the Live game rule (announced, offline, hash-checked restore); the pictures for Thomas's acceptance (D-0237,
D-0181). Nothing irreversible.
