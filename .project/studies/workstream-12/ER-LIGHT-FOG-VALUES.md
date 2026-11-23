# ER-LIGHT-FOG-VALUES: what Elden Ring's shipped draw-parameter data say about light sets, volumetric fog, environment and ray tracing

**Non-canonical until the study thread re-reads it.** Date 2026-11-22. Written by a Sonnet worker for the study thread of workstream 12
(physical light sources and a traced volumetric fog). Data only: no game, build or Ghidra launched, no new game file extracted, no raw game
file copied (values and names only). Tags: OBSERVED = read in a named file/table; INFERRED = my reading of a name or a structure (never a
claim about engine use). No design proposals.

## 0. Inputs, scripts, conventions

- Value table (read-only): `evidence/I-0138/artifacts/er-values/er-gparam-values.tsv` (246,563 rows: file, group, group2, param, param2, type, id, time, value),
  parsed by the earlier worker's `parse_gparam.py` (reused as the data source, not re-run). 295 files carry data (the 319 draw-parameter files of
  ER-VALUES.md minus 24 without value rows, OBSERVED): `default` (1), `m*` (208, including `mapbase` and the `m60_*`/`m61_*` overworld/DLC variants), `s*` cutscene files (86).
- DS3 side: `evidence/I-0066/queries/gparam-all/param_drawparam_m30_00_0000.tsv` and `..._m40_00_0000.tsv` (hex = float32/int32).
- My scripts and tables: `C:/Users/User/Tools/DS3-Decompilation/study-ws12/er-values/` (Python 3: `common.py`, `stats_groups.py`, `ratios.py`, `dump.py`, `wide.py`, `build_parts.py`, `ds3.py`, `enables.py`, `build_report.py`).
  Large tables (TSV there): `param-stats-gameplay.tsv`, `param-stats-allfiles.tsv` (every parameter, every component, statistics over m* files / over all files), `groups.tsv`, `bool-params.tsv` (all boolean parameters: per-file counts), `wide-LightSet.tsv`, `wide-VolumetricFog.tsv`, `wide-FogVolume.tsv`, `wide-RayTracing.tsv`, `wide-EnvironmentConfig.tsv` (one row per file+id, one column per parameter, `time=value` series), `lightset-luma-rows.tsv` (every ratio row), `env-per-file.txt`, `check1.txt`..`check3.txt`.
