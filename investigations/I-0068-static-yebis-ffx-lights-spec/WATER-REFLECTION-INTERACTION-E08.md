# I-0068 part W: water reflection view, water interaction (D07), E08 effect light shafts

Request (coordinator, 2026-10-10): the items left open after part V, in this order:

1. the planar water reflection view (m37);
2. the water interaction (D07);
3. E08.

Labels: STATIC, OBSERVED, INFERRED.

**Evidence** (`evidence/I-0068/queries/water/`):

- `m37-reflection-candidates.tsv`;
- the Ghidra query indexes;
- the decompiles `0FB0E0` (exported) and `103510` (the frame renderer, cited by line).

## 0. Correction to WATER-NATIVE

**The five witness zones do contain water.** m37 draws the same pair, `WatDpt` (ps 7864) then
`FwdSdw` (ps 7856), with 9261 indices. The I-0063 programme ids are stable across sessions. The I-0063 family inventory does not list
`WaterBumpTri` at m37; its cause is not investigated here.

So **m37 is the water witness** (W1–W5 of WATER-NATIVE apply there), in addition to Firelink.
WATER-NATIVE §1 is corrected accordingly.

## 1. The planar reflection view (OBSERVED m37, STATIC)

### 1.1 What m37 draws

A separate **half-resolution view**, once per frame:

- **Targets:** 640×360 RGBA16F, with its own 640×360 depth (`R32G8X24`).
- **Three draws:**
  - ps 2793: 5520 indices; depth write, stencil written (ref/REPLACE, mask 0xFF00 as in the
    G-buffer);
  - ps 342: 6108 indices; the same state;
  - ps 295: 789 indices; no stencil. It samples a **2048×512** texture; INFERRED: the sky
    dome. ps 295 is also drawn in the main view.
- **Blend:** blend 6, opaque colour.

The water `FwdSdw` then reads this target as **t54 `g_ReflectionTexture`** (INFERRED from the
binder `1BF180` §2.3 of WATER-NATIVE, and the size).

### 1.2 The driver: `FB0E0` (STATIC)

**Gates:**

- `[GXRS(144961AB0)]+0x3E1 == 0`;
- the frame's reflection object `R = frame+0x1D0` exists and is armed (`R+0x3F7C`);
- the option `[[144961B50]+0x5360]+0x6C` is set. INFERRED: the WaterSurfaceQuality
  reflection switch; Thomas's setting is HIGH.

**Camera:**

- the plane `R+0x3F90` (normalised xyz, d);
- the reflection matrix `I − 2·n·nᵀ`, composed into the view (lines ~120–200 of the decompile);
- the view parameters from the caller's view (`+0x40..+0x7C`). When `+0x70 == 1`, the size
  comes from `R+0x20/+0x24`, i.e. 640×360.

**Render list:**

1. `138280(list, …, 0x39)` opens it with **view group 0x39**;
2. the commands `10C400(R)` (targets), `10C9A0(R)`, `10F710(R+0x80)`, `10C410`;
3. a clear to `1439F2500`;
4. **`FCE30` → `103510`** with group 0x39 and flag `|0x10`.

**What group 0x39 changes** (STATIC):

- `103510` skips the deferred decals (`if (iVar26 != 0x39)` before `51AA0`) and several
  post steps.
- `57A20` gives **no row to water descriptors** (kind 0x27) in a group-0x39 view: the water
  does not reflect itself.
- `FCE30` tests the view's `+0x6D/+0x6E` flags for group 0x39 (lines 142/166).

**Plane selection** (`103510`):

- Lines 1900–1925: a default plane and `R+0x3FA0 = 0.5` when nothing better exists.
- Lines 2277–2287: at the end of each main frame, the candidate plane gathered from the
  frame's water surfaces replaces `R+0x3F90` when its distance is below `R+0x3FA0`.
- `R+0x3F7F` is set, and **`R+0x3F80 = 100`**: a hold countdown, decremented at line 2084.

So the reflection of frame N uses the water plane found in frame N−1.

### 1.3 Ready to code

