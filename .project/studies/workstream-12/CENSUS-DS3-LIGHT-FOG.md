# Non-canonical until the study thread re-reads it

# CENSUS-DS3-LIGHT-FOG: the original game's light sets, fog and light-scattering records by area

Date: 2026-11-22. Worker output for the study of workstream 12 (physical light sources, traced volumetric fog). Data work only:
no game, build or Ghidra was run. Statements are tagged **OBSERVED** (read in a named file) or **INFERRED** (my reading).

## 0. Inputs, scripts, method

- OBSERVED inputs: `evidence/I-0066/queries/gparam-all/param_drawparam_*.tsv` (168 files; the value column is hex), read from the worktree
  `C:/Users/User/Desktop/DS3 Decompilation/.claude/worktrees/recursing-lamarr-d44be4`. Method and variant list follow
  `investigations/I-0138-modern-post-chain/CENSUS-WORLD.md`; the reading and inheritance code is adapted from
  `evidence/I-0138/artifacts/census-world/census.py`. Units context: `STUDY-R7-DS3-AMBIENT.md` 3.2 (hemisphere colours are squared and
  multiplied by an intensity), `LIGHT-UNITS.md` 4 (a local light's colour is the param colour squared x strength x scale): read, not re-derived.
- Scripts and full tables (TSV, UTF-8) in `C:/Users/User/Tools/DS3-Decompilation/study-ws12/ds3-census/`:
  `gp.py` (reader), `common.py`, `census.py` (TSV tables), `variants.py` (variant diffs), `part1.py` ... `part4.py`, `checks.py` writes part5, `assemble.py` joins them (this report), `checks.py` (section 10).
  Big tables: `params-by-group.tsv`, `ids-per-file.tsv`, `lightset-base-areas.tsv` (344 area-id rows, every LightSet parameter),
  `lightset-variants.tsv`, `fog-base-areas.tsv`, `lightscattering-base-areas.tsv`, `dominant-light-base-areas.tsv`,
  `envmap-base-areas.tsv`, `shadowparam-base-areas.tsv`, `variant-diffs.tsv` (every differing value of every variant); all under the folder above.
- Decoding (OBSERVED, from the tsv): types 9, 12, 14 are float32 (14 = r,g,b,a; 12 = two floats, an angle pair stored in radians:
  -0.767945 = -44.0 deg), type 2 int16 (signed), 3 and 7 int32, 1, 5 and 11 one byte, 15 an int32 (ARGB, `FF000000`). Floats are printed to
  4 significant digits here, 6 in the TSVs. **luma** = 0.2126 r + 0.7152 g + 0.0722 b **of the stored value, not squared**; alpha is not in it.
  Repeated names inside one id are written `Name`, `Name#2`, `Name#3`, `Name#4`; in LightSet that is the file's four directional lights, which this
  report calls L0 (the sun, by position and by its angle/colour), L1, L2, L3; the file gives only the repeated names, no light index.
- Inheritance rule (the earlier census's): effective value of (group, parameter, id) = own file, else `mAA_BB_0000`, else `mapbase`, else `default`.
  Group presence (OBSERVED, section 1): `LightSet`, `Fog`, `LightScattering`, `Env Map` are in `default` and NOT in `mapbase`;
  `ShadowParam` and `SSAO` are in both; `AOFields` and `AOFields Model` are in `mapbase` only; `Dominant Light` is in neither
  (only in map and cutscene files, so it has no inherited value); `Ls` exists only in `s_template` (its copy of the
  LightScattering names, section 1). Inside the 21 base files no id is partial (an id a file carries has every parameter), and variant
  files carry a subset of the base's ids (m30_00_0100 carries 5 of the base's 43 LightSet ids): consistent with a per-id
  override; that the engine falls back per id and not per group is INFERRED from the earlier census, not checked here.
  Tables list the ids the area's own file carries; where it carries none of a group, the ids of the next layer that has some (`mapbase` for
  ShadowParam/SSAO). The `default` file's own ids are 0 and 10-15 in every group that has any (LightScattering: 0).