- "Gameplay files" = the 208 files named `m*` (includes `mapbase`, excludes `default` and `s*`). Statistics are over **value rows** (every file x id x time key), as asked ("all ids and time keys"): a row is one (file, param, id, time) value, so a file with many ids or time keys weighs more. Percentiles are linear interpolation. "most common" = value x rows.
- Colours are stored (r, g, b, a) as floats: r, g, b in 0..1 in every table below, a in 0..16. Luma = 0.2126 r + 0.7152 g + 0.0722 b. `luma*a` = luma times the fourth component. That the fourth component is an intensity multiplier is INFERRED (DS3 light-0 alpha reaches 16 in `m30_00_0000`, p90 3.1; ER's sun alpha median 0.35 with a=0 at some time keys); all ratios are given both without and with it.
- Time keys: each parameter of an id has its own keyframes (hours); keys differ between parameters (OBSERVED: 424 of 1,879 LightSet file+id sets have parameters with different key sets). Where two parameters are combined (ratios), I evaluate each at the union of the keys by linear interpolation between its keys (INFERRED: the engine's interpolation is not known), constant outside.
- Parameter names are exactly as stored (including typos and doubled spaces: "Directional Light  LightMask Influence", "heightFogIntencity" (DS3), "Strengh"). One name occurs with two types (`FogVolume DrawArea HeightFallOff`: type 9 in 142 files, type 3 in 1 file): statistics are merged under the name.

## 1. All groups

OBSERVED (`groups.tsv`). 21 groups exist in ER's files; the number of files holding each is below. ER has **no** group whose name contains Sky, Cloud, Scatter, Atmosphere, Ambient, Probe or Fog other than `VolumetricFog` and `FogVolume`; the only Env* group is `EnvironmentConfig`; the only Light* groups are `LightSet` and `LightShaft` (OBSERVED: no other group names). DS3's `Fog`, `Dominant Light`, `LightScattering`, `Env Map`, `SeaWave`, `Snow`, `ColorParaffin Filter` groups do not exist in ER (section 3 and 6). Sky/ambient/scatter-like *parameters* in other groups: `VolumetricFog SkyLight *`, `LightShaft` "Sky Multiply/Add Color", `Lens Simulation Param SkyMaskRate` (names only).

| group (short name) | files holding it (of 295 with data) | m* (of 208) | s* (of 86) | default | covered here |
|---|---|---|---|---|---|
| Tone Map | 265 | 179 | 85 | 1 | no (post-process, not light/fog) |
| VolumetricFog | 236 | 156 | 80 | 0 | full |
| LightSet | 221 | 136 | 84 | 1 | full |
| LightShaft | 168 | 105 | 62 | 1 | short (name contains Light) |
| FogVolume | 143 | 96 | 47 | 0 | full |
| ColorGrading | 126 | 106 | 19 | 1 | no (post-process, not light/fog) |
| ShadowParam | 100 | 19 | 80 | 1 | short |
| Lens Simulation | 98 | 12 | 85 | 1 | no (post-process, not light/fog) |
| Bloom | 96 | 16 | 79 | 1 | no (post-process, not light/fog) |
| RayTracing | 47 | 1 | 45 | 1 | full |
| EnvironmentConfig | 44 | 40 | 4 | 0 | full |
| SSR | 37 | 1 | 36 | 0 | short |
| SSAO | 35 | 1 | 33 | 1 | short |
| Glow Filter | 19 | 1 | 17 | 1 | no (post-process, not light/fog) |
| MotionBlur | 11 | 1 | 9 | 1 | no (post-process, not light/fog) |
| FeedBackBlur | 7 | 1 | 5 | 1 | no (post-process, not light/fog) |
| Wet | 7 | 1 | 6 | 0 | short |
| Anti-Alias | 5 | 1 | 3 | 1 | no (post-process, not light/fog) |
| FurBlur | 3 | 1 | 1 | 1 | no (post-process, not light/fog) |
| Setting Info | 2 | 1 | 0 | 1 | no (post-process, not light/fog) |
| WaterInteraction Simulation | 1 | 1 | 0 | 0 | listed only |

Covered in full below: LightSet, VolumetricFog, FogVolume, EnvironmentConfig, RayTracing. Short: ShadowParam, SSAO, SSR, Wet, plus LightShaft (name contains Light; its table is only in `param-stats-gameplay.tsv`, 24 parameters of the Yebis light-shaft post effect) and WaterInteraction (1 file, 5 parameters, listed in `er-groups-params.txt`). The other groups are post-process (Tone Map, Bloom, ColorGrading, Lens Simulation, Glow Filter, MotionBlur, FeedBackBlur, Anti-Alias, FurBlur, Setting Info), covered by ER-VALUES.md, not here.

Reading the tables: each parameter table lists the parameters in stored order (the `#` column), with type (3 = int/enum, 7 = int, 9 = float, 11 = bool byte, 12 = 2 floats, 13 = 3 floats, 14 = 4 floats rgba), the number of files holding it (all files / files in the subset used) and statistics over the subset. Component `v` = the value, `c0/c1/c2` = vector components (angles in radians), `r g b a` = colour channels.

## 2. LightSet

OBSERVED: 221 files hold `LightSet` (136 `m*`, 84 `s*`, `default`); every one of them holds all 26 parameters (`er-groups-params.txt`), except 7 file+id sets. A LightSet file holds **ids** (light-set variants: `default` ids 0, 10, 11; `mapbase` ids 0, 100; `m60_00_0000` ids 0, 100, 1000, 1100; most `m*` 4 ids), each with the 26 parameters and its own time keys. Structure (OBSERVED from names/types): 4 directional lights, each with Angle (2 floats), DiffColor, SpecColor (rgba), Sharpness, SsaoWeight; names for lights 0 and 1 are `Directional Light <X>0/<X>1`, for lights 2 and 3 `Directional Light LocalLight <X>1/<X>2` (INFERRED: the order light 0, light 1, local light 1, local light 2, which is also the order of the `SsaoWeight0..3` and of the two `Enable LocalLight1/2` flags); one hemisphere pair (Hemi Color Up / Down); `Light1 FollowedCamera`, `Enable LocalLight1/2`, `LightMask Influence`.

### 2.1 Distribution over gameplay files (all 26 parameters, all ids and time keys)

Subset: the 136 `m*` files holding LightSet. Full table in `param-stats-gameplay.tsv`.

| # | param (type) | files all/subset | comp | n | min | p10 | median | p90 | max | most common |
|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Directional Light Angle0 (12) | 221/136 | c0 | 2620 | -1.5481 | -0.87267 | -0.69813 | 0 | 0.17453 | -0.17453 x524, -0.87267 x510 |
|  |  |  | c1 | 2620 | -2.8798 | -2.1991 | 0.87267 | 2.7925 | 3.1416 | 0.87267 x930, -2.1991 x696 |
| 2 | Directional Light DiffColor0 (14) | 221/136 | r | 2656 | 0 | 0.56863 | 0.83 | 1 | 1 | 1 x915, 0.66667 x496 |
|  |  |  | g | 2656 | 0 | 0.66667 | 0.78431 | 0.98431 | 1 | 0.78431 x662, 0.70588 x229 |
|  |  |  | b | 2656 | 0 | 0.52157 | 0.96078 | 1 | 1 | 1 x1150, 0.96078 x168 |
|  |  |  | a | 2656 | 0 | 0 | 0.35 | 0.6 | 7 | 0.5 x622, 0 x520 |
|  |  |  | luma(rgb) | 2656 | 0 | 0.68547 | 0.77487 | 0.98314 | 1 | 0.77487 x478, 0.68547 x197 |
|  |  |  | luma*a | 2656 | 0 | 0 | 0.23889 | 0.49157 | 6.6163 | 0 x520, 0.23246 x235 |
| 3 | Directional Light SpecColor0 (14) | 221/136 | luma(rgb) | 2656 | 0 | 0.69816 | 0.78185 | 0.98314 | 1 | 0.77487 x478, 0.78185 x260 |
| 4 | Directional Light Sharpness0 (9) | 221/136 | v | 2268 | 0.1 | 0.1 | 0.25 | 1 | 50 | 0.3 x543, 0.15 x506 |
| 5 | Directional Light SsaoWeight0 (9) | 221/136 | v | 1542 | 0 | 0.7 | 1 | 1 | 1 | 1 x1382, 0.65 x65 |
| 6 | Directional Light Angle1 (12) | 221/136 | c0 | 1094 | -1.5708 | -1.5708 | -1.5708 | -0.87267 | 1.3614 | -1.5708 x725, -1.309 x216 |
|  |  |  | c1 | 1094 | -3.1416 | -3.1416 | 0 | 0.43633 | 2.4819 | 0 x726, -3.1416 x227 |
| 7 | Directional Light DiffColor1 (14) | 221/136 | r | 1092 | 0 | 0.54902 | 0.67451 | 1 | 1 | 0.67451 x449, 1 x237 |
|  |  |  | g | 1092 | 0 | 0.7451 | 0.78431 | 1 | 1 | 0.78431 x531, 1 x236 |
|  |  |  | b | 1092 | 0 | 0.98039 | 1 | 1 | 1 | 1 x973, 0 x26 |
|  |  |  | a | 1092 | 0 | 0 | 0 | 0.05 | 4 | 0 x794, 0.05 x220 |
|  |  |  | luma(rgb) | 1092 | 0 | 0.72182 | 0.77654 | 1 | 1 | 0.77654 x441, 1 x232 |
|  |  |  | luma*a | 1092 | 0 | 0 | 0 | 0.05 | 1.9543 | 0 x801, 0.05 x218 |
| 8 | Directional Light SpecColor1 (14) | 221/136 | luma(rgb) | 958 | 0 | 0.64606 | 0.7327 | 1 | 1 | 0.7327 x378, 1 x233 |
| 9 | Directional Light Sharpness1 (9) | 221/136 | v | 1042 | 0.1 | 0.1 | 0.2 | 0.3 | 50 | 0.2 x297, 0.15 x231 |
| 10 | Directional Light SsaoWeight1 (9) | 221/136 | v | 1042 | 0 | 0 | 0.5 | 1 | 1 | 0 x467, 1 x341 |
| 11 | Directional Light LocalLight Angle1 (12) | 221/136 | c0 | 1584 | -1.5708 | -0.78539 | -0.62832 | -0.62832 | 0.43633 | -0.62832 x1383, -0.7854 x104 |
|  |  |  | c1 | 1584 | -3.1416 | -1.0123 | -1.0123 | 0 | 2.2689 | -1.0123 x1379, 1.5708 x132 |
| 12 | Directional Light LocalLight DiffColor1 (14) | 221/136 | r | 1608 | 0 | 0.64706 | 0.76078 | 0.76078 | 1 | 0.76078 x985, 0.65882 x371 |
|  |  |  | g | 1608 | 0 | 0.72549 | 0.81569 | 0.81569 | 1 | 0.81569 x985, 0.75686 x371 |
|  |  |  | b | 1608 | 0 | 0.84314 | 1 | 1 | 1 | 1 x1443, 0.78431 x62 |
|  |  |  | a | 1608 | 0 | 0 | 0 | 0 | 1 | 0 x1551, 1 x17 |
|  |  |  | luma(rgb) | 1608 | 0 | 0.71468 | 0.81732 | 0.81732 | 1 | 0.81732 x985, 0.75357 x371 |
|  |  |  | luma*a | 1608 | 0 | 0 | 0 | 0 | 0.40866 | 0 x1568, 0.2144 x10 |
| 13 | Directional Light LocalLight SpecColor1 (14) | 221/136 | luma(rgb) | 1588 | 0 | 0.71638 | 0.84196 | 0.87084 | 1 | 0.84196 x963, 0.87084 x363 |
| 14 | Directional LocalLight Light Sharpness1 (9) | 221/136 | v | 1504 | 0.1 | 0.3 | 1 | 1 | 50 | 1 x1101, 0.3 x305 |
| 15 | Directional Light SsaoWeight2 (9) | 221/136 | v | 1523 | 0 | 0 | 0.65 | 0.65 | 1 | 0.65 x984, 0 x459 |
| 16 | Directional Light LocalLight Angle2 (12) | 221/136 | c0 | 1054 | -0.7854 | -0.69813 | -0.43633 | -0.27925 | 0.87267 | -0.43633 x608, -0.27925 x154 |
|  |  |  | c1 | 1054 | -3.1416 | 0 | 0 | 3.0543 | 3.1416 | 0 x830, 3.1416 x98 |
| 17 | Directional Light LocalLight DiffColor2 (14) | 221/136 | r | 1068 | 0 | 0 | 0.86 | 1 | 1 | 0.86 x344, 1 x241 |
|  |  |  | g | 1068 | 0 | 0 | 0.91299 | 1 | 1 | 0.91299 x266, 1 x233 |
|  |  |  | b | 1068 | 0 | 0 | 1 | 1 | 1 | 1 x858, 0 x145 |
|  |  |  | a | 1068 | 0 | 0 | 0 | 0.03 | 1 | 0 x737, 0.015 x216 |
|  |  |  | luma(rgb) | 1068 | 0 | 0 | 0.908 | 1 | 1 | 0.908 x266, 1 x233 |
|  |  |  | luma*a | 1068 | 0 | 0 | 0 | 0.015 | 0.90722 | 0 x760, 0.015 x216 |
| 18 | Directional Light LocalLight SpecColor2 (14) | 221/136 | luma(rgb) | 1068 | 0 | 0.81256 | 0.9007 | 1 | 1 | 0.9007 x352, 1 x223 |
| 19 | Directional LocalLight Light Sharpness2 (9) | 221/136 | v | 1032 | 0.1 | 0.15 | 0.25 | 1 | 50 | 0.25 x586, 0.15 x225 |
| 20 | Directional Light SsaoWeight3 (9) | 221/136 | v | 1032 | 0 | 0 | 1 | 1 | 1 | 1 x836, 0 x190 |
| 21 | Directional Light Hemi Color Up (14) | 221/136 | r | 2464 | 0 | 0.58039 | 0.77647 | 0.9451 | 1 | 0.75686 x758, 0.9451 x392 |
|  |  |  | g | 2464 | 0 | 0.62745 | 0.77647 | 0.9451 | 1 | 0.75686 x758, 0.9451 x392 |
|  |  |  | b | 2464 | 0 | 0.70588 | 0.77647 | 1 | 1 | 0.75686 x758, 0.9451 x392 |
|  |  |  | a | 2464 | 0 | 0.1 | 0.1 | 0.2 | 1.5 | 0.1 x2002, 0 x156 |
|  |  |  | luma(rgb) | 2464 | 0 | 0.61847 | 0.77647 | 0.9451 | 1 | 0.75686 x758, 0.9451 x392 |
|  |  |  | luma*a | 2464 | 0 | 0.075686 | 0.079181 | 0.099718 | 1.2856 | 0.075686 x758, 0.09451 x392 |
| 22 | Directional Light Hemi Color Down (14) | 221/136 | r | 2376 | 0 | 0 | 0 | 0.60392 | 1 | 0 x1518, 0.60392 x183 |
|  |  |  | g | 2376 | 0 | 0 | 0 | 0.66432 | 1 | 0 x1518, 0.7098 x183 |
|  |  |  | b | 2376 | 0 | 0 | 0 | 0.70588 | 1 | 0 x1518, 1 x191 |
|  |  |  | a | 2376 | 0 | 0 | 0 | 0.1 | 1 | 0 x1765, 0.05 x204 |
|  |  |  | luma(rgb) | 2376 | 0 | 0 | 0 | 0.64554 | 1 | 0 x1518, 0.70825 x183 |
|  |  |  | luma*a | 2376 | 0 | 0 | 0 | 0.053945 | 0.50196 | 0 x1773, 0.017332 x33 |
| 23 | Directional LightLight1 FollowedCamera (11) | 221/136 | v | 606 | 0 | 0 | 0 | 1 | 1 | 0 x369, 1 x237 |
| 24 | Directional Enable LocalLight1 (11) | 221/136 | v | 614 | 0 | 0 | 0 | 1 | 1 | 0 x472, 1 x142 |
| 25 | Directional Enable LocalLight2 (11) | 221/136 | v | 1552 | 0 | 0 | 1 | 1 | 1 | 1 x1264, 0 x288 |
| 26 | Directional Light  LightMask Influence (9) | 221/136 | v | 1690 | 1 | 1 | 1 | 1 | 1 | 1 x1690 |

Readings (OBSERVED unless tagged):
- The sun (light 0) rgb is near white/warm; its alpha has median 0.35, p90 0.6, max 7 and is exactly 0 in 520 of 2,656 gameplay rows (the `a` row of `DiffColor0`). INFERRED: alpha = intensity, so those rows have the sun off.
- Light 1 and the two "LocalLight" lights have alpha 0 in most rows: light 1 in 794 of 1,092, light 2 in 1,551 of 1,608, light 3 in 737 of 1,068 (the `a` rows above, most common values).
- Hemi Color Up has alpha 0.1 in 2,002 of 2,464 rows (81 %); Hemi Color Down has black rgb (r = g = b = 0) in 1,518 of 2,376 rows (64 %) and alpha 0 in 1,765.
- `LightMask Influence` = 1 in all 1,690 rows. `SsaoWeight` of light 0 is 1 in 1,382 of 1,542 rows.

### 2.2 Hemisphere and secondary lights against light 0

Per value row (file, id, time), luma of the hemisphere (mean of Hemi Up and Hemi Down) over luma of directional light 0's diffuse, and luma of lights 1, 2, 3 over light 0 (the "L1/L2/L3" columns; light order as in the section 2 introduction, INFERRED). Cells are min / p10 / median / p90 / max. Rows whose light-0 luma is 0 are skipped (counted in the second column; under the alpha reading that is every row where the sun alpha is 0). Rows = union of time keys of the six colour parameters over every id of the file. Per-row values in `lightset-luma-rows.tsv`.


#### RGB only (alpha ignored)

| set | rows (skipped: light 0 luma = 0) | Hemi/L0 min/p10/med/p90/max | L1/L0 | L2/L0 | L3/L0 |
|---|---|---|---|---|---|
| all gameplay m* (incl. mapbase) | 2634 (skip 8) | 0 / 0.473 / 0.552 / 0.886 / 1.64 | 0 / 0.783 / 1.05 / 1.43 / 2.38 | 0 / 0.759 / 0.973 / 1.19 / 2.05 | 0 / 0 / 1.17 / 1.43 / 2.27 |
| default | 9 (skip 0) | 0.502 / 0.502 / 0.502 / 0.502 / 0.502 | 0 / 0 / 0 / 0.25 / 0.25 | 0 / 0 / 0 / 0.18 / 0.18 | 0 / 0 / 0 / 0.13 / 0.13 |
| mapbase | 8 (skip 0) | 0.502 / 0.502 / 0.502 / 0.502 / 0.502 | 0 / 0 / 0 / 0 / 0 | 0 / 0 / 0 / 0 / 0 | 0 / 0 / 0 / 0 / 0 |
| m60_00_0000 (overworld) | 36 (skip 0) | 0.395 / 0.438 / 0.488 / 0.591 / 0.629 | 0.735 / 0.791 / 1.02 / 1.36 / 1.46 | 0.768 / 0.799 / 1.05 / 1.18 / 1.19 | 0.88 / 0.924 / 1.17 / 1.38 / 1.46 |
| m10_00_0000 (Stormveil) | 28 (skip 0) | 0.638 / 0.638 / 0.686 / 0.789 / 0.789 | 0.78 / 0.78 / 1.01 / 1.3 / 1.3 | 0.757 / 0.757 / 0.795 / 0.983 / 0.983 | 0.912 / 0.912 / 1.03 / 1.3 / 1.3 |
| m11_00_0000 (Leyndell) | 28 (skip 0) | 0.643 / 0.643 / 0.681 / 0.685 / 0.685 | 0.775 / 0.816 / 0.991 / 1.27 / 1.27 | 0.754 / 0.754 / 0.865 / 0.955 / 0.955 | 0 / 0 / 0.5 / 1.27 / 1.27 |
| m12_01_0000 (underground) | 4 (skip 0) | 0 / 0 / 0 / 0 / 0 | 0.862 / 0.862 / 0.986 / 1.11 / 1.11 | 0.753 / 0.753 / 0.753 / 0.753 / 0.753 | 0.745 / 0.745 / 0.928 / 1.11 / 1.11 |
| m30_00_0000 (dungeon) | 36 (skip 0) | 0.395 / 0.438 / 0.488 / 0.591 / 0.629 | 0.735 / 0.791 / 1.02 / 1.36 / 1.46 | 0.768 / 0.799 / 1.05 / 1.18 / 1.19 | 0.88 / 0.924 / 1.17 / 1.38 / 1.46 |
| m34_11_0000 (dungeon, m31 absent) | 28 (skip 0) | 0.433 / 0.433 / 0.488 / 0.629 / 0.629 | 0.727 / 0.727 / 1.02 / 1.43 / 1.46 | 0.759 / 0.759 / 0.973 / 1.1 / 1.1 | 0.936 / 0.936 / 1.2 / 1.43 / 1.46 |
| s* cutscenes | 2154 (skip 37) | 0 / 0.48 / 0.686 / 1.03 / 100 | 0 / 0.78 / 0.997 / 1.16 / 100 | 0 / 0.76 / 0.983 / 1.16 / 100 | 0 / 0 / 0.933 / 1.25 / 100 |

#### RGB x alpha (alpha read as an intensity multiplier: INFERRED)

| set | rows (skipped: light 0 luma = 0) | Hemi/L0 min/p10/med/p90/max | L1/L0 | L2/L0 | L3/L0 |
|---|---|---|---|---|---|
| all gameplay m* (incl. mapbase) | 2122 (skip 520) | 0 / 0.0894 / 0.144 / 0.349 / 3.07 | 0 / 0 / 0.0128 / 0.231 / 1.38 | 0 / 0 / 0 / 0 / 0.829 | 0 / 0 / 0.0138 / 0.0968 / 4.36 |
| default | 8 (skip 1) | 0.1 / 0.1 / 0.139 / 0.837 / 0.837 | 0 / 0 / 0 / 0.417 / 0.417 | 0 / 0 / 0 / 0.3 / 0.3 | 0 / 0 / 0 / 0.217 / 0.217 |
| mapbase | 7 (skip 1) | 0.1 / 0.1 / 0.112 / 0.301 / 0.502 | 0 / 0 / 0 / 0 / 0 | 0 / 0 / 0 / 0 / 0 | 0 / 0 / 0 / 0 / 0 |
| m60_00_0000 (overworld) | 28 (skip 8) | 0.0879 / 0.0879 / 0.105 / 0.184 / 0.184 | 0 / 0 / 0.0509 / 0.215 / 0.243 | 0 / 0 / 0 / 0 / 0 | 0 / 0 / 0.0305 / 0.0729 / 0.88 |
| m10_00_0000 (Stormveil) | 28 (skip 0) | 0.143 / 0.143 / 0.157 / 0.349 / 0.349 | 0 / 0 / 0.0628 / 0.326 / 0.326 | 0 / 0 / 0 / 0 / 0 | 0 / 0 / 0.0188 / 0.0978 / 0.0978 |
| m11_00_0000 (Leyndell) | 28 (skip 0) | 0.0894 / 0.0894 / 0.152 / 0.379 / 0.379 | 0 / 0 / 0.0438 / 0.423 / 0.423 | 0 / 0 / 0 / 0 / 0 | 0 / 0 / 0.0132 / 0.127 / 0.127 |
| m12_01_0000 (underground) | 0 (skip 4) | - | - | - | - |
| m30_00_0000 (dungeon) | 28 (skip 8) | 0.0879 / 0.0879 / 0.105 / 0.184 / 0.184 | 0 / 0 / 0.0509 / 0.215 / 0.243 | 0 / 0 / 0 / 0 / 0 | 0 / 0 / 0.0305 / 0.0729 / 0.88 |
| m34_11_0000 (dungeon, m31 absent) | 24 (skip 4) | 0.0963 / 0.0963 / 0.122 / 0.276 / 0.276 | 0 / 0 / 0.062 / 0.323 / 0.365 | 0 / 0 / 0 / 0 / 0 | 0 / 0 / 0.0186 / 0.0968 / 0.109 |
| s* cutscenes | 1497 (skip 694) | 0 / 0.014 / 0.244 / 2.08 / 185 | 0 / 0 / 0.0466 / 0.544 / 9.57 | 0 / 0 / 0 / 0.485 / 37 | 0 / 0 / 0 / 1.12 / 80 |



Reading: OBSERVED that with the colour only (alpha ignored) the hemisphere sits at about half the sun's luma (median 0.55 over all gameplay rows, 0.49 on the overworld) and lights 1..3 are about equal to the sun's colour luma (median 1.05, 0.97, 1.17), because their rgb is stored non-zero even when their alpha is 0. OBSERVED that with alpha as multiplier (INFERRED reading) the hemisphere is 0.14 of the sun (p10 0.09, p90 0.35; overworld 0.105 median) and the secondary lights vanish (medians 0.013, 0, 0.014). So ER still ships a hemisphere ambient and three extra lights in the data; whether the extra lights are lit depends on alpha and on the enables of 2.3. `m30_00_0000` has exactly the overworld `m60_00_0000` ratios (identical LightSet values; OBSERVED in `wide-LightSet.tsv`). In `s*` cutscene files the ratios reach 100 and 184 (light-0 luma near 0), they are not gameplay.

Overworld `m60_00_0000`, ids 0 and 100 (ids 1000 and 1100 hold the same values as 0 and 100, OBSERVED), every time key (alpha-multiplied columns use the INFERRED reading; `n/a` = sun alpha 0 at that time, the key at 15 and 15.08 h):


| id | time | L0 luma*a | Hemi luma*a (mean up/down) | Hemi/L0 (rgb) | Hemi/L0 (rgb*a) | L1/L0 (rgb*a) | L2/L0 | L3/L0 |
|---|---|---|---|---|---|---|---|---|
| 0 | 0 | 0.442 | 0.0388 | 0.395 | 0.0879 | 0 | 0 | 0 |
| 0 | 4 | 0.492 | 0.0473 | 0.481 | 0.0961 | 0 | 0 | 0.88 |
| 0 | 8 | 0.492 | 0.0473 | 0.481 | 0.0961 | 0 | 0 | 0 |
| 0 | 12 | 0.419 | 0.0439 | 0.629 | 0.105 | 0 | 0 | 0 |
| 0 | 15 | 0 | 0.0378 | 0.488 | n/a (L0=0) | n/a (L0=0) | n/a (L0=0) | n/a (L0=0) |
| 0 | 15.08 | 0 | 0.0378 | 0.488 | n/a (L0=0) | n/a (L0=0) | n/a (L0=0) | n/a (L0=0) |
| 0 | 15.33 | 0.232 | 0.0378 | 0.488 | 0.163 | 0 | 0 | 0 |
| 0 | 21 | 0.232 | 0.0378 | 0.488 | 0.163 | 0 | 0 | 0 |
| 0 | 22.5 | 0.206 | 0.0378 | 0.552 | 0.184 | 0 | 0 | 0 |
| 100 | 0 | 0.442 | 0.0388 | 0.395 | 0.0879 | 0.113 | 0 | 0.034 |
| 100 | 4 | 0.492 | 0.0473 | 0.481 | 0.0961 | 0.102 | 0 | 0.0305 |
| 100 | 8 | 0.492 | 0.0473 | 0.481 | 0.0961 | 0.102 | 0 | 0.0305 |
| 100 | 12 | 0.419 | 0.0439 | 0.629 | 0.105 | 0.119 | 0 | 0.0358 |
| 100 | 15 | 0 | 0.0378 | 0.488 | n/a (L0=0) | n/a (L0=0) | n/a (L0=0) | n/a (L0=0) |
| 100 | 15.08 | 0 | 0.0378 | 0.488 | n/a (L0=0) | n/a (L0=0) | n/a (L0=0) | n/a (L0=0) |
| 100 | 15.33 | 0.232 | 0.0378 | 0.488 | 0.163 | 0.215 | 0 | 0.0645 |
| 100 | 21 | 0.232 | 0.0378 | 0.488 | 0.163 | 0.215 | 0 | 0.0645 |
| 100 | 22.5 | 0.206 | 0.0378 | 0.552 | 0.184 | 0.243 | 0 | 0.0729 |

### 2.3 Enables, FollowedCamera, baseline files

OBSERVED (`bool-params.tsv`; rows = file x id x time; "mixed" = a file whose values differ between ids or times):

| parameter | subset | rows 0 / 1 | files all-0 / all-1 / mixed |
|---|---|---|---|
| Enable LocalLight1 | m* (136 files) | 472 / 142 | 96 / 12 / 28 |
| Enable LocalLight1 | s* (84) | 468 / 1,401 | 7 / 18 / 59 |
| Enable LocalLight2 | m* (136) | 288 / 1,264 | 7 / 1 / 128 |
| Enable LocalLight2 | s* (84) | 1,141 / 776 | 13 / 6 / 65 |
| Light1 FollowedCamera | m* (136) | 369 / 237 | 24 / 2 / 110 |
| Light1 FollowedCamera | s* (84) | 1,807 / 47 | 69 / 0 / 15 |

So the "LocalLight" enables are on in a minority of gameplay rows for LocalLight1 (23 %) and a majority for LocalLight2 (81 %); FollowedCamera is on in 39 % of gameplay rows. INFERRED from the names only: "FollowedCamera" = light 1's direction follows the camera.

Baselines (values; `default` and `mapbase` hold LightSet ids 0/10/11 and 0/100; time series as `time: value`, `||` separates ids):


| param (type) | default | mapbase |
|---|---|---|
| Directional Light DiffColor0 (14) | id0: 0: 1,1,1,0.45 ; 4,8,12: 1,1,1,0.5 ; 15: 1,1,1,0 ; 21,22.5: 1,1,1,0.3 || id10/11: 1,1,1,0.6 | id0: 0: 1,1,1,0.45 ; 4,8,12: 1,1,1,0.5 ; 15: 1,1,1,0 ; 21,22.5: 1,1,1,0.3 || id100: 1,1,1,1 |
| Directional Light Sharpness0 (9) | id0: 0.15 || id10/11: 0.3 | id0: 0.15 || id100: 50 |
| Directional Light SsaoWeight0 (9) | 1 (all 3 ids) | id0: 1 || id100: 0 |
| Directional Light DiffColor1 (14) | id0: 0,0,0,0 || id10/11: 0.235,0.252,0.27,1 | id0: 0,0,0,0 || id100: 0,0,0,1 |
| Directional Light LocalLight DiffColor1 (14) | id0: 0,0,0,0 || id10/11: 0.169,0.182,0.194,1 | id0: 0,0,0,0 || id100: 0,0,0,1 |
| Directional Light LocalLight DiffColor2 (14) | id0: 0,0,0,0 || id10/11: 0.13,0.13,0.13,1 | id0: 0,0,0,0 || id100: 0,0,0,1 |
| Directional Light Hemi Color Up (14) | id0: 0.502,0.502,0.502,0.1 || id10/11: 0.502,0.502,0.502,1 | id0: 0.502,0.502,0.502,0.1 || id100: 0.502,0.502,0.502,1 |
| Directional Light Hemi Color Down (14) | id0: 0.502,0.502,0.502,0.1 || id10/11: 0.502,0.502,0.502,1 | id0: 0.502,0.502,0.502,0.1 || id100: 0.502,0.502,0.502,1 |
| Directional LightLight1 FollowedCamera (11) | id0: 0 || id10/11: 1 | 0 (all 2 ids) |
| Directional Enable LocalLight1 (11) | id0: 0 || id10/11: 1 | 0 (all 2 ids) |
| Directional Enable LocalLight2 (11) | 0 (all 3 ids) | 0 (all 2 ids) |

OBSERVED: `default` and `mapbase` both give a grey hemisphere (0.502, 0.502, 0.502) with alpha 0.1 for id 0, a white sun alpha 0.45..0.5 (0 at 15 h, 0.3 at 21 and 22.5 h) and, for id 0, lights 1..3 black alpha 0; ids 10/11 (default) and 100 (mapbase) are flat variants with hemisphere alpha 1 and a sun alpha 0.6 / 1 (default ids 10/11 also have lights 1..3 non-black at alpha 1) (INFERRED: neutral/test sets). Per ER-VALUES.md the inheritance of absent groups from `default`/`mapbase` is the earlier worker's INFERENCE, not verified here.

## 3. LightSet: DS3 against ER

DS3 (OBSERVED in `param_drawparam_m30_00_0000.tsv` and `..._m40_00_0000.tsv`, identical parameter set in both): group `LightSet` with 12 parameter names, five of them repeated four times per id (172 rows for 43 ids in m30_00_0000, 60 for 15 in m40; same id order in each of the 4 blocks; the 4 blocks are INFERRED to be lights 0..3). ER: 26 parameters, where each of those five is split into four named parameters.

| DS3 parameter (group `LightSet`) | ER parameter(s) (group `LightSet`) | change |
|---|---|---|
| Angle (x4: one value per light, 43 ids in m30_00_0000, 15 in m40_00_0000) | Directional Light Angle0, Angle1, LocalLight Angle1, LocalLight Angle2 | kept, renamed per light |
| DiffColor (x4) | Directional Light DiffColor0, DiffColor1, LocalLight DiffColor1, LocalLight DiffColor2 | kept, renamed per light |
| SpecColor (x4) | Directional Light SpecColor0, SpecColor1, LocalLight SpecColor1, LocalLight SpecColor2 | kept, renamed per light |
| Sharpness (x4) | Directional Light Sharpness0, Sharpness1, "Directional LocalLight Light Sharpness1", "...Sharpness2" | kept, renamed per light |
| SsaoWeight (x4) | Directional Light SsaoWeight0, SsaoWeight1, SsaoWeight2, SsaoWeight3 | kept, renamed per light |
| Hemi Color Up | Directional Light Hemi Color Up | kept |
| Hemi Color Down | Directional Light Hemi Color Down | kept |
| Light1 FollowedCamera | Directional LightLight1 FollowedCamera | kept (name garbled by a prefix) |
| Enable LocalLight1 | Directional Enable LocalLight1 | kept |
| Enable LocalLight2 | Directional Enable LocalLight2 | kept |
| LightMask Influence | Directional Light  LightMask Influence | kept (value 1 in all 1,690 gameplay rows) |
| giOcclusionRate | (absent) | REMOVED in ER (LightSet) |

OBSERVED: ER added **no** new LightSet parameter; it removed `giOcclusionRate` (DS3: value 0 in the rows sampled; a global-illumination occlusion rate by name, INFERRED) and renamed/indexed the four-light arrays. DS3 groups with no ER counterpart: `Dominant Light` (11 parameters: Dominant Sharpness Min Value / Offset / Exponant, Dominant Color Scale, Dominant R/G/B Scale, SH Color Scale, SH R/G/B Scale), `LightScattering` (15 parameters, Japanese names: light X/Y rotation, distance scale, light colour RGB, RGB scale, scattering direction factor, Rayleigh factor, Mie factor, blend factor, ground diffuse colour RGB, RGB scale), `Env Map`, `SeaWave`, `Snow`, `Snow Model`, `ColorParaffin Filter`, `WaterInteraction Filter`.

Side by side on the quantity of section 2.2 (same method, no interpolation, light 0 = the first 4-row block, INFERRED): DS3 `m30_00_0000` (43 ids): hemisphere/light-0 luma median 0.53 colour-only, 0.50 with alpha (p10 0.02, p90 0.80, 28 of 43 sets have a lit sun), hemisphere Up alpha median 1.0; DS3 `m40_00_0000` (15 ids): 1.09 colour-only, median 0 with alpha (hemisphere alpha 0 in most sets). ER gameplay: median 0.55 colour-only, 0.14 with alpha (hemisphere alpha mostly 0.1, sun alpha median 0.35). OBSERVED difference: ER sets the hemisphere alpha at 0.1 where DS3 sets it at 1 (m30) or 0 (m40).

## 4. VolumetricFog and FogVolume

### 4.1 VolumetricFog

OBSERVED: `VolumetricFog` is in 236 files (156 of the 208 `m*`, 80 `s*`; not in `default` nor `mapbase`; in 29 of the 50 `m60_*`/`m61_*` files); 48 parameters; 45 in 220 files, three (`FogVolume InjectingZSliceOffset`, `FogVolume LightInjectionEnabled`, `FogVolume CombineOwnDrawingWithLightInjection`) in a single file; 9 files hold only 40 parameters (all but `m12_07_0100` are `s*`), 5 `m15_*` files hold 13 and one 14 (partial, OBSERVED from `wide-VolumetricFog.tsv` columns). Every gameplay (`m*`) VolumetricFog file holds a single id (0); cutscene files hold up to 17 ids; time keys sit on the colour/density parameters (`m60_00_0000`: 0, 4, 8, 11.5, 15, 21, 22.5 h). The 52 gameplay files without the group include `mapbase`, `m00_00_0000`, 21 `m60_*`/`m61_*` variants and the `_0100`.. variants of several maps (OBSERVED by diff of file lists).

Enable parameter: `VolumetricFog Enable` (type 11): on in all 166 gameplay rows (150 files), off in 34 of 590 cutscene rows (1 file all-off, 12 mixed). `EnableAnalyticalFog`: on in 16 files, off in 134 (gameplay); `CullingBySceneDepth`: off everywhere (157 rows); `DirLight EnableReproj`: on in 148 files, off in 2; `FogVolume InjectingReprojEnabled`: off in all 491 rows.

Parameters whose names look like (INFERRED from names only, I did not read code):

| aspect | parameters |
|---|---|
| grid size / range | `VolumeWidth` (96 in 100 of 157 rows, up to 160), `VolumeDepth` (24 / 32 / 40), `NearDepth` (0.1), `FarDepth` (2000 in 74 rows; median 1800), `FogEndDepth` (0 = unset in 75 rows, median 1550, max 10000), `NearOffset`, `VolumeSliceSampleOffset`, `GameScale` (1 everywhere) |
| density / height fog | `Global FixedDensity` (0 in 112 of 210 rows; max 0.01), `Global HeightFogDistribution`, `Global HeightFogDensity Scale` (median 0.0023, up to 0.12), `Global HeightFogOffset`, `Global HeightFogReferenceOffset`, `Global HeightFogReferenceModeRate` |
| scattering / absorption | `Global ScatteringG` (0.2 in all 156 rows: an anisotropy factor by name), `Global AbsorptionScale` (type 13; 0,0,0 in 156 of 157 rows) |
| directional-light contribution | `DirLight DirectColor`, `DirLight OppositeColor` (rgba), `DirLight LightBankId/LightId` (which light; LightId 0 in all rows; BankId 160000000 in 102 rows), `HeightAtten`, `StartDist` (median 60, 0 in 79 rows), `ShadowSampleCount` (8), `ShadowDither` (0.4), `AngleClip` (0.2) |
| local-light contribution | `Global LocalLightScale` (1 in 140 of 162 rows, 0 in 7), `Global LocalLightDistScale` (0.5 in 154) |
| ambient / sky contribution | `SkyLight SkyColor` (rgba), `SkyLight HeightAtten` (0 in all), `SkyLight StartDist` (median 8) |
| shadow use | `DirLight ShadowSampleCount`, `ShadowDither`, `AngleClip` (all constant in the files that hold them) |
| temporal | `DirLight EnableReproj`, `JitterCount` (4), `JitterXYSize` (1), `FeedbackWeight` (0.8); `FogVolume Injecting* ` (SampleCount 16, Dither 0.4, ReprojEnabled 0, FeedbackWeight 0.8); `Config SampleCount` (8), `Config Dither` (0) |
| noise | none in VolumetricFog (OBSERVED: no name with Noise); the noise parameters are in FogVolume (4.2) |
| other | `DrawTiming` (type 3: 1 in 131 rows, 0 in 26), `CancelInScatteringRateFromSSR` (0 in all 157) |

Statistics over the 156 `m*` files (components `r g b` omitted for colours, they are in `param-stats-gameplay.tsv`):


| # | param (type) | files all/subset | comp | n | min | p10 | median | p90 | max | most common |
|---|---|---|---|---|---|---|---|---|---|---|
| 1 | VolumetricFog Enable (11) | 230/150 | v | 166 | 1 | 1 | 1 | 1 | 1 | 1 x166 |
| 2 | VolumetricFog VolumeWidth (7) | 230/150 | v | 157 | 80 | 96 | 96 | 160 | 160 | 96 x100 |
| 3 | VolumetricFog VolumeDepth (7) | 230/150 | v | 157 | 8 | 24 | 24 | 36 | 40 | 24 x91 |
| 4 | VolumetricFog GameScale (9) | 230/150 | v | 157 | 1 | 1 | 1 | 1 | 1 | 1 x157 |
| 5 | VolumetricFog NearDepth (9) | 230/150 | v | 157 | 0.01 | 0.1 | 0.1 | 0.1 | 0.1 | 0.1 x149 |
| 6 | VolumetricFog FarDepth (9) | 236/156 | v | 163 | 100 | 200 | 1800 | 2000 | 2000 | 2000 x74 |
| 7 | VolumetricFog NearOffset (9) | 230/150 | v | 157 | 0 | 0 | 1 | 1 | 1 | 1 x133 |
| 8 | VolumetricFog FogEndDepth (9) | 221/149 | v | 192 | 0 | 0 | 1550 | 4000 | 10000 | 0 x75 |
| 9 | VolumetricFog DrawTiming (3) | 230/150 | v | 157 | 0 | 0 | 1 | 1 | 1 | 1 x131 |
| 10 | VolumetricFog EnableAnalyticalFog (11) | 230/150 | v | 157 | 0 | 0 | 0 | 1 | 1 | 0 x135 |
| 11 | VolumetricFog VolumeSliceSampleOffset (9) | 230/150 | v | 157 | 0 | 0 | 0 | 0 | 0 | 0 x157 |
| 12 | VolumetricFog CullingBySceneDepth (11) | 230/150 | v | 157 | 0 | 0 | 0 | 0 | 0 | 0 x157 |
| 13 | VolumetricFog CancelInScatteringRateFromSSR (9) | 230/150 | v | 157 | 0 | 0 | 0 | 0 | 0 | 0 x157 |
| 14 | VolumetricFog Global FixedDensity (9) | 236/156 | v | 210 | 0 | 0 | 0 | 0.0013 | 0.01 | 0 x112 |
| 15 | VolumetricFog Global ScatteringG (9) | 230/150 | v | 156 | 0.2 | 0.2 | 0.2 | 0.2 | 0.2 | 0.2 x156 |
| 16 | VolumetricFog Global LocalLightScale (9) | 230/150 | v | 162 | 0 | 0.31 | 1 | 1 | 1 | 1 x140 |
| 17 | VolumetricFog Global LocalLightDistScale (9) | 230/150 | v | 156 | 0.5 | 0.5 | 0.5 | 0.5 | 1 | 0.5 x154 |
| 18 | VolumetricFog Global HeightFogDistribution (9) | 236/156 | v | 431 | -0.001 | 0.003 | 0.014 | 0.075 | 0.8 | 0.01 x80 |
| 19 | VolumetricFog Global HeightFogOffset (9) | 231/151 | v | 161 | -680 | 0 | 0 | 0 | 100 | 0 x158 |
| 20 | VolumetricFog Global HeightFogReferenceOffset (9) | 236/156 | v | 196 | -100 | 0 | 0 | 0 | 300 | 0 x184 |
| 21 | VolumetricFog Global HeightFogReferenceModeRate (9) | 236/156 | v | 193 | 0 | 1 | 1 | 1 | 1 | 1 x188 |
| 22 | VolumetricFog Global HeightFogDensity Scale (9) | 236/156 | v | 495 | 0 | 0.0004 | 0.0023 | 0.01 | 0.12 | 0.0025 x73 |
| 23 | VolumetricFog Global AbsorptionScale (13) | 230/150 | c0 | 157 | 0 | 0 | 0 | 0 | 1 | 0 x156 |
|  |  |  | c1 | 157 | 0 | 0 | 0 | 0 | 0.8 | 0 x156 |
|  |  |  | c2 | 157 | 0 | 0 | 0 | 0 | 0.6 | 0 x156 |
| 24 | VolumetricFog SkyLight SkyColor (14) | 236/156 | a | 629 | 0 | 0.05 | 0.3 | 1 | 16 | 1 x91 |
|  |  |  | luma(rgb) | 629 | 0 | 0.098359 | 0.26759 | 0.53411 | 0.93406 | 0.21 x21 |
| 25 | VolumetricFog SkyLight HeightAtten (9) | 236/156 | v | 165 | 0 | 0 | 0 | 0 | 0 | 0 x165 |
| 26 | VolumetricFog SkyLight StartDist (9) | 236/156 | v | 293 | 0 | 3 | 8 | 15 | 100 | 10 x100 |
| 27 | VolumetricFog DirLight LightBankId (7) | 230/150 | v | 165 | 0 | 1.21e+08 | 1.6e+08 | 1.61e+08 | 1.61e+08 | 1.6e+08 x102 |
| 28 | VolumetricFog DirLight LightId (7) | 230/150 | v | 159 | 0 | 0 | 0 | 0 | 0 | 0 x159 |
| 29 | VolumetricFog DirLight DirectColor (14) | 236/156 | a | 630 | 0 | 0.05 | 1 | 1.6 | 5 | 1 x200 |
|  |  |  | luma(rgb) | 630 | 0 | 0.25925 | 0.60607 | 0.87405 | 1 | 0.25925 x80 |
| 30 | VolumetricFog DirLight OppositeColor (14) | 236/156 | a | 630 | 0 | 0.05 | 0.5 | 1 | 2.5 | 1 x157 |
|  |  |  | luma(rgb) | 630 | 0.075678 | 0.1959 | 0.35969 | 0.81535 | 1 | 0.25925 x86 |
| 31 | VolumetricFog DirLight HeightAtten (9) | 236/156 | v | 181 | 0 | 0 | 0 | 0 | 0 | 0 x181 |
| 32 | VolumetricFog DirLight StartDist (9) | 236/156 | v | 349 | 0 | 0 | 60 | 150 | 320 | 0 x79 |
| 33 | VolumetricFog DirLight ShadowSampleCount (7) | 230/150 | v | 175 | 8 | 8 | 8 | 8 | 8 | 8 x175 |
| 34 | VolumetricFog DirLight ShadowDither (9) | 230/150 | v | 175 | 0.4 | 0.4 | 0.4 | 0.4 | 0.4 | 0.4 x175 |
| 35 | VolumetricFog DirLight AngleClip (9) | 230/150 | v | 175 | 0.2 | 0.2 | 0.2 | 0.2 | 0.2 | 0.2 x175 |
| 36 | VolumetricFog DirLight EnableReproj (11) | 230/150 | v | 180 | 0 | 1 | 1 | 1 | 1 | 1 x178 |
| 37 | VolumetricFog DirLight JitterCount (7) | 230/150 | v | 175 | 4 | 4 | 4 | 4 | 4 | 4 x175 |
| 38 | VolumetricFog DirLight JitterXYSize (9) | 230/150 | v | 175 | 1 | 1 | 1 | 1 | 1 | 1 x175 |
| 39 | VolumetricFog DirLight FeedbackWeight (9) | 230/150 | v | 175 | 0.8 | 0.8 | 0.8 | 0.8 | 0.8 | 0.8 x175 |
| 40 | VolumetricFog FogVolume InjectingSampleCount (7) | 221/149 | v | 491 | 16 | 16 | 16 | 16 | 16 | 16 x491 |
| 41 | VolumetricFog FogVolume InjectingDither (9) | 221/149 | v | 491 | 0.4 | 0.4 | 0.4 | 0.4 | 0.4 | 0.4 x491 |
| 42 | VolumetricFog FogVolume InjectingReprojEnabled (11) | 221/149 | v | 491 | 0 | 0 | 0 | 0 | 0 | 0 x491 |
| 43 | VolumetricFog FogVolume InjectingFeedbackWeight (9) | 221/149 | v | 491 | 0.8 | 0.8 | 0.8 | 0.8 | 0.8 | 0.8 x491 |
| 44 | VolumetricFog Config SampleCount (7) | 230/150 | v | 175 | 8 | 8 | 8 | 8 | 8 | 8 x175 |
| 45 | VolumetricFog Config Dither (9) | 230/150 | v | 175 | 0 | 0 | 0 | 0 | 0 | 0 x175 |
| 46 | VolumetricFog FogVolume InjectingZSliceOffset (9) | 1/1 | v | 7 | 0 | 0 | 0 | 0 | 0 | 0 x7 |
| 47 | VolumetricFog FogVolume LightInjectionEnabled (11) | 1/1 | v | 7 | 0 | 0 | 0 | 0 | 0 | 0 x7 |
| 48 | VolumetricFog FogVolume CombineOwnDrawingWithLightInjection (11) | 1/1 | v | 7 | 0 | 0 | 0 | 0 | 0 | 0 x7 |

Full values (OBSERVED, from `wide-VolumetricFog.tsv`; time series as `t,t: value ; t: value`, hours). `default` and `mapbase` hold **no** VolumetricFog (so no column). Shown here: `m60_00_0000` (overworld, each time key) and `m10_00_0000` (legacy castle). The same table for `m11_00_0000` (capital), `m12_01_0000` (underground) and `m30_00_0000` (dungeon) is in `C:/Users/User/Tools/DS3-Decompilation/study-ws12/er-values/vf-values-m11-m12_01-m30.md` (m30_00_0000 equals m60_00_0000 except FarDepth, FogEndDepth, HeightFogDistribution, HeightFogDensity Scale and DirLight OppositeColor, OBSERVED by cell comparison):


| param (type) | m60_00_0000 | m10_00_0000 |
|---|---|---|
| VolumetricFog Enable (11) | 1 | 1 |
| VolumetricFog VolumeWidth (7) | 96 | 96 |
| VolumetricFog VolumeDepth (7) | 24 | 24 |
| VolumetricFog GameScale (9) | 1 | 1 |
| VolumetricFog NearDepth (9) | 0.1 | 0.1 |
| VolumetricFog FarDepth (9) | 2000 | 300 |
| VolumetricFog NearOffset (9) | 1 | 1 |
| VolumetricFog FogEndDepth (9) | 4000 | 300 |
| VolumetricFog DrawTiming (3) | 1 | 1 |
| VolumetricFog EnableAnalyticalFog (11) | 0 | 0 |
| VolumetricFog VolumeSliceSampleOffset (9) | 0 | 0 |
| VolumetricFog CullingBySceneDepth (11) | 0 | 0 |
| VolumetricFog CancelInScatteringRateFromSSR (9) | 0 | 0 |
| VolumetricFog Global FixedDensity (9) | 0 | 1e-05 |
| VolumetricFog Global ScatteringG (9) | 0.2 | 0.2 |
| VolumetricFog Global LocalLightScale (9) | 1 | 1 |
| VolumetricFog Global LocalLightDistScale (9) | 0.5 | 0.5 |
| VolumetricFog Global HeightFogDistribution (9) | 0,4,8: 0.01 ; 11.5: 0.008 ; 15,21,22.5: 0.003 | 0.06 |
| VolumetricFog Global HeightFogOffset (9) | 0 | 0 |
| VolumetricFog Global HeightFogReferenceOffset (9) | 0 | 0 |
| VolumetricFog Global HeightFogReferenceModeRate (9) | 1 | 1 |
| VolumetricFog Global HeightFogDensity Scale (9) | 0: 0.0016 ; 4,8,11.5: 0.0012 ; 15,21,22.5: 0.0025 | 0.004 |
| VolumetricFog Global AbsorptionScale (13) | 0,0,0 | 0,0,0 |
| VolumetricFog SkyLight SkyColor (14) | 0: 0.137,0.157,0.165,0.4 ; 4: 0.157,0.188,0.188,0.4 ; 8: 0.157,0.157,0.137,0.6 ; 11.5: 0.196,0.235,0.275,0.25 ; 15: 0.0392,0.0392,0.0781,1 ; 21: 0.0387,0.0392,0.0784,1 ; 22.5: 0.0387,0.0387,0.0784,1 | 0,4,8: 0.49,0.549,0.51,0.05 ; 11.5: 0.784,0.784,0.784,0 ; 15,21,22.5: 0.392,0.508,0.627,0 |
| VolumetricFog SkyLight HeightAtten (9) | 0 | 0 |
| VolumetricFog SkyLight StartDist (9) | 3 | 3 |
| VolumetricFog DirLight LightBankId (7) | 160000000 | 160000000 |
| VolumetricFog DirLight LightId (7) | 0 | 0 |
| VolumetricFog DirLight DirectColor (14) | 0: 0.615,0.563,0.479,1.6 ; 4: 0.49,0.49,0.373,1 ; 8: 0.51,0.471,0.392,0.9 ; 11.5: 0.495,0.321,0.2,1 ; 15,21: 0.196,0.275,0.294,0.5 ; 22.5: 0.196,0.275,0.294,1 | 0: 0.902,0.98,0.843,0.3 ; 4: 0.902,0.98,0.843,0.2 ; 8: 0.902,0.98,0.843,0.3 ; 11.5: 0.895,0.618,0.479,0.3 ; 15,21,22.5: 0.706,0.804,0.784,0.066 |
| VolumetricFog DirLight OppositeColor (14) | 0: 0.288,0.441,0.555,0.8 ; 4: 0.275,0.353,0.353,1 ; 8: 0.392,0.465,0.463,0.9 ; 11.5: 0.196,0.156,0.196,1 ; 15,21: 0.196,0.275,0.294,0.5 ; 22.5: 0.196,0.275,0.294,1 | 0,4,8: 1,1,1,0.05 ; 11.5: 1,1,1,0.1 ; 15,21,22.5: 0.706,0.804,0.784,0.05 |
| VolumetricFog DirLight HeightAtten (9) | 0 | 0 |
| VolumetricFog DirLight StartDist (9) | 150 | 3 |
| VolumetricFog DirLight ShadowSampleCount (7) | 8 | 8 |
| VolumetricFog DirLight ShadowDither (9) | 0.4 | 0.4 |
| VolumetricFog DirLight AngleClip (9) | 0.2 | 0.2 |
| VolumetricFog DirLight EnableReproj (11) | 1 | 1 |
| VolumetricFog DirLight JitterCount (7) | 4 | 4 |
| VolumetricFog DirLight JitterXYSize (9) | 1 | 1 |
| VolumetricFog DirLight FeedbackWeight (9) | 0.8 | 0.8 |
| VolumetricFog FogVolume InjectingSampleCount (7) | 16 | 16 |
| VolumetricFog FogVolume InjectingDither (9) | 0.4 | 0.4 |
| VolumetricFog FogVolume InjectingReprojEnabled (11) | 0 | 0 |
| VolumetricFog FogVolume InjectingFeedbackWeight (9) | 0.8 | 0.8 |
| VolumetricFog Config SampleCount (7) | 8 | 8 |
| VolumetricFog Config Dither (9) | 0 | 0 |
| VolumetricFog FogVolume InjectingZSliceOffset (9) | - | - |
| VolumetricFog FogVolume LightInjectionEnabled (11) | - | - |
| VolumetricFog FogVolume CombineOwnDrawingWithLightInjection (11) | - | - |

### 4.2 FogVolume

OBSERVED: `FogVolume` is in 143 files (96 `m*`: 95 map files plus `mapbase`, including 20 `m61_*` DLC files; 47 `s*`; not in `default`, in no `m60_*` file). 37 distinct parameter names (38 entries): 30 in 142-143 holding files, 8 entries in a single file (`DrawArea FarFadeMinDistance`, `FarFadeMaxDistance`, `ZOffset`, `Light LightInjectingType`, `ColorMapTextureIntensity`, `ColorMapTextureUVTiling-x/-y`, `DrawArea HeightFallOff` as type 3). A FogVolume file holds one **id per fog-volume preset** (`mapbase` 195 ids numbered 0..1750; `m10_00_0000` 100, 101; `m11_00_0000` 110..114; `m12_01_0000` 120..127; `m30_00_0200` id 0), and almost no preset parameter carries time keys (OBSERVED: 398 of 34,246 rows have a time other than 0). `mapbase` alone contributes 5,954 of the gameplay rows.

There is **no enable parameter** in `FogVolume` (OBSERVED: no name with Enable); the volume's presence is by the preset id referenced from outside this file (INFERRED, not read), and `FogVolume Light Overwrite` (type 11) is a per-preset switch: on in 109 of 886 gameplay rows (7 files all-1, 60 all-0, 29 mixed), INFERRED "use this volume's own light instead of the global one" (it is followed by the volume's own DirectionalLightAngle/Color/OppositeColor/AmbientColor).

Parameter groups by name (INFERRED): `DrawArea` (near/edge/height fade, MaxInjectingDepth: where the volume draws), `RayMarching` (MaxSampleCount 8..64, SteppingDistance 0.1/0.2, SuspendedTransmittance 0.001/0.01, LightProcessingInterval/Threshold), `Light` (overwrite, directional light angle and colours, AmbientColor, ShadowingIntensity, AttenuationIntensity/Offset, OctaveNumPerlinNoise), `Density Intensity` (median 0.015, max 8), `PerlinNoise` (Frequency, Lacunarity, Gain, Offset), `Scroll` (Speed-x/-y/-z, DetailScale): density noise with scrolling. No scattering, anisotropy or temporal parameter lives in FogVolume (those are the `VolumetricFog FogVolume Injecting*` ones above).

Statistics over the 96 `m*` files (channels r g b omitted):


| # | param (type) | files all/subset | comp | n | min | p10 | median | p90 | max | most common |
|---|---|---|---|---|---|---|---|---|---|---|
| 1 | FogVolume DrawArea NearFadeMinDistance (9) | 143/96 | v | 832 | 0 | 0 | 0 | 20 | 2000 | 0 x567 |
| 2 | FogVolume DrawArea NearFadeMaxDistance (9) | 143/96 | v | 832 | 0 | 0 | 30 | 500 | 2000 | 0 x211 |
| 3 | FogVolume DrawArea EdgeFadeDistance (9) | 143/96 | v | 831 | 0.01 | 0.25 | 10 | 60 | 500 | 0.25 x247 |
| 4 | FogVolume DrawArea EdgeFadeExponent (9) | 143/96 | v | 831 | 0.001 | 0.5 | 0.5 | 0.5 | 5 | 0.5 x686 |
| 5 | FogVolume DrawArea HeightFallOff (9) | 143/96 | v | 836 | -4 | 0 | 1.11 | 5 | 10 | 0 x161 |
| 6 | FogVolume DrawArea FadeDistanceForHeightMap (9) | 142/96 | v | 831 | 0 | 0 | 0 | 100 | 1000 | 0 x576 |
| 7 | FogVolume DrawArea MaxInjectingDepth (9) | 142/96 | v | 855 | 0 | 0 | 20000 | 20000 | 20000 | 20000 x489 |
| 8 | FogVolume RayMarching MaxSampleCount (7) | 143/96 | v | 831 | 8 | 8 | 32 | 32 | 64 | 32 x408 |
| 9 | FogVolume RayMarching SteppingDistance (9) | 143/96 | v | 831 | 0.1 | 0.1 | 0.1 | 0.2 | 0.2 | 0.1 x584 |
| 10 | FogVolume RayMarching SuspendedTransmittance (9) | 143/96 | v | 831 | 0.001 | 0.001 | 0.001 | 0.01 | 0.01 | 0.001 x557 |
| 11 | FogVolume RayMarching LightProcessingInterval (7) | 143/96 | v | 831 | 1 | 4 | 4 | 8 | 8 | 4 x515 |
| 12 | FogVolume RayMarching LightProcessingIntervalThreshold (9) | 143/96 | v | 831 | 0.25 | 0.5 | 0.5 | 0.75 | 0.75 | 0.5 x601 |
| 13 | FogVolume Light Overwrite (11) | 143/96 | v | 886 | 0 | 0 | 0 | 1 | 1 | 0 x777 |
| 14 | FogVolume Light DirectionalLightAngle (12) | 143/96 | c0 | 850 | -1.5708 | -0.7854 | -0.7854 | -0.7854 | 0 | -0.7854 x785 |
|  |  |  | c1 | 850 | -1.3195 | 1.5708 | 1.5708 | 1.5708 | 3.1416 | 1.5708 x786 |
| 15 | FogVolume Light DirectionalLightColor (14) | 143/96 | a | 858 | 0 | 1 | 1 | 1 | 2.1 | 1 x786 |
|  |  |  | luma(rgb) | 858 | 0 | 0.19994 | 1 | 1 | 1 | 1 x699 |
| 16 | FogVolume Light DirectionalLightOppositeColor (14) | 143/96 | a | 858 | 0 | 1 | 1 | 1 | 3 | 1 x747 |
|  |  |  | luma(rgb) | 858 | 0 | 0.13726 | 1 | 1 | 1 | 1 x712 |
| 17 | FogVolume Light AmbientColor (14) | 143/96 | a | 890 | 0 | 0.95 | 1 | 1 | 1 | 1 x801 |
|  |  |  | luma(rgb) | 890 | 0 | 0 | 0 | 0.14312 | 1 | 0 x719 |
| 18 | FogVolume Light ShadowingIntensity (9) | 143/96 | v | 837 | 0 | 0 | 0 | 0.676 | 1 | 0 x684 |
| 19 | FogVolume Light AttenuationIntensity (9) | 143/96 | v | 831 | 0 | 0 | 0 | 0.25 | 1 | 0 x675 |
| 20 | FogVolume Light AttenuationOffset (9) | 143/96 | v | 831 | 0.01 | 1.25 | 1.25 | 3 | 12 | 1.25 x656 |
| 21 | FogVolume Light OctaveNumPerlinNoise (7) | 143/96 | v | 831 | 1 | 1 | 3 | 3 | 3 | 3 x447 |
| 22 | FogVolume Density Intensity (9) | 143/96 | v | 976 | 0 | 0 | 0.015 | 0.4 | 8 | 0 x288 |
| 23 | FogVolume PerlinNoise Frequency (9) | 143/96 | v | 832 | 0 | 0 | 0.01 | 0.1 | 1 | 0 x101 |
| 24 | FogVolume PerlinNoise Lacunarity (9) | 143/96 | v | 832 | 1.1 | 2.5 | 3.3333 | 3.9333 | 4 | 3.3333 x232 |
| 25 | FogVolume PerlinNoise Gain (9) | 143/96 | v | 832 | 0 | 0 | 0.5 | 0.5 | 1 | 0.5 x404 |
| 26 | FogVolume PerlinNoise Offset (9) | 143/96 | v | 839 | -0.5 | 0 | 0.1 | 1 | 1 | 0 x232 |
| 27 | FogVolume Scroll Speed-x (9) | 143/96 | v | 837 | -25 | -0.06 | 0 | 1 | 10 | 0 x435 |
| 28 | FogVolume Scroll Speed-y (9) | 143/96 | v | 837 | -10 | -1 | 0 | 0 | 3.2 | 0 x433 |
| 29 | FogVolume Scroll Speed-z (9) | 143/96 | v | 837 | -20 | -1 | 0 | 1 | 9 | 0 x401 |
| 30 | FogVolume Scroll DetailScale (9) | 143/96 | v | 831 | 0.0001 | 1 | 1 | 1 | 4 | 1 x739 |
| 31 | FogVolume DrawArea FarFadeMinDistance (9) | 1/1 | v | 6 | 0 | 0 | 0 | 0 | 0 | 0 x6 |
| 32 | FogVolume DrawArea FarFadeMaxDistance (9) | 1/1 | v | 6 | 0 | 0 | 0 | 0 | 0 | 0 x6 |
| 33 | FogVolume DrawArea ZOffset (9) | 1/1 | v | 6 | 0 | 0 | 0 | 0 | 0 | 0 x6 |
| 34 | FogVolume Light LightInjectingType (3) | 1/1 | v | 6 | 0 | 0 | 0 | 0 | 0 | 0 x6 |
| 35 | FogVolume Light ColorMapTextureIntensity (9) | 1/1 | v | 6 | 1 | 1 | 1 | 1 | 1 | 1 x6 |
| 36 | FogVolume Light ColorMapTextureUVTiling-x (9) | 1/1 | v | 6 | 1 | 1 | 1 | 1 | 1 | 1 x6 |
| 37 | FogVolume Light ColorMapTextureUVTiling-y (9) | 1/1 | v | 6 | 1 | 1 | 1 | 1 | 1 | 1 x6 |

Values per file (OBSERVED; each cell is the value when constant over the file's ids, `min..max (n ids)` for numbers that differ, or the first two distinct values for vectors; every id in `wide-FogVolume.tsv`). `default`, `m60_00_0000` and `m30_00_0000` hold no FogVolume (the dungeon file is `m30_00_0200`). Shown: `mapbase` (195 preset ids) and `m11_00_0000` (capital, 5 ids); `m10_00_0000` (legacy castle, 2 ids), `m12_01_0000` (underground, 8 ids) and `m30_00_0200` (dungeon, 1 id) are in `C:/Users/User/Tools/DS3-Decompilation/study-ws12/er-values/fv-values-m10-m12_01-m30_00_0200.md`:


| param (type) | mapbase | m11_00_0000 |
|---|---|---|
| FogVolume DrawArea NearFadeMinDistance (9) | 0..800 (195 ids) | 0 (all 5 ids) |
| FogVolume DrawArea NearFadeMaxDistance (9) | 0..2e+03 (195 ids) | 0..200 (5 ids) |
| FogVolume DrawArea EdgeFadeDistance (9) | 0.01..500 (195 ids) | 5..50 (5 ids) |
| FogVolume DrawArea EdgeFadeExponent (9) | 0.001..5 (195 ids) | 0.5..1 (5 ids) |
| FogVolume DrawArea HeightFallOff (9) | 0 / 2 ... (195 ids) | 0.6..3.5 (5 ids) |
| FogVolume DrawArea FadeDistanceForHeightMap (9) | 0..1e+03 (195 ids) | 0..10 (5 ids) |
| FogVolume DrawArea MaxInjectingDepth (9) | 0 / 2000 ... (195 ids) | 2000 (all 5 ids) |
| FogVolume RayMarching MaxSampleCount (7) | 8..64 (195 ids) | 16 (all 5 ids) |
| FogVolume RayMarching SteppingDistance (9) | 0.1..0.2 (195 ids) | 0.2 (all 5 ids) |
| FogVolume RayMarching SuspendedTransmittance (9) | 0.001..0.01 (195 ids) | 0.01 (all 5 ids) |
| FogVolume RayMarching LightProcessingInterval (7) | 1..8 (195 ids) | 8 (all 5 ids) |
| FogVolume RayMarching LightProcessingIntervalThreshold (9) | 0.25..0.75 (195 ids) | 0.75 (all 5 ids) |
| FogVolume Light Overwrite (11) | 0..1 (195 ids) | 0 (all 5 ids) |
| FogVolume Light DirectionalLightAngle (12) | -0.785,1.57 / -1.57,0 ... (195 ids) | -0.785,1.57 (all 5 ids) |
| FogVolume Light DirectionalLightColor (14) | 1,1,1,1 / 0.652,0.792,0.81,1 ... (195 ids) | 1,1,1,1 (all 5 ids) |
| FogVolume Light DirectionalLightOppositeColor (14) | 1,1,1,1 / 1,1,1,0 ... (195 ids) | 1,1,1,1 (all 5 ids) |
| FogVolume Light AmbientColor (14) | 0,0,0,1 / 0,0,0,0 ... (195 ids) | 0,0,0,1 (all 5 ids) |
| FogVolume Light ShadowingIntensity (9) | 0..1 (195 ids) | 0 (all 5 ids) |
| FogVolume Light AttenuationIntensity (9) | 0..1 (195 ids) | 0 (all 5 ids) |
| FogVolume Light AttenuationOffset (9) | 0.01..12 (195 ids) | 0.01..1.25 (5 ids) |
| FogVolume Light OctaveNumPerlinNoise (7) | 1..3 (195 ids) | 1 (all 5 ids) |
| FogVolume Density Intensity (9) | 1 / 0.0015 ... (195 ids) | 0.006..0.2 (5 ids) |
| FogVolume PerlinNoise Frequency (9) | 0..0.1 (195 ids) | 0.005..0.2 (5 ids) |
| FogVolume PerlinNoise Lacunarity (9) | 1.1..4 (195 ids) | 2.9..3.89 (5 ids) |
| FogVolume PerlinNoise Gain (9) | 0..1 (195 ids) | 0.1..0.9 (5 ids) |
| FogVolume PerlinNoise Offset (9) | 0 / -0.2 ... (195 ids) | 0.1..0.9 (5 ids) |
| FogVolume Scroll Speed-x (9) | -8..10 (195 ids) | 0.03..1.5 (5 ids) |
| FogVolume Scroll Speed-y (9) | -6..3.2 (195 ids) | -0.2..-0.1 (5 ids) |
| FogVolume Scroll Speed-z (9) | -20..9 (195 ids) | -0.5..-0.003 (5 ids) |
| FogVolume Scroll DetailScale (9) | 0.0001..4 (195 ids) | 1 (all 5 ids) |
| FogVolume DrawArea FarFadeMinDistance (9) | - | - |
| FogVolume DrawArea FarFadeMaxDistance (9) | - | - |
| FogVolume DrawArea ZOffset (9) | - | - |
| FogVolume Light LightInjectingType (3) | - | - |
| FogVolume Light ColorMapTextureIntensity (9) | - | - |
| FogVolume Light ColorMapTextureUVTiling-x (9) | - | - |
| FogVolume Light ColorMapTextureUVTiling-y (9) | - | - |

## 5. EnvironmentConfig, RayTracing, and the short groups

### 5.1 EnvironmentConfig

OBSERVED: 44 files (40 `m*`: `m20`, `m21`, `m22`, `m28`, `m40`..`m43`, `m61_00_*` variants; 4 `s*`); not in `default`/`mapbase`; 4 parameters, all `EnvMapColorConvert` (INFERRED: a colour conversion applied to the environment map): Hue, Saturate (floats), Shift and Scale (rgba). Statistics over the 40 `m*` files:


| # | param (type) | files all/subset | comp | n | min | p10 | median | p90 | max | most common |
|---|---|---|---|---|---|---|---|---|---|---|
| 1 | EnvironmentConfig EnvMapColorConvert Hue (9) | 44/40 | v | 148 | 0 | 0 | 0 | 0 | 0 | 0 x148 |
| 2 | EnvironmentConfig EnvMapColorConvert Saturate (9) | 44/40 | v | 148 | 0.5 | 1 | 1 | 1 | 1 | 1 x147, 0.5 x1 |
| 3 | EnvironmentConfig EnvMapColorConvert Shift (14) | 44/40 | r | 176 | 0.3212 | 0.715 | 1 | 1 | 1 | 1 x126, 0.92491 x8 |
|  |  |  | g | 176 | 0.36991 | 0.79503 | 1 | 1 | 1 | 1 x114, 0.915 x8 |
|  |  |  | b | 176 | 0.44 | 0.92098 | 1 | 1 | 1 | 1 x156, 0.755 x5 |
|  |  |  | luma(rgb) | 176 | 0.36461 | 0.80939 | 1 | 1 | 1 | 1 x111, 0.92324 x8 |
| 4 | EnvironmentConfig EnvMapColorConvert Scale (14) | 44/40 | r | 186 | 0 | 0.59833 | 0.94118 | 1 | 1 | 1 x86, 0.94118 x22 |
|  |  |  | g | 186 | 0 | 0.55515 | 0.94118 | 1 | 1 | 1 x54, 0.94118 x32 |
|  |  |  | b | 186 | 0 | 0.49108 | 0.82353 | 1 | 1 | 1 x76, 0.82353 x32 |
|  |  |  | luma(rgb) | 186 | 0 | 0.55597 | 0.94519 | 1 | 1 | 1 x54, 0.94519 x32 |

OBSERVED: Hue is 0 in all 148 rows; Saturate is 1 except one file (`m22_00_0300`: 0.5); Shift is exactly (1,1,1) in about 111 of 176 rows (luma 1); Scale channel = 1 in 54..86 of 186 rows and otherwise darkens (down to 0 in `m61_00_0678`, 0.17 in `m61_00_0650`, a time-of-day series 0.43..1 in `m22_00_0900`, `m41_00_0700`, `m61_00_0060`). Per-file lines in `env-per-file.txt`.

### 5.2 RayTracing

OBSERVED: 47 files: `default`, `mapbase` (the only `m*` file) and 45 `s*` cutscene files (`s12_06`, `s14_*`, `s15_*`, `s16_00_0020`, `s19_*`, `s20_*`, `s21_*`, `s28_00_0000`, `s34_11_*`, `s35_00_0000`, `s60_42_*`, `s60_43_0000`, `s60_54_*`, `s61_44_0000`); 25 parameters (23 in all 47 files, `DirLightShadow AdjustRayStartPosition` and `AO PostProcess ATrousFilterSteps` in 46). Two features by name: directional-light shadow (`Enable DirLightShadow`, `DirLightShadow *`) and ambient occlusion (`Enable AO`, `AO *`), each with ray count/length, an alpha-test flag, a denoise switch with feedback weight, and a composite fade by distance. There is **no** RT reflection, GI or fog parameter in the group (OBSERVED from the 25 names). A file holds several ids (10, 20, 30, 40, 50, ... up to 110: 240 file+id rows over 47 files); all values below are over all 240 rows. Values `default` / `mapbase` are in the last two columns; the complete values of every file and id (including which files turn each feature off) are in `wide-RayTracing.tsv`.


| param (type) | distinct values over the 240 (file, id) rows of the 47 files: value x rows | default | mapbase |
|---|---|---|---|
| RayTracing Enable DirLightShadow (11) | 1 x192, 0 x48 | 1 | 1 |
| RayTracing Enable AO (11) | 1 x234, 0 x6 | 1 | 1 |
| RayTracing DirLightShadow Ray SampleCount (7) | 1 x239, 4 x1 | 1 | 1 |
| RayTracing DirLightShadow Ray LengthMin (9) | 0.01 x202, 0.005 x23, 0.0005 x8, 0.003 x4, 0.002 x2, 0.03 x1 | 0.01 | 0.01 |
| RayTracing DirLightShadow Ray LengthMax (9) | 3000 x227, 1 x8, 150 x4, 10000 x1 | 3000 | 3000 |
| RayTracing DirLightShadow Ray EnableAlpha (11) | 1 x240 | 1 | 1 |
| RayTracing DirLightShadow ConeRadius (9) | 0.005 x236, 0 x3, 0.05 x1 | 0.005 | 0.005 |
| RayTracing DirLightShadow PostProcess EnableDenoise (11) | 1 x240 | 1 | 1 |
| RayTracing DirLightShadow PostProcess FeedbackWeight (9) | 0.9 x240 | 0.9 | 0.9 |
| RayTracing DirLightShadow Composite EnableFadeRate (9) | 1 x240 | 1 | 1 |
| RayTracing DirLightShadow Composite FadeStartDist (9) | 100 x238, 10 x1, 60 x1 | 100 | 100 |
| RayTracing DirLightShadow Composite FadeRangeDist (9) | 10 x240 | 10 | 10 |
| RayTracing AO Ray SampleCount (7) | 2 x216, 4 x24 | 4 | 2 |
| RayTracing AO Ray LengthMin (9) | 0.01 x204, 0.1 x6, 0 x5, 0.2 x5, 0.125 x4, 0.03 x3, 0.15 x3 ...(15 distinct) | 0.01 | 0.01 |
| RayTracing AO Ray LengthMax (9) | 3 x104, 1 x44, 2 x32, 1.5 x11, 0.1 x11, 5 x10, 0.5 x9 ...(15 distinct) | 3 | 3 |
| RayTracing AO Ray EnableAlpha (11) | 1 x230, 0 x10 | 1 | 1 |
| RayTracing AO Scale (9) | 1 x68, 1.6 x64, 1.5 x41, 1.3 x15, 2 x13, 1.2 x8, 1.4 x7 ...(16 distinct) | 1 | 1 |
| RayTracing AO Offset (9) | 0 x143, 0.1 x37, 0.2 x20, 0.3 x9, -0.6 x7, 0.25 x4, -0.2 x4 ...(16 distinct) | 0 | 0 |
| RayTracing AO PostProcess EnableDenoise (11) | 1 x240 | 1 | 1 |
| RayTracing AO PostProcess FeedbackWeight (9) | 0.9 x240 | 0.9 | 0.9 |
| RayTracing AO Composite EnableFadeRate (9) | 1 x240 | 1 | 1 |
| RayTracing AO Composite FadeStartDist (9) | 100 x232, 150 x3, 130 x1, 120 x1, 800 x1, 10 x1, 700 x1 | 100 | 100 |
| RayTracing AO Composite FadeRangeDist (9) | 10 x240 | 10 | 10 |
| RayTracing DirLightShadow AdjustRayStartPosition (11) | 1 x239 | - | 1 |
| RayTracing AO PostProcess ATrousFilterSteps (9) | 1.4 x212, 2 x24, 3 x3 | - | 1.4 |

OBSERVED reading: shadow rays 1 per pixel, 4 only in one row; AO rays 2 per pixel in 216 rows, 4 in 24 (`default` 4, `mapbase` 2); shadow ray LengthMax 3000 in 227 of 240 rows (1, 150 or 10000 otherwise); AO ray LengthMax most often 3 (104 rows), then 1 (44) and 2 (32) (units INFERRED metres); AO Scale values around 1..1.6; both features denoised with feedback weight 0.9 and fading out from 100 for 10 units (INFERRED: metres). `Enable DirLightShadow` is 0 in 48 of 240 rows (6 files all-off) and `Enable AO` 0 in 6 rows.

### 5.3 ShadowParam, SSAO, SSR, Wet (statistics over all files holding them, since `m*` holds 19, 1, 1, 1 of them respectively; min, median, max only here, p10/p90 in `param-stats-allfiles.tsv`; `SP` = ShadowParam)

OBSERVED: ShadowParam (stored name of its first parameter: "hadowParam - Shadow: Angle0", typo kept) is in 100 files (19 `m*`, 80 `s*`, `default`), SSAO in 35 (`mapbase`, 33 `s*`, `default`), SSR in 37 (`mapbase`, 36 `s*`), Wet in 7 (`mapbase`, 6 `s*`). Boolean switches in these groups: `SSR Enable` on in 21 of 36 `s*` files and in the single `m*` file; `Shadow Model Cull Flip` off everywhere; `AlignTexel` on in all `m*` and `s*` files (`default` mixed); `TempAA Auto Jitter` on in 18 of 19 `m*`.


| # | param (type) | files | comp | min | median | max | most common |
|---|---|---|---|---|---|---|---|
| 1 | hadowParam - Shadow: Angle0 (12) | 100 | c0 | -0.77954 | 0 | 0 | 0 x468 |
|  |  |  | c1 | -1.4766 | 0 | 0 | 0 x468 |
| 2 | shadowParam -   Shadow:  Shadow Light Id (7) | 100 | v | -1 | 40 | 950 | 0 x64 |
| 3 | SP-Shadow: Near (9) | 100 | v | 0.001 | 0.1 | 10 | 0.1 x367 |
| 4 | SP-Shadow: Cascade Dist 0->1 (9) | 100 | v | 1 | 10 | 1000 | 10 x178 |
| 5 | SP-Shadow: Cascade Dist 1->2 (9) | 100 | v | 3 | 30 | 6000 | 30 x171 |
| 6 | SP-Shadow: Cascade Dist 2->3 (9) | 100 | v | 7 | 70 | 10000 | 70 x214 |
| 7 | SP-Shadow: Far Fade Start (9) | 100 | v | 20 | 1000 | 8000 | 1000 x349 |
| 8 | SP-Shadow: Far Fade Dist (9) | 100 | v | 10 | 10 | 300 | 10 x467 |
| 9 | SP-Shadow: Cascade Fade Range Scale (9) | 100 | v | 0.05 | 0.05 | 0.05 | 0.05 x471 |
| 10 | SP-Shadow: Shadow Map FilterMode (3) | 100 | v | 1 | 1 | 2 | 1 x458 |
| 11 | SP-Shadow: Depth Offset (9) | 100 | v | -0.01 | -0.001 | 0 | -0.001 x468 |
| 12 | SP-Volume Depth (9) | 100 | v | 1 | 100 | 4000 | 100 x339 |
| 13 | SP-Depth Bias (3) | 100 | v | 0 | 2 | 2 | 2 x440 |
| 14 | SP-Slope Scaled Depth Bias (9) | 100 | v | 2 | 3 | 6 | 3 x516 |
| 15 | Shadow Model Cull Flip (11) | 100 | v | 0 | 0 | 0 | 0 x471 |
| 16 | SP-AlignTexel (11) | 100 | v | 0 | 1 | 1 | 1 x469 |
| 17 | SP-DepthRate (9) | 100 | v | 0.5 | 1 | 1 | 1 x429 |
| 18 | SP-DepthScale (9) | 100 | v | 5 | 5 | 30 | 5 x429 |
| 19 | SP-KernelScale (9) | 100 | v | 1 | 1.5 | 2 | 1.5 x467 |
| 20 | SP-DepthOffsetScale (9) | 100 | v | -0.085 | -0.085 | 0 | -0.085 x469 |
| 21 | SP-Blur Count (7) | 100 | v | 0 | 0 | 4 | 0 x427 |
| 22 | SP-Blur Radius (9) | 100 | v | 2 | 2 | 8 | 2 x452 |
| 23 | SP-TempAA Auto Jitter (11) | 100 | v | 0 | 0 | 1 | 0 x409 |
| 24 | SP Cascade Count (7) | 91 | v | 0 | 4 | 4 | 4 x418 |
| 25 | SP-Shadow: Shadow Map Resolution (3) | 100 | v | -1 | 0 | 3 | -1 x219 |
| 26 | SP Cascade0 Resolution (3) | 100 | v | 0 | 1 | 6 | 1 x301 |
| 27 | SP Cascade1 Resolution (3) | 100 | v | 0 | 1 | 2 | 1 x493 |
| 28 | SP Cascade2 Resolution (3) | 100 | v | 0 | 1 | 2 | 1 x468 |
| 29 | SP Cascade3 Resolution (3) | 100 | v | 0 | 1 | 2 | 1 x505 |
| 30 | SP CompartmentShadow Resolution (3) | 100 | v | 0 | 1 | 3 | 1 x509 |

SSAO:

| # | param (type) | files | comp | min | median | max | most common |
|---|---|---|---|---|---|---|---|
| 1 | SSAO - Edit-UseNormal (11) | 35 | v | 0 | 0 | 1 | 0 x60 |
| 2 | SSAO - Edit-UseReProj (11) | 35 | v | 0 | 1 | 1 | 1 x72 |
| 3 | SSAO - Edit-PreventGhost (11) | 35 | v | 0 | 1 | 1 | 1 x72 |
| 4 | SSAO - Edit-EnableUpScale (11) | 35 | v | 0 | 1 | 1 | 1 x72 |
| 5 | SSAO - Edit-Ver2 Sample Offset (9) | 35 | v | 0.5 | 4 | 10 | 4 x24 |
| 6 | SSAO - Edit-Ver2 AO Scale (9) | 35 | v | 0.44 | 1 | 1.5 | 1 x34 |
| 7 | SSAO - Edit-Ver2 AO Offset (9) | 35 | v | -0.6 | 0 | 0.05 | 0 x56 |
| 8 | SSAO - Edit-Ver2 Kernel Scale Base Dist (9) | 35 | v | 1 | 10 | 100 | 10 x52 |
| 9 | SSAO - Edit-Ver2 Kernel Scale Min (9) | 35 | v | 0.1 | 0.2 | 2 | 0.2 x67 |
| 10 | SSAO - Edit-Ver2 Kernel Scale Max (9) | 35 | v | 1 | 2 | 10 | 2 x49 |
| 11 | SSAO - Edit-Ver2 PrevFrame Strengh (9) | 35 | v | 0.65 | 0.7 | 0.9 | 0.7 x63 |
| 12 | SSAO - Edit-Ver2 DepthFadeStart (9) | 35 | v | 100 | 3800 | 3800 | 3800 x83 |
| 13 | SSAO - Edit-Ver2 DepthFadeRange (9) | 35 | v | 50 | 100 | 200 | 100 x83 |
| 14 | SSAO - Edit-Ver2 PrecisionDepth (9) | 35 | v | 4000 | 4000 | 4000 | 4000 x91 |
| 15 | SSAO - Edit-Ver2 PrevFrame Depth Aware (9) | 35 | v | 1 | 1 | 1 | 1 x91 |
| 16 | SSAO - Edit-Ver2 Depth Reject Rate (9) | 35 | v | 0.02 | 0.02 | 0.02 | 0.02 x91 |
| 17 | SSAO - Edit-Ver2 Geom Bent Offset (9) | 35 | v | 0 | 0 | 0.3 | 0 x69 |

SSR:

| # | param (type) | files | comp | min | median | max | most common |
|---|---|---|---|---|---|---|---|
| 1 | SSR Enable (11) | 37 | v | 0 | 1 | 1 | 1 x26 |
| 2 | SSR TraverseMode (3) | 37 | v | 0 | 0 | 0 | 0 x41 |
| 3 | SSR Shininess Reject Threshold (9) | 37 | v | 0.35 | 0.35 | 0.73 | 0.35 x24 |
| 4 | SSR Depth Reject Threshold (9) | 37 | v | 1000 | 5000 | 10000 | 5000 x22 |
| 5 | SSR Fade To Viewer Start (9) | 37 | v | -0.6 | 0.1 | 0.14 | 0.14 x19 |
| 6 | SSR Fresnel Reject Threshold (9) | 37 | v | 0.1 | 0.3 | 1 | 0.3 x27 |
| 7 | SSR Ray Trace Step Scale Min (9) | 37 | v | 2 | 2 | 6 | 2 x39 |
| 8 | SSR Ray Trace Step Scale Max (9) | 37 | v | 10 | 10 | 10 | 10 x41 |
| 9 | SSR Display Mip Scale (9) | 37 | v | 1 | 1 | 1 | 1 x41 |
| 10 | SSR Display Mip Offset (9) | 37 | v | 0 | 0 | 1 | 0 x40 |
| 11 | SSR Depth Fade Range (9) | 37 | v | 10 | 10 | 100 | 10 x39 |
| 12 | SSR Geom Crack Detect Factor (9) | 37 | v | 1.02 | 1.02 | 2 | 1.02 x30 |
| 13 | SSR Tex Blur Sigma (9) | 37 | v | 0 | 1 | 1 | 1 x36 |
| 14 | SSR Tex Blur Sigma for Mip (9) | 37 | v | 1 | 1 | 4 | 1 x36 |

Wet:

| # | param (type) | files | comp | min | median | max | most common |
|---|---|---|---|---|---|---|---|
| 1 | Wet-OverrideAlbedo (14) | 7 | luma(rgb) | 0 | 0 | 0 | 0 x7 |
| 2 | Wet-OverrideShininess (9) | 7 | v | 0.4 | 1 | 1 | 1 x6 |
| 3 | Wet-AlbedoRate (9) | 7 | v | 0 | 0 | 0.3 | 0 x6 |
| 4 | Wet-ShininessRate (9) | 7 | v | 0 | 0 | 0.4 | 0 x6 |

## 6. DS3 `Fog` against ER's fog groups

DS3 group `Fog` (OBSERVED, both DS3 files: same 11 parameters; 9 ids in m30_00_0000, 7 in m40_00_0000): `Color` (rgba), `heightColor` (rgba), `Dist`, `heightDist`, `DistOffset`, `HeightOffset`, `heightFogDensity`, `heightFogIntencity`, `ToUpper` (bool), `Alpha`, `Contrast`. DS3 also holds `LightScattering` and `LightShaft`, ER `LightShaft` (24 parameters, Yebis post effect, still present).

**No DS3 `Fog` parameter name survives in ER** (OBSERVED: none of the 11 names occurs in any ER group; ER has no `Fog` group). Closest ER parameters by *name only* (INFERRED, not a claim of equivalence):

| DS3 `Fog` parameter | ER parameter in `VolumetricFog` | note |
|---|---|---|
| Color / heightColor | `SkyLight SkyColor`, `DirLight DirectColor`, `DirLight OppositeColor` (rgba, time keys) | ER splits the fog colour into sky-fill and sun-lit colours |
| Dist / DistOffset | `FarDepth`, `FogEndDepth`, `NearDepth`, `NearOffset` | range of the froxel volume and the analytical fog end |
| heightDist / HeightOffset | `Global HeightFogDistribution`, `Global HeightFogOffset`, `Global HeightFogReferenceOffset`, `Global HeightFogReferenceModeRate` | height-fog falloff and reference height |
| heightFogDensity / heightFogIntencity | `Global HeightFogDensity Scale`, `Global FixedDensity` | density (ER typical 0.0004..0.01 height scale; `FixedDensity` 0 in half the rows) |
| Alpha / Contrast / ToUpper | no counterpart by name | `EnableAnalyticalFog` (on in 16 of 150 gameplay files) is the only switch that may keep a non-volumetric fog path |
| (none in DS3) | `Enable`, `VolumeWidth/Depth`, `Global ScatteringG`, `AbsorptionScale`, `LocalLightScale/DistScale`, `DirLight *`, `FogVolume Injecting*`, `Config *` | added by ER: froxel grid, phase g, absorption, local-light injection, directional shadow sampling, temporal jitter/feedback |

ER also adds the whole `FogVolume` group (per-preset noise-density volumes with their own light overwrite) which DS3 does not have in any form.

## 7. Checks (three parameters re-read by hand from the tsv rows)

I re-read these with awk directly on `er-gparam-values.tsv` (independent of my Python statistics) and compared:


1. `VolumetricFog FarDepth`, `FogEndDepth`, `Enable` of `m60_00_0000` (awk rows, `check1.txt`):

```
CHECK 1: VolumetricFog FarDepth / FogEndDepth, file m60_00_0000 (awk, columns file group2 param type id time value)
m60_00_0000	VolumetricFog	VolumetricFog Enable	11	0	0	1
m60_00_0000	VolumetricFog	VolumetricFog Enable	11	0	8	1
m60_00_0000	VolumetricFog	VolumetricFog FarDepth	9	0	0	2000
m60_00_0000	VolumetricFog	VolumetricFog FogEndDepth	9	0	0	4000
```
Report value: FarDepth 2000, FogEndDepth 4000 in the 4.1 table. Match.


2. `Directional Light Hemi Color Up` alpha over all `m*` rows (awk, `check2.txt`):

```
CHECK 2: Hemi Color Up in m* files, alpha (4th component) statistics by awk (n, min, max, count of 0.1)
n=2464 min=0 max=1.5 count(a==0.1)=2002 count(a==0)=156
sample rows:
m10_00_0000	Directional Light Hemi Color Up	0	0	0.927553,0.918865,0.89045,0.1
m10_00_0000	Directional Light Hemi Color Up	0	4	0.885438,0.917553,0.911904,0.1
m10_00_0000	Directional Light Hemi Color Up	0	8	0.885438,0.917553,0.912201,0.1
m10_00_0000	Directional Light Hemi Color Up	0	12	0.927553,0.829738,0.783782,0.1
m10_00_0000	Directional Light Hemi Color Up	0	15	0.750042,0.789516,0.937553,0.1
m10_00_0000	Directional Light Hemi Color Up	0	21	0.793057,0.821876,0.927553,0.1
m10_00_0000	Directional Light Hemi Color Up	0	22.5	0.921202,0.937553,0.90005,0.1
```
Report (2.1 table, `a` row): n 2,464, min 0, max 1.5, 0.1 x 2,002. Match. The sample rows also show the raw colour of `m10_00_0000` at 0 h (0.927553, 0.918865, 0.89045, 0.1).


3. `RayTracing AO Ray SampleCount` over all files (awk, `check3.txt`):

```
CHECK 3: RayTracing AO Ray SampleCount, value counts over all rows of all files (awk)
value 2: 216 rows
value 4: 24 rows
rows=240 files=47
mapbase and default rows:
default	RayTracing AO Ray SampleCount	0	0	4
mapbase	RayTracing AO Ray SampleCount	0	0	2
```
Report (5.2 table): 2 x216, 4 x24, `default` 4, `mapbase` 2; 240 rows over 47 files. Match.


## 8. What I could not do or did not settle

- OBSERVED limits: the alpha meaning (intensity or not) is INFERRED, so the hemisphere-to-sun ratio is given in two readings (0.55 / 0.14); the true engine value needs the shader/code read, not done here.
- Time keys differ per parameter; ratios interpolate linearly between keys (INFERRED interpolation).
- DS3's four-light block order (light 0 = first block) is INFERRED from the repeated id order.
- `m31_*` files do not exist in the table (dungeon examples: `m30_00_0000`, `m34_11_0000`, `m30_00_0200`; `m30_00_0000` equals the overworld in LightSet (all 26 parameters, OBSERVED) so `m34_11_0000` is the independent dungeon witness in the ratio table). The "underground" file used is `m12_01_0000` (its sun alpha is 0 in all 4 rows, so the alpha-multiplied ratio has no row there; the colour-only ratio is 0 because its hemisphere colours are black).
- Which fog/volume ids a map really references, and what a missing group falls back to, are not in these tables (not read).
- `LightShaft` and the other post-process groups: table only in `param-stats-gameplay.tsv`.