| # | Step | Native |
|---|---|---|
| 1 | Reflection object | a 640×360 RGBA16F target + depth; the armed flag, the plane and the hold counter, updated from the water meshes drawn in the main view (§1.2) |
| 2 | View | mirror the main camera by the plane; same projection at 640×360 |
| 3 | Draw | the same scene gathering as the main view, with group-0x39 rules: no water rows, no decals. m37 draws only 3 meshes (two map parts and the sky), so the gather culls hard. INFERRED: by the view's frustum and a reflection flag on parts; read `FCE30`'s `+0x6D/+0x6E` tests before coding the filter |
| 4 | Bind | the result as t54 for the water surface |

**Checks:**

- R1: at m37, one 640×360 view per frame with three draws and the same index counts;
- R2: the t54 image against the original's.

**Risk:** which parts enter the reflection (the gather filter) is not read here. That is the
open step.

## 2. The water interaction (D07)

**OBSERVED:**

- **No draw writes an R16G16_FLOAT target in any of the 123 retained captures.** The 640×360
  interaction map (`12484` at Firelink) is only cleared, to (0.502, 0.502), every frame.
- No capture has a character moving through water.

**STATIC:**

- **Programmes** (`gxrenderershader`):
  - `GXWaterInteractionHeightMap`: PS b0 `cbWaterHeightMap` with `g_ClickPos`,
    `g_heightMapWidth/Height`, `g_areaWidth/Height`, `g_totalTime`, `g_interactionType`,
    `g_waveSpeed`, `g_waveLength`, `g_waveAmp`, `g_circularAge`, `g_circularWaveNum`,
    `g_femAttenuation`, `g_femDt`; t0/t7 (previous height states); a finite-element wave step.
  - `GXWaterInteractionNormalMap`: b0 `cbWaterNormalMap` with the map width/height and area
    width; t3 the height map → normals.
- **Owner:** `GXSR::GXWaterInteractionManager`, with `GXWaterInteractionHandle`,
  `GXWaterInteractionMaterialPatch` and the FFX appearance `GXFFX::GXWaterInteractionAppearance`.
- **Settings** (debug strings): "WaterInteraction-Enable", "-Screen Space Enable",
  "Create/Delete Distance", "MaxResolution", "ScreenSpace TexelSize/Max Resolution".
- **Material:** the built-in `g_GXFT_WaterInteraction` and `g_WaterInteractionSpeedParam`
  (native parses the first, `native/frame/game_model_preparation_test.cpp:324`).

**Ready to code now:**

- the neutral map: 640×360 `R16G16_FLOAT`, cleared to (0.502, 0.502) each frame, bound at t57.
  It is exact for every witness.

**Still open:** the simulation (`HeightMap` then `NormalMap` passes driven by the manager's
handles). Specify it only with a witness capture of a character walking in water (m37's pool,
for example).

## 3. E08: effect light shafts

**OBSERVED:** never drawn. E08 exists only in the m30, m33 and m38 effect bundles
(FFX-FAMILIES-PHASE2 §1), and none of these zones has a retained capture.

**STATIC** (enough to code it on the shared classic machinery):

- **Programmes:** shader-set indices **71–74** of `291520`:
  - VS `GXFfxLightShaft` (POSITION, four TEXCOORDs);
  - PS `GXFfxLightShaftProcedual`, `…ProcedualMask`, `…Texture`, `…Dust`.
- **Constants:** PS b0 `PSCommonBuffer` (FFX-FAMILIES-PHASE2 §2.2):
  - `g_Decay` (+240), `g_DecayWeight` (+244), `g_Dencity` (+248), `g_Exposure` (+252);
  - `g_SampleNum` (+264);
  - `g_LightShaftScreenCenter` (+272), `g_ps_LightShaftDir` (+496).
- **Textures:** t0 diffuse (the dust variant adds t1); sampler s9 ClampLinear.
- **Draw:** through the common apply `264230`, the same constant upload and binding as
  E01/E02 (EFFECTS-PHASE2-E01-E02-NATIVE).

The draw-kind → index branch for 71–74 is not isolated in `264230`'s switch. Read it when a
witness exists.

- Not to be confused with the **YEBIS** light shafts (`CRenderGlare::GenerateGlare_LightShafts`,
  `tech_LightShaftMask`), which YEBIS-NATIVE covers.

**Ready-to-code decision:**

- E08 rides on the E01/E02 implementation: four more programme indices and the light-shaft
  fields of `PSCommonBuffer`.
- Qualify it with a capture at m30, m33 or m38.