- The task named 18 areas including `m51_01`. OBSERVED: there is no `m51_01` file. The files `mAA_BB_0000` are 21: m21_00, m22_00, m30_00, m30_01, m31_00,
  m32_00, m33_00, m34_01, m35_00, m36_00, m37_00, m38_00, m39_00, m40_00, m41_00, m45_00, m46_00, m47_00, m50_00, m51_00, m53_00. All 21 are used (m21, m22, m36 and m53
  are extra to the earlier census's list; m21 and m22 hold 2 and 9 LightSet ids).


## Key numbers (all computed from the tsv files named; details in the sections)

- 21 base files, 344 LightSet area-id rows. Hemisphere-to-sun ratio (stored rgb luma): positive median 0.646, min 0.129 (m38_00 id 38001), max 1.431 (m47_00 id 30, hemisphere 4th float 0). 94 rows have a black hemisphere (m51_00: 15 of 15 ids).
- m30_00_0000 id 0: sun angle (-44.0, -55.0) deg, diffuse (1, 0.945, 0.869, 1.23), hemisphere up and down (0.5, 0.5, 0.5, 1.0); L1 diffuse 4th float 0.1, L2 4th float 0, L3 black (0,0,0,1).
- Both hemisphere colours have a 4th float of 0 in 180 of 344 rows; with each colour squared and multiplied by its 4th float the largest ratio is 19.45 (m22_00 id 33).
- L0 diffuse 4th float: 0 in 102 rows, up to 20 in the base files (m30_01_0000 id 121), 100 in a variant (m36_00_0001 id 1), 64 in m32_00_0100/0110.
- Strongest hemisphere intensity outside the bases: 16 in m32_00_0010 and m32_00_0110 id 20, 64 times the m32_00 base maximum (0.25); the base maximum overall is 2.352 (m33_00 id 33000).
- Fog: Dist 0 to 2000 (max m30_00 id 320, s41_00_0070 id 9), 10 base rows at 0; heightFogDensity 0.1 to 100 (never 0); eclipse.tsv has 16 Fog ids, all black (0,0,0,1) with Dist 0.1.
- LightScattering: all 89 files other than `default` equal the default values (blend 0, colours 0): no area has scattering set. `Ls` exists in `s_template` only.
- AOFields and AOFields Model: `mapbase` only (strength 1, distance max 100). SSAO has no light-weight parameter (the light weight is `LightSet.SsaoWeight`).
- Dominant Light: absent from m21_00, m22_00, m36_00, m38_00; Dominant Color Scale 0.2 to 1.2 over the 17 bases that have it.
- There is no `m51_01` file; the 21 `_0000` map files are used.

## 1. Parameters of each group (file order, type, number of files holding it)

OBSERVED in the tsv; 168 files counted; "D/M" = present in `default` / `mapbase`. Japanese names are quoted as stored; the English glosses are my reading (INFERRED).

### LightSet (130 of 168 files hold the group)

The file order is: four blocks (Angle, DiffColor, SpecColor, Sharpness, SsaoWeight) for L0..L3, then the hemisphere pair and the flags. L2 and L3 and the Local-light flags are absent from 4 files (126 of 130 LightSet files hold them); the occurrence suffix is only an order.

| # | parameter | type | files | D/M |
|---|---|---|---|---|
| 1 | Angle (L0.Angle) | 12 | 130 | D/- |
| 2 | DiffColor (L0.Diff) | 14 | 130 | D/- |
| 3 | SpecColor (L0.Spec) | 14 | 130 | D/- |
| 4 | Sharpness (L0.Sharp) | 9 | 127 | D/- |
| 5 | SsaoWeight (L0.SsaoW) | 9 | 126 | D/- |
| 6 | Angle#2 (L1.Angle) | 12 | 130 | D/- |
| 7 | DiffColor#2 (L1.Diff) | 14 | 130 | D/- |
| 8 | SpecColor#2 (L1.Spec) | 14 | 130 | D/- |
| 9 | Sharpness#2 (L1.Sharp) | 9 | 127 | D/- |
| 10 | SsaoWeight#2 (L1.SsaoW) | 9 | 126 | D/- |
| 11 | Angle#3 (L2.Angle) | 12 | 126 | D/- |
| 12 | DiffColor#3 (L2.Diff) | 14 | 126 | D/- |
| 13 | SpecColor#3 (L2.Spec) | 14 | 126 | D/- |
| 14 | Sharpness#3 (L2.Sharp) | 9 | 126 | D/- |
| 15 | SsaoWeight#3 (L2.SsaoW) | 9 | 126 | D/- |
| 16 | Angle#4 (L3.Angle) | 12 | 126 | D/- |
| 17 | DiffColor#4 (L3.Diff) | 14 | 126 | D/- |
| 18 | SpecColor#4 (L3.Spec) | 14 | 126 | D/- |
| 19 | Sharpness#4 (L3.Sharp) | 9 | 126 | D/- |
| 20 | SsaoWeight#4 (L3.SsaoW) | 9 | 126 | D/- |
| 21 | Hemi Color Up | 14 | 130 | D/- |
| 22 | Hemi Color Down | 14 | 130 | D/- |
| 23 | giOcclusionRate | 9 | 127 | D/- |
| 24 | Light1 FollowedCamera | 11 | 130 | D/- |
| 25 | Enable LocalLight1 | 11 | 126 | D/- |
| 26 | Enable LocalLight2 | 11 | 126 | D/- |
| 27 | LightMask Influence | 9 | 126 | D/- |

### Fog (142 of 168 files hold the group)

| # | parameter | type | files | D/M |
|---|---|---|---|---|
| 1 | Color | 14 | 142 | D/- |
| 2 | heightColor | 14 | 142 | D/- |
| 3 | Dist | 9 | 142 | D/- |
| 4 | heightDist | 9 | 142 | D/- |
| 5 | DistOffset | 9 | 142 | D/- |
| 6 | HeightOffset | 9 | 142 | D/- |
| 7 | heightFogBase | 9 | 1 | -/- |
| 8 | heightFogDensity | 9 | 142 | D/- |
| 9 | heightFogIntencity | 9 | 142 | D/- |
| 10 | ToUpper | 11 | 142 | D/- |
| 11 | Alpha | 9 | 142 | D/- |
| 12 | Contrast | 9 | 142 | D/- |

### LightScattering (90 of 168 files hold the group)

| # | parameter | type | files | D/M |
|---|---|---|---|---|
| 1 | 光源X軸回転角度[deg] = light X-axis rotation [deg] | 2 | 90 | D/- |
| 2 | 光源Y軸回転角度[deg] = light Y-axis rotation [deg] | 2 | 90 | D/- |
| 3 | 距離倍率[%] = distance multiplier [%] | 2 | 90 | D/- |
| 4 | 光源色R = light colour R | 2 | 90 | D/- |
| 5 | 光源色G = light colour G | 2 | 90 | D/- |
| 6 | 光源色B = light colour B | 2 | 90 | D/- |
| 7 | 光源RGB倍率[%] = light RGB multiplier [%] | 2 | 90 | D/- |
| 8 | 散乱方向係数 = scatter direction coefficient | 9 | 90 | D/- |
| 9 | レイリー散乱係数 = Rayleigh scatter coefficient | 9 | 90 | D/- |
| 10 | ミー散乱係数 = Mie scatter coefficient | 9 | 90 | D/- |
| 11 | ブレンド係数[%] = blend coefficient [%] | 2 | 90 | D/- |
| 12 | 地上の乱反射光色R = ground diffuse-reflected light colour R | 2 | 90 | D/- |
| 13 | 地上の乱反射光色G = ... G | 2 | 90 | D/- |
| 14 | 地上の乱反射光色B = ... B | 2 | 90 | D/- |
| 15 | 地上の乱反射光RGB倍率[%] = ground diffuse-reflected light RGB multiplier [%] | 2 | 90 | D/- |

### Ls (1 of 168 files hold the group)

OBSERVED: `Ls` is in one file, `s_template`, with the same 15 names and types as LightScattering, all at their template values (angles 0, multipliers 100 %, colours 0, coefficients 0).

| # | parameter | type | files | D/M |
|---|---|---|---|---|
| 1 | 光源X軸回転角度[deg] = light X-axis rotation [deg] | 2 | 1 | -/- |
| 2 | 光源Y軸回転角度[deg] = light Y-axis rotation [deg] | 2 | 1 | -/- |
| 3 | 距離倍率[%] = distance multiplier [%] | 2 | 1 | -/- |
| 4 | 光源色R = light colour R | 2 | 1 | -/- |
| 5 | 光源色G = light colour G | 2 | 1 | -/- |
| 6 | 光源色B = light colour B | 2 | 1 | -/- |
| 7 | 光源RGB倍率[%] = light RGB multiplier [%] | 2 | 1 | -/- |
| 8 | 散乱方向係数 = scatter direction coefficient | 9 | 1 | -/- |
| 9 | レイリー散乱係数 = Rayleigh scatter coefficient | 9 | 1 | -/- |
| 10 | ミー散乱係数 = Mie scatter coefficient | 9 | 1 | -/- |
| 11 | ブレンド係数[%] = blend coefficient [%] | 2 | 1 | -/- |
| 12 | 地上の乱反射光色R = ground diffuse-reflected light colour R | 2 | 1 | -/- |
| 13 | 地上の乱反射光色G = ... G | 2 | 1 | -/- |
| 14 | 地上の乱反射光色B = ... B | 2 | 1 | -/- |
| 15 | 地上の乱反射光RGB倍率[%] = ground diffuse-reflected light RGB multiplier [%] | 2 | 1 | -/- |

### Dominant Light (66 of 168 files hold the group)

| # | parameter | type | files | D/M |
|---|---|---|---|---|
| 1 | Dominant Sharpness Min Value | 9 | 66 | -/- |
| 2 | Dominant Sharpness Offset | 9 | 66 | -/- |
| 3 | Dominant Sharpness Exponant | 9 | 66 | -/- |
| 4 | Dominant Color Scale | 9 | 65 | -/- |
| 5 | Dominant R Scale | 9 | 65 | -/- |
| 6 | Dominant G Scale | 9 | 65 | -/- |
| 7 | Dominant B Scale | 9 | 65 | -/- |
| 8 | SH Color Scale | 9 | 62 | -/- |
| 9 | SH R Scale | 9 | 62 | -/- |
| 10 | SH G Scale | 9 | 62 | -/- |
| 11 | SH B Scale | 9 | 62 | -/- |
| 12 | Dominant RGB Scale | 9 | 1 | -/- |

### Env Map (83 of 168 files hold the group)

| # | parameter | type | files | D/M |
|---|---|---|---|---|
| 1 | EnvMap:ID | 7 | 83 | D/- |
| 2 | EnvMap:ColorScale | 9 | 83 | D/- |

### AOFields (1 of 168 files hold the group)

| # | parameter | type | files | D/M |
|---|---|---|---|---|
| 1 | Enable | 7 | 1 | -/M |
| 2 | Enable UpScale | 7 | 1 | -/M |
| 3 | Global Strength | 9 | 1 | -/M |
| 4 | Depth Thresh | 9 | 1 | -/M |
| 5 | Distance Max | 9 | 1 | -/M |
| 6 | Distance Margin | 9 | 1 | -/M |

### AOFields Model (1 of 168 files hold the group)

| # | parameter | type | files | D/M |
|---|---|---|---|---|
| 1 | 3DTexture - AO Strength | 9 | 1 | -/M |
| 2 | Sphere Occluder - Light Cone Angle | 9 | 1 | -/M |
| 3 | Sphere Occluder - Light Cone Coefficient | 9 | 1 | -/M |
| 4 | Sphere Occluder - Length Limit | 9 | 1 | -/M |

### ShadowParam (41 of 168 files hold the group)

Only `Angle`, `Shadow  Light Id` (two spaces), `Shadow Light Id`, `Shadow Color` name a light, an id, an angle or a colour; they are tabulated in section 7. `Angle` is type 14 in `s_template` only. The rest are listed for completeness.

Names (type, files): Angle (12, 41); Shadow  Light  Id (7, 38); Near (9, 41); Far Fade Start (9, 41); Far Fade Dist (9, 41); Cascade Dist 0->1 (9, 41); Cascade Dist 1->2 (9, 41); Cascade Dist 2->3 (9, 41); Shadow Map Mode (3, 8); Shadow Map FilterMode (3, 40); Depth Offset (9, 41); Depth Offset(forPSM) (9, 8); CaliburateFarRate(forPSM) (9, 8); Grad Factor(forPSM) (9, 8); Volume Depth (9, 41); Depth Smooth Compare (9, 8); Slope Scaled Depth Bias (9, 41); Shadow Model Cull Flip (11, 41); AlignTexel (11, 40); DepthRate (9, 40); DepthScale (9, 40); KernelScale (9, 40); DepthOffsetScale (9, 40); Blur Count (7, 40); Blur Radius (9, 40); Shadow Color (15, 41); Shadow Map Resolution (3, 37); Shadow Light Id (1, 3)

### SSAO (16 of 168 files hold the group)

SSAO has no parameter that names a light or a light weight; the per-light SSAO weight is `LightSet.SsaoWeight` (L0..L3). All SSAO names are listed for completeness; none is tabulated per area.

Names (type, files): fSampleOffset (9, 7); fDepthClipRange (9, 16); DepthEvalRange 0 (9, 7); DepthEvalRange 1 (9, 7); DepthEvalRange 2 (9, 7); fAOScale (9, 7); fAOOffset (9, 7); AO Blend Scale 0 (9, 7); AO Blend Scale 1 (9, 7); AO Blend Scale 2 (9, 7); Blur Radius 0 (9, 7); Blur Radius 1 (9, 7); Blur Radius 2 (9, 7); Final Blur Radius (9, 7); Final Blur Radius2 (9, 7); FinalBlur4x2 (11, 7); UseNewVersion (11, 7); UseNormal (11, 16); UseReProj (11, 13); PreventGhost (11, 12); EnableUpScale (11, 13); Ver2 Sample Offset (9, 16); Ver2 AO Scale (9, 16); Ver2 AO Offset (9, 16); Ver2 Kernel Scale Base Dist (9, 16); Ver2 Kernel Scale Min (9, 16); Ver2 Kernel Scale Max (9, 16); PrevFrame Strengh (9, 13); DepthFadeStart (9, 12); DepthFadeRange (9, 12); PrecisionDepth (9, 12); PrevFrame Depth Aware (9, 12); PrevFrame Depth Reject Rate (9, 12); Geom Bent Offset (9, 12)

### Value ids per file

OBSERVED (`ids-per-file.tsv` has all 168 files and every group). LightSet and Fog ids of the 21 base files:

| file | LightSet ids | Fog ids |
|---|---|---|
| m21_00_0000 | 0 2 | 0 2 |
| m22_00_0000 | 0 1 2 22 30 31 32 33 34 | 0 1 2 |
| m30_00_0000 | 0 1 2 3 4 10 11 12 13 20 21 22 23 24 25 26 27 28 29 30 50 51 100 120 121 122 123 130 131 132 133 134 135 136 137 138 150 151 152 200 300 310 320 | 0 10 11 12 13 20 300 310 320 |
| m30_01_0000 | 0 1 2 3 4 10 11 12 20 21 22 23 24 25 26 50 51 100 120 121 130 131 132 133 134 135 136 200 300 | 0 1 10 11 12 20 300 |
| m31_00_0000 | 0 1 2 3 4 5 6 10 11 30 31 100 101 110 120 121 210 211 31000 | 0 1 2 3 10 11 12 13 14 15 16 17 18 19 |
| m32_00_0000 | 0 30 31 100 110 111 210 211 212 213 214 215 220 350 360 370 32000 | 0 1 2 3 4 5 10 50 60 300 310 350 |
| m33_00_0000 | 0 5 6 10 11 20 30 31 32 100 200 210 215 220 300 301 33000 | 0 1 10 11 100 300 |
| m34_01_0000 | 0 30 31 40 100 110 200 | 0 10 11 20 |
| m35_00_0000 | 0 5 6 10 20 30 31 100 110 111 112 120 121 122 200 201 202 203 210 211 220 300 301 302 | 0 5 6 10 11 13 20 100 120 300 301 |
| m36_00_0000 | 0 1 2 3 4 10 20 70 | 0 10 |
| m37_00_0000 | 0 1 2 3 4 6 8 9 10 100 101 108 109 110 37000 37001 | 0 1 2 4 6 7 8 9 10 |
| m38_00_0000 | 0 1 2 3 4 10 15 20 30 100 110 111 112 120 210 300 38000 38001 38002 | 0 1 2 3 4 10 11 15 50 51 300 |
| m39_00_0000 | 0 1 5 6 10 30 40 50 100 110 111 210 211 212 300 | 0 1 10 100 300 |
| m40_00_0000 | 0 20 22 30 31 32 40 100 110 111 120 121 122 300 310 | 0 1 8 9 10 11 135 |
| m41_00_0000 | 0 1 2 8 9 10 30 100 101 200 300 301 310 | 0 1 8 9 10 11 12 |
| m45_00_0000 | 0 1 5 6 7 8 9 10 25 100 101 102 105 110 111 120 130 | 0 2 8 9 |
| m46_00_0000 | 0 30 31 40 100 110 200 300 | 0 10 11 20 300 |
| m47_00_0000 | 0 1 2 8 9 10 30 100 101 200 300 301 310 | 0 1 8 9 10 11 12 |
| m50_00_0000 | 0 1 2 3 8 11 20 21 22 23 30 100 105 110 115 116 120 150 300 301 310 | 0 1 8 9 10 11 12 300 310 |
| m51_00_0000 | 0 1 2 3 4 5 6 7 100 101 115 120 121 150 300 | 0 2 8 9 |
| m53_00_0000 | 0 30 31 100 110 111 210 211 212 213 214 215 220 350 360 370 32000 | 0 1 2 3 4 5 10 50 60 300 310 350 |

## 2. LightSet: the four directional lights and the hemisphere, by area

OBSERVED values in the 21 `mAA_BB_0000` files (`lightset-base-areas.tsv` has every id, every parameter, the angle in radians and degrees, the lumas and the ratios; 344 area-id rows). Here: id 0 of every area (every base file carries an id 0), then summaries over all ids. Angle = (x, y) in degrees as stored in radians. Colours are (r,g,b,a) as stored; `a` is the 4th stored float (for the diffuse colour of L0 it ranges 0 to 20 in the base files, section 2.5; INFERRED to be an intensity, like the hemisphere colours in STUDY-R7 3.2).

### 2.1 Light 0 (the sun), id 0

| area | angle deg | diffuse (r,g,b,a) | diff luma | specular (r,g,b,a) | spec luma | sharpness | ssaoW |
|---|---|---|---|---|---|---|---|
| m21_00 | (-32,140) | (0.64,0.846,1,1.18) | 0.8134 | (0.839,0.932,1,0.345) | 0.9174 | 2.5 | 0 |
| m22_00 | (-39,-141) | (1,0.809,0.655,1.6) | 0.8387 | (0.839,1,0.929,0) | 0.9607 | 2.5 | 0 |
| m30_00 | (-44,-55) | (1,0.945,0.869,1.23) | 0.9509 | (1,0.936,0.857,0.5) | 0.9438 | 1.7 | 0.7 |
| m30_01 | (-31,-119) | (0.863,0.784,0.676,1.3) | 0.7932 | (1,0.869,0.729,0.6) | 0.8866 | 2 | 0.7 |
| m31_00 | (-29,38) | (0.968,0.953,0.933,1.5) | 0.955 | (0.93,0.921,0.907,1.2) | 0.9221 | 0.6 | 0.53 |
| m32_00 | (-40,67) | (0.915,0.959,1,1.6) | 0.9528 | (0.848,0.918,1,1) | 0.9094 | 1.3 | 0.6 |
| m33_00 | (-41,25) | (1,0.86,0.566,1) | 0.8685 | (1,0.857,0.554,0.5) | 0.8656 | 0.85 | 0.72 |
| m34_01 | (-37,-121) | (0.924,0.755,0.525,1.35) | 0.7744 | (0.889,0.761,0.606,1.07) | 0.777 | 1.7 | 0.7 |
| m35_00 | (-35,-152) | (1,0.822,0.647,3.8) | 0.8473 | (1,0.843,0.665,1) | 0.8632 | 1 | 0.65 |
| m36_00 | (-57,144) | (0.569,0.627,0.627,0.5) | 0.6149 | (0.569,0.627,0.627,1) | 0.6149 | 5 | 1 |
| m37_00 | (-30,26) | (0.687,0.825,0.902,1.2) | 0.8013 | (0.658,0.846,0.941,0.7) | 0.8133 | 30 | 0 |
| m38_00 | (-27,-8) | (0.458,0.673,1,0) | 0.6512 | (0.452,0.671,1,0) | 0.6479 | 50 | 1 |
| m39_00 | (-30,122) | (0.745,0.878,1,1.5) | 0.8589 | (0.776,0.878,1,1.5) | 0.8655 | 1 | 0.8 |
| m40_00 | (-33,89) | (0.907,0.915,0.927,0.8) | 0.9144 | (0.717,0.825,1,0.746) | 0.8148 | 0.7 | 0.75 |
| m41_00 | (-26,24) | (0.901,0.945,1,0.25) | 0.9393 | (0.86,0.796,1,0.6) | 0.8243 | 2 | 0.73 |
| m45_00 | (-45,-70) | (0.808,0.904,1,0.8) | 0.8903 | (0.676,0.819,0.98,1.5) | 0.8005 | 0.6 | 0.8 |
| m46_00 | (-37,-121) | (0.924,0.755,0.525,1.35) | 0.7744 | (0.889,0.761,0.606,1.07) | 0.777 | 1.7 | 0.7 |
| m47_00 | (-36,72) | (0.793,0.726,0.571,1) | 0.729 | (0.673,0.595,0.42,0.98) | 0.5989 | 25.7 | 0 |
| m50_00 | (-32,142) | (0.828,0.875,0.913,1) | 0.8676 | (0.656,0.738,1,0.6) | 0.7392 | 1 | 0.73 |
| m51_00 | (-48,120) | (0.941,0.91,0.688,2) | 0.9003 | (0.902,0.902,0.902,2) | 0.902 | 1 | 0.8 |
| m53_00 | (-45,118) | (0.915,0.959,1,1.6) | 0.9528 | (0.848,0.918,1,1) | 0.9094 | 1.3 | 0.6 |

### 2.2 Lights 1, 2, 3 and the flags, id 0

Names as the file gives them: `Angle#2/DiffColor#2/...` = L1, `#3` = L2, `#4` = L3; flags `Light1 FollowedCamera`, `Enable LocalLight1`, `Enable LocalLight2`, `giOcclusionRate`, `LightMask Influence`. The flags named LocalLight are two booleans; the file has no further parameters for local lights (their colours are not in this group).

| area | L1 diffuse | L2 diffuse | L3 diffuse | Light1 FollowedCamera | Enable LocalLight1/2 | giOcclusionRate | LightMask Infl. |
|---|---|---|---|---|---|---|---|
| m21_00 | (0.871,1,1,0.102) luma 0.9725 ang(14.7,-66.4) sh 1 ssao 0 | (0.595,0.781,0.86,0.5) luma 0.7474 ang(-10,-152) sh 0.2 ssao 0 | (0.971,0.939,0.866,0.56) luma 0.9403 ang(-30,-47) sh 0.4 ssao 0 | 0 | 1/1 | 0 | 0 |
| m22_00 | (0.563,0.828,1,0) luma 0.7839 ang(18,144) sh 1 ssao 0 | (0.595,0.781,0.86,0.5) luma 0.7474 ang(-10,-152) sh 0.2 ssao 0 | (0.971,0.939,0.866,0.56) luma 0.9403 ang(-30,-47) sh 0.4 ssao 0 | 0 | 1/1 | 0 | 0 |
| m30_00 | (0.7,0.784,0.872,0.1) luma 0.7726 ang(-18,102) sh 1.2 ssao 0.76 | (0.735,0.843,1,0) luma 0.831 ang(-23,-116) sh 3 ssao 1 | (0,0,0,1) luma 0 ang(45,0) sh 50 ssao 1 | 0 | 1/1 | 0 | 1 |
| m30_01 | (0.659,0.834,1,0.05) luma 0.8086 ang(-14,72) sh 1 ssao 0.76 | (0.735,0.843,1,0) luma 0.831 ang(-41,-173) sh 0.5 ssao 1 | (0,0,0,1) luma 0 ang(45,0) sh 50 ssao 1 | 0 | 1/1 | 0 | 1 |
| m31_00 | (0.603,0.706,1,0.2) luma 0.7051 ang(-31,-111) sh 1 ssao 0.75 | (1,1,1,0) luma 1 ang(-22,-29) sh 0.6 ssao 1 | (0,0,0,1) luma 0 ang(45,0) sh 50 ssao 1 | 0 | 1/0 | 0 | 1 |
| m32_00 | (0.86,0.936,1,0.05) luma 0.9244 ang(-55,-58) sh 1 ssao 1 | (0.787,0.816,0.985,0.02) luma 0.8223 ang(-50,-99) sh 50 ssao 1 | (0,0,0,1) luma 0 ang(45,0) sh 50 ssao 1 | 1 | 1/1 | 0 | 0.96 |
| m33_00 | (0.601,0.752,1,0.05) luma 0.7378 ang(-33,79) sh 0.8 ssao 0.77 | (0,0,0,1) luma 0 ang(-30,-90) sh 1.1 ssao 1 | (0,0,0,1) luma 0 ang(45,0) sh 50 ssao 1 | 0 | 1/0 | 0 | 1 |
| m34_01 | (0.901,0.79,0.688,0.06) luma 0.8063 ang(-79,10) sh 0.4 ssao 0.76 | (0.735,0.843,1,0) luma 0.831 ang(-41,-173) sh 0.5 ssao 1 | (0,0,0,1) luma 0 ang(45,0) sh 50 ssao 1 | 0 | 1/1 | 0 | 1 |
| m35_00 | (0.694,0.787,1,0.3) luma 0.7827 ang(-26,46) sh 0.5 ssao 0.5 | (1,0.921,0.851,0) luma 0.933 ang(-90,3) sh 2 ssao 1 | (0,0,0,1) luma 0 ang(-35,125) sh 50 ssao 1 | 0 | 1/1 | 0.3 | 1 |
| m36_00 | (0.275,0.294,0.287,0.2) luma 0.2894 ang(-61,-130) sh 7.6 ssao 1 | (0,0,0,1) luma 0 ang(45,0) sh 50 ssao 1 | (0,0,0,1) luma 0 ang(45,0) sh 50 ssao 1 | 0 | 1/1 | 0 | 1 |
| m37_00 | (0.802,0.845,0.883,0.05) luma 0.8388 ang(33,-110) sh 50 ssao 0 | (0,0,0,1) luma 0 ang(45,0) sh 50 ssao 1 | (0,0,0,0.997) luma 0 ang(45,0) sh 50 ssao 1 | 0 | 0/0 | 0.3 | 1 |
| m38_00 | (0.892,0.95,1,0.08) luma 0.9416 ang(-90,0) sh 1 ssao 0.6 | (1,1,1,0) luma 1 ang(-27,115) sh 1 ssao 0.6 | (0,0,0,1) luma 0 ang(45,0) sh 50 ssao 1 | 1 | 1/0 | 0 | 1 |
| m39_00 | (0.506,0.834,1,0.06) luma 0.7761 ang(-90,7) sh 0.3 ssao 1 | (0,0,0,0) luma 0 ang(-52,-34) sh 2 ssao 1 | (0,0,0,3.39) luma 0 ang(31,7) sh 50 ssao 1 | 0 | 1/1 | 0 | 1 |
| m40_00 | (0.621,0.77,1,0.1) luma 0.7547 ang(-34,-4) sh 50 ssao 0 | (0,0,0,1) luma 0 ang(-40,-164) sh 1.9 ssao 1 | (0,0,0,0.997) luma 0 ang(45,0) sh 50 ssao 1 | 0 | 1/0 | 0.3 | 1 |
| m41_00 | (0.714,0.799,1,0.15) luma 0.7954 ang(-15,-134) sh 1 ssao 0.6 | (0,0,0,0.7) luma 0 ang(-45,82) sh 1.9 ssao 1 | (0,0,0,0.997) luma 0 ang(45,0) sh 50 ssao 1 | 0 | 1/0 | 0.3 | 1 |
| m45_00 | (0.618,0.767,1,0.3) luma 0.7519 ang(-45,110) sh 0.6 ssao 0.75 | (1,1,1,0) luma 1 ang(-40,-180) sh 0.8 ssao 1 | (0,0,0,1) luma 0 ang(45,0) sh 50 ssao 1 | 0 | 1/0 | 0 | 1 |
| m46_00 | (0.901,0.79,0.688,0.06) luma 0.8063 ang(-79,10) sh 0.4 ssao 0.76 | (0.735,0.843,1,0) luma 0.831 ang(-41,-173) sh 0.5 ssao 1 | (0,0,0,1) luma 0 ang(45,0) sh 50 ssao 1 | 0 | 1/1 | 0 | 1 |
| m47_00 | (0.0641,0.108,0.0729,2.38) luma 0.09605 ang(-32,-90) sh 1 ssao 0.6 | (0,0,0,0.7) luma 0 ang(-45,82) sh 1.9 ssao 1 | (0,0,0,0.997) luma 0 ang(45,0) sh 50 ssao 1 | 0 | 1/0 | 0.3 | 1 |
| m50_00 | (0.714,0.799,1,0.15) luma 0.7954 ang(-90,0) sh 0.2 ssao 0.6 | (0,0,0,0.7) luma 0 ang(-43,-180) sh 0.5 ssao 1 | (0,0,0,0.997) luma 0 ang(45,0) sh 50 ssao 1 | 0 | 1/0 | 0.3 | 1 |
| m51_00 | (0.618,0.825,0.98,0.05) luma 0.7922 ang(-25,-30) sh 0.6 ssao 0.75 | (0.608,0.824,0.914,0.15) luma 0.7842 ang(-90,0) sh 0.6 ssao 1 | (0,0,0,1) luma 0 ang(45,0) sh 50 ssao 1 | 0 | 1/0 | 0 | 1 |
| m53_00 | (0.86,0.936,1,0.05) luma 0.9244 ang(-55,-58) sh 1 ssao 1 | (0.787,0.816,0.985,0.02) luma 0.8223 ang(-50,-99) sh 50 ssao 1 | (0,0,0,1) luma 0 ang(45,0) sh 50 ssao 1 | 1 | 1/1 | 0 | 0.96 |

### 2.3 Hemisphere colours and the ratio, id 0

ratio = ((luma(up) + luma(down)) / 2) / luma(L0 diffuse), all stored values, alpha ignored (the figure the task asked for). ratio-sq-a = the same with each colour squared and multiplied by its 4th float first (INFERRED convention from STUDY-R7 3.2; it is not the engine's proven formula).

| area | hemi up | up luma | hemi down | down luma | hemi mean luma | sun diff luma | ratio | ratio-sq-a |
|---|---|---|---|---|---|---|---|---|
| m21_00 | (0.5,0.5,0.5,1.02) | 0.5 | (0.5,0.5,0.5,1) | 0.5 | 0.5 | 0.8134 | 0.6147 | 0.3184 |
| m22_00 | (0.5,0.5,0.5,0) | 0.5 | (0.0564,0.128,0.169,0) | 0.1155 | 0.3077 | 0.8387 | 0.3669 | 0 |
| m30_00 | (0.5,0.5,0.5,1) | 0.5 | (0.5,0.5,0.5,1) | 0.5 | 0.5 | 0.9509 | 0.5258 | 0.2245 |
| m30_01 | (0.5,0.5,0.5,1) | 0.5 | (0.5,0.5,0.5,1) | 0.5 | 0.5 | 0.7932 | 0.6304 | 0.3046 |
| m31_00 | (0,0,0,0) | 0 | (0,0,0,0) | 0 | 0 | 0.955 | 0 | 0 |
| m32_00 | (0,0,0,0) | 0 | (0,0,0,0) | 0 | 0 | 0.9528 | 0 | 0 |
| m33_00 | (0.5,0.5,0.5,0.222) | 0.5 | (0.5,0.5,0.5,1) | 0.5 | 0.5 | 0.8685 | 0.5757 | 0.1997 |
| m34_01 | (0.5,0.5,0.5,1) | 0.5 | (0.5,0.5,0.5,1) | 0.5 | 0.5 | 0.7744 | 0.6456 | 0.3039 |
| m35_00 | (1,1,1,0) | 1 | (1,1,1,0) | 1 | 1 | 0.8473 | 1.18 | 0 |
| m36_00 | (0.5,0.5,0.5,1) | 0.5 | (0.5,0.5,0.5,1) | 0.5 | 0.5 | 0.6149 | 0.8131 | 1.32 |
| m37_00 | (1,1,1,0) | 1 | (1,1,1,0) | 1 | 1 | 0.8013 | 1.248 | 0 |
| m38_00 | (0,0,0,0) | 0 | (0,0,0,0) | 0 | 0 | 0.6512 | 0 | n/a |
| m39_00 | (0,0,0,0) | 0 | (0,0,0,0) | 0 | 0 | 0.8589 | 0 | 0 |
| m40_00 | (1,1,1,0) | 1 | (1,1,1,0) | 1 | 1 | 0.9144 | 1.094 | 0 |
| m41_00 | (1,1,1,0) | 1 | (1,1,1,0) | 1 | 1 | 0.9393 | 1.065 | 0 |
| m45_00 | (0,0,0,0) | 0 | (0,0,0,0) | 0 | 0 | 0.8903 | 0 | 0 |
| m46_00 | (0.5,0.5,0.5,1) | 0.5 | (0.5,0.5,0.5,1) | 0.5 | 0.5 | 0.7744 | 0.6456 | 0.3039 |
| m47_00 | (1,1,1,0) | 1 | (1,1,1,0) | 1 | 1 | 0.729 | 1.372 | 0 |
| m50_00 | (1,1,1,0) | 1 | (1,1,1,0) | 1 | 1 | 0.8676 | 1.153 | 0 |
| m51_00 | (0,0,0,0) | 0 | (0,0,0,0) | 0 | 0 | 0.9003 | 0 | 0 |
| m53_00 | (0,0,0,0) | 0 | (0,0,0,0) | 0 | 0 | 0.9528 | 0 | 0 |

### 2.4 Ratio summary over all areas and ids (344 area-id rows)

- OBSERVED/computed: 344 rows; ratio undefined (sun diffuse luma 0): 4 (m31_00#6, m37_00#1, m37_00#3, m37_00#4); ratio exactly 0 (hemisphere colours black): 94; positive: 246.
- ratio over the positive rows: min 0.1289 (m38_00#38001), median 0.6456, max 1.431 (m47_00#30). Over all defined rows including the zeros: median 0.5628.
- Highest 6: m47_00#30 1.431, m47_00#0 1.372, m50_00#310 1.317, m41_00#310 1.317, m47_00#310 1.302, m37_00#2 1.284. Lowest positive 6: m38_00#38001 0.1289, m38_00#38002 0.1379, m22_00#2 0.3558, m22_00#0 0.3669, m30_00#2 0.5, m30_00#3 0.5.
- ratio-sq-a (each colour squared times its 4th float): 125 rows are exactly 0 (a hemisphere 4th float of 0 or black), 117 positive, 102 undefined (the sun 4th float is 0); positive: min 0.01422 (m30_01#121), median 0.25, max 19.45 (m22_00#33). Highest 5: m22_00#33 19.45, m22_00#32 12.08, m22_00#34 11.21, m22_00#31 7.785, m38_00#1 6.027.
- OBSERVED: rows where both hemisphere colours have 4th float 0: 180 of 344 (the stored rgb is then often non-zero, e.g. m47_00 id 30 hemisphere rgb (1,1,1) with 4th float 0, so the stored-rgb ratio there is 1.43 while the squared-times-4th-float figure is 0).

| area | ids | ids with ratio 0 | positive ratio min / median / max | median incl. zeros | max ratio-sq-a |
|---|---|---|---|---|---|
| m21_00 | 2 | 1 | 0.6147 / 0.6147 / 0.6147 | 0.3073 | 0.3184 |
| m22_00 | 9 | 0 | 0.3558 / 0.5432 / 0.932 | 0.5432 | 19.45 |
| m30_00 | 43 | 4 | 0.5 / 0.5456 / 0.8632 | 0.5349 | 3.587 |
| m30_01 | 29 | 2 | 0.5 / 0.5899 / 0.6754 | 0.5797 | 3.96 |
| m31_00 | 19 | 12 | 0.5777 / 1.043 / 1.075 | 0 | 0.5434 |
| m32_00 | 17 | 14 | 0.562 / 0.562 / 0.562 | 0 | 0.1576 |
| m33_00 | 17 | 0 | 0.5301 / 0.5732 / 0.9065 | 0.5732 | 2.883 |
| m34_01 | 7 | 0 | 0.5312 / 0.6159 / 0.6456 | 0.6159 | 0.3039 |
| m35_00 | 24 | 0 | 0.5 / 1.106 / 1.226 | 1.106 | 0.8863 |
| m36_00 | 8 | 0 | 0.5 / 0.558 / 0.8667 | 0.558 | 1.32 |
| m37_00 | 16 | 0 | 0.5103 / 1.126 / 1.284 | 1.126 | 0.8529 |
| m38_00 | 19 | 4 | 0.1289 / 0.5702 / 0.9285 | 0.5436 | 6.027 |
| m39_00 | 15 | 12 | 0.5369 / 0.5991 / 0.6038 | 0 | 1.809 |
| m40_00 | 15 | 0 | 0.6088 / 1.094 / 1.18 | 1.094 | 1.182 |
| m41_00 | 13 | 0 | 0.5103 / 1.032 / 1.317 | 1.032 | 0.2096 |
| m45_00 | 17 | 16 | 0.5 / 0.5 / 0.5 | 0 | 0.05 |
| m46_00 | 8 | 0 | 0.5312 / 0.5937 / 0.6456 | 0.5937 | 0.3039 |
| m47_00 | 13 | 0 | 0.5155 / 1.032 / 1.431 | 1.032 | 0.217 |
| m50_00 | 21 | 0 | 0.5628 / 1.153 / 1.317 | 1.153 | 0.1577 |
| m51_00 | 15 | 15 | none | 0 | 0 |
| m53_00 | 17 | 14 | 0.562 / 0.562 / 0.562 | 0 | 0.1576 |

Extreme areas (stored-rgb ratio, computed): hemisphere black (ratio 0) in most ids of m21_00 1/2, m31_00 12/19, m32_00 14/17, m39_00 12/15, m45_00 16/17, m51_00 15/15, m53_00 14/17; areas whose median positive ratio exceeds 1: m31_00, m35_00, m37_00, m40_00, m41_00, m47_00, m50_00; lowest values m38_00 ids 38001, 38002 (0.129, 0.138). m22_00 holds the largest ratio-sq-a (up to 19.45 at ids 31-34; id 33: sun 4th float 0.047 against a hemisphere-up 4th float 2.02).

### 2.5 Other LightSet statistics over the 344 rows (OBSERVED)

| quantity | rows | min | median | max | rows equal 0 |
|---|---|---|---|---|---|
| L0 diffuse 4th float | 344 | 0 (m30_00#122) | 1 | 20 (m30_01#121) | 102 |
| L0 diffuse luma | 344 | 0 (m31_00#6) | 0.8842 | 1 (m53_00#31) | 4 |
| L0 specular luma | 344 | 0 (m31_00#6) | 0.8766 | 1 (m53_00#31) | 4 |
| L0 Sharpness | 335 | 0.1 (m30_00#1) | 1.7 | 50 (m50_00#120) | 0 |
| L0 SsaoWeight | 335 | 0 (m21_00#0) | 0.73 | 1 (m53_00#31) | 42 |
| L1 diffuse 4th float | 344 | 0 (m22_00#0) | 0.3 | 9.236 (m30_01#24) | 72 |
| L1 diffuse luma | 344 | 0 (m30_00#11) | 0.795 | 1 (m53_00#350) | 34 |
| L2 diffuse luma | 335 | 0 (m30_00#11) | 0.7827 | 1 (m51_00#6) | 116 |
| L3 diffuse luma | 335 | 0 (m30_00#0) | 0 | 1 (m51_00#120) | 264 |
| hemi up 4th float | 344 | 0 (m21_00#2) | 0 | 16 (m33_00#33000) | 192 |
| hemi down 4th float | 344 | 0 (m21_00#2) | 0 | 10.73 (m38_00#38002) | 191 |
| hemi up luma | 344 | 0 (m21_00#2) | 0.5 | 1 (m50_00#310) | 95 |
| hemi down luma | 344 | 0 (m21_00#2) | 0.5 | 1 (m50_00#310) | 95 |
| giOcclusionRate | 335 | 0 (m21_00#0) | 0 | 0.3 (m50_00#310) | 252 |
| LightMask Influence | 335 | 0 (m21_00#0) | 1 | 1 (m53_00#111) | 9 |

Flag combinations (Light1 FollowedCamera, Enable LocalLight1, Enable LocalLight2) over the 344 rows: (0, 1, 1): 125, (0, 1, 0): 75, (0, 0, 0): 74, (1, 1, 1): 42, (1, 1, 0): 12, (0, None, None): 9, (1, 0, 0): 6, (1, 0, 1): 1.
L1 diffuse is black (luma 0) in 34 rows, L2 in 116, L3 in 264. (OBSERVED)

## 3. Fog (OBSERVED, `fog-base-areas.tsv` has every parameter of every area and id; 150 area-id rows in the 21 base files)

Meaning of names is not stated in the file; they are reported as stored. Types: Color/heightColor type 14 (r,g,b,a), ToUpper type 11 (byte), the rest type 9 (float).

### 3.1 Ranges over all area-id rows of the 21 base files

| parameter | rows | min | median | max | rows = 0 |
|---|---|---|---|---|---|
| Color (luma) | 150 | 0 (m21_00#2) | 0.598 | 3.006 (m38_00#4) | 4 |
| heightColor (luma) | 150 | 0 (m21_00#2) | 0.6486 | 3.334 (m37_00#9) | 13 |
| Dist | 150 | 0 (m21_00#2) | 120 | 2000 (m30_00#320) | 10 |
| heightDist | 150 | 0 (m21_00#2) | 3 | 2000 (m40_00#9) | 13 |
| DistOffset | 150 | -2000 (m32_00#2) | 13.5 | 2000 (m30_00#320) | 35 |
| HeightOffset | 150 | -1350 (m32_00#350) | -5.6 | 2000 (m38_00#11) | 23 |
| heightFogDensity | 150 | 0.1 (m33_00#1) | 75.9 | 100 (m21_00#2) | 0 |
| heightFogIntencity | 150 | -10 (m33_00#1) | -0.01181 | 10 (m45_00#2) | 0 |
| ToUpper | 150 | 0 (m21_00#0) | 0 | 1 (m21_00#2) | 113 |
| Alpha | 150 | 0 (m22_00#2) | 1 | 1 (m21_00#0) | 5 |
| Contrast | 150 | 0 (m21_00#2) | 0.2 | 0.77 (m38_00#15) | 55 |

- Color 4th float: min 0 (m36_00#10), median 0.4338, max 3 (m47_00#1); equal 0 in 5 rows.
- heightColor 4th float: min 0 (m21_00#0), median 0.975, max 100 (m50_00#9); equal 0 in 20 rows.

- Dist = 0 (OBSERVED): m21_00 ids 2; m30_00 ids 11 12; m32_00 ids 350; m34_01 ids 11; m37_00 ids 9; m38_00 ids 11; m46_00 ids 11; m47_00 ids 12; m53_00 ids 350.
- heightFogDensity = 0 (OBSERVED): no row (min 0.1).
- Dist = 0 and heightFogDensity = 0 together (no fog term from these two; INFERRED as "fog off"): 0 rows.
- Alpha = 0: 5 rows: m22_00#2, m30_00#20, m30_01#20, m34_01#20, m46_00#20.
- The group has no enable flag; `ToUpper` is the only byte flag (value 0 or 1: 37 rows with 1).
- `heightFogBase` appears only in `s_template` (value 0). In the 21 base files `Alpha` range is min..max as above; `default` Fog ids 0, 10-15.

### 3.2 Fog id 0 of each area (all parameters; the other ids in `fog-base-areas.tsv`)

| area | Color | luma | heightColor | luma | Dist | heightDist | DistOffset | HeightOffset | heightFogDensity | heightFogIntencity | ToUpper | Alpha | Contrast |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| m21_00 | (0.76,0.905,1,0.134) | 0.881 | (0.835,0.922,0.812,0) | 0.8953 | 20 | 49.2 | 0 | 0 | 34.5 | -0.02899 | 0 | 1 | 0.17 |
| m22_00 | (0.386,0.399,0.419,0.665) | 0.3979 | (0.291,0.306,0.326,0.789) | 0.304 | 100 | 10 | 50 | -30 | 5 | 0.2 | 1 | 0.65 | 0.14 |
| m30_00 | (1,0.932,0.814,0.12) | 0.9382 | (0.899,0.885,1,0) | 0.8958 | 120 | 5 | 20 | 100 | 70.9 | -0.0141 | 0 | 1 | 0.3 |
| m30_01 | (0.448,0.482,0.487,0.206) | 0.4749 | (0.614,0.589,0.544,0.177) | 0.5909 | 50 | 1 | 10 | 133.3 | 84.2 | 0.01188 | 1 | 1 | 0.4 |
| m31_00 | (0.358,0.414,0.456,0.268) | 0.4052 | (0.447,0.514,0.529,0.864) | 0.5006 | 50 | 0.62 | 15 | -60 | 100 | -0.01 | 0 | 1 | 0.4 |
| m32_00 | (0.772,0.842,0.952,0.2) | 0.8352 | (0.627,0.667,0.706,1) | 0.6612 | 120 | 2 | 15 | -15 | 75.9 | -0.01318 | 0 | 1 | 0.2 |
| m33_00 | (0.506,0.49,0.456,0.35) | 0.491 | (0.749,0.752,0.766,0.245) | 0.7525 | 30 | 1.8 | 15 | 30 | 100 | 0.01 | 1 | 1 | 0.4 |
| m34_01 | (0.448,0.482,0.487,0.206) | 0.4749 | (0.67,0.808,0.851,0.144) | 0.7822 | 50 | 1 | 10 | 133.3 | 84.2 | 0.01188 | 1 | 1 | 0.4 |
| m35_00 | (0.459,0.513,0.552,0.245) | 0.5041 | (0.69,0.75,0.783,0) | 0.7396 | 30 | 0.2 | 10 | -5.6 | 94.3 | 0.0106 | 1 | 1 | 0.3 |
| m36_00 | (0.784,0.784,0.902,0.5) | 0.7928 | (0.314,0.314,0.391,4) | 0.3193 | 80 | 327.9 | 100 | -200 | 0.2 | -5 | 0 | 0.15 | 0.5 |
| m37_00 | (0.588,0.863,1,1) | 0.8143 | (0.588,0.863,1,0) | 0.8143 | 1000 | 3 | 0.5 | -22 | 25 | -0.04 | 0 | 0.7 | 0.55 |
| m38_00 | (0.276,0.335,0.366,0.637) | 0.3249 | (1,0.952,0.904,0) | 0.9588 | 60 | 1 | 10 | 0 | 0.1 | -10 | 0 | 1 | 0.31 |
| m39_00 | (0.267,0.361,0.478,0.344) | 0.3493 | (0.0648,0.0789,0.0901,1) | 0.07669 | 110 | 1.5 | 15 | -120 | 22.6 | -0.04425 | 0 | 1 | 0.35 |
| m40_00 | (0.417,0.459,0.524,0.256) | 0.4548 | (0.572,0.623,0.71,1) | 0.6181 | 60 | 10 | 15 | -833.3 | 22.6 | -0.04425 | 0 | 1 | 0.35 |
| m41_00 | (0.437,0.49,0.538,0.05) | 0.4822 | (0.617,0.594,0.625,1) | 0.6014 | 120 | 6 | 200 | -790 | 100 | -0.01 | 0 | 1 | 0.35 |
| m45_00 | (0.455,0.537,0.627,0.5) | 0.5263 | (0.514,0.62,0.749,1) | 0.6064 | 60 | 300 | 10 | -20 | 100 | -1 | 0 | 1 | 0.15 |
| m46_00 | (0.448,0.482,0.487,0.206) | 0.4749 | (0.67,0.808,0.851,0.144) | 0.7822 | 50 | 1 | 10 | 133.3 | 84.2 | 0.01188 | 1 | 1 | 0.4 |
| m47_00 | (0.437,0.49,0.538,0.05) | 0.4822 | (0.617,0.594,0.625,1) | 0.6014 | 120 | 6 | 200 | -790 | 100 | -0.01 | 0 | 1 | 0.35 |
| m50_00 | (0.437,0.49,0.538,0.1) | 0.4822 | (0.51,0.569,0.627,1) | 0.5604 | 120 | 80 | 12 | 100 | 100 | -1 | 0 | 1 | 0.25 |
| m51_00 | (0.6,0.6,0.51,0.13) | 0.5935 | (0.431,0.352,0.138,1) | 0.3535 | 180 | 1000 | 30 | -1000 | 100 | -1 | 0 | 1 | 0 |
| m53_00 | (0.772,0.842,0.952,0.2) | 0.8352 | (0.627,0.667,0.706,1) | 0.6612 | 120 | 2 | 15 | -15 | 75.9 | -0.01318 | 0 | 1 | 0.2 |

Per-area ranges (all ids): Dist and heightDist, plus count of ids:

| area | ids | Dist | heightDist | heightFogDensity | heightFogIntencity |
|---|---|---|---|---|---|
| m21_00 | 2 | 0..20 | 0..49.2 | 34.5..100 | -0.02899..0.01 |
| m22_00 | 3 | 20..100 | 2..10 | 5..9.9 | 0.101..0.2 |
| m30_00 | 9 | 0..2000 | 0..120 | 4.5..100 | -0.2222..0.2 |
| m30_01 | 7 | 50..300 | 0..33.3 | 5..84.2 | -0.1..0.2 |
| m31_00 | 14 | 50..2000 | 0..20 | 3..100 | -0.3333..0.01 |
| m32_00 | 12 | 0..2000 | 0.4..183.3 | 39.2..100 | -0.2..-0.01 |
| m33_00 | 6 | 30..300 | 0..1475 | 0.1..100 | -10..0.2 |
| m34_01 | 4 | 0..300 | 0..10 | 5..100 | -0.05..0.2 |
| m35_00 | 11 | 30..1533 | 0.2..6 | 67..94.3 | -0.08..0.01493 |
| m36_00 | 2 | 80..300 | 213.1..327.9 | 0.2..60 | -5..-0.01667 |
| m37_00 | 9 | 0..2000 | 0..1500 | 0.5..100 | -2..1 |
| m38_00 | 11 | 0..327.9 | 0..508.2 | 0.1..100 | -10..0.2 |
| m39_00 | 5 | 100..110 | 0..15 | 20..100 | -0.05..0.01 |
| m40_00 | 7 | 23.3..2000 | 0.1..2000 | 0.1..100 | -10..-0.01 |
| m41_00 | 7 | 23.3..2000 | 0.1..2000 | 0.1..100 | -10..-0.01 |
| m45_00 | 4 | 12..2000 | 0.2..300 | 100..100 | -1..10 |
| m46_00 | 5 | 0..300 | 0..10 | 5..100 | -0.05..0.2 |
| m47_00 | 7 | 0..2000 | 0.1..2000 | 0.1..100 | -10..-0.01 |
| m50_00 | 9 | 23.3..2000 | 24..2000 | 0.1..100 | -10..-0.01 |
| m51_00 | 4 | 12..2000 | 0.2..1000 | 100..100 | -1..10 |
| m53_00 | 12 | 0..2000 | 0.4..183.3 | 39.2..100 | -0.2..-0.01 |

## 4. LightScattering and Ls

OBSERVED: one id (0) per file. Names (Japanese, glosses in section 1): angles X, Y in deg; distance multiplier %; light colour R, G, B (int16); light RGB multiplier %; scatter direction coefficient; Rayleigh coefficient; Mie coefficient; blend coefficient %; ground-reflected-light colour R, G, B; its RGB multiplier %. (`光源色` = "light colour", `地上の乱反射光` = "ground diffuse-reflected light": my reading, INFERRED.)

`default` id 0: 光源X軸回転角度[deg]=0, 光源Y軸回転角度[deg]=0, 距離倍率[%]=100, 光源色R=0, 光源色G=0, 光源色B=0, 光源RGB倍率[%]=100, 散乱方向係数=0, レイリー散乱係数=0, ミー散乱係数=0, ブレンド係数[%]=0, 地上の乱反射光色R=0, 地上の乱反射光色G=0, 地上の乱反射光色B=0, 地上の乱反射光RGB倍率[%]=100.

- OBSERVED: 89 files besides `default` hold LightScattering; **0 of them differ from `default` in any parameter** (every one holds exactly the default values: all angles 0, distance multiplier 100, colours 0, light RGB multiplier 100, scatter/Rayleigh/Mie 0, blend 0, ground colours 0, ground multiplier 100).
- INFERRED: with the blend coefficient 0 and colours 0 in every file, the stored record is a neutral placeholder; nothing here shows the game using it in the 168 files (no code was read).
- Base areas with a non-zero blend coefficient: none.

`Ls` (OBSERVED): only `s_template` holds it, 15 parameters, the same names and the same default-like values as above.

## 5. Dominant Light (own file only; no `default`/`mapbase` copy)

`Dominant Light` has one id (0) per file that has it; of the 21 base files it is absent from m21_00, m22_00, m36_00, m38_00. Values (OBSERVED; the parameter named `Dominant RGB Scale` appears in one file only):

| area | Dom Sharp Min Value | Dom Sharp Offset | Dom Sharp Exponant | Dom Color Sc | Dom R Sc | Dom G Sc | Dom B Sc | SH Color Sc | SH R Sc | SH G Sc | SH B Sc | Dom RGB Sc |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| m30_00 | 0.5 | 0.55 | 1.25 | 0.5 | 1 | 1 | 1 | 0.5 | 1 | 1 | 1 | - |
| m30_01 | 0.5 | 1 | 1.25 | 0.5 | 1 | 1 | 1 | 1 | 1 | 1 | 1 | - |
| m31_00 | 0.5 | 0.2 | 1.25 | 0.8 | 1 | 1 | 1 | 1 | 1 | 1 | 1 | - |
| m32_00 | 0.5 | 0.85 | 1.25 | 0.9 | 1 | 1 | 1.03 | 1 | 1 | 1 | 1 | - |
| m33_00 | 0.5 | 0 | 1.25 | 1 | 0.8 | 1.1 | 1.1 | 1 | 1 | 1 | 1 | - |
| m34_01 | 0.5 | 0.55 | 1.25 | 0.2 | 1 | 1 | 1 | 0.2 | 1 | 1 | 1 | - |
| m35_00 | 0.5 | 1 | 1 | 1 | 1 | 1 | 1 | 1 | 1 | 1 | 1 | - |
| m37_00 | 0.5 | 0 | 1.25 | 0.45 | 1 | 1 | 1 | 1 | 1 | 1 | 1 | - |
| m39_00 | 0.5 | 0 | 1.25 | 1 | 1 | 1 | 1 | 1 | 1 | 1 | 1 | - |
| m40_00 | 0.5 | 1 | 0.86 | 1.2 | 0.6 | 0.8 | 1 | 1 | 1 | 1 | 1 | - |
| m41_00 | 0.5 | 1 | 0.86 | 0.5 | 1 | 1 | 1 | 1 | 1 | 1 | 1 | - |
| m45_00 | 0.5 | 0.2 | 1.25 | 1 | 1 | 1 | 1 | 1 | 1 | 1 | 1 | - |
| m46_00 | 0.5 | 0.55 | 1.25 | 0.2 | 1 | 1 | 1 | 0.2 | 1 | 1 | 1 | - |
| m47_00 | 0.5 | 1 | 0.86 | 0.5 | 1 | 1 | 1 | 1 | 1 | 1 | 1 | - |
| m50_00 | 0.5 | 1 | 0.86 | 0.5 | 1 | 1 | 1 | 1 | 1 | 1 | 1 | - |
| m51_00 | 0.5 | 0.2 | 1.25 | 0.8 | 1 | 1 | 1 | 1 | 1 | 1 | 1 | - |
| m53_00 | 0.5 | 0.85 | 1.25 | 0.9 | 1 | 1 | 1.03 | 1 | 1 | 1 | 1 | - |

`Dominant RGB Scale` is in: m36_00_0002.

## 6. Env Map, AOFields, AOFields Model

### 6.1 Env Map (`EnvMap:ID` type 7, `EnvMap:ColorScale` type 9; `envmap-base-areas.tsv` lists every id)

| area | ids | distinct EnvMap:ID | ColorScale min / median / max | ids with scale 0 |
|---|---|---|---|---|
| m21_00 | 1 | 1 | 1 / 1 / 1 | 0 |
| m22_00 | 2 | 2 | 1 / 1 / 1 | 0 |
| m30_00 | 40 | 40 | 0.8 / 1 / 2 | 0 |
| m30_01 | 35 | 35 | 1 / 1 / 1 | 0 |
| m31_00 | 28 | 27 | 1 / 1 / 1 | 0 |
| m32_00 | 36 | 36 | 1 / 1 / 1 | 0 |
| m33_00 | 51 | 51 | 0.5 / 1 / 1.2 | 0 |
| m34_01 | 26 | 26 | 1 / 1 / 1 | 0 |
| m35_00 | 23 | 23 | 1 / 1 / 1.25 | 0 |
| m36_00 | 3 | 3 | 0.5 / 0.5 / 0.7 | 0 |
| m37_00 | 58 | 38 | 1 / 1 / 1 | 0 |
| m38_00 | 35 | 34 | 0.5 / 1 / 1.2 | 0 |
| m39_00 | 32 | 25 | 0 / 1 / 1 | 1 |
| m40_00 | 38 | 14 | 0.5 / 1 / 1 | 0 |
| m41_00 | 6 | 6 | 1 / 1 / 1 | 0 |
| m45_00 | 81 | 57 | 1 / 1 / 1 | 0 |
| m46_00 | 2 | 2 | 1 / 1 / 1 | 0 |
| m47_00 | 1 | 1 | 1 / 1 / 1 | 0 |
| m50_00 | 32 | 32 | 1 / 1 / 1 | 0 |
| m51_00 | 35 | 35 | 1 / 1 / 1 | 0 |
| m53_00 | 7 | 7 | 1 / 1 / 1 | 0 |

OBSERVED over the base files: ColorScale min 0 (m39_00#1000), max 2 (m30_00#401), median 1; `default` ids: 0: ID 10 scale 1.5, 10: ID 10 scale 1, 11: ID 10 scale 1, 12: ID 10 scale 1, 13: ID 10 scale 1, 14: ID 10 scale 1, 15: ID 10 scale 1.

### 6.2 AOFields and AOFields Model

OBSERVED: these two groups are in `mapbase` only (one id 0, no other file); every map inherits them. The values are:

| group | parameter | type | mapbase id 0 |
|---|---|---|---|
| AOFields | Enable | 7 | 1 |
| AOFields | Enable UpScale | 7 | 1 |
| AOFields | Global Strength | 9 | 1 |
| AOFields | Depth Thresh | 9 | 0.03 |
| AOFields | Distance Max | 9 | 100 |
| AOFields | Distance Margin | 9 | 10 |
| AOFields Model | 3DTexture - AO Strength | 9 | 1 |
| AOFields Model | Sphere Occluder - Light Cone Angle | 9 | 30 |
| AOFields Model | Sphere Occluder - Light Cone Coefficient | 9 | 1 |
| AOFields Model | Sphere Occluder - Length Limit | 9 | 10 |

## 7. ShadowParam (parameters that name a light, a light-set id, an angle or a colour)

OBSERVED: `default` holds ids 0 10 11 12 13 14 15; `mapbase` holds id 0 only (22 rows); of the 21 base files only m21_00, m22_00 and m33_00 carry a ShadowParam id (id 0), the others inherit `mapbase` id 0 then `default`. Of the 86 variant files, ShadowParam is in: m32_00_0100, m33_00_0010, m36_00_0003, m51_00_0200.

| file | id | Angle deg (x,y) | Shadow  Light Id (type 7) | Shadow Light Id (type 1) | Shadow Color (hex ARGB) |
|---|---|---|---|---|---|
| default | 0 | -43,54 | - | - | FF000000 |
| default | 10 | -42,139 | - | - | FF000000 |
| default | 11 | -58,144 | - | - | FF000000 |
| default | 12 | -42,139 | - | - | FF000000 |
| default | 13 | -42,139 | - | - | FF000000 |
| default | 14 | -42,139 | - | - | FF000000 |
| default | 15 | -42,139 | - | - | FF000000 |
| mapbase | 0 | 0,0 | - | - | FF000000 |
| m21_00_0000 | 0 | -27,118 | - | 255 | FF000000 |
| m22_00_0000 | 0 | -27,-162 | - | 255 | D5001125 |
| m33_00_0000 | 0 | 0,0 | - | - | FF000000 |

Shadow Color values over all files: FF000000 x108, D5001125 x1, FF232323 x1, FF040407 x1. `Shadow Light Id` (type 1) is in m21_00_0000, m22_00_0000 (value 255) and s_template (value 0) only.

## 8. Variant files (`mAA_BB_NNNN`, NNNN != 0000) whose effective values differ from their base

Comparison (OBSERVED, computed): for every group in scope and every id the variant file carries, the effective value through the chain variant, base `_0000`, `mapbase`, `default` against the base's own effective value for the same id. Ids the variant carries that the base does not (new ids) count as differing. Each line: variant, number of differing values, then per group the parameters that differ with the number of ids, and an example (group#id param base -> variant). The full list (every value pair) is `variant-diffs.tsv`. Variants carrying no group in scope, or only values equal to the base: listed last.

- `m30_00_0010` 101: LightSet: L0.Diff x14, L0.Spec x13, L1.Diff x11 +12; Fog: Color x3, heightColor x3, heightFogIntencity x3 +5; e.g. LightSet#0 L0.Diff (1,0.945,0.869,1.23) -> (0.961,0.718,0.6,1.6)
- `m30_00_0100` 76: LightSet: L0.Angle x5, L0.Diff x5, L0.Spec x5 +15; Fog: Color x1, heightColor x1, Dist x1 +8; Dominant Light: Dominant Sharpness Offset x1, Dominant Color Scale x1, Dominant R Scale x1 +2; Env Map: EnvMap:ID x1, EnvMap:ColorScale x1; e.g. LightSet#0 L0.Angle (-0.768,-0.96) -> (-0.297,-0.122)
- `m30_00_0110` 76: LightSet: L0.Angle x5, L0.Diff x5, L0.Spec x5 +15; Fog: Color x1, heightColor x1, Dist x1 +8; Dominant Light: Dominant Sharpness Offset x1, Dominant Color Scale x1, Dominant R Scale x1 +2; Env Map: EnvMap:ID x1, EnvMap:ColorScale x1; e.g. LightSet#0 L0.Angle (-0.768,-0.96) -> (-0.297,-0.122)
- `m30_00_0200` 101: LightSet: L0.Diff x7, L2.Diff x7, L1.Diff x6 +15; Fog: Alpha x4, Color x4, heightColor x4 +7; e.g. LightSet#10 L0.Angle (-0.209,-0.82) -> (-0.977,-0.576)
- `m30_01_0010` 100: LightSet: L0.Diff x10, L0.Spec x9, L1.Diff x7 +20; Fog: Color x3, heightColor x3, Dist x3 +7; Dominant Light: Dominant Sharpness Offset x1, Dominant Color Scale x1; e.g. LightSet#0 L0.Diff (0.863,0.784,0.676,1.3) -> (1,0.723,0.557,1.2)
- `m30_01_0100` 3: Dominant Light: Dominant Sharpness Offset x1, Dominant Color Scale x1, SH Color Scale x1; e.g. Dominant Light#0 Dominant Sharpness Offset 1 -> 0.7
- `m30_01_0110` 3: Dominant Light: Dominant Sharpness Offset x1, Dominant Color Scale x1, SH Color Scale x1; e.g. Dominant Light#0 Dominant Sharpness Offset 1 -> 0.7
- `m30_01_0210` 8: Fog: Color x1, heightColor x1, Dist x1 +5; e.g. Fog#0 Color (0.448,0.482,0.487,0.206) -> (0.315,0.287,0.242,0.617)
- `m31_00_0010` 66: LightSet: L2.Angle x3, L0.Diff x2, L0.Spec x2 +13; Fog: Color x10, heightColor x9, Contrast x7 +5; e.g. LightSet#0 L0.Diff (0.968,0.953,0.933,1.5) -> (1,0.819,0.691,1.5) new ids: Fog#300 Fog#310 LightSet#300 LightSet#310
- `m31_00_0020` 31: LightSet: L0.Angle x1, L0.Diff x1, L0.Spec x1 +7; Fog: Color x3, heightColor x3, heightDist x3 +5; Dominant Light: Dominant Sharpness Offset x1, Dominant Color Scale x1, Dominant R Scale x1 +1; e.g. LightSet#0 L0.Angle (-0.506,0.663) -> (-0.995,0.0524)
- `m31_00_0030` 31: LightSet: L0.Angle x1, L0.Diff x1, L0.Spec x1 +7; Fog: Color x3, heightColor x3, heightDist x3 +6; Dominant Light: Dominant Sharpness Offset x1, Dominant Color Scale x1, Dominant R Scale x1 +1; e.g. LightSet#0 L0.Angle (-0.506,0.663) -> (-1.13,1.43)
- `m31_00_0100` 11: LightSet: L0.Diff x1, L0.Spec x1, L1.Diff x1 +8; e.g. LightSet#100 L0.Diff (0.84,0.915,1,1) -> (0.839,0.914,1,1)
- `m31_00_0110` 11: LightSet: L0.Diff x1, L0.Spec x1, L1.Diff x1 +8; e.g. LightSet#100 L0.Diff (0.84,0.915,1,1) -> (0.839,0.914,1,1)
- `m32_00_0010` 246: LightSet: L0.Diff x15, L0.Spec x15, L1.Diff x15 +23; Fog: heightDist x8, Color x6, heightColor x6 +6; e.g. LightSet#0 L0.Diff (0.915,0.959,1,1.6) -> (0.688,0.776,1,0.8) new ids: Fog#320 LightSet#1 LightSet#2 LightSet#20 LightSet#21 LightSet#3 +2
- `m32_00_0100` 189: LightSet: L0.Angle x4, L0.Diff x4, L0.Spec x4 +23; Fog: Color x8, heightColor x8, heightDist x8 +8; Dominant Light: Dominant Sharpness Offset x1, Dominant Sharpness Exponant x1, Dominant Color Scale x1 +1; ShadowParam: Angle x1, Far Fade Start x1, Far Fade Dist x1 +10; SSAO: UseReProj x1, PreventGhost x1, EnableUpScale x1 +3; e.g. LightSet#0 L0.Angle (-0.698,1.17) -> (-0.698,-0.192) new ids: LightSet#1 LightSet#2 LightSet#20 LightSet#3 LightSet#4
- `m32_00_0110` 273: LightSet: L0.Diff x15, L0.Spec x15, L1.Spec x15 +23; Fog: heightDist x8, Color x6, heightColor x6 +6; e.g. LightSet#0 L0.Diff (0.915,0.959,1,1.6) -> (0.688,0.776,1,0.8) new ids: Fog#320 Fog#70 LightSet#1 LightSet#2 LightSet#20 LightSet#21 +3
- `m33_00_0010` 45: LightSet: L0.Diff x7, L0.Spec x7, L1.Diff x7 +8; Fog: Color x2, heightColor x2, Contrast x1; e.g. LightSet#0 L0.Diff (1,0.86,0.566,1) -> (1,0.848,0.7,0.8)
- `m33_00_0100` 14: LightSet: L0.Angle x1, L0.Diff x1, L0.Spec x1 +7; Fog: Color x1, heightColor x1, HeightOffset x1 +1; e.g. LightSet#0 L0.Angle (-0.716,0.436) -> (-0.716,0.454)
- `m33_00_0110` 14: LightSet: L0.Angle x1, L0.Diff x1, L0.Spec x1 +7; Fog: Color x1, heightColor x1, HeightOffset x1 +1; e.g. LightSet#0 L0.Angle (-0.716,0.436) -> (-0.716,0.454)
- `m33_00_0200` 36: LightSet: L0.Diff x2, L0.Spec x2, L0.SsaoW x2 +17; Fog: Color x1, heightColor x1, DistOffset x1 +4; e.g. LightSet#0 L0.Angle (-0.716,0.436) -> (-0.681,0.314)
- `m33_00_0210` 37: LightSet: L0.Diff x2, L0.Spec x2, L0.SsaoW x2 +17; Fog: Color x1, heightColor x1, Dist x1 +5; e.g. LightSet#0 L0.Angle (-0.716,0.436) -> (-0.681,0.314)
- `m33_00_0300` 14: LightSet: L0.Diff x1, L0.Spec x1, L0.SsaoW x1 +6; Fog: Color x1, heightColor x1, Dist x1 +2; e.g. LightSet#100 L0.Diff (0.965,0.945,0.866,0.5) -> (0.871,0.945,1,1.7)
- `m33_00_0310` 21: LightSet: L0.Angle x1, L0.Diff x1, L0.Spec x1 +14; Fog: Color x1, heightColor x1, Dist x1 +1; e.g. LightSet#0 L0.Angle (-0.716,0.436) -> (-0.716,0.454)
- `m33_00_0400` 23: LightSet: L0.Angle x1, L0.Diff x1, L0.Spec x1 +15; Fog: Color x1, heightColor x1, HeightOffset x1 +2; e.g. LightSet#0 L0.Angle (-0.716,0.436) -> (-0.681,0.314)
- `m33_00_0410` 23: LightSet: L0.Angle x1, L0.Diff x1, L0.Spec x1 +15; Fog: Color x1, heightColor x1, HeightOffset x1 +2; e.g. LightSet#0 L0.Angle (-0.716,0.436) -> (-0.681,0.314)
- `m33_00_0600` 5: Fog: Color x1, heightColor x1, Dist x1 +2; e.g. Fog#0 Color (0.506,0.49,0.456,0.35) -> (0.282,0.349,0.412,0.913)
- `m33_00_0700` 4: Fog: Color x1, heightColor x1, Dist x1 +1; e.g. Fog#0 Color (0.506,0.49,0.456,0.35) -> (0.506,0.49,0.455,0.15)
- `m34_01_0010` 15: LightSet: L0.Diff x2, L0.Spec x2, L1.Diff x2 +6; Fog: Color x1, heightColor x1; e.g. LightSet#0 L0.Angle (-0.646,-2.11) -> (-0.559,-2.11) new ids: LightSet#120
- `m35_00_0010` 91: LightSet: L0.Diff x7, L1.Diff x7, L2.Spec x7 +18; Fog: Color x4, heightColor x3, heightDist x2 +2; e.g. LightSet#0 L0.Angle (-0.611,-2.65) -> (-0.785,-2.65)
- `m35_00_0100` 26: LightSet: L0.Angle x1, L0.Diff x1, L0.Spec x1 +14; Fog: Color x1, heightColor x1, Dist x1 +6; e.g. LightSet#0 L0.Angle (-0.611,-2.65) -> (-0.576,-2.65)
- `m35_00_0110` 35: LightSet: L0.Angle x1, L0.Diff x1, L0.Spec x1 +14; Fog: Color x2, heightColor x2, Dist x2 +6; e.g. LightSet#0 L0.Angle (-0.611,-2.65) -> (-0.576,-2.65)
- `m35_00_0200` 19: LightSet: L0.Angle x1, L0.Diff x1, L0.Spec x1 +16; e.g. LightSet#0 L0.Angle (-0.611,-2.65) -> (-0.576,-2.65)
- `m35_00_0210` 19: LightSet: L0.Angle x1, L0.Diff x1, L0.Spec x1 +16; e.g. LightSet#0 L0.Angle (-0.611,-2.65) -> (-0.576,-2.65)
- `m35_00_0300` 26: LightSet: L0.Angle x1, L0.Diff x1, L0.Spec x1 +14; Fog: Color x1, heightColor x1, Dist x1 +6; e.g. LightSet#0 L0.Angle (-0.611,-2.65) -> (-0.576,-2.65)
- `m35_00_0310` 26: LightSet: L0.Angle x1, L0.Diff x1, L0.Spec x1 +14; Fog: Color x1, heightColor x1, Dist x1 +6; e.g. LightSet#0 L0.Angle (-0.611,-2.65) -> (-0.576,-2.65)
- `m35_00_0500` 17: Fog: Color x2, heightColor x2, heightDist x2 +6; e.g. Fog#0 Color (0.459,0.513,0.552,0.245) -> (0.506,0.49,0.455,0.35)
- `m36_00_0001` 105: LightSet: L0.Angle x4, L0.Diff x4, L0.Spec x4 +13; Fog: Color x6, heightColor x6, Dist x6 +7; Env Map: EnvMap:ColorScale x3; e.g. LightSet#0 L0.Angle (-0.995,2.51) -> (-0.436,0.0698) new ids: Dominant Light#0 Env Map#3 Env Map#4 Env Map#5 Env Map#50 Env Map#6 +5
- `m36_00_0002` 105: LightSet: L0.Angle x4, L0.Diff x4, L0.Spec x4 +11; Fog: Color x6, heightColor x6, Dist x6 +8; Env Map: EnvMap:ColorScale x3; e.g. LightSet#0 L0.Angle (-0.995,2.51) -> (-0.873,-0.82) new ids: Dominant Light#0 Env Map#3 Env Map#4 Env Map#5 Env Map#50 Env Map#6 +5
- `m36_00_0003` 132: LightSet: L0.Angle x4, L0.Diff x4, L0.Spec x4 +11; Fog: Color x6, heightColor x6, Dist x6 +7; Env Map: EnvMap:ColorScale x3; ShadowParam: Angle x2, Far Fade Start x2, Far Fade Dist x2 +13; SSAO: EnableUpScale x1, Ver2 Sample Offset x1, Ver2 AO Scale x1; e.g. LightSet#0 L0.Angle (-0.995,2.51) -> (-0.436,0.0698) new ids: Env Map#3 Env Map#4 Env Map#5 Env Map#50 Env Map#6 Fog#1 +7
- `m36_00_0100` 3: Fog: heightColor x1, heightFogIntencity x1, ToUpper x1; e.g. Fog#0 heightColor (0.314,0.314,0.391,4) -> (0.431,0.268,0.192,4)
- `m37_00_0010` 57: LightSet: L0.Diff x4, L0.Spec x4, L1.Diff x4 +12; e.g. LightSet#100 L0.Diff (0.77,0.913,1,3) -> (0.769,0.914,1,3)
- `m37_00_0100` 9: Fog: Color x1, heightColor x1, heightDist x1 +4; Env Map: EnvMap:ID x1, EnvMap:ColorScale x1; e.g. Fog#0 Color (0.588,0.863,1,1) -> (0.588,0.863,1,0.65)
- `m37_00_0110` 9: Fog: Color x1, heightColor x1, heightDist x1 +4; Env Map: EnvMap:ID x1, EnvMap:ColorScale x1; e.g. Fog#0 Color (0.588,0.863,1,1) -> (0.588,0.863,1,0.65)
- `m37_00_0200` 6: Fog: Dist x1, heightDist x1, HeightOffset x1 +2; Env Map: EnvMap:ColorScale x1; e.g. Fog#0 Dist 1e+03 -> 1.5e+03
- `m37_00_0210` 6: Fog: Dist x1, heightDist x1, HeightOffset x1 +2; Env Map: EnvMap:ColorScale x1; e.g. Fog#0 Dist 1e+03 -> 1.5e+03
- `m37_00_0300` 7: Fog: heightColor x1, Dist x1, heightDist x1 +4; e.g. Fog#0 heightColor (0.588,0.863,1,0) -> (0.588,0.863,1,1)
- `m37_00_0310` 7: Fog: heightColor x1, Dist x1, heightDist x1 +4; e.g. Fog#0 heightColor (0.588,0.863,1,0) -> (0.588,0.863,1,1)
- `m37_00_0400` 9: Fog: Color x1, heightColor x1, heightDist x1 +4; Env Map: EnvMap:ID x1, EnvMap:ColorScale x1; e.g. Fog#0 Color (0.588,0.863,1,1) -> (0.588,0.863,1,0.65)
- `m37_00_0410` 9: Fog: Color x1, heightColor x1, heightDist x1 +4; Env Map: EnvMap:ID x1, EnvMap:ColorScale x1; e.g. Fog#0 Color (0.588,0.863,1,1) -> (0.588,0.863,1,0.65)
- `m37_00_0500` 22: LightSet: L0.Angle x1, L0.Diff x1, L0.Spec x1 +14; Fog: Color x1, Dist x1, DistOffset x1 +2; e.g. LightSet#0 L0.Angle (-0.524,0.454) -> (-0.524,1.4)
- `m37_00_0510` 22: LightSet: L0.Angle x1, L0.Diff x1, L0.Spec x1 +14; Fog: Color x1, Dist x1, DistOffset x1 +2; e.g. LightSet#0 L0.Angle (-0.524,0.454) -> (-0.524,1.4)
- `m38_00_0002` 51: LightSet: L0.Angle x3, L0.Diff x3, L1.Diff x3 +19; Fog: Color x2, DistOffset x2, heightDist x1 +2; SSAO: UseNormal x1, UseReProj x1, PreventGhost x1 +4; e.g. LightSet#0 L0.Angle (-0.471,-0.14) -> (-1.03,2.2) new ids: SSAO#0
- `m38_00_0100` 12: LightSet: L0.Diff x1, L0.Spec x1, L0.Sharp x1 +9; e.g. LightSet#0 L0.Diff (0.458,0.673,1,0) -> (1,1,1,0)
- `m38_00_0200` 17: LightSet: L0.Angle x1, L0.Diff x1, L0.Spec x1 +10; Fog: Color x1, heightColor x1, Dist x1 +1; e.g. LightSet#100 L0.Angle (-1.57,0) -> (-0.785,1.57)
- `m39_00_0100` 42: LightSet: L0.Angle x2, L0.Diff x2, L0.Spec x2 +15; Fog: Color x2, heightColor x2, heightDist x2 +8; e.g. LightSet#0 L0.Angle (-0.524,2.13) -> (-0.471,2.06)
- `m39_00_0200` 51: LightSet: L0.Angle x2, L0.Diff x2, L0.Spec x2 +16; Fog: Color x2, heightColor x2, Dist x2 +8; e.g. LightSet#0 L0.Angle (-0.524,2.13) -> (-0.646,2.04)
- `m39_00_0300` 11: LightSet: L0.Angle x1, L0.Diff x1, L0.Spec x1 +5; Fog: Color x1, heightColor x1, heightFogIntencity x1; e.g. LightSet#0 L0.Angle (-0.524,2.13) -> (-0.838,1.57)
- `m39_00_0500` 11: Fog: Color x1, heightColor x1, Dist x1 +6; Dominant Light: Dominant Sharpness Offset x1, Dominant Color Scale x1; e.g. Fog#0 Color (0.267,0.361,0.478,0.344) -> (0.275,0.427,0.761,0.3)
- `m40_00_0010` 55: LightSet: L1.Spec x6, L2.Diff x6, L2.Angle x5 +14; Fog: heightColor x4, Color x3, Dist x2; e.g. LightSet#0 L0.Diff (0.907,0.915,0.927,0.8) -> (0.843,0.802,1,0.1) new ids: LightSet#200
- `m40_00_0510` 12: Fog: Color x2, heightColor x2, Dist x2 +5; e.g. Fog#0 Color (0.417,0.459,0.524,0.256) -> (0,0,0,1)
- `m41_00_0200` 22: LightSet: L0.Diff x1, L0.Spec x1, L0.SsaoW x1 +15; Fog: Color x1, heightColor x1, Dist x1 +1; e.g. LightSet#2 L0.Diff (0.901,0.945,1,0.5) -> (0.902,0.945,1,0.5)
- `m41_00_0800` 18: LightSet: L0.Diff x1, L0.Spec x1, L0.SsaoW x1 +15; e.g. LightSet#2 L0.Diff (0.901,0.945,1,0.5) -> (0.902,0.945,1,0.5)
- `m41_00_0900` 18: LightSet: L0.Diff x1, L0.Spec x1, L0.SsaoW x1 +15; e.g. LightSet#2 L0.Diff (0.901,0.945,1,0.5) -> (0.902,0.945,1,0.5)
- `m45_00_0100` 18: Fog: Color x3, heightColor x3, heightDist x3 +5; e.g. Fog#0 Color (0.455,0.537,0.627,0.5) -> (0.431,0.49,0.549,1)
- `m45_00_0200` 20: LightSet: L0.Diff x1, L0.Spec x1, L0.Sharp x1 +12; Fog: Color x1, heightColor x1, heightDist x1 +2; e.g. LightSet#0 L0.Diff (0.808,0.904,1,0.8) -> (0.942,0.971,0.857,0.2)
- `m45_00_0300` 5: Fog: Color x1, Dist x1, heightDist x1 +2; e.g. Fog#0 Color (0.455,0.537,0.627,0.5) -> (0.251,0.304,0.361,0.552)
- `m45_00_0400` 38: LightSet: L0.Angle x1, L0.Diff x1, L0.Spec x1 +11; Fog: Color x3, heightColor x3, Dist x3 +6; e.g. LightSet#100 L0.Angle (-0.785,-1.57) -> (-0.785,-1.57)
- `m45_00_0500` 15: Fog: Color x2, heightColor x2, Dist x2 +6; e.g. Fog#0 Color (0.455,0.537,0.627,0.5) -> (0.327,0.377,0.434,0.893)
- `m45_00_0600` 14: Fog: Color x2, heightColor x2, Dist x2 +6; e.g. Fog#0 Color (0.455,0.537,0.627,0.5) -> (0.454,0.538,0.628,0.25)
- `m45_00_0700` 24: Fog: Color x3, heightColor x3, heightDist x3 +6; e.g. Fog#0 Color (0.455,0.537,0.627,0.5) -> (0.176,0.227,0.282,1)
- `m45_00_0800` 18: Fog: Color x3, heightColor x3, heightDist x3 +5; e.g. Fog#0 Color (0.455,0.537,0.627,0.5) -> (0.431,0.49,0.549,1)
- `m47_00_0200` 23: LightSet: L0.Diff x1, L0.Spec x1, L0.SsaoW x1 +15; Fog: Color x1, heightColor x1, Dist x1 +2; e.g. LightSet#2 L0.Diff (0.901,0.945,1,1.91) -> (0.902,0.945,1,0.5)
- `m47_00_0800` 18: LightSet: L0.Diff x1, L0.Spec x1, L0.SsaoW x1 +15; e.g. LightSet#2 L0.Diff (0.901,0.945,1,1.91) -> (0.902,0.945,1,0.5)
- `m47_00_0900` 18: LightSet: L0.Diff x1, L0.Spec x1, L0.SsaoW x1 +15; e.g. LightSet#2 L0.Diff (0.901,0.945,1,1.91) -> (0.902,0.945,1,0.5)
- `m50_00_0100` 32: LightSet: L0.Spec x1, L0.SsaoW x1, L1.Diff x1 +15; Fog: heightColor x4, heightDist x4, HeightOffset x4 +2; e.g. LightSet#1 L0.Spec (0.655,0.737,1,0.6) -> (0.655,0.737,1,0.5)
- `m50_00_0200` 84: LightSet: L0.SsaoW x4, L1.Diff x4, L1.Spec x4 +20; Fog: heightDist x3, HeightOffset x3, heightColor x1 +2; e.g. LightSet#0 L0.Diff (0.828,0.875,0.913,1) -> (0.913,0.863,0.799,1.2)
- `m50_00_0300` 91: LightSet: L1.Diff x5, L1.Spec x5, L2.Diff x5 +17; Fog: heightColor x3, heightDist x3, HeightOffset x3 +3; e.g. LightSet#1 L0.Diff (0.827,0.875,0.914,1) -> (0.827,0.875,0.914,0)
- `m51_00_0100` 43: LightSet: L0.Diff x4, L1.Diff x4, L1.Spec x4 +11; Fog: Color x2, heightColor x1, Dist x1 +3; e.g. LightSet#0 L0.Diff (0.941,0.91,0.688,2) -> (0.667,0.784,0.941,2.6)
- `m51_00_0200` 33: LightSet: L0.Diff x1, L0.Spec x1, L0.Sharp x1 +19; Fog: Color x1, heightColor x1, Dist x1 +3; ShadowParam: Shadow Map FilterMode x1, DepthRate x1, Blur Count x1 +2; e.g. LightSet#100 L0.Diff (1,0.971,0.924,1.8) -> (0.802,0.91,1,0.5)

Variants with no difference in these groups (no group in scope, or values equal to the base): m33_00_0500, m33_00_0510, m34_01_0110, m35_00_0510, m38_00_0600, m41_00_0100, m47_00_0100.

## 9. Cutscene files `sAA_*` and the global files: counts and striking values

### 9.1 Cutscene files (49 files `sAA_BB_NNNN`; `s_template` is a template)

| group | files holding it | ids per file | total ids |
|---|---|---|---|
| LightSet | 49 | 1..38 | 628 |
| Fog | 49 | 1..21 | 310 |
| LightScattering | 31 | 1..7 | 90 |
| Ls | 0 | - | - |
| Dominant Light | 28 | 1..3 | 33 |
| Env Map | 28 | 1..81 | 1109 |
| AOFields | 0 | - | - |
| AOFields Model | 0 | - | - |
| ShadowParam | 29 | 1..10 | 92 |
| SSAO | 7 | 1..3 | 12 |

Strongest hemisphere intensity (max over up/down of luma(rgb^2) x 4th float; INFERRED convention) over all non-base files, against the largest value in the area's base file (`m` + the file's area digits):

| file | id | hemisphere intensity | base area max |
|---|---|---|---|
| m32_00_0010 | 20 | 16 | 0.25 |
| m32_00_0110 | 20 | 16 | 0.25 |
| m33_00_0010 | 33000 | 2.352 | 2.352 |
| s33_00_0000 | 33000 | 2.352 | 2.352 |
| s33_00_0020 | 33000 | 2.352 | 2.352 |
| s33_00_0021 | 33000 | 2.352 | 2.352 |
| s33_00_0022 | 33000 | 2.352 | 2.352 |
| s33_00_0030 | 33000 | 2.352 | 2.352 |

For comparison the largest hemisphere intensity in the 21 base files is 2.352 (m33_00).

Largest L0 (sun) diffuse 4th float over all 168 files: 100 m36_00_0001#1, 64 m32_00_0110#13, 64 m32_00_0110#12, 64 m32_00_0100#13. Largest in the base files: 20 m30_01#121, 18.89 m38_00#20, 18.89 m36_00#20.

Fog Dist maximum over all files: 2000 (s41_00_0070#9); minimum: 0 (m21_00 id 2 and others, section 3).

Cutscene LightSet hemisphere intensity against its area's base maximum (ratio, only where the base maximum is > 0): s34_01_0000#1 1.3x (0.3324 vs 0.25), s45_00_0031#301 1.0x (0.25 vs 0.25), s45_00_0031#202 1.0x (0.25 vs 0.25), s45_00_0031#201 1.0x (0.25 vs 0.25), s45_00_0031#101 1.0x (0.25 vs 0.25).

### 9.2 Global files

OBSERVED: ids carried per group (group name then number of ids):

| file | groups in scope and number of ids |
|---|---|
| default | LightSet 7; Fog 7; LightScattering 1; Env Map 7; ShadowParam 7; SSAO 7 |
| mapbase | AOFields 1; AOFields Model 1; ShadowParam 1; SSAO 1 |
| remobase | ShadowParam 1; SSAO 1 |
| eclipse | Fog 16 |
| empty | none of the groups in scope |
| mapoverride_0000 | none of the groups in scope |
| mapoverride_0001 | LightScattering 1; Env Map 17 |
| mapoverride_0002 | none of the groups in scope |
| mapoverride_0003 | LightScattering 1; Env Map 17 |
| m_template | LightSet 1; Fog 1; LightScattering 1; Env Map 1; ShadowParam 1; SSAO 1 |
| ml_template | none of the groups in scope |
| s_template | LightSet 1; Fog 1; Ls 1; ShadowParam 1 |

- `remobase` ShadowParam: id 0 (a copy of `mapbase`/`default` values: not compared).
- `remobase` SSAO: id 0 (a copy of `mapbase`/`default` values: not compared).
- `eclipse` Fog: 16 ids; distinct value sets: 1; id 0: Color (0,0,0,1), Dist 0.1, heightFogDensity 100, Alpha 1.
- `mapoverride_0001` LightScattering: default values (section 4).
- `mapoverride_0001` Env Map: 17 ids, ColorScale 0.5..1.2.
- `mapoverride_0003` LightScattering: default values (section 4).
- `mapoverride_0003` Env Map: 17 ids, ColorScale 0.5..1.2.

## 10. Checks (the reader against the raw hex)

Three parameters of `param_drawparam_m30_00_0000.tsv`, hex read by hand (sign, 8-bit exponent, 23-bit mantissa; integer arithmetic only), compared with the reader (`gp.py`) and with the tsv's own decimal column:

| group / parameter / id | type | raw hex (file) | decoded by hand | decoded by reader | tsv decimal column (comma-separated) | result |
|---|---|---|---|---|---|---|
| LightSet / DiffColor / id 0 | 14 | 3F800000,3F71D1B3,3F5E6A01,3F9D70A4 | (1, 0.944606, 0.868805, 1.23) | (1, 0.944606, 0.868805, 1.23) | 1,0,944606,0,868805,1,23 | match |
| Fog / Dist / id 0 | 9 | 42F00000 | (120) | (120) | 120 | match |
| LightSet / Angle / id 0 | 12 | BF44980B,BF75BE0A | (-0.767945, -0.959931) | (-0.767945, -0.959931) | -0,767945,-0,959931 | match |
| LightSet / Hemi Color Up / id 0 | 14 | 3F000000,3F000000,3F000000,3F800000 | (0.5, 0.5, 0.5, 1) | (0.5, 0.5, 0.5, 1) | 0,5,0,5,0,5,1 | match |

By hand, luma of m30_00 id 0 diffuse = 0.2126 x 1.000000 + 0.7152 x 0.944606 + 0.0722 x 0.868805 = 0.950910; the table (section 2.1) shows 0.9509. Angle y of the same id: -0.959931 rad x 180/pi = -55.0000 deg.

Whole-corpus check: 73011 float rows (types 9, 12, 14) of 168 files, reader vs the integer-arithmetic decoder: 0 mismatches. 50870 scalar float rows (type 9): the tsv's own decimal column (comma as the decimal mark) differs from the decoded hex by more than 1e-4 relative in 0 rows.

Inheritance example: `m30_00_0100` carries LightSet ids 0 1 2 3 4; its id 0 L0 angle is its own ((-0.296706,-0.122173) rad); for id 10, absent from that file, the chain `m30_00_0100`, `m30_00_0000`, `mapbase`, `default` returns `m30_00_0000` ((-0.20944,-0.820305)).
